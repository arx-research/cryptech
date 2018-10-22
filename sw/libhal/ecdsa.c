/*
 * ecdsa.c
 * -------
 * Elliptic Curve Digital Signature Algorithm for NIST prime curves.
 *
 * At some point we may want to refactor this code to separate
 * functionality that applies to all elliptic curve cryptography into
 * a separate module from functions specific to ECDSA over the NIST
 * prime curves, but it's simplest to keep this all in one place
 * initially.
 *
 * Much of the code in this module is based, at least loosely, on Tom
 * St Denis's libtomcrypt code.  Algorithms for point addition and
 * point doubling courtesy of the hyperelliptic.org formula database.
 *
 * Authors: Rob Austein
 * Copyright (c) 2015, NORDUnet A/S
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

#include <stdint.h>

#include "hal.h"
#include "hal_internal.h"
#include <tfm.h>
#include "asn1_internal.h"

/*
 * Whether we're using static test vectors instead of the random
 * number generator.  Do NOT enable this in production (doh).
 */

#ifndef HAL_ECDSA_DEBUG_ONLY_STATIC_TEST_VECTOR_RANDOM
#define HAL_ECDSA_DEBUG_ONLY_STATIC_TEST_VECTOR_RANDOM 0
#endif

#if defined(RPC_CLIENT) && RPC_CLIENT != RPC_CLIENT_LOCAL
#define hal_get_random(core, buffer, length) hal_rpc_get_random(buffer, length)
#endif

/*
 * Whether to use the Verilog point multipliers.
 */

#ifndef HAL_ECDSA_VERILOG_ECDSA256_MULTIPLIER
#define HAL_ECDSA_VERILOG_ECDSA256_MULTIPLIER 1
#endif

#ifndef HAL_ECDSA_VERILOG_ECDSA384_MULTIPLIER
#define HAL_ECDSA_VERILOG_ECDSA384_MULTIPLIER 1
#endif

/*
 * Whether we want debug output.
 */

static int debug = 0;

void hal_ecdsa_set_debug(const int onoff)
{
  debug = onoff;
}

/*
 * ECDSA curve descriptor.  We only deal with named curves; at the
 * moment, we only deal with NIST prime curves where the elliptic
 * curve's "a" parameter is always -3 and its "h" value (order of
 * elliptic curve group divided by order of base point) is always 1.
 *
 * Since the Montgomery parameters we need for the point arithmetic
 * depend only on the underlying field prime, we precompute them when
 * we load the curve and store them in the field descriptor, even
 * though they aren't really curve parameters per se.
 *
 * For similar reasons, we also include the ASN.1 OBJECT IDENTIFIERs
 * used to name these curves.
 */

typedef struct {
  fp_int q[1];                          /* Modulus of underlying prime field */
  fp_int b[1];                          /* Curve's "b" parameter */
  fp_int Gx[1];                         /* x component of base point G */
  fp_int Gy[1];                         /* y component of base point G */
  fp_int n[1];                          /* Order of base point G */
  fp_int mu[1];                         /* Montgomery normalization factor */
  fp_digit rho;                         /* Montgomery reduction value */
  const uint8_t *oid;                   /* OBJECT IDENTIFIER */
  size_t oid_len;                       /* Length of OBJECT IDENTIFIER */
  hal_curve_name_t curve;               /* Curve name */
} ecdsa_curve_t;

/*
 * ECDSA key implementation.  This structure type is private to this
 * module, anything else that needs to touch one of these just gets a
 * typed opaque pointer.  We do, however, export the size, so that we
 * can make memory allocation the caller's problem.
 *
 * EC points are stored in Jacobian format such that (x, y, z) =>
 * (x/z**2, y/z**3, 1) when interpretted as affine coordinates.
 *
 * There are really three different representations in use here:
 *
 * 1) Plain affine representation (z == 1);
 * 2) Montgomery form affine representation (z == curve->mu); and
 * 3) Montgomery form Jacobian representation (whatever).
 *
 * Only form (1) is ever visible outside this module.  We perform
 * explicit conversions from form (1) to form (2) and from forms (2,3)
 * to form (1).  Form (3) only occurs as the result of compuation.
 *
 * In theory, we could shave some microscopic amount of time off of
 * signature verification by supporting an explicit conversion from
 * form (3) to form (2), but it's not worth the additional complexity.
 */

typedef struct {
  fp_int x[1], y[1], z[1];
} ec_point_t;

struct hal_ecdsa_key {
  hal_key_type_t type;                  /* Public or private */
  hal_curve_name_t curve;               /* Curve descriptor */
  ec_point_t Q[1];                      /* Public key */
  fp_int d[1];                          /* Private key */
};

const size_t hal_ecdsa_key_t_size = sizeof(struct hal_ecdsa_key);

/*
 * Initializers.  We want to be able to initialize automatic fp_int
 * and ec_point_t variables to a sane value (less error prone), but
 * picky compilers whine about the number of curly braces required.
 * So we define macros which isolate that madness in one place, and
 * use those macros everywhere.
 */

#define INIT_FP_INT	{{{0}}}
#define	INIT_EC_POINT_T	{{INIT_FP_INT}}

/*
 * Error handling.
 */

#define lose(_code_) do { err = _code_; goto fail; } while (0)

/*
 * We can't (usefully) initialize fp_int variables to non-zero values
 * at compile time, so instead we load all the curve parameters the
 * first time anything asks for any of them.
 */

static const ecdsa_curve_t * get_curve(const hal_curve_name_t curve)
{
  static ecdsa_curve_t curve_p256, curve_p384, curve_p521;
  static int initialized = 0;

  if (!initialized) {

#include "ecdsa_curves.h"

    fp_read_unsigned_bin(curve_p256.q,  unconst_uint8_t(p256_q),  sizeof(p256_q));
    fp_read_unsigned_bin(curve_p256.b,  unconst_uint8_t(p256_b),  sizeof(p256_b));
    fp_read_unsigned_bin(curve_p256.Gx, unconst_uint8_t(p256_Gx), sizeof(p256_Gx));
    fp_read_unsigned_bin(curve_p256.Gy, unconst_uint8_t(p256_Gy), sizeof(p256_Gy));
    fp_read_unsigned_bin(curve_p256.n,  unconst_uint8_t(p256_n),  sizeof(p256_n));
    if (fp_montgomery_setup(curve_p256.q, &curve_p256.rho) != FP_OKAY)
      return NULL;
    fp_zero(curve_p256.mu);
    fp_montgomery_calc_normalization(curve_p256.mu, curve_p256.q);
    curve_p256.oid = p256_oid;
    curve_p256.oid_len = sizeof(p256_oid);
    curve_p256.curve = HAL_CURVE_P256;

    fp_read_unsigned_bin(curve_p384.q,  unconst_uint8_t(p384_q),  sizeof(p384_q));
    fp_read_unsigned_bin(curve_p384.b,  unconst_uint8_t(p384_b),  sizeof(p384_b));
    fp_read_unsigned_bin(curve_p384.Gx, unconst_uint8_t(p384_Gx), sizeof(p384_Gx));
    fp_read_unsigned_bin(curve_p384.Gy, unconst_uint8_t(p384_Gy), sizeof(p384_Gy));
    fp_read_unsigned_bin(curve_p384.n,  unconst_uint8_t(p384_n),  sizeof(p384_n));
    if (fp_montgomery_setup(curve_p384.q, &curve_p384.rho) != FP_OKAY)
      return NULL;
    fp_zero(curve_p384.mu);
    fp_montgomery_calc_normalization(curve_p384.mu, curve_p384.q);
    curve_p384.oid = p384_oid;
    curve_p384.oid_len = sizeof(p384_oid);
    curve_p384.curve = HAL_CURVE_P384;

    fp_read_unsigned_bin(curve_p521.q,  unconst_uint8_t(p521_q),  sizeof(p521_q));
    fp_read_unsigned_bin(curve_p521.b,  unconst_uint8_t(p521_b),  sizeof(p521_b));
    fp_read_unsigned_bin(curve_p521.Gx, unconst_uint8_t(p521_Gx), sizeof(p521_Gx));
    fp_read_unsigned_bin(curve_p521.Gy, unconst_uint8_t(p521_Gy), sizeof(p521_Gy));
    fp_read_unsigned_bin(curve_p521.n,  unconst_uint8_t(p521_n),  sizeof(p521_n));
    if (fp_montgomery_setup(curve_p521.q, &curve_p521.rho) != FP_OKAY)
      return NULL;
    fp_zero(curve_p521.mu);
    fp_montgomery_calc_normalization(curve_p521.mu, curve_p521.q);
    curve_p521.oid = p521_oid;
    curve_p521.oid_len = sizeof(p521_oid);
    curve_p521.curve = HAL_CURVE_P521;

    initialized = 1;
  }

  switch (curve) {
  case HAL_CURVE_P256:  return &curve_p256;
  case HAL_CURVE_P384:  return &curve_p384;
  case HAL_CURVE_P521:  return &curve_p521;
  default:              return NULL;
  }
}

