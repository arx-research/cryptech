/*
 * rsa.c
 * -----
 * Basic RSA functions based on Cryptech ModExp core.
 *
 * The mix of what we're doing in software vs what we're doing on the
 * FPGA is a moving target.  Goal for now is to have the bits we need
 * to do in C be straightforward to review and as simple as possible
 * (but no simpler).
 *
 * Much of the code in this module is based, at least loosely, on Tom
 * St Denis's libtomcrypt code.
 *
 * Authors: Rob Austein
 * Copyright (c) 2015-2018, NORDUnet A/S
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the NORDUnet nor the names of its contributors may
 *   be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * We use "Tom's Fast Math" library for our bignum implementation.
 * This particular implementation has a couple of nice features:
 *
 * - The code is relatively readable, thus reviewable.
 *
 * - The bignum representation doesn't use dynamic memory, which
 *   simplifies things for us.
 *
 * The price tag for not using dynamic memory is that libtfm has to be
 * configured to know about the largest bignum one wants it to be able
 * to support at compile time.  This should not be a serious problem.
 *
 * We use a lot of one-element arrays (fp_int[1] instead of plain
 * fp_int) to avoid having to prefix every use of an fp_int with "&".
 * Perhaps we should encapsulate this idiom in a typedef.
 *
 * Unfortunately, libtfm is bad about const-ification, but we want to
 * hide that from our users, so our public API uses const as
 * appropriate and we use inline functions to remove const constraints
 * in a relatively type-safe manner before calling libtom.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "hal.h"
#include "hal_internal.h"
#include <tfm.h>
#include "asn1_internal.h"

/*
 * Whether to use ModExp core.  It works, but it's painfully slow.
 */

#ifndef HAL_RSA_SIGN_USE_MODEXP
#define HAL_RSA_SIGN_USE_MODEXP 1
#endif

#ifndef HAL_RSA_KEYGEN_USE_MODEXP
#define HAL_RSA_KEYGEN_USE_MODEXP 0
#endif

#if defined(RPC_CLIENT) && RPC_CLIENT != RPC_CLIENT_LOCAL
#define hal_get_random(core, buffer, length) hal_rpc_get_random(buffer, length)
#endif

/*
 * How big to make the buffers for the modulus coefficient and
 * Montgomery factor.  This will almost certainly want tuning.
 */

#ifndef HAL_RSA_MAX_OPERAND_LENGTH
#define HAL_RSA_MAX_OPERAND_LENGTH MODEXPA7_OPERAND_BYTES
#endif

/*
 * How big to make the blinding factors cache.
 * Zero disables the cache entirely.
 */

#ifndef HAL_RSA_BLINDING_CACHE_SIZE
#define HAL_RSA_BLINDING_CACHE_SIZE 2
#endif

/*
 * Whether we want debug output.
 */

static int debug = 0;

void hal_rsa_set_debug(const int onoff)
{
  debug = onoff;
}

/*
 * Whether we want RSA blinding.
 */

static int blinding = 1;

void hal_rsa_set_blinding(const int onoff)
{
  blinding = onoff;
}

#if HAL_RSA_BLINDING_CACHE_SIZE > 0

typedef struct {
  unsigned lru;
  fp_int n[1], bf[1], ubf[1];
} bfc_slot_t;

static struct {
  unsigned lru;
  bfc_slot_t slot[HAL_RSA_BLINDING_CACHE_SIZE];
} bfc;

#endif

/*
 * RSA key implementation.  This structure type is private to this
 * module, anything else that needs to touch one of these just gets a
 * typed opaque pointer.  We do, however, export the size, so that we
 * can make memory allocation the caller's problem.
 */

struct hal_rsa_key {
  hal_key_type_t type;          /* What kind of key this is */
  fp_int n[1];                  /* The modulus */
  fp_int e[1];                  /* Public exponent */
  fp_int d[1];                  /* Private exponent */
  fp_int p[1];                  /* 1st prime factor */
  fp_int q[1];                  /* 2nd prime factor */
  fp_int u[1];                  /* 1/q mod p */
  fp_int dP[1];                 /* d mod (p - 1) */
  fp_int dQ[1];                 /* d mod (q - 1) */
  unsigned flags;               /* Internal key flags */
  uint8_t                       /* ModExpA7 speedup factors */
    nC[HAL_RSA_MAX_OPERAND_LENGTH],   nF[HAL_RSA_MAX_OPERAND_LENGTH],
    pC[HAL_RSA_MAX_OPERAND_LENGTH/2], pF[HAL_RSA_MAX_OPERAND_LENGTH/2],
    qC[HAL_RSA_MAX_OPERAND_LENGTH/2], qF[HAL_RSA_MAX_OPERAND_LENGTH/2];
};

#define RSA_FLAG_NEEDS_SAVING    (1 << 0)
#define RSA_FLAG_PRECALC_N_DONE  (1 << 1)
#define RSA_FLAG_PRECALC_PQ_DONE (1 << 2)

const size_t hal_rsa_key_t_size = sizeof(hal_rsa_key_t);

/*
 * Initializers.  We want to be able to initialize automatic fp_int
 * variables a sane value (less error prone), but picky compilers
 * whine about the number of curly braces required.  So we define a
 * macro which isolates that madness in one place.
 */

#define INIT_FP_INT     {{{0}}}

/*
 * Error handling.
 */

#define lose(_code_)                                    \
  do { err = _code_; goto fail; } while (0)

#define FP_CHECK(_expr_)                                \
  do {                                                  \
    switch (_expr_) {                                   \
    case FP_OKAY: break;                                \
    case FP_VAL:  lose(HAL_ERROR_BAD_ARGUMENTS);        \
    case FP_MEM:  lose(HAL_ERROR_ALLOCATION_FAILURE);   \
    default:      lose(HAL_ERROR_IMPOSSIBLE);           \
    }                                                   \
  } while (0)


/*
 * Unpack a bignum into a byte array, with length check.
 */

static hal_error_t unpack_fp(const fp_int * const bn, uint8_t *buffer, const size_t length)
{
  hal_error_t err = HAL_OK;

  if (bn == NULL || buffer == NULL)
    return HAL_ERROR_IMPOSSIBLE;

  const size_t bytes = fp_unsigned_bin_size(unconst_fp_int(bn));

  if (bytes > length)
    lose(HAL_ERROR_RESULT_TOO_LONG);

  memset(buffer, 0, length);
  fp_to_unsigned_bin(unconst_fp_int(bn), buffer + length - bytes);

 fail:
  return err;
}

#if HAL_RSA_SIGN_USE_MODEXP

/*
 * Unwrap bignums into byte arrays, feed them into hal_modexp(), and
 * wrap result back up as a bignum.
 */

static hal_error_t modexp(hal_core_t *core,
                          const int precalc,
                          const fp_int * const msg,
                          const fp_int * const exp,
                          const fp_int * const mod,
                          fp_int       *       res,
                          uint8_t *coeff, const size_t coeff_len,
                          uint8_t *mont,  const size_t mont_len)
{
  hal_error_t err = HAL_OK;

  if (msg == NULL || exp == NULL || mod == NULL || res == NULL || coeff == NULL || mont == NULL)
    return HAL_ERROR_IMPOSSIBLE;

  const size_t msg_len = (fp_unsigned_bin_size(unconst_fp_int(msg)) + 3) & ~3;
    const size_t exp_len = (fp_unsigned_bin_size(unconst_fp_int(exp)) + 3) & ~3;
    const size_t mod_len = (fp_unsigned_bin_size(unconst_fp_int(mod)) + 3) & ~3;

  uint8_t msgbuf[msg_len];
  uint8_t expbuf[exp_len];
  uint8_t modbuf[mod_len];
  uint8_t resbuf[mod_len];

    hal_modexp_arg_t args = {
      .core   = core,
      .msg    = msgbuf, .msg_len    = sizeof(msgbuf),
      .exp    = expbuf, .exp_len    = sizeof(expbuf),
      .mod    = modbuf, .mod_len    = sizeof(modbuf),
      .result = resbuf, .result_len = sizeof(resbuf),
      .coeff  = coeff,  .coeff_len  = coeff_len,
      .mont   = mont,   .mont_len   = mont_len
    };

  if ((err = unpack_fp(msg, msgbuf, sizeof(msgbuf))) != HAL_OK ||
      (err = unpack_fp(exp, expbuf, sizeof(expbuf))) != HAL_OK ||
      (err = unpack_fp(mod, modbuf, sizeof(modbuf))) != HAL_OK ||
      (err = hal_modexp(precalc, &args))             != HAL_OK)
      goto fail;

  fp_read_unsigned_bin(res, resbuf, sizeof(resbuf));

 fail:
  memset(msgbuf, 0, sizeof(msgbuf));
  memset(expbuf, 0, sizeof(expbuf));
  memset(modbuf, 0, sizeof(modbuf));
  memset(resbuf, 0, sizeof(resbuf));
  memset(&args,  0, sizeof(args));
  return err;
}

static hal_error_t modexp2(const int precalc,
                           const fp_int * const msg,
                           hal_core_t *core1,
                           const fp_int * const exp1,
                           const fp_int * const mod1,
                           fp_int       *       res1,
                           uint8_t *coeff1, const size_t coeff1_len,
                           uint8_t *mont1,  const size_t mont1_len,
                           hal_core_t *core2,
                           const fp_int * const exp2,
                           const fp_int * const mod2,
                           fp_int       *       res2,
                           uint8_t *coeff2, const size_t coeff2_len,
                           uint8_t *mont2,  const size_t mont2_len)
{
  hal_error_t err = HAL_OK;

  if (msg  == NULL ||
      exp1 == NULL || mod1 == NULL || res1 == NULL || coeff1 == NULL || mont1 == NULL ||
      exp2 == NULL || mod2 == NULL || res2 == NULL || coeff2 == NULL || mont2 == NULL)
    return HAL_ERROR_IMPOSSIBLE;

  const size_t msg_len  = (fp_unsigned_bin_size(unconst_fp_int(msg))  + 3) & ~3;
    const size_t exp1_len = (fp_unsigned_bin_size(unconst_fp_int(exp1)) + 3) & ~3;
    const size_t mod1_len = (fp_unsigned_bin_size(unconst_fp_int(mod1)) + 3) & ~3;
    const size_t exp2_len = (fp_unsigned_bin_size(unconst_fp_int(exp2)) + 3) & ~3;
    const size_t mod2_len = (fp_unsigned_bin_size(unconst_fp_int(mod2)) + 3) & ~3;

  uint8_t msgbuf[msg_len];
  uint8_t expbuf1[exp1_len], modbuf1[mod1_len], resbuf1[mod1_len];
  uint8_t expbuf2[exp2_len], modbuf2[mod2_len], resbuf2[mod2_len];

    hal_modexp_arg_t args1 = {
      .core   = core1,
      .msg    = msgbuf,  .msg_len    = sizeof(msgbuf),
      .exp    = expbuf1, .exp_len    = sizeof(expbuf1),
      .mod    = modbuf1, .mod_len    = sizeof(modbuf1),
      .result = resbuf1, .result_len = sizeof(resbuf1),
      .coeff  = coeff1,  .coeff_len  = coeff1_len,
      .mont   = mont1,   .mont_len   = mont1_len
    };

    hal_modexp_arg_t args2 = {
      .core   = core2,
      .msg    = msgbuf,  .msg_len    = sizeof(msgbuf),
      .exp    = expbuf2, .exp_len    = sizeof(expbuf2),
      .mod    = modbuf2, .mod_len    = sizeof(modbuf2),
      .result = resbuf2, .result_len = sizeof(resbuf2),
      .coeff  = coeff2,  .coeff_len  = coeff2_len,
      .mont   = mont2,   .mont_len   = mont2_len
    };

  if ((err = unpack_fp(msg,  msgbuf,  sizeof(msgbuf)))  != HAL_OK ||
      (err = unpack_fp(exp1, expbuf1, sizeof(expbuf1))) != HAL_OK ||
      (err = unpack_fp(mod1, modbuf1, sizeof(modbuf1))) != HAL_OK ||
      (err = unpack_fp(exp2, expbuf2, sizeof(expbuf2))) != HAL_OK ||
      (err = unpack_fp(mod2, modbuf2, sizeof(modbuf2))) != HAL_OK ||
      (err = hal_modexp2(precalc, &args1, &args2))      != HAL_OK)
      goto fail;

  fp_read_unsigned_bin(res1, resbuf1, sizeof(resbuf1));
  fp_read_unsigned_bin(res2, resbuf2, sizeof(resbuf2));

 fail:
  memset(msgbuf,  0, sizeof(msgbuf));
  memset(expbuf1, 0, sizeof(expbuf1));
  memset(modbuf1, 0, sizeof(modbuf1));
  memset(resbuf1, 0, sizeof(resbuf1));
  memset(&args1,  0, sizeof(args1));
  memset(expbuf2, 0, sizeof(expbuf2));
  memset(modbuf2, 0, sizeof(modbuf2));
  memset(resbuf2, 0, sizeof(resbuf2));
  memset(&args2,  0, sizeof(args2));
  return err;
}