hal_error_t hal_ecdsa_oid_to_curve(hal_curve_name_t *curve_name,
                                   const uint8_t * const oid,
                                   const size_t oid_len)
{
  if (curve_name == NULL || oid == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  *curve_name = HAL_CURVE_NONE;
  const ecdsa_curve_t *curve;

  while ((curve = get_curve(++*curve_name)) != NULL)
    if (oid_len == curve->oid_len && memcmp(oid, curve->oid, oid_len) == 0)
      return HAL_OK;

  return HAL_ERROR_UNSUPPORTED_KEY;
}

/*
 * Finite field operations (hence "ff_").  These are basically just
 * the usual bignum operations, constrained by the field modulus.
 *
 * All of these are operations in the field underlying the specified
 * curve, and assume that operands are already in Montgomery form.
 *
 * The ff_add() and ff_sub() are written a bit oddly, in an attempt to
 * make them run in constant time.  An optimizing compiler may be
 * clever enough to defeat this.  In the long run, we probably want to
 * perform these field operations in Verilog anyway.
 *
 * We might be able to squeeze a bit more speed out of the point
 * arithmetic by making using fp_mul_2d() when multiplying by a power
 * of two.  Skipping for now as a premature optimization, but if we do
 * need this, it'd probably be simplest to add a ff_dbl() function
 * which handles overflow in the same way that ff_add() does.
 */

static inline void ff_add(const ecdsa_curve_t * const curve,
                          const fp_int * const a,
                          const fp_int * const b,
                          fp_int *c)
{
  fp_int t[2][1] = {INIT_FP_INT};

  fp_add(unconst_fp_int(a), unconst_fp_int(b), t[0]);
  fp_sub(t[0], unconst_fp_int(curve->q), t[1]);

  fp_copy(t[fp_cmp_d(t[1], 0) != FP_LT], c);

  memset(t, 0, sizeof(t));
}

static inline void ff_sub(const ecdsa_curve_t * const curve,
                          const fp_int * const a,
                          const fp_int * const b,
                          fp_int *c)
{
  fp_int t[2][1] = {INIT_FP_INT};

  fp_sub(unconst_fp_int(a), unconst_fp_int(b), t[0]);
  fp_add(t[0], unconst_fp_int(curve->q), t[1]);

  fp_copy(t[fp_cmp_d(t[0], 0) == FP_LT], c);

  memset(t, 0, sizeof(t));
}

static inline void ff_mul(const ecdsa_curve_t * const curve,
                          const fp_int * const a,
                          const fp_int * const b,
                          fp_int *c)
{
  fp_mul(unconst_fp_int(a), unconst_fp_int(b), c);
  fp_montgomery_reduce(c, unconst_fp_int(curve->q), curve->rho);
}

static inline void ff_sqr(const ecdsa_curve_t * const curve,
                          const fp_int * const a,
                          fp_int *b)
{
  fp_sqr(unconst_fp_int(a), b);
  fp_montgomery_reduce(b, unconst_fp_int(curve->q), curve->rho);
}

/*
 * Test whether a point is the point at infinity.
 *
 * In Jacobian projective coordinate, any point of the form
 *
 *   (j ** 2, j **3, 0) for j in [1..q-1]
 *
 * is on the line at infinity, but for practical purposes simply
 * checking the z coordinate is probably sufficient.
 */

static inline int point_is_infinite(const ec_point_t * const P)
{
  return P == NULL || fp_iszero(P->z);
}

/*
 * Set a point to be the point at infinity.  For Jacobian projective
 * coordinates, it's customary to use (1 : 1 : 0) as the
 * representitive value.
 *
 * If a curve is supplied, we want the Montgomery form of the point at
 * infinity for that curve.
 */

static inline hal_error_t point_set_infinite(ec_point_t *P, const ecdsa_curve_t * const curve)
{
  hal_assert(P != NULL);

  if (curve != NULL) {
    fp_copy(unconst_fp_int(curve->mu), P->x);
    fp_copy(unconst_fp_int(curve->mu), P->y);
    fp_zero(P->z);
  }

  else {
    fp_set(P->x, 1);
    fp_set(P->y, 1);
    fp_zero(P->z);
  }

  return HAL_OK;
}

/*
 * Copy a point.
 */

static inline void point_copy(const ec_point_t * const P, ec_point_t *R)
{
  if (P != NULL && R != NULL && P != R)
    *R = *P;
}

/**
 * Convert a point into Montgomery form.
 * @param P        [in/out] The point to map
 * @param curve    The curve parameters structure
 */

static inline hal_error_t point_to_montgomery(ec_point_t *P,
                                              const ecdsa_curve_t * const curve)
{
  hal_assert(P != NULL && curve != NULL);

  if (fp_cmp_d(unconst_fp_int(P->z), 1) != FP_EQ)
    return HAL_ERROR_BAD_ARGUMENTS;

  if (fp_mulmod(P->x, unconst_fp_int(curve->mu), unconst_fp_int(curve->q), P->x) != FP_OKAY ||
      fp_mulmod(P->y, unconst_fp_int(curve->mu), unconst_fp_int(curve->q), P->y) != FP_OKAY)
    return HAL_ERROR_IMPOSSIBLE;

  fp_copy(unconst_fp_int(curve->mu), P->z);
  return HAL_OK;
}

/**
 * Map a point in projective Jacbobian coordinates back to affine
 * space.  This also converts back from Montgomery to plain form.
 * @param P        [in/out] The point to map
 * @param curve    The curve parameters structure
 *
 * It's not possible to represent the point at infinity in plain
 * affine coordinates, and the calling function will have to handle
 * the point at infinity specially in any case, so we declare this to
 * be the calling function's problem.
 */

static inline hal_error_t point_to_affine(ec_point_t *P,
                                          const ecdsa_curve_t * const curve)
{
  hal_assert(P != NULL && curve != NULL);

  if (point_is_infinite(P))
    return HAL_ERROR_IMPOSSIBLE;

  hal_error_t err = HAL_ERROR_IMPOSSIBLE;

  fp_int t1[1] = INIT_FP_INT;
  fp_int t2[1] = INIT_FP_INT;

  fp_int * const q = unconst_fp_int(curve->q);

  fp_montgomery_reduce(P->z, q, curve->rho);

  if (fp_invmod (P->z,   q, t1) != FP_OKAY ||    /* t1 = 1 / z    */
      fp_sqrmod (t1,     q, t2) != FP_OKAY ||    /* t2 = 1 / z**2 */
      fp_mulmod (t1, t2, q, t1) != FP_OKAY)      /* t1 = 1 / z**3 */
    goto fail;

  fp_mul (P->x,  t2,  P->x);                     /* x = x / z**2 */
  fp_mul (P->y,  t1,  P->y);                     /* y = y / z**3 */
  fp_set (P->z,  1);                             /* z = 1        */

  fp_montgomery_reduce(P->x, q, curve->rho);
  fp_montgomery_reduce(P->y, q, curve->rho);

  err = HAL_OK;

 fail:
  fp_zero(t1);
  fp_zero(t2);
  return err;
}

/**
 * Double an EC point.
 * @param P             The point to double
 * @param R             [out] The destination of the double
 * @param curve         The curve parameters structure
 *
 * Algorithm is dbl-2001-b from
 * http://www.hyperelliptic.org/EFD/g1p/auto-shortw-jacobian-3.html
 */

static inline hal_error_t point_double(const ec_point_t * const P,
                                       ec_point_t *R,
                                       const ecdsa_curve_t * const curve)
{
  hal_assert(P != NULL && R != NULL && curve != NULL);

  const int was_infinite = point_is_infinite(P);

  fp_int alpha[1] = INIT_FP_INT;
  fp_int beta[1]  = INIT_FP_INT;
  fp_int gamma[1] = INIT_FP_INT;
  fp_int delta[1] = INIT_FP_INT;
  fp_int t1[1]    = INIT_FP_INT;
  fp_int t2[1]    = INIT_FP_INT;

  ff_sqr  (curve,  P->z,          delta);       /* delta = Pz ** 2 */
  ff_sqr  (curve,  P->y,          gamma);       /* gamma = Py ** 2 */
  ff_mul  (curve,  P->x,  gamma,  beta);        /* beta  = Px * gamma */
  ff_sub  (curve,  P->x,  delta,  t1);          /* alpha = 3 * (Px - delta) * (Px + delta) */
  ff_add  (curve,  P->x,  delta,  t2);
  ff_mul  (curve,  t1,    t2,     t1);
  ff_add  (curve,  t1,    t1,     t2);
  ff_add  (curve,  t1,    t2,     alpha);

  ff_sqr  (curve,  alpha,         t1);          /* Rx = (alpha ** 2) - (8 * beta) */
  ff_add  (curve,  beta,  beta,   t2);
  ff_add  (curve,  t2,    t2,     t2);
  ff_add  (curve,  t2,    t2,     t2);
  ff_sub  (curve,  t1,    t2,     R->x);

  ff_add  (curve,  P->y,  P->z,   t1);          /* Rz = ((Py + Pz) ** 2) - gamma - delta */
  ff_sqr  (curve,  t1,            t1);
  ff_sub  (curve,  t1,    gamma,  t1);
  ff_sub  (curve,  t1,    delta,  R->z);

  ff_add  (curve,  beta,  beta,   t1);          /* Ry = (((4 * beta) - Rx) * alpha) - (8 * (gamma ** 2)) */
  ff_add  (curve,  t1,    t1,     t1);
  ff_sub  (curve,  t1,    R->x,   t1);
  ff_mul  (curve,  t1,    alpha,  t1);
  ff_sqr  (curve,  gamma,         t2);
  ff_add  (curve,  t2,    t2,     t2);
  ff_add  (curve,  t2,    t2,     t2);
  ff_add  (curve,  t2,    t2,     t2);
  ff_sub  (curve,  t1,    t2,     R->y);

  hal_assert(was_infinite == point_is_infinite(P));

  fp_zero(alpha); fp_zero(beta); fp_zero(gamma); fp_zero(delta); fp_zero(t1); fp_zero(t2);

  return HAL_OK;
}

/**
 * Add two EC points
 * @param P             The point to add
 * @param Q             The point to add
 * @param R             [out] The destination of the double
 * @param curve         The curve parameters structure
 *
 * Algorithm is madd-2007-bl from
 * http://www.hyperelliptic.org/EFD/g1p/auto-shortw-jacobian-3.html
 *
 * The special cases are unfortunate, but are probably unavoidable for
 * this type of curve.  We do what we can to make this constant-time
 * in spite of the special cases.  The one we really can't do much
 * about is P == Q, because in that case we have to switch to the
 * point doubling algorithm.
 */

static inline hal_error_t point_add(const ec_point_t * const P,
                                    const ec_point_t * const Q,
                                    ec_point_t *R,
                                    const ecdsa_curve_t * const curve)
{
  hal_assert(P != NULL && Q != NULL && R != NULL && curve != NULL);

  /*
   * Q must be affine in Montgomery form.
   */

  hal_assert(fp_cmp(unconst_fp_int(Q->z), unconst_fp_int(curve->mu)) == FP_EQ);

#warning What happens here if P and Q are not equal but map to the same point in affine space?

  const int same_xz = (fp_cmp(unconst_fp_int(P->z), unconst_fp_int(Q->z)) == FP_EQ &&
                       fp_cmp(unconst_fp_int(P->x), unconst_fp_int(Q->x)) == FP_EQ);

  /*
   * If P == Q, we must use point doubling instead of point addition,
   * and there's nothing we can do to mask the timing differences.
   * So just do it, right away.
   */

  if (same_xz && fp_cmp(unconst_fp_int(P->y), unconst_fp_int(Q->y)) == FP_EQ)
    return point_double(P, R, curve);

  /*
   * Check now for the other special cases, but defer handling them
   * until the end, to mask timing differences.
   */

  const int P_was_infinite = point_is_infinite(P);

  fp_int Qy_neg[1] = INIT_FP_INT;
  fp_sub(unconst_fp_int(curve->q), unconst_fp_int(Q->y), Qy_neg);
  const int result_is_infinite = fp_cmp(unconst_fp_int(P->y), Qy_neg) == FP_EQ && same_xz;
  fp_zero(Qy_neg);

  /*
   * Main point addition algorithm.
   */

  fp_int Z1Z1[1] = INIT_FP_INT;
  fp_int H[1]    = INIT_FP_INT;
  fp_int HH[1]   = INIT_FP_INT;
  fp_int I[1]    = INIT_FP_INT;
  fp_int J[1]    = INIT_FP_INT;
  fp_int r[1]    = INIT_FP_INT;
  fp_int V[1]    = INIT_FP_INT;
  fp_int t[1]    = INIT_FP_INT;

  ff_sqr  (curve,  P->z,           Z1Z1);       /* Z1Z1 = Pz ** 2 */

  ff_mul  (curve,  Q->x,   Z1Z1,   H);          /* H = (Qx * Z1Z1) - Px */
  ff_sub  (curve,  H,      P->x,   H);

  ff_sqr  (curve,  H,              HH);         /* HH = H ** 2 */

  ff_add  (curve,  HH,     HH,     I);          /* I = 4 * HH */
  ff_add  (curve,  I,      I,      I);

  ff_mul  (curve,  H,      I,      J);          /* J = H * I */

  ff_mul  (curve,  P->z,   Z1Z1,   r);          /* r = 2 * ((Qy * Pz * Z1Z1) - Py) */
  ff_mul  (curve,  Q->y,   r,      r);
  ff_sub  (curve,  r,      P->y,   r);
  ff_add  (curve,  r,      r,      r);

  ff_mul  (curve,  P->x,   I,      V);          /* V = Px * I */

  ff_sqr  (curve,  r,              R->x);       /* Rx = (r ** 2) - J - (2 * V) */
  ff_sub  (curve,  R->x,   J,      R->x);
  ff_sub  (curve,  R->x,   V,      R->x);
  ff_sub  (curve,  R->x,   V,      R->x);

  ff_mul  (curve,  P->y,   J,      t);         /* Ry = (r * (V - Rx)) - (2 * Py * J) */
  ff_sub  (curve,  V,      R->x,   R->y);
  ff_mul  (curve,  r,      R->y,   R->y);
  ff_sub  (curve,  R->y,   t,      R->y);
  ff_sub  (curve,  R->y,   t,      R->y);

  ff_add  (curve,  P->z,   H,      R->z);       /* Rz = ((Pz + H) ** 2) - Z1Z1 - HH */
  ff_sqr  (curve,  R->z,           R->z);
  ff_sub  (curve,  R->z,   Z1Z1,   R->z);
  ff_sub  (curve,  R->z,   HH,     R->z);

  fp_zero(Z1Z1), fp_zero(H), fp_zero(HH), fp_zero(I), fp_zero(J), fp_zero(r), fp_zero(V), fp_zero(t);

  /*
   * Handle deferred special cases, then we're done.
   */

  if (P_was_infinite)
    point_copy(Q, R);

  else if (result_is_infinite)
    point_set_infinite(R, curve);

  return HAL_OK;
}

/**
 * Perform a point multiplication.
 * @param k             The scalar to multiply by
 * @param P             The base point
 * @param R             [out] Destination for kP
 * @param curve         Curve parameters
 * @return HAL_OK on success
 *
 * P must be in affine form.
 */

static hal_error_t point_scalar_multiply(const fp_int * const k,
                                         const ec_point_t * const P_,
                                         ec_point_t *R,
                                         const ecdsa_curve_t * const curve)
{
  hal_assert(k != NULL && P_ != NULL && R != NULL &&  curve != NULL);

  if (fp_iszero(k) || fp_cmp_d(unconst_fp_int(P_->z), 1) != FP_EQ)
    return HAL_ERROR_BAD_ARGUMENTS;

  hal_error_t err;

  /*
   * Convert P to Montgomery form.
   */

  ec_point_t P[1];
  point_copy(P_, P);

  if ((err = point_to_montgomery(P, curve)) != HAL_OK) {
    memset(P, 0, sizeof(P));
    return err;
  }

  /*
   * Initialize table.
   * M[0] is a dummy for constant timing.
   * M[1] is where we accumulate the result.
   */

  ec_point_t M[2][1] = {INIT_EC_POINT_T};

  point_set_infinite(M[0], curve);
  point_set_infinite(M[1], curve);

  /*
   * Walk down bits of the scalar, performing dummy operations to mask
   * timing.
   *
   * Note that, in order for the timing protection to work, the
   * number of iterations in the loop has to depend on the order of
   * the base point rather than on the scalar.
   */

  for (int bit_index = fp_count_bits(unconst_fp_int(curve->n)) - 1; bit_index >= 0; bit_index--) {

    const int digit_index = bit_index / DIGIT_BIT;
    const fp_digit  digit = digit_index < k->used ? k->dp[digit_index] : 0;
    const fp_digit   mask = ((fp_digit) 1) << (bit_index % DIGIT_BIT);
    const int         bit = (digit & mask) != 0;

    point_double (M[1],        M[1],    curve);
    point_add    (M[bit],  P,  M[bit],  curve);

    hal_task_yield_maybe();
  }

  /*
   * Copy result, map back to affine, then done.
   */

  point_copy(M[1], R);

  err = point_to_affine(R, curve);

  memset(P, 0, sizeof(P));
  memset(M, 0, sizeof(M));

  return err;
}

/*
 * Testing only: ECDSA key generation and signature both have a
 * critical dependency on random numbers, but we can't use the random
 * number generator when testing against static test vectors. So add a
 * wrapper around the random number generator calls, with a hook to
 * let us override the generator for test purposes.  Do NOT use this
 * in production, kids.
 */

#if HAL_ECDSA_DEBUG_ONLY_STATIC_TEST_VECTOR_RANDOM

#warning hal_ecdsa random number generator overridden for test purposes
#warning DO NOT USE THIS IN PRODUCTION

typedef hal_error_t (*rng_override_test_function_t)(void *, const size_t);

static rng_override_test_function_t rng_test_override_function = 0;

rng_override_test_function_t hal_ecdsa_set_rng_override_test_function(rng_override_test_function_t new_func)
{
  rng_override_test_function_t old_func = rng_test_override_function;
  rng_test_override_function = new_func;
  return old_func;
}

static inline hal_error_t get_random(void *buffer, const size_t length)
{
  if (rng_test_override_function)
    return rng_test_override_function(buffer, length);
  else
    return hal_get_random(NULL, buffer, length);
}

#else /* HAL_ECDSA_DEBUG_ONLY_STATIC_TEST_VECTOR_RANDOM */

static inline hal_error_t get_random(void *buffer, const size_t length)
{
  return hal_get_random(NULL, buffer, length);
}

#endif /* HAL_ECDSA_DEBUG_ONLY_STATIC_TEST_VECTOR_RANDOM */

/*
 * Use experimental Verilog base point multiplier cores to calculate
 * public key given a private key.  point_pick_random() has already
 * selected a suitable private key for us, we just need to calculate
 * the corresponding public key.
 */

#if HAL_ECDSA_VERILOG_ECDSA256_MULTIPLIER || HAL_ECDSA_VERILOG_ECDSA384_MULTIPLIER

typedef struct {
  size_t bytes;
  const char *name;
  hal_addr_t k_addr;
  hal_addr_t x_addr;
  hal_addr_t y_addr;
} verilog_ecdsa_driver_t;

static hal_error_t verilog_point_pick_random(const verilog_ecdsa_driver_t * const driver,
                                             fp_int *k,
                                             ec_point_t *P)
{
  hal_assert(k != NULL && P != NULL);

  const size_t len = fp_unsigned_bin_size(k);
  uint8_t b[driver->bytes];
  const uint8_t zero[4] = {0, 0, 0, 0};
  hal_core_t *core = NULL;
  hal_error_t err;

  if (len > sizeof(b))
    return HAL_ERROR_RESULT_TOO_LONG;

  if ((err = hal_core_alloc(driver->name, &core, NULL)) != HAL_OK)
    goto fail;

#define check(_x_) do { if ((err = (_x_)) != HAL_OK) goto fail; } while (0)

  memset(b, 0, sizeof(b));
  fp_to_unsigned_bin(k, b + sizeof(b) - len);

  for (size_t i = 0; i < sizeof(b); i += 4)
    check(hal_io_write(core, driver->k_addr + i/4, &b[sizeof(b) - 4 - i], 4));

  check(hal_io_write(core, ADDR_CTRL, zero, sizeof(zero)));
  check(hal_io_next(core));
  check(hal_io_wait_valid(core));

  for (size_t i = 0; i < sizeof(b); i += 4)
    check(hal_io_read(core, driver->x_addr + i/4, &b[sizeof(b) - 4 - i], 4));
  fp_read_unsigned_bin(P->x, b, sizeof(b));

  for (size_t i = 0; i < sizeof(b); i += 4)
    check(hal_io_read(core, driver->y_addr + i/4, &b[sizeof(b) - 4 - i], 4));
  fp_read_unsigned_bin(P->y, b, sizeof(b));

  fp_set(P->z, 1);

#undef check

  err = HAL_OK;

 fail:
  hal_core_free(core);
  memset(b, 0, sizeof(b));
  return err;
}

#endif

/*
 * Pick a random point on the curve, return random scalar and
 * resulting point.
 */

static hal_error_t point_pick_random(const ecdsa_curve_t * const curve,
                                     fp_int *k,
                                     ec_point_t *P)
{
  hal_error_t err;

  hal_assert(curve != NULL && k != NULL && P != NULL);

  /*
   * Pick a random scalar corresponding to a point on the curve.  Per
   * the NSA (gulp) Suite B guidelines, we ask the CSPRNG for 64 more
   * bits than we need, which should be enough to mask any bias
   * induced by the modular reduction.
   *
   * We're picking a point out of the subgroup generated by the base
   * point on the elliptic curve, so the modulus for this calculation
   * is the order of the base point.
   *
   * Zero is an excluded value, but the chance of a non-broken CSPRNG
   * returning zero is so low that it would almost certainly indicate
   * an undiagnosed bug in the CSPRNG.
   */

  uint8_t k_buf[fp_unsigned_bin_size(unconst_fp_int(curve->n)) + 8];

  do {

    if ((err = get_random(k_buf, sizeof(k_buf))) != HAL_OK)
      return err;

    fp_read_unsigned_bin(k, k_buf, sizeof(k_buf));

    if (fp_iszero(k) || fp_mod(k, unconst_fp_int(curve->n), k) != FP_OKAY)
      return HAL_ERROR_IMPOSSIBLE;

  } while (fp_iszero(k));

  memset(k_buf, 0, sizeof(k_buf));

  switch (curve->curve) {

#if HAL_ECDSA_VERILOG_ECDSA256_MULTIPLIER
  case HAL_CURVE_P256:;
    static const verilog_ecdsa_driver_t p256_driver = {
      .name   = ECDSA256_NAME,
      .bytes  = ECDSA256_OPERAND_BITS / 8,
      .k_addr = ECDSA256_ADDR_K,
      .x_addr = ECDSA256_ADDR_X,
      .y_addr = ECDSA256_ADDR_Y
    };
    if ((err = verilog_point_pick_random(&p256_driver, k, P)) != HAL_ERROR_CORE_NOT_FOUND)
      return err;
    break;
#endif

#if HAL_ECDSA_VERILOG_ECDSA384_MULTIPLIER
  case HAL_CURVE_P384:;
    static const verilog_ecdsa_driver_t p384_driver = {
      .name   = ECDSA384_NAME,
      .bytes  = ECDSA384_OPERAND_BITS / 8,
      .k_addr = ECDSA384_ADDR_K,
      .x_addr = ECDSA384_ADDR_X,
      .y_addr = ECDSA384_ADDR_Y
    };
    if ((err = verilog_point_pick_random(&p384_driver, k, P)) != HAL_ERROR_CORE_NOT_FOUND)
      return err;
    break;
#endif

  default:
    break;
  }

  /*
   * Calculate P = kG and return.
   */

  fp_copy(curve->Gx, P->x);
  fp_copy(curve->Gy, P->y);
  fp_set(P->z, 1);

  return point_scalar_multiply(k, P, P, curve);
}

/*
 * Test whether a point really is on a particular curve.  This is
 * called "validation" when applied to a public key, and is required
 * before verifying a signature.
 */

static int point_is_on_curve(const ec_point_t * const P,
                             const ecdsa_curve_t * const curve)
{
  if (P == NULL || curve == NULL)
    return 0;

  fp_int t1[1] = INIT_FP_INT;
  fp_int t2[1] = INIT_FP_INT;

  /*
   * Compute y**2 - x**3 + 3*x.
   */

  fp_sqr(unconst_fp_int(P->y), t1);             /* t1 = y**2 */
  fp_sqr(unconst_fp_int(P->x), t2);             /* t2 = x**2 */
  if (fp_mod(t2, unconst_fp_int(curve->q), t2) != FP_OKAY)
    return 0;
  fp_mul(unconst_fp_int(P->x), t2, t2);         /* t2 = x**3 */
  fp_sub(t1, t2, t1);                           /* t1 = y**2 - x**3 */
  fp_add(t1, unconst_fp_int(P->x), t1);         /* t1 = y**2 - x**3 + 1*x */
  fp_add(t1, unconst_fp_int(P->x), t1);         /* t1 = y**2 - x**3 + 2*x */
  fp_add(t1, unconst_fp_int(P->x), t1);         /* t1 = y**2 - x**3 + 3*x */

  /*
   * Normalize and test whether computed value matches b.
   */

  if (fp_mod(t1, unconst_fp_int(curve->q), t1) != FP_OKAY)
    return 0;
  while (fp_cmp_d(t1, 0) == FP_LT)
    fp_add(t1, unconst_fp_int(curve->q), t1);
  while (fp_cmp(t1, unconst_fp_int(curve->q)) != FP_LT)
    fp_sub(t1, unconst_fp_int(curve->q), t1);

  return fp_cmp(t1, unconst_fp_int(curve->b)) == FP_EQ;
}

#warning hal_ecdsa_xxx() functions currently ignore core arguments, works but suboptimal, fix this

/*
 * Generate a new ECDSA key.
 */

hal_error_t hal_ecdsa_key_gen(hal_core_t *core,
                              hal_ecdsa_key_t **key_,
                              void *keybuf, const size_t keybuf_len,
                              const hal_curve_name_t curve_)
{
  const ecdsa_curve_t * const curve = get_curve(curve_);
  hal_ecdsa_key_t *key = keybuf;
  hal_error_t err;

  if (key_ == NULL || key == NULL || keybuf_len < sizeof(*key) || curve == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  memset(keybuf, 0, keybuf_len);

  key->type = HAL_KEY_TYPE_EC_PRIVATE;
  key->curve = curve_;

  if ((err = point_pick_random(curve, key->d, key->Q)) != HAL_OK)
    return err;

  if (!point_is_on_curve(key->Q, curve))
    return HAL_ERROR_KEY_NOT_ON_CURVE;

  *key_ = key;
  return HAL_OK;
}

/*
 * Extract key type (public or private).
 */

hal_error_t hal_ecdsa_key_get_type(const hal_ecdsa_key_t * const key,
                                   hal_key_type_t *key_type)
{
  if (key == NULL || key_type == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  *key_type = key->type;
  return HAL_OK;
}

/*
 * Extract name of curve underlying a key.
 */

hal_error_t hal_ecdsa_key_get_curve(const hal_ecdsa_key_t * const key,
                                    hal_curve_name_t *curve)
{
  if (key == NULL || curve == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  *curve = key->curve;
  return HAL_OK;
}

/*
 * Extract public key components.
 */

hal_error_t hal_ecdsa_key_get_public(const hal_ecdsa_key_t * const key,
                                     uint8_t *x, size_t *x_len, const size_t x_max,
                                     uint8_t *y, size_t *y_len, const size_t y_max)
{
  if (key == NULL || (x_len == NULL && x != NULL) || (y_len == NULL && y != NULL))
    return HAL_ERROR_BAD_ARGUMENTS;

  if (x_len != NULL)
    *x_len = fp_unsigned_bin_size(unconst_fp_int(key->Q->x));

  if (y_len != NULL)
    *y_len = fp_unsigned_bin_size(unconst_fp_int(key->Q->y));

  if ((x != NULL && *x_len > x_max) ||
      (y != NULL && *y_len > y_max))
    return HAL_ERROR_RESULT_TOO_LONG;

  if (x != NULL)
    fp_to_unsigned_bin(unconst_fp_int(key->Q->x), x);

  if (y != NULL)
    fp_to_unsigned_bin(unconst_fp_int(key->Q->y), y);

  return HAL_OK;
}

/*
 * Clear a key.
 */

void hal_ecdsa_key_clear(hal_ecdsa_key_t *key)
{
  if (key != NULL)
    memset(key, 0, sizeof(*key));
}

/*
 * Load a public key from components, and validate that the public key
 * really is on the named curve.
 */

hal_error_t hal_ecdsa_key_load_public(hal_ecdsa_key_t **key_,
                                      void *keybuf, const size_t keybuf_len,
                                      const hal_curve_name_t curve_,
                                      const uint8_t * const x, const size_t x_len,
                                      const uint8_t * const y, const size_t y_len)
{
  const ecdsa_curve_t * const curve = get_curve(curve_);
  hal_ecdsa_key_t *key = keybuf;

  if (key_ == NULL || key == NULL || keybuf_len < sizeof(*key) || curve == NULL || x == NULL || y == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  memset(keybuf, 0, keybuf_len);

  key->type = HAL_KEY_TYPE_EC_PUBLIC;
  key->curve = curve_;

  fp_read_unsigned_bin(key->Q->x, unconst_uint8_t(x), x_len);
  fp_read_unsigned_bin(key->Q->y, unconst_uint8_t(y), y_len);
  fp_set(key->Q->z, 1);

  if (!point_is_on_curve(key->Q, curve))
    return HAL_ERROR_KEY_NOT_ON_CURVE;

  *key_ = key;

  return HAL_OK;
}

/*
 * Load a private key from components: does the same things as
 * hal_ecdsa_key_load_public(), but also checks the private key, and
 * generates the public key from the private key if necessary.
 */

hal_error_t hal_ecdsa_key_load_private(hal_ecdsa_key_t **key_,
                                       void *keybuf, const size_t keybuf_len,
                                       const hal_curve_name_t curve_,
                                       const uint8_t * const x, const size_t x_len,
                                       const uint8_t * const y, const size_t y_len,
                                       const uint8_t * const d, const size_t d_len)
{
  const ecdsa_curve_t * const curve = get_curve(curve_);
  hal_ecdsa_key_t *key = keybuf;
  hal_error_t err;

  if (key_ == NULL || key == NULL || keybuf_len < sizeof(*key) || curve == NULL ||
      d == NULL || d_len == 0 || (x == NULL && x_len != 0) || (y == NULL && y_len != 0))
    return HAL_ERROR_BAD_ARGUMENTS;

  memset(keybuf, 0, keybuf_len);

  key->type = HAL_KEY_TYPE_EC_PRIVATE;
  key->curve = curve_;

  fp_read_unsigned_bin(key->d, unconst_uint8_t(d), d_len);

  if (fp_iszero(key->d) || fp_cmp(key->d, unconst_fp_int(curve->n)) != FP_LT)
    lose(HAL_ERROR_BAD_ARGUMENTS);

  fp_set(key->Q->z, 1);

  if (x_len != 0 || y_len != 0) {
    fp_read_unsigned_bin(key->Q->x, unconst_uint8_t(x), x_len);
    fp_read_unsigned_bin(key->Q->y, unconst_uint8_t(y), y_len);
  }

  else {
    fp_copy(curve->Gx, key->Q->x);
    fp_copy(curve->Gy, key->Q->y);
    if ((err = point_scalar_multiply(key->d, key->Q, key->Q, curve)) != HAL_OK)
      goto fail;
  }

  if (!point_is_on_curve(key->Q, curve))
    lose(HAL_ERROR_KEY_NOT_ON_CURVE);

  *key_ = key;
  return HAL_OK;

 fail:
  memset(keybuf, 0, keybuf_len);
  return err;
}

/*
 * Write public key in X9.62 ECPoint format (ASN.1 OCTET STRING, first octet is compression flag).
 */

hal_error_t hal_ecdsa_key_to_ecpoint(const hal_ecdsa_key_t * const key,
                                     uint8_t *der, size_t *der_len, const size_t der_max)
{
  if (key == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  const ecdsa_curve_t * const curve = get_curve(key->curve);
  if (curve == NULL)
    return HAL_ERROR_IMPOSSIBLE;

  const size_t q_len  = fp_unsigned_bin_size(unconst_fp_int(curve->q));
  const size_t Qx_len = fp_unsigned_bin_size(unconst_fp_int(key->Q->x));
  const size_t Qy_len = fp_unsigned_bin_size(unconst_fp_int(key->Q->y));
  hal_assert(q_len >= Qx_len && q_len >= Qy_len);

  const size_t vlen = q_len * 2 + 1;
  size_t hlen;

  hal_error_t err = hal_asn1_encode_header(ASN1_OCTET_STRING, vlen, der, &hlen, der_max);

  if (der_len != NULL)
    *der_len = hlen + vlen;

  if (der == NULL || err != HAL_OK)
    return err;

  hal_assert(hlen + vlen <= der_max);

  uint8_t *d = der + hlen;
  memset(d, 0, vlen);

  *d++ = 0x04;                  /* uncompressed */

  fp_to_unsigned_bin(unconst_fp_int(key->Q->x), d + q_len - Qx_len);
  d += q_len;

  fp_to_unsigned_bin(unconst_fp_int(key->Q->y), d + q_len - Qy_len);
  d += q_len;

  hal_assert(d <= der + der_max);

  return HAL_OK;
}

/*
 * Convenience wrapper to return how many bytes a key would take if
 * encoded as an ECPoint.
 */

size_t hal_ecdsa_key_to_ecpoint_len(const hal_ecdsa_key_t * const key)
{
  size_t len;
  return hal_ecdsa_key_to_ecpoint(key, NULL, &len, 0) == HAL_OK ? len : 0;
}

/*
 * Read public key in X9.62 ECPoint format (ASN.1 OCTET STRING, first octet is compression flag).
 * ECPoint format doesn't include a curve identifier, so caller has to supply one.
 */

hal_error_t hal_ecdsa_key_from_ecpoint(hal_ecdsa_key_t **key_,
                                       void *keybuf, const size_t keybuf_len,
                                       const uint8_t * const der, const size_t der_len,
                                       const hal_curve_name_t curve)
{
  hal_ecdsa_key_t *key = keybuf;

  if (key_ == NULL || key == NULL || keybuf_len < sizeof(*key) || get_curve(curve) == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  memset(keybuf, 0, keybuf_len);
  key->type = HAL_KEY_TYPE_EC_PUBLIC;
  key->curve = curve;

  size_t hlen, vlen;
  hal_error_t err;

  if ((err = hal_asn1_decode_header(ASN1_OCTET_STRING, der, der_len, &hlen, &vlen)) != HAL_OK)
    return err;

  const uint8_t * const der_end = der + hlen + vlen;
  const uint8_t *d = der + hlen;

  if (vlen < 3 || (vlen & 1) == 0 || *d++ != 0x04)
    lose(HAL_ERROR_ASN1_PARSE_FAILED);

  vlen /= 2;

  fp_read_unsigned_bin(key->Q->x, unconst_uint8_t(d), vlen);
  d += vlen;

  fp_read_unsigned_bin(key->Q->y, unconst_uint8_t(d), vlen);
  d += vlen;

  fp_set(key->Q->z, 1);

  if (d != der_end)
    lose(HAL_ERROR_ASN1_PARSE_FAILED);

  *key_ = key;
  return HAL_OK;

 fail:
  memset(keybuf, 0, keybuf_len);
  return err;
}

/*
 * Write private key in PKCS #8 PrivateKeyInfo DER format (RFC 5208).
 * This is basically just the PKCS #8 wrapper around the ECPrivateKey
 * format from RFC 5915, except that the OID naming the curve is in
 * the privateKeyAlgorithm.parameters field in the PKCS #8 wrapper and
 * is therefore omitted from the ECPrivateKey.
 *
 * This is hand-coded, and is approaching the limit where one should
 * probably be using an ASN.1 compiler like asn1c instead.
 */

hal_error_t hal_ecdsa_private_key_to_der(const hal_ecdsa_key_t * const key,
                                         uint8_t *der, size_t *der_len, const size_t der_max)
{
  if (key == NULL || key->type != HAL_KEY_TYPE_EC_PRIVATE)
    return HAL_ERROR_BAD_ARGUMENTS;

  const ecdsa_curve_t * const curve = get_curve(key->curve);
  if (curve == NULL)
    return HAL_ERROR_IMPOSSIBLE;

  const size_t q_len  = fp_unsigned_bin_size(unconst_fp_int(curve->q));
  const size_t d_len  = fp_unsigned_bin_size(unconst_fp_int(key->d));
  const size_t Qx_len = fp_unsigned_bin_size(unconst_fp_int(key->Q->x));
  const size_t Qy_len = fp_unsigned_bin_size(unconst_fp_int(key->Q->y));
  hal_assert(q_len >= d_len && q_len >= Qx_len && q_len >= Qy_len);

  fp_int version[1] = INIT_FP_INT;
  fp_set(version, 1);

  hal_error_t err;

  size_t version_len, hlen, hlen_oct, hlen_bit, hlen_exp1;

  if ((err = hal_asn1_encode_integer(version,                                    NULL, &version_len, 0)) != HAL_OK ||
      (err = hal_asn1_encode_header(ASN1_OCTET_STRING,          q_len,           NULL, &hlen_oct,    0)) != HAL_OK ||
      (err = hal_asn1_encode_header(ASN1_BIT_STRING,            (q_len + 1) * 2, NULL, &hlen_bit,    0)) != HAL_OK ||
      (err = hal_asn1_encode_header(ASN1_EXPLICIT_1, hlen_bit + (q_len + 1) * 2, NULL, &hlen_exp1,   0)) != HAL_OK)
    return err;

  const size_t vlen = version_len + hlen_oct + q_len + hlen_exp1 + hlen_bit + (q_len + 1) * 2;

  if ((err = hal_asn1_encode_header(ASN1_SEQUENCE, vlen, NULL, &hlen, 0)) != HAL_OK)
    return err;

  if ((err = hal_asn1_encode_pkcs8_privatekeyinfo(hal_asn1_oid_ecPublicKey, hal_asn1_oid_ecPublicKey_len,
                                                  curve->oid, curve->oid_len,
                                                  NULL, hlen + vlen,
                                                  NULL, der_len, der_max)) != HAL_OK)
    return err;

  if (der == NULL)
    return HAL_OK;

  if ((err = hal_asn1_encode_header(ASN1_SEQUENCE, vlen, der, &hlen, der_max)) != HAL_OK)
    return err;

  uint8_t *d = der + hlen;
  memset(d, 0, vlen);

  if ((err = hal_asn1_encode_integer(version, d, NULL, der + der_max - d)) != HAL_OK)
    return err;
  d += version_len;

  if ((err = hal_asn1_encode_header(ASN1_OCTET_STRING, q_len, d, &hlen, der + der_max - d)) != HAL_OK)
    return err;
  d += hlen;
  fp_to_unsigned_bin(unconst_fp_int(key->d), d + q_len - d_len);
  d += q_len;

  if ((err = hal_asn1_encode_header(ASN1_EXPLICIT_1, hlen_bit + (q_len + 1) * 2, d, &hlen, der + der_max - d)) != HAL_OK)
    return err;
  d += hlen;
  if ((err = hal_asn1_encode_header(ASN1_BIT_STRING, (q_len + 1) * 2, d, &hlen, der + der_max - d)) != HAL_OK)
    return err;
  d += hlen;
  *d++ = 0x00;
  *d++ = 0x04;
  fp_to_unsigned_bin(unconst_fp_int(key->Q->x), d + q_len - Qx_len);
  d += q_len;
  fp_to_unsigned_bin(unconst_fp_int(key->Q->y), d + q_len - Qy_len);
  d += q_len;

  return hal_asn1_encode_pkcs8_privatekeyinfo(hal_asn1_oid_ecPublicKey, hal_asn1_oid_ecPublicKey_len,
                                              curve->oid, curve->oid_len,
                                              der, d - der,
                                              der, der_len, der_max);
}

/*
 * Convenience wrapper to return how many bytes a private key would
 * take if encoded as DER.
 */

size_t hal_ecdsa_private_key_to_der_len(const hal_ecdsa_key_t * const key)
{
  size_t len;
  return hal_ecdsa_private_key_to_der(key, NULL, &len, 0) == HAL_OK ? len : 0;
}

/*
 * Read private key in PKCS #8 PrivateKeyInfo DER format (RFC 5208, RFC 5915).
 *
 * This is hand-coded, and is approaching the limit where one should
 * probably be using an ASN.1 compiler like asn1c instead.
 */

hal_error_t hal_ecdsa_private_key_from_der(hal_ecdsa_key_t **key_,
                                           void *keybuf, const size_t keybuf_len,
                                           const uint8_t * const der, const size_t der_len)
{
  hal_ecdsa_key_t *key = keybuf;

  if (key_ == NULL || key == NULL || keybuf_len < sizeof(*key))
    return HAL_ERROR_BAD_ARGUMENTS;

  memset(keybuf, 0, keybuf_len);
  key->type = HAL_KEY_TYPE_EC_PRIVATE;

  size_t hlen, vlen, alg_oid_len, curve_oid_len, privkey_len;
  const uint8_t     *alg_oid,    *curve_oid,    *privkey;
  hal_error_t err;

  if ((err = hal_asn1_decode_pkcs8_privatekeyinfo(&alg_oid, &alg_oid_len,
                                                  &curve_oid, &curve_oid_len,
                                                  &privkey, &privkey_len,
                                                  der, der_len)) != HAL_OK)
    return err;

  if (alg_oid_len != hal_asn1_oid_ecPublicKey_len ||
      memcmp(alg_oid, hal_asn1_oid_ecPublicKey, alg_oid_len) != 0 ||
      hal_ecdsa_oid_to_curve(&key->curve, curve_oid, curve_oid_len) != HAL_OK)
    return HAL_ERROR_ASN1_PARSE_FAILED;

  if ((err = hal_asn1_decode_header(ASN1_SEQUENCE, privkey, privkey_len, &hlen, &vlen)) != HAL_OK)
    return err;

  const uint8_t * const der_end = privkey + hlen + vlen;
  const uint8_t *d = privkey + hlen;
  fp_int version[1] = INIT_FP_INT;

  if ((err = hal_asn1_decode_integer(version, d, &hlen, vlen)) != HAL_OK)
    return err;
  if (fp_cmp_d(version, 1) != FP_EQ)
    return HAL_ERROR_ASN1_PARSE_FAILED;
  d += hlen;

  if ((err = hal_asn1_decode_header(ASN1_OCTET_STRING, d, der_end - d, &hlen, &vlen)) != HAL_OK)
    goto fail;
  d += hlen;
  fp_read_unsigned_bin(key->d, unconst_uint8_t(d), vlen);
  d += vlen;

  if ((err = hal_asn1_decode_header(ASN1_EXPLICIT_1, d, der_end - d, &hlen, &vlen)) != HAL_OK)
    goto fail;
  d += hlen;
  if (vlen > (size_t)(der_end - d))
    lose(HAL_ERROR_ASN1_PARSE_FAILED);
  if ((err = hal_asn1_decode_header(ASN1_BIT_STRING, d, vlen, &hlen, &vlen)) != HAL_OK)
    goto fail;
  d += hlen;
  if (vlen < 4 || (vlen & 1) != 0 || *d++ != 0x00 || *d++ != 0x04)
    lose(HAL_ERROR_ASN1_PARSE_FAILED);
  vlen = vlen/2 - 1;
  fp_read_unsigned_bin(key->Q->x, unconst_uint8_t(d), vlen);
  d += vlen;
  fp_read_unsigned_bin(key->Q->y, unconst_uint8_t(d), vlen);
  d += vlen;
  fp_set(key->Q->z, 1);

  if (d != der_end)
    lose(HAL_ERROR_ASN1_PARSE_FAILED);

  *key_ = key;
  return HAL_OK;

 fail:
  memset(keybuf, 0, keybuf_len);
  return err;
}

/*
 * Write public key in SubjectPublicKeyInfo format, see RFCS 5280 and 5480.
 */

hal_error_t hal_ecdsa_public_key_to_der(const hal_ecdsa_key_t * const key,
                                        uint8_t *der, size_t *der_len, const size_t der_max)
{
  if (key == NULL || (key->type != HAL_KEY_TYPE_EC_PRIVATE &&
                      key->type != HAL_KEY_TYPE_EC_PUBLIC))
    return HAL_ERROR_BAD_ARGUMENTS;

  const ecdsa_curve_t * const curve = get_curve(key->curve);
  if (curve == NULL)
    return HAL_ERROR_IMPOSSIBLE;

  const size_t q_len  = fp_unsigned_bin_size(unconst_fp_int(curve->q));
  const size_t Qx_len = fp_unsigned_bin_size(unconst_fp_int(key->Q->x));
  const size_t Qy_len = fp_unsigned_bin_size(unconst_fp_int(key->Q->y));
  const size_t ecpoint_len = q_len * 2 + 1;
  hal_assert(q_len >= Qx_len && q_len >= Qy_len);

  if (der != NULL && ecpoint_len < der_max) {
    memset(der, 0, ecpoint_len);

    uint8_t *d = der;
    *d++ = 0x04;                /* Uncompressed */

    fp_to_unsigned_bin(unconst_fp_int(key->Q->x), d + q_len - Qx_len);
    d += q_len;

    fp_to_unsigned_bin(unconst_fp_int(key->Q->y), d + q_len - Qy_len);
    d += q_len;

    hal_assert(d < der + der_max);
  }

  return hal_asn1_encode_spki(hal_asn1_oid_ecPublicKey, hal_asn1_oid_ecPublicKey_len,
                              curve->oid, curve->oid_len,
                              der, ecpoint_len,
                              der, der_len, der_max);
}

/*
 * Convenience wrapper to return how many bytes a public key would
 * take if encoded as DER.
 */

size_t hal_ecdsa_public_key_to_der_len(const hal_ecdsa_key_t * const key)
{
  size_t len;
  return hal_ecdsa_public_key_to_der(key, NULL, &len, 0) == HAL_OK ? len : 0;
}

/*
 * Read public key in SubjectPublicKeyInfo format, see RFCS 5280 and 5480.
 */

hal_error_t hal_ecdsa_public_key_from_der(hal_ecdsa_key_t **key_,
                                           void *keybuf, const size_t keybuf_len,
                                           const uint8_t * const der, const size_t der_len)
{
  hal_ecdsa_key_t *key = keybuf;

  if (key_ == NULL || key == NULL || keybuf_len < sizeof(*key))
    return HAL_ERROR_BAD_ARGUMENTS;

  memset(keybuf, 0, keybuf_len);
  key->type = HAL_KEY_TYPE_EC_PUBLIC;

  const uint8_t *alg_oid = NULL, *curve_oid = NULL, *pubkey = NULL;
  size_t         alg_oid_len,     curve_oid_len,     pubkey_len;
  hal_error_t err;

  if ((err = hal_asn1_decode_spki(&alg_oid, &alg_oid_len, &curve_oid, &curve_oid_len,
                                  &pubkey, &pubkey_len,
                                  der, der_len)) != HAL_OK)
    return err;

  if (alg_oid == NULL || curve_oid == NULL || pubkey == NULL ||
      alg_oid_len != hal_asn1_oid_ecPublicKey_len ||
      memcmp(alg_oid, hal_asn1_oid_ecPublicKey, alg_oid_len) != 0 ||
      hal_ecdsa_oid_to_curve(&key->curve, curve_oid, curve_oid_len) != HAL_OK ||
      pubkey_len < 3 || (pubkey_len & 1) == 0 || pubkey[0] != 0x04 ||
      pubkey_len / 2 != (size_t)(fp_unsigned_bin_size(unconst_fp_int(get_curve(key->curve)->q))))
    return HAL_ERROR_ASN1_PARSE_FAILED;

  const uint8_t * const Qx = pubkey + 1;
  const uint8_t * const Qy = Qx + pubkey_len / 2;

  fp_read_unsigned_bin(key->Q->x, unconst_uint8_t(Qx), pubkey_len / 2);
  fp_read_unsigned_bin(key->Q->y, unconst_uint8_t(Qy), pubkey_len / 2);
  fp_set(key->Q->z, 1);

  *key_ = key;
  return HAL_OK;
}

/*
 * Encode a signature in PKCS #11 format: an octet string consisting
 * of concatenated values for r and s, each padded (if necessary) out
 * to the byte length of the order of the base point.
 */

static hal_error_t encode_signature_pkcs11(const ecdsa_curve_t * const curve,
                                           const fp_int * const r, const fp_int * const s,
                                           uint8_t *signature, size_t *signature_len, const size_t signature_max)
{
  hal_assert(curve != NULL && r != NULL && s != NULL);

  const size_t n_len = fp_unsigned_bin_size(unconst_fp_int(curve->n));
  const size_t r_len = fp_unsigned_bin_size(unconst_fp_int(r));
  const size_t s_len = fp_unsigned_bin_size(unconst_fp_int(s));

  if (n_len < r_len || n_len < s_len)
    return HAL_ERROR_IMPOSSIBLE;

  if (signature_len != NULL)
    *signature_len = n_len * 2;

  if (signature == NULL)
    return HAL_OK;

  if (signature_max < n_len * 2)
    return HAL_ERROR_RESULT_TOO_LONG;

  memset(signature, 0, n_len * 2);
  fp_to_unsigned_bin(unconst_fp_int(r), signature + 1 * n_len - r_len);
  fp_to_unsigned_bin(unconst_fp_int(s), signature + 2 * n_len - s_len);

  return HAL_OK;
}

/*
 * Decode a signature from PKCS #11 format: an octet string consisting
 * of concatenated values for r and s, each of which occupies half of
 * the octet string (which must therefore be of even length).
 */

static hal_error_t decode_signature_pkcs11(const ecdsa_curve_t * const curve,
                                           fp_int *r, fp_int *s,
                                           const uint8_t * const signature, const size_t signature_len)
{
  hal_assert(curve != NULL && r != NULL && s != NULL);

  if (signature == NULL || (signature_len & 1) != 0)
    return HAL_ERROR_BAD_ARGUMENTS;

  const size_t n_len = signature_len / 2;

  if (n_len > (size_t)(fp_unsigned_bin_size(unconst_fp_int(curve->n))))
    return HAL_ERROR_BAD_ARGUMENTS;

  fp_read_unsigned_bin(r, unconst_uint8_t(signature) + 0 * n_len, n_len);
  fp_read_unsigned_bin(s, unconst_uint8_t(signature) + 1 * n_len, n_len);

  return HAL_OK;
}

/*
 * Sign a caller-supplied hash.
 */

hal_error_t hal_ecdsa_sign(hal_core_t *core,
                           const hal_ecdsa_key_t * const key,
                           const uint8_t * const hash, const size_t hash_len,
                           uint8_t *signature, size_t *signature_len, const size_t signature_max)
{
  if (key == NULL || hash == NULL || signature == NULL || signature_len == NULL || key->type != HAL_KEY_TYPE_EC_PRIVATE)
    return HAL_ERROR_BAD_ARGUMENTS;

  const ecdsa_curve_t * const curve = get_curve(key->curve);
  if (curve == NULL)
    return HAL_ERROR_IMPOSSIBLE;

  fp_int k[1] = INIT_FP_INT;
  fp_int r[1] = INIT_FP_INT;
  fp_int s[1] = INIT_FP_INT;
  fp_int e[1] = INIT_FP_INT;

  fp_int * const n = unconst_fp_int(curve->n);
  fp_int * const d = unconst_fp_int(key->d);

  ec_point_t R[1] = INIT_EC_POINT_T;

  hal_error_t err;

  fp_read_unsigned_bin(e, unconst_uint8_t(hash), hash_len);

  do {

    /*
     * Pick random curve point R, then calculate r = Rx % n.
     * If r == 0, we can't use this point, so go try again.
     */

    if ((err = point_pick_random(curve, k, R)) != HAL_OK)
      goto fail;

    if (!point_is_on_curve(R, curve))
      lose(HAL_ERROR_IMPOSSIBLE);

    if (fp_mod(R->x, n, r) != FP_OKAY)
      lose(HAL_ERROR_IMPOSSIBLE);

    if (fp_iszero(r))
      continue;

    /*
     * Calculate s = ((e + dr)/k) % n.
     * If s == 0, we can't use this point, so go try again.
     */

    if (fp_mulmod (d, r, n, s) != FP_OKAY)
      lose(HAL_ERROR_IMPOSSIBLE);

    fp_add        (e, s, s);

    if (fp_mod    (s, n, s)    != FP_OKAY ||
        fp_invmod (k, n, k)    != FP_OKAY ||
        fp_mulmod (s, k, n, s) != FP_OKAY)
      lose(HAL_ERROR_IMPOSSIBLE);

  } while (fp_iszero(s));

  /*
   * Encode the signature, then we're done.
   */

  if ((err = encode_signature_pkcs11(curve, r, s, signature, signature_len, signature_max)) != HAL_OK)
    goto fail;

  err = HAL_OK;

 fail:
  fp_zero(k); fp_zero(r); fp_zero(s); fp_zero(e);
  memset(R, 0, sizeof(R));
  return err;
}

/*
 * Verify a signature using a caller-supplied hash.
 */

hal_error_t hal_ecdsa_verify(hal_core_t *core,
                             const hal_ecdsa_key_t * const key,
                             const uint8_t * const hash, const size_t hash_len,
                             const uint8_t * const signature, const size_t signature_len)
{
  hal_assert(key != NULL && hash != NULL && signature != NULL);

  const ecdsa_curve_t * const curve = get_curve(key->curve);

  if (curve == NULL)
    return HAL_ERROR_IMPOSSIBLE;

  if (!point_is_on_curve(key->Q, curve))
    return HAL_ERROR_KEY_NOT_ON_CURVE;

  fp_int * const n = unconst_fp_int(curve->n);

  hal_error_t err;

  fp_int r[1]  = INIT_FP_INT;
  fp_int s[1]  = INIT_FP_INT;
  fp_int e[1]  = INIT_FP_INT;
  fp_int w[1]  = INIT_FP_INT;
  fp_int u1[1] = INIT_FP_INT;
  fp_int u2[1] = INIT_FP_INT;
  fp_int v[1]  = INIT_FP_INT;

  ec_point_t u1G[1] = INIT_EC_POINT_T;
  ec_point_t u2Q[1] = INIT_EC_POINT_T;
  ec_point_t R[1]   = INIT_EC_POINT_T;

  /*
   * Start by decoding the signature.
   */

  if ((err = decode_signature_pkcs11(curve, r, s, signature, signature_len)) != HAL_OK)
    return err;

  /*
   * Check that r and s are in the allowed range, read the hash, then
   * compute:
   *
   * w  = 1 / s
   * u1 = e * w
   * u2 = r * w
   * R  = u1 * G + u2 * Q.
   */

  if (fp_cmp_d(r, 1) == FP_LT || fp_cmp(r, n) != FP_LT ||
      fp_cmp_d(s, 1) == FP_LT || fp_cmp(s, n) != FP_LT)
    return HAL_ERROR_INVALID_SIGNATURE;

  fp_read_unsigned_bin(e, unconst_uint8_t(hash), hash_len);

  if (fp_invmod(s, n, w)     != FP_OKAY ||
      fp_mulmod(e, w, n, u1) != FP_OKAY ||
      fp_mulmod(r, w, n, u2) != FP_OKAY)
    return HAL_ERROR_IMPOSSIBLE;

  fp_copy(unconst_fp_int(curve->Gx), u1G->x);
  fp_copy(unconst_fp_int(curve->Gy), u1G->y);
  fp_set(u1G->z, 1);

  if ((err = point_scalar_multiply(u1, u1G,    u1G, curve)) != HAL_OK ||
      (err = point_scalar_multiply(u2, key->Q, u2Q, curve)) != HAL_OK)
    return err;

  if (point_is_infinite(u1G))
    point_copy(u2Q, R);
  else if (point_is_infinite(u2Q))
    point_copy(u1G, R);
  else if ((err = point_to_montgomery(u1G, curve)) != HAL_OK ||
           (err = point_to_montgomery(u2Q, curve)) != HAL_OK)
    return err;
  else
    point_add(u1G, u2Q, R, curve);

  /*
   * Signature is OK if
   *   R is not the point at infinity, and
   *   Rx is congruent to r mod n.
   */

  if (point_is_infinite(R))
    return HAL_ERROR_INVALID_SIGNATURE;

  if ((err = point_to_affine(R, curve)) != HAL_OK)
    return err;

  if (fp_mod(R->x, n, v) != FP_OKAY)
    return HAL_ERROR_IMPOSSIBLE;

  return fp_cmp(v, r) == FP_EQ ? HAL_OK : HAL_ERROR_INVALID_SIGNATURE;
}

/*
 * Local variables:
 * indent-tabs-mode: nil
 * End:
 */