#else /* HAL_RSA_SIGN_USE_MODEXP */

/*
 * Use libtfm's software implementation of modular exponentiation.
 * Now that the ModExpA7 core performs about as well as the software
 * implementation, there's probably no need to use this, but we're
 * still tuning things, so leave the hook here for now.
 */

static hal_error_t modexp(const hal_core_t *core, /* ignored */
                          const int precalc,      /* ignored */
                          const fp_int * const msg,
                          const fp_int * const exp,
                          const fp_int * const mod,
                          fp_int *res,
                          uint8_t *coeff, const size_t coeff_len, /* ignored */
                          uint8_t *mont,  const size_t mont_len) /* ignored */

{
  hal_error_t err = HAL_OK;
  FP_CHECK(fp_exptmod(unconst_fp_int(msg), unconst_fp_int(exp), unconst_fp_int(mod), res));
 fail:
  return err;
}

static hal_error_t modexp2(const int precalc, /* ignored */
                           const fp_int * const msg,
                           hal_core_t *core1, /* ignored */
                           const fp_int * const exp1,
                           const fp_int * const mod1,
                           fp_int       *       res1,
                           uint8_t *coeff1, const size_t coeff1_len, /* ignored */
                           uint8_t *mont1,  const size_t mont1_len, /* ignored */
                           hal_core_t *core2, /* ignored */
                           const fp_int * const exp2,
                           const fp_int * const mod2,
                           fp_int       *       res2,
                           uint8_t *coeff2, const size_t coeff2_len, /* ignored */
                           uint8_t *mont2,  const size_t mont2_len) /* ignored */
{
  hal_error_t err = HAL_OK;
  FP_CHECK(fp_exptmod(unconst_fp_int(msg), unconst_fp_int(exp1), unconst_fp_int(mod1), res1));
  FP_CHECK(fp_exptmod(unconst_fp_int(msg), unconst_fp_int(exp2), unconst_fp_int(mod2), res2));
 fail:
  return err;
}

#endif /* HAL_RSA_SIGN_USE_MODEXP */

/*
 * Wrapper to let us export our modexp function as a replacement for
 * libtfm's when running libtfm's Miller-Rabin test code.
 *
 * At the moment, the libtfm software implementation performs
 * disproportionately better than our core does for the specific case
 * of Miller-Rabin tests, for reasons we don't really understand.
 * So there's not much point in enabling this, except as a test to
 * confirm this behavior.
 *
 * This code is here rather than in a separate module because of the
 * error handling: libtfm's error codes aren't really capable of
 * expressing all the things that could go wrong here.
 */

#if HAL_RSA_SIGN_USE_MODEXP && HAL_RSA_KEYGEN_USE_MODEXP

int fp_exptmod(fp_int *a, fp_int *b, fp_int *c, fp_int *d)
{
  const size_t len = (fp_unsigned_bin_size(unconst_fp_int(b)) + 3) & ~3;
  uint8_t C[len], F[len];
  const hal_error_t err = modexp(NULL, 0, a, b, c, d, C, sizeof(C), F, sizeof(F));
  memset(C, 0, sizeof(C));
  memset(F, 0, sizeof(F));
  return err == HAL_OK ? FP_OKAY : FP_VAL;
}

#endif /* HAL_RSA_SIGN_USE_MODEXP && HAL_RSA_KEYGEN_USE_MODEXP */

/*
 * Create blinding factors.
 */

static hal_error_t create_blinding_factors(hal_rsa_key_t *key, fp_int *bf, fp_int *ubf)
{
  if (key == NULL || bf == NULL || ubf == NULL)
    return HAL_ERROR_IMPOSSIBLE;

  const int precalc = !(key->flags & RSA_FLAG_PRECALC_N_DONE);
  uint8_t rnd[fp_unsigned_bin_size(unconst_fp_int(key->n))];
  hal_error_t err = HAL_OK;

  hal_rsa_bf_lock();

#if HAL_RSA_BLINDING_CACHE_SIZE > 0
  unsigned best_delta = 0;
  int      best_index = 0;

  for (int i = 0; i < HAL_RSA_BLINDING_CACHE_SIZE; i++) {
    bfc_slot_t *b = &bfc.slot[i];
    const unsigned delta = bfc.lru - b->lru;
    if (delta > best_delta) {
      best_delta = delta;
      best_index = i;
    }
    if (fp_cmp_mag(b->n, key->n) == FP_EQ) {
      if (fp_sqrmod(b->bf,  key->n, b->bf)  != FP_OKAY ||
          fp_sqrmod(b->ubf, key->n, b->ubf) != FP_OKAY)
        continue;               /* should never happen, but be safe */
      fp_copy(b->bf, bf);
      fp_copy(b->ubf, ubf);
      err = HAL_OK;
      goto fail;
    }
  }
#endif

  if ((err = hal_get_random(NULL, rnd, sizeof(rnd))) != HAL_OK)
    goto fail;

  fp_init(bf);
  fp_read_unsigned_bin(bf, rnd, sizeof(rnd));
  fp_copy(bf, ubf);

  if ((err = modexp(NULL, precalc, bf, key->e, key->n, bf,
                    key->nC, sizeof(key->nC), key->nF, sizeof(key->nF))) != HAL_OK)
    goto fail;

  if (precalc)
    key->flags |= RSA_FLAG_PRECALC_N_DONE | RSA_FLAG_NEEDS_SAVING;

  FP_CHECK(fp_invmod(ubf, unconst_fp_int(key->n), ubf));

#if HAL_RSA_BLINDING_CACHE_SIZE > 0
  {
    bfc_slot_t *b = &bfc.slot[best_index];
    fp_copy(key->n, b->n);
    fp_copy(bf,  b->bf);
    fp_copy(ubf, b->ubf);
    b->lru = ++bfc.lru;
  }
#endif

 fail:
  hal_rsa_bf_unlock();
  memset(rnd, 0, sizeof(rnd));
  return err;
}

/*
 * RSA decryption via Chinese Remainder Theorem (Garner's formula).
 */

static hal_error_t rsa_crt(hal_core_t *core1, hal_core_t *core2, hal_rsa_key_t *key, fp_int *msg, fp_int *sig)
{
  if (key == NULL || msg == NULL || sig == NULL)
    return HAL_ERROR_IMPOSSIBLE;

  const int precalc = !(key->flags & RSA_FLAG_PRECALC_PQ_DONE);
  hal_error_t err = HAL_OK;
  fp_int t[1]   = INIT_FP_INT;
  fp_int m1[1]  = INIT_FP_INT;
  fp_int m2[1]  = INIT_FP_INT;
  fp_int bf[1]  = INIT_FP_INT;
  fp_int ubf[1] = INIT_FP_INT;

  /*
   * Handle blinding if requested.
   */
  if (blinding) {
    if ((err = create_blinding_factors(key, bf, ubf)) != HAL_OK)
      goto fail;
    FP_CHECK(fp_mulmod(msg, bf, unconst_fp_int(key->n), msg));
  }

  /*
   * m1 = msg ** dP mod p
   * m2 = msg ** dQ mod q
   */
  if ((err = modexp2(precalc, msg,
                     core1, key->dP, key->p, m1, key->pC, sizeof(key->pC), key->pF, sizeof(key->pF),
                     core2, key->dQ, key->q, m2, key->qC, sizeof(key->qC), key->qF, sizeof(key->qF))) != HAL_OK)
    goto fail;

  if (precalc)
    key->flags |= RSA_FLAG_PRECALC_PQ_DONE | RSA_FLAG_NEEDS_SAVING;

  /*
   * t = m1 - m2.
   */
  fp_sub(m1, m2, t);

  /*
   * Add zero (mod p) if needed to make t positive.  If doing this
   * once or twice doesn't help, something is very wrong.
   */
  if (fp_cmp_d(t, 0) == FP_LT)
    fp_add(t, unconst_fp_int(key->p), t);
  if (fp_cmp_d(t, 0) == FP_LT)
    fp_add(t, unconst_fp_int(key->p), t);
  if (fp_cmp_d(t, 0) == FP_LT)
    lose(HAL_ERROR_IMPOSSIBLE);

  /*
   * sig = (t * u mod p) * q + m2
   */
  FP_CHECK(fp_mulmod(t, unconst_fp_int(key->u), unconst_fp_int(key->p), t));
  fp_mul(t, unconst_fp_int(key->q), t);
  fp_add(t, m2, sig);

  /*
   * Unblind if necessary.
   */
  if (blinding)
    FP_CHECK(fp_mulmod(sig, ubf, unconst_fp_int(key->n), sig));

 fail:
  fp_zero(t);
  fp_zero(m1);
  fp_zero(m2);
  return err;
}

/*
 * Public API for raw RSA encryption and decryption.
 *
 * NB: This does not handle PKCS #1.5 padding, at the moment that's up
 * to the caller.
 */

hal_error_t hal_rsa_encrypt(hal_core_t *core,
                            hal_rsa_key_t *key,
                            const uint8_t * const input,  const size_t input_len,
                            uint8_t * output, const size_t output_len)
{
  hal_error_t err = HAL_OK;

  if (key == NULL || input == NULL || output == NULL || input_len > output_len)
    return HAL_ERROR_BAD_ARGUMENTS;

  const int precalc = !(key->flags & RSA_FLAG_PRECALC_N_DONE);
  fp_int i[1] = INIT_FP_INT;
  fp_int o[1] = INIT_FP_INT;

  fp_read_unsigned_bin(i, unconst_uint8_t(input), input_len);

  err = modexp(core, precalc, i, key->e, key->n, o,
               key->nC, sizeof(key->nC), key->nF, sizeof(key->nF));

  if (err == HAL_OK && precalc)
    key->flags |= RSA_FLAG_PRECALC_N_DONE | RSA_FLAG_NEEDS_SAVING;

  if (err == HAL_OK)
    err = unpack_fp(o, output, output_len);

  fp_zero(i);
  fp_zero(o);
  return err;
}

hal_error_t hal_rsa_decrypt(hal_core_t *core1,
                            hal_core_t *core2,
                            hal_rsa_key_t *key,
                            const uint8_t * const input,  const size_t input_len,
                            uint8_t * output, const size_t output_len)
{
  hal_error_t err = HAL_OK;

  if (key == NULL || input == NULL || output == NULL || input_len > output_len)
    return HAL_ERROR_BAD_ARGUMENTS;

  fp_int i[1] = INIT_FP_INT;
  fp_int o[1] = INIT_FP_INT;

  fp_read_unsigned_bin(i, unconst_uint8_t(input), input_len);

  /*
   * Do CRT if we have all the necessary key components, otherwise
   * just do brute force ModExp.
   */

  if (!fp_iszero(key->p) && !fp_iszero(key->q) && !fp_iszero(key->u) &&
      !fp_iszero(key->dP) && !fp_iszero(key->dQ))
    err = rsa_crt(core1, core2, key, i, o);

  else {
    const int precalc = !(key->flags & RSA_FLAG_PRECALC_N_DONE);
    err = modexp(core1, precalc, i, key->d, key->n, o, key->nC, sizeof(key->nC),
                 key->nF, sizeof(key->nF));
    if (err == HAL_OK && precalc)
      key->flags |= RSA_FLAG_PRECALC_N_DONE | RSA_FLAG_NEEDS_SAVING;
  }

  if (err != HAL_OK || (err = unpack_fp(o, output, output_len)) != HAL_OK)
    goto fail;

 fail:
  fp_zero(i);
  fp_zero(o);
  return err;
}

/*
 * Clear a key.  We might want to do something a bit more energetic
 * than plain old memset() eventually.
 */

void hal_rsa_key_clear(hal_rsa_key_t *key)
{
  if (key != NULL)
    memset(key, 0, sizeof(*key));
}

/*
 * Load a key from raw components.  This is a simplistic version: we
 * don't attempt to generate missing private key components, we just
 * reject the key if it doesn't have everything we expect.
 *
 * In theory, the only things we'd really need for the private key if
 * we were being nicer about this would be e, p, and q, as we could
 * calculate everything else from them.
 */

static hal_error_t load_key(const hal_key_type_t type,
                            hal_rsa_key_t **key_,
                            void *keybuf, const size_t keybuf_len,
                            const uint8_t * const n,  const size_t n_len,
                            const uint8_t * const e,  const size_t e_len,
                            const uint8_t * const d,  const size_t d_len,
                            const uint8_t * const p,  const size_t p_len,
                            const uint8_t * const q,  const size_t q_len,
                            const uint8_t * const u,  const size_t u_len,
                            const uint8_t * const dP, const size_t dP_len,
                            const uint8_t * const dQ, const size_t dQ_len)
{
  if (key_ == NULL || keybuf == NULL || keybuf_len < sizeof(hal_rsa_key_t))
    return HAL_ERROR_BAD_ARGUMENTS;

  memset(keybuf, 0, keybuf_len);

  hal_rsa_key_t *key = keybuf;

  key->type = type;

#define _(x) do { fp_init(key->x); if (x == NULL) goto fail; fp_read_unsigned_bin(key->x, unconst_uint8_t(x), x##_len); } while (0)
  switch (type) {
  case HAL_KEY_TYPE_RSA_PRIVATE:
    _(d); _(p); _(q); _(u); _(dP); _(dQ);
  case HAL_KEY_TYPE_RSA_PUBLIC:
    _(n); _(e);
    *key_ = key;
    return HAL_OK;
  default:
    goto fail;
  }
#undef _

 fail:
  memset(key, 0, sizeof(*key));
  return HAL_ERROR_BAD_ARGUMENTS;
}

/*
 * Public API to load_key().
 */

hal_error_t hal_rsa_key_load_private(hal_rsa_key_t **key_,
                                     void *keybuf, const size_t keybuf_len,
                                     const uint8_t * const n,  const size_t n_len,
                                     const uint8_t * const e,  const size_t e_len,
                                     const uint8_t * const d,  const size_t d_len,
                                     const uint8_t * const p,  const size_t p_len,
                                     const uint8_t * const q,  const size_t q_len,
                                     const uint8_t * const u,  const size_t u_len,
                                     const uint8_t * const dP, const size_t dP_len,
                                     const uint8_t * const dQ, const size_t dQ_len)
{
  return load_key(HAL_KEY_TYPE_RSA_PRIVATE, key_, keybuf, keybuf_len,
                  n, n_len, e, e_len,
                  d, d_len, p, p_len, q, q_len, u, u_len, dP, dP_len, dQ, dQ_len);
}

hal_error_t hal_rsa_key_load_public(hal_rsa_key_t **key_,
                                    void *keybuf, const size_t keybuf_len,
                                    const uint8_t * const n,  const size_t n_len,
                                    const uint8_t * const e,  const size_t e_len)
{
  return load_key(HAL_KEY_TYPE_RSA_PUBLIC, key_, keybuf, keybuf_len,
                  n, n_len, e, e_len,
                  NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0);
}

/*
 * Extract the key type.
 */

hal_error_t hal_rsa_key_get_type(const hal_rsa_key_t * const key,
                                 hal_key_type_t *key_type)
{
  if (key == NULL || key_type == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  *key_type = key->type;
  return HAL_OK;
}

/*
 * Extract public key components.
 */

static hal_error_t extract_component(const hal_rsa_key_t * const key,
                                     const size_t offset,
                                     uint8_t *res, size_t *res_len, const size_t res_max)
{
  if (key == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  const fp_int * const bn = (const fp_int *) (((const uint8_t *) key) + offset);

  const size_t len = fp_unsigned_bin_size(unconst_fp_int(bn));

  if (res_len != NULL)
    *res_len = len;

  if (res == NULL)
    return HAL_OK;

  if (len > res_max)
    return HAL_ERROR_RESULT_TOO_LONG;

  memset(res, 0, res_max);
  fp_to_unsigned_bin(unconst_fp_int(bn), res);
  return HAL_OK;
}

hal_error_t hal_rsa_key_get_modulus(const hal_rsa_key_t * const key,
                                    uint8_t *res, size_t *res_len, const size_t res_max)
{
  return extract_component(key, offsetof(hal_rsa_key_t, n), res, res_len, res_max);
}

hal_error_t hal_rsa_key_get_public_exponent(const hal_rsa_key_t * const key,
                                            uint8_t *res, size_t *res_len, const size_t res_max)
{
  return extract_component(key, offsetof(hal_rsa_key_t, e), res, res_len, res_max);
}

/*
 * Generate a prime factor for an RSA keypair.
 *
 * Get random bytes, munge a few bits, and stuff into a bignum to
 * construct our initial candidate.
 *
 * Initialize table of remainders when dividing candidate by each
 * entry in corresponding table of small primes.  We'd have to perform
 * these tests in any case for any succesful candidate, and doing it
 * up front lets us amortize the cost over the entire search, so we do
 * this unconditionally before entering the search loop.
 *
 * If all of the remainders were non-zero, run the requisite number of
 * Miller-Rabin tests using the first few entries from that same table
 * of small primes as the test values.  If we get past Miller-Rabin,
 * the candidate is (probably) prime, to a confidence level which we
 * can tune by adjusting the number of Miller-Rabin tests.
 *
 * For RSA, we also need (result - 1) to be relatively prime with
 * respect to the public exponent.  If a (probable) prime passes that
 * test, we have a winner.
 *
 * If any of the above tests failed, we increment the candidate and
 * all remainders by two, then loop back to the remainder test.  This
 * is where the table pays off: incrementing remainders is really
 * cheap, and since most composite numbers fail the small primes test,
 * making that cheap makes the whole loop run significantly faster.
 *
 * General approach suggested by HAC note 4.51.  Range of small prime
 * table and default number of Miller-Rabin tests suggested by Schneier.
 */

#ifndef HAL_RSA_MILLER_RABIN_TESTS
#define HAL_RSA_MILLER_RABIN_TESTS (5)
#endif

static const uint16_t small_prime[] = {
  2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61,
  67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137,
  139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199,
  211, 223, 227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277,
  281, 283, 293, 307, 311, 313, 317, 331, 337, 347, 349, 353, 359,
  367, 373, 379, 383, 389, 397, 401, 409, 419, 421, 431, 433, 439,
  443, 449, 457, 461, 463, 467, 479, 487, 491, 499, 503, 509, 521,
  523, 541, 547, 557, 563, 569, 571, 577, 587, 593, 599, 601, 607,
  613, 617, 619, 631, 641, 643, 647, 653, 659, 661, 673, 677, 683,
  691, 701, 709, 719, 727, 733, 739, 743, 751, 757, 761, 769, 773,
  787, 797, 809, 811, 821, 823, 827, 829, 839, 853, 857, 859, 863,
  877, 881, 883, 887, 907, 911, 919, 929, 937, 941, 947, 953, 967,
  971, 977, 983, 991, 997, 1009, 1013, 1019, 1021, 1031, 1033, 1039,
  1049, 1051, 1061, 1063, 1069, 1087, 1091, 1093, 1097, 1103, 1109,
  1117, 1123, 1129, 1151, 1153, 1163, 1171, 1181, 1187, 1193, 1201,
  1213, 1217, 1223, 1229, 1231, 1237, 1249, 1259, 1277, 1279, 1283,
  1289, 1291, 1297, 1301, 1303, 1307, 1319, 1321, 1327, 1361, 1367,
  1373, 1381, 1399, 1409, 1423, 1427, 1429, 1433, 1439, 1447, 1451,
  1453, 1459, 1471, 1481, 1483, 1487, 1489, 1493, 1499, 1511, 1523,
  1531, 1543, 1549, 1553, 1559, 1567, 1571, 1579, 1583, 1597, 1601,
  1607, 1609, 1613, 1619, 1621, 1627, 1637, 1657, 1663, 1667, 1669,
  1693, 1697, 1699, 1709, 1721, 1723, 1733, 1741, 1747, 1753, 1759,
  1777, 1783, 1787, 1789, 1801, 1811, 1823, 1831, 1847, 1861, 1867,
  1871, 1873, 1877, 1879, 1889, 1901, 1907, 1913, 1931, 1933, 1949,
  1951, 1973, 1979, 1987, 1993, 1997, 1999
};

static hal_error_t find_prime(const unsigned prime_length,
                              const fp_int * const e,
                              fp_int *result)
{
  uint16_t remainder[sizeof(small_prime)/sizeof(*small_prime)];
  uint8_t buffer[prime_length];
  fp_int t[1] = INIT_FP_INT;
  hal_error_t err;

  if ((err = hal_get_random(NULL, buffer, sizeof(buffer))) != HAL_OK)
    return err;

  buffer[0]                  &= ~0x01; /* Headroom for search */
  buffer[0]                  |=  0xc0; /* Result large enough */
  buffer[sizeof(buffer) - 1] |=  0x01; /* Candidates are odd  */

  fp_read_unsigned_bin(result, buffer, sizeof(buffer));
  memset(buffer, 0, sizeof(buffer));

  for (size_t i = 0; i < sizeof(small_prime)/sizeof(*small_prime); i++) {
    fp_digit d;
    fp_mod_d(result, small_prime[i], &d);
    remainder[i] = d;
  }

  for (;;) {
    int possible = 1;

    for (size_t i = 0; i < sizeof(small_prime)/sizeof(*small_prime); i++)
      possible &= remainder[i] != 0;

    for (size_t i = 0; possible && i < HAL_RSA_MILLER_RABIN_TESTS; i++) {
      fp_set(t, small_prime[i]);
      fp_prime_miller_rabin(result, t, &possible);
    }

    if (possible) {
      fp_sub_d(result, 1, t);
      fp_gcd(t, unconst_fp_int(e), t);
      possible = fp_cmp_d(t, 1) == FP_EQ;
    }

    if (possible)
      break;

    fp_add_d(result, 2, result);

    for (size_t i = 0; i < sizeof(small_prime)/sizeof(*small_prime); i++)
      if ((remainder[i] += 2) >= small_prime[i])
        remainder[i] -= small_prime[i];
  }

  memset(remainder, 0, sizeof(remainder));
  fp_zero(t);
  return HAL_OK;
}

/*
 * Generate a new RSA keypair.
 */

hal_error_t hal_rsa_key_gen(hal_core_t *core,
                            hal_rsa_key_t **key_,
                            void *keybuf, const size_t keybuf_len,
                            const unsigned key_length,
                            const uint8_t * const public_exponent, const size_t public_exponent_len)
{
  hal_rsa_key_t *key = keybuf;
  hal_error_t err = HAL_OK;
  fp_int p_1[1] = INIT_FP_INT;
  fp_int q_1[1] = INIT_FP_INT;

  if (key_ == NULL || keybuf == NULL || keybuf_len < sizeof(hal_rsa_key_t))
    return HAL_ERROR_BAD_ARGUMENTS;

  memset(keybuf, 0, keybuf_len);
  key->type = HAL_KEY_TYPE_RSA_PRIVATE;
  fp_read_unsigned_bin(key->e, (uint8_t *) public_exponent, public_exponent_len);

  if (key_length < bitsToBytes(1024) || key_length > bitsToBytes(8192))
    return HAL_ERROR_UNSUPPORTED_KEY;

  if (fp_cmp_d(key->e, 0x010001) != FP_EQ)
    return HAL_ERROR_UNSUPPORTED_KEY;

  /*
   * Find a good pair of prime numbers.
   */

  if ((err = find_prime(key_length / 2, key->e, key->p)) != HAL_OK ||
      (err = find_prime(key_length / 2, key->e, key->q)) != HAL_OK)
    return err;

  /*
   * Calculate remaining key components.
   */

  fp_init(p_1); fp_sub_d(key->p, 1, p_1);
  fp_init(q_1); fp_sub_d(key->q, 1, q_1);
  fp_mul(key->p, key->q, key->n);                    /* n = p * q */
  fp_lcm(p_1, q_1, key->d);
  FP_CHECK(fp_invmod(key->e, key->d, key->d));       /* d = (1/e) % lcm(p-1, q-1) */
  FP_CHECK(fp_mod(key->d, p_1, key->dP));            /* dP = d % (p-1) */
  FP_CHECK(fp_mod(key->d, q_1, key->dQ));            /* dQ = d % (q-1) */
  FP_CHECK(fp_invmod(key->q, key->p, key->u));       /* u = (1/q) % p */

  key->flags |= RSA_FLAG_NEEDS_SAVING;

  *key_ = key;

  /* Fall through to cleanup */

 fail:
  if (err != HAL_OK)
    memset(keybuf, 0, keybuf_len);
  fp_zero(p_1);
  fp_zero(q_1);
  return err;
}

/*
 * Whether a key contains new data that need saving (newly generated
 * key, updated speedup components, whatever).
 */

int hal_rsa_key_needs_saving(const hal_rsa_key_t * const key)
{
  return key != NULL && (key->flags & RSA_FLAG_NEEDS_SAVING);
}

/*
 * Just enough ASN.1 to read and write PKCS #1.5 RSAPrivateKey syntax
 * (RFC 2313 section 7.2) wrapped in a PKCS #8 PrivateKeyInfo (RFC 5208).
 *
 * RSAPrivateKey fields in the required order.
 *
 * The "extra" fields are additional key components specific to the
 * systolic modexpa7 core.  We represent these in ASN.1 as OPTIONAL
 * fields using IMPLICIT PRIVATE tags, since this is neither
 * standardized nor meaningful to anybody else.  Underlying encoding
 * is INTEGER or OCTET STRING (currently the latter).
 */

#define RSAPrivateKey_fields    \
  _(version);                   \
  _(key->n);                    \
  _(key->e);                    \
  _(key->d);                    \
  _(key->p);                    \
  _(key->q);                    \
  _(key->dP);                   \
  _(key->dQ);                   \
  _(key->u);

#define RSAPrivateKey_extra_fields                      \
  _(ASN1_PRIVATE + 0, nC, RSA_FLAG_PRECALC_N_DONE);     \
  _(ASN1_PRIVATE + 1, nF, RSA_FLAG_PRECALC_N_DONE);     \
  _(ASN1_PRIVATE + 2, pC, RSA_FLAG_PRECALC_PQ_DONE);    \
  _(ASN1_PRIVATE + 3, pF, RSA_FLAG_PRECALC_PQ_DONE);    \
  _(ASN1_PRIVATE + 4, qC, RSA_FLAG_PRECALC_PQ_DONE);    \
  _(ASN1_PRIVATE + 5, qF, RSA_FLAG_PRECALC_PQ_DONE);

hal_error_t hal_rsa_private_key_to_der_internal(const hal_rsa_key_t * const key,
                                                const int include_extra,
                                                uint8_t *der, size_t *der_len, const size_t der_max)
{
  hal_error_t err = HAL_OK;

  if (key == NULL || key->type != HAL_KEY_TYPE_RSA_PRIVATE)
    return HAL_ERROR_BAD_ARGUMENTS;

  fp_int version[1] = INIT_FP_INT;

  /*
   * Calculate data length.
   */

  size_t hlen = 0, vlen = 0;

#define _(x)                                                            \
  {                                                                     \
    size_t n = 0;                                                       \
    err = hal_asn1_encode_integer(x, NULL, &n, der_max - vlen);         \
    if (err != HAL_OK)                                                  \
      return err;                                                       \
    vlen += n;                                                          \
  }

  RSAPrivateKey_fields;
#undef _

#define _(x,y,z)                                                        \
  if ((key->flags & z) != 0) {                                          \
    size_t n = 0;                                                       \
    if ((err = hal_asn1_encode_header(x, sizeof(key->y), NULL,          \
                                      &n, 0)) != HAL_OK)                \
      return err;                                                       \
    vlen += n + sizeof(key->y);                                         \
  }

  if (include_extra) {
    RSAPrivateKey_extra_fields;
  }
#undef _

  if ((err = hal_asn1_encode_header(ASN1_SEQUENCE, vlen, NULL, &hlen, 0)) != HAL_OK)
    return err;

  if ((err = hal_asn1_encode_pkcs8_privatekeyinfo(hal_asn1_oid_rsaEncryption, hal_asn1_oid_rsaEncryption_len,
                                                  NULL, 0, NULL, hlen + vlen, NULL, der_len, der_max)) != HAL_OK)
    return err;

  if (der == NULL)
    return HAL_OK;

  /*
   * Encode data.
   */

  if ((err = hal_asn1_encode_header(ASN1_SEQUENCE, vlen, der, &hlen, der_max)) != HAL_OK)
    return err;

  uint8_t *d = der + hlen;
  memset(d, 0, vlen);

#define _(x)                                                            \
  {                                                                     \
    size_t n = 0;                                                       \
    err = hal_asn1_encode_integer(x, d, &n, vlen);                      \
    if (err != HAL_OK)                                                  \
      return err;                                                       \
    d += n;                                                             \
    vlen -= n;                                                          \
  }

  RSAPrivateKey_fields;
#undef _

#define _(x,y,z)                                                        \
  if ((key->flags & z) != 0) {                                          \
    size_t n = 0;                                                       \
    if ((err = hal_asn1_encode_header(x, sizeof(key->y), d,             \
                                      &n, vlen)) != HAL_OK)             \
      return err;                                                       \
    d    += n;                                                          \
    vlen -= n;                                                          \
    memcpy(d, key->y, sizeof(key->y));                                  \
    d    += sizeof(key->y);                                             \
    vlen -= sizeof(key->y);                                             \
  }

  if (include_extra) {
    RSAPrivateKey_extra_fields;
  }
#undef _

  return hal_asn1_encode_pkcs8_privatekeyinfo(hal_asn1_oid_rsaEncryption, hal_asn1_oid_rsaEncryption_len,
                                              NULL, 0, der, d - der, der, der_len, der_max);
}

hal_error_t hal_rsa_private_key_to_der(const hal_rsa_key_t * const key,
                                       uint8_t *der, size_t *der_len, const size_t der_max)
{
  return hal_rsa_private_key_to_der_internal(key, 0, der, der_len, der_max);
}

hal_error_t hal_rsa_private_key_to_der_extra(const hal_rsa_key_t * const key,
                                       uint8_t *der, size_t *der_len, const size_t der_max)
{
  return hal_rsa_private_key_to_der_internal(key, 1, der, der_len, der_max);
}

hal_error_t hal_rsa_private_key_from_der(hal_rsa_key_t **key_,
                                         void *keybuf, const size_t keybuf_len,
                                         const uint8_t *der, const size_t der_len)
{
  if (key_ == NULL || keybuf == NULL || keybuf_len < sizeof(hal_rsa_key_t) || der == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  memset(keybuf, 0, keybuf_len);

  hal_rsa_key_t *key = keybuf;

  key->type = HAL_KEY_TYPE_RSA_PRIVATE;

  size_t hlen, vlen, alg_oid_len, curve_oid_len, privkey_len;
  const uint8_t     *alg_oid,    *curve_oid,    *privkey;
  hal_error_t err;

  if ((err = hal_asn1_decode_pkcs8_privatekeyinfo(&alg_oid, &alg_oid_len,
                                                  &curve_oid, &curve_oid_len,
                                                  &privkey, &privkey_len,
                                                  der, der_len)) != HAL_OK)
    return err;

  if (alg_oid_len != hal_asn1_oid_rsaEncryption_len ||
      memcmp(alg_oid, hal_asn1_oid_rsaEncryption, alg_oid_len) != 0 ||
      curve_oid_len != 0)
    return HAL_ERROR_ASN1_PARSE_FAILED;

  if ((err = hal_asn1_decode_header(ASN1_SEQUENCE, privkey, privkey_len, &hlen, &vlen)) != HAL_OK)
    return err;

  const uint8_t *d = privkey + hlen;

  fp_int version[1] = INIT_FP_INT;

#define _(x)                                                            \
  {                                                                     \
    size_t n;                                                           \
    err = hal_asn1_decode_integer(x, d, &n, vlen);                      \
    if (err != HAL_OK)                                                  \
      return err;                                                       \
    d += n;                                                             \
    vlen -= n;                                                          \
  }

  RSAPrivateKey_fields;
#undef _

#define _(x,y,z)                                                        \
  if (hal_asn1_peek(x, d, vlen)) {                                      \
    size_t hl = 0, vl = 0;                                              \
    if ((err = hal_asn1_decode_header(x, d, vlen, &hl, &vl)) != HAL_OK) \
      return err;                                                       \
    if (vl > sizeof(key->y)) {                                          \
      hal_log(HAL_LOG_DEBUG, "extra factor %s too big (%lu > %lu)",     \
              #y, (unsigned long) vl, (unsigned long) sizeof(key->y));  \
      return HAL_ERROR_ASN1_PARSE_FAILED;                               \
    }                                                                   \
    memcpy(key->y, d + hl, vl);                                         \
    key->flags |= z;                                                    \
    d    += hl + vl;                                                    \
    vlen -= hl + vl;                                                    \
  }

  RSAPrivateKey_extra_fields;
#undef _

  if (d != privkey + privkey_len) {
    hal_log(HAL_LOG_DEBUG, "not at end of buffer (0x%lx != 0x%lx)",
            (unsigned long) d, (unsigned long) privkey + privkey_len);
    return HAL_ERROR_ASN1_PARSE_FAILED;
    }

  if (!fp_iszero(version)) {
    hal_log(HAL_LOG_DEBUG, "nonzero version");
    return HAL_ERROR_ASN1_PARSE_FAILED;
  }

  *key_ = key;

  return HAL_OK;
}

/*
 * ASN.1 public keys in SubjectPublicKeyInfo form, see RFCs 2313, 4055, and 5280.
 */

hal_error_t hal_rsa_public_key_to_der(const hal_rsa_key_t * const key,
                                      uint8_t *der, size_t *der_len, const size_t der_max)
{
  if (key == NULL || (key->type != HAL_KEY_TYPE_RSA_PRIVATE &&
                      key->type != HAL_KEY_TYPE_RSA_PUBLIC))
    return HAL_ERROR_BAD_ARGUMENTS;

  size_t hlen, n_len, e_len;
  hal_error_t err;

  if ((err = hal_asn1_encode_integer(key->n, NULL, &n_len, 0)) != HAL_OK ||
      (err = hal_asn1_encode_integer(key->e, NULL, &e_len, 0)) != HAL_OK)
    return err;

  const size_t vlen = n_len + e_len;

  if ((err = hal_asn1_encode_header(ASN1_SEQUENCE, vlen, der, &hlen, der_max)) != HAL_OK)
    return err;

  if (der != NULL) {
    uint8_t * const n_out = der + hlen;
    uint8_t * const e_out = n_out + n_len;

    if ((err = hal_asn1_encode_integer(key->n, n_out, NULL, der + der_max - n_out)) != HAL_OK ||
        (err = hal_asn1_encode_integer(key->e, e_out, NULL, der + der_max - e_out)) != HAL_OK)
      return err;
  }

  return hal_asn1_encode_spki(hal_asn1_oid_rsaEncryption, hal_asn1_oid_rsaEncryption_len,
                              NULL, 0, der, hlen + vlen,
                              der, der_len, der_max);

}

size_t hal_rsa_public_key_to_der_len(const hal_rsa_key_t * const key)
{
  size_t len = 0;
  return hal_rsa_public_key_to_der(key, NULL, &len, 0) == HAL_OK ? len : 0;
}

hal_error_t hal_rsa_public_key_from_der(hal_rsa_key_t **key_,
                                        void *keybuf, const size_t keybuf_len,
                                        const uint8_t * const der, const size_t der_len)
{
  hal_rsa_key_t *key = keybuf;

  if (key_ == NULL || key == NULL || keybuf_len < sizeof(*key) || der == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  memset(keybuf, 0, keybuf_len);

  key->type = HAL_KEY_TYPE_RSA_PUBLIC;

  const uint8_t *alg_oid = NULL, *null = NULL, *pubkey = NULL;
  size_t         alg_oid_len,     null_len,     pubkey_len;
  hal_error_t err;

  if ((err = hal_asn1_decode_spki(&alg_oid, &alg_oid_len, &null, &null_len, &pubkey, &pubkey_len, der, der_len)) != HAL_OK)
    return err;

  if (null != NULL || null_len != 0 || alg_oid == NULL ||
      alg_oid_len != hal_asn1_oid_rsaEncryption_len || memcmp(alg_oid, hal_asn1_oid_rsaEncryption, alg_oid_len) != 0)
    return HAL_ERROR_ASN1_PARSE_FAILED;

  size_t len, hlen, vlen;

  if ((err = hal_asn1_decode_header(ASN1_SEQUENCE, pubkey, pubkey_len, &hlen, &vlen)) != HAL_OK)
    return err;

  const uint8_t * const pubkey_end = pubkey + hlen + vlen;
  const uint8_t *d = pubkey + hlen;

  if ((err = hal_asn1_decode_integer(key->n, d, &len, pubkey_end - d)) != HAL_OK)
    return err;
  d += len;

  if ((err = hal_asn1_decode_integer(key->e, d, &len, pubkey_end - d)) != HAL_OK)
    return err;
  d += len;

  if (d != pubkey_end)
    return HAL_ERROR_ASN1_PARSE_FAILED;

  *key_ = key;

  return HAL_OK;
}

/*
 * Local variables:
 * indent-tabs-mode: nil
 * End:
 */
