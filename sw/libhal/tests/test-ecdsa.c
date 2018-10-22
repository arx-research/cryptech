/*
 * test-ecdsa.c
 * ------------
 * Test harness for Cryptech ECDSA code.
 *
 * At the moment, the ECDSA code is a pure software implementation,
 * Verilog will be along eventually.
 *
 * Testing ECDSA is a bit tricky because ECDSA depends heavily on
 * using a new random secret for each signature.  So we can test some
 * things against the normal ECDSA implemenation, but some tests
 * require a side door replacement of the random number generator so
 * that we can use a known values from our test vector in place of the
 * random secret that would be used in real operation.  Test code for
 * the latter mode depends on the library having been compiled with
 * the testing hook enable, which it should not be for production use.
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

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <sys/time.h>

#include <hal.h>

#include "test-ecdsa.h"

#if HAL_ECDSA_DEBUG_ONLY_STATIC_TEST_VECTOR_RANDOM

/*
 * Code to let us replace ECDSA's random numbers with test data, if
 * the ECDSA library code has been compiled with support for this.
 */

typedef hal_error_t (*rng_override_test_function_t)(void *, const size_t);

extern rng_override_test_function_t hal_ecdsa_set_rng_override_test_function(rng_override_test_function_t new_func);

static const uint8_t               *next_random_value = NULL;
static size_t                       next_random_length = 0;

static hal_error_t next_random_handler(void *data, const size_t length)
{
  if (data == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  if (next_random_value == NULL || length < next_random_length)
    return HAL_ERROR_IMPOSSIBLE;

  memset(data, 0, length);
  memcpy(data + length - next_random_length, next_random_value, next_random_length);

  next_random_value  = NULL;
  next_random_length = 0;

  (void) hal_ecdsa_set_rng_override_test_function(0);

  return HAL_OK;
}

static void set_next_random(const uint8_t * const data, const size_t length)
{
  (void) hal_ecdsa_set_rng_override_test_function(next_random_handler);
  next_random_value  = data;
  next_random_length = length;
}

/*
 * Run one keygen test from test vectors.
 */

static int test_against_static_vectors(const ecdsa_tc_t * const tc)

{
  char fn[sizeof("test-ecdsa-private-key-xxxxxx.der")];
  hal_error_t err;
  FILE *f;

  printf("Starting static test vector tests for P-%lu\n", (unsigned long) (tc->d_len * 8));

  set_next_random(tc->d, tc->d_len);

  uint8_t keybuf1[hal_ecdsa_key_t_size];
  hal_ecdsa_key_t *key1 = NULL;

  if ((err = hal_ecdsa_key_gen(NULL, &key1, keybuf1, sizeof(keybuf1), tc->curve)) != HAL_OK)
    return printf("hal_ecdsa_key_gen() failed: %s\n", hal_error_string(err)), 0;

  uint8_t Qx[tc->Qx_len], Qy[tc->Qy_len];
  size_t Qx_len, Qy_len;

  if ((err = hal_ecdsa_key_get_public(key1, Qx, &Qx_len, sizeof(Qx), Qy, &Qy_len, sizeof(Qy))) != HAL_OK)
    return printf("hal_ecdsa_key_get_public() failed: %s\n", hal_error_string(err)), 0;

  if (tc->Qx_len != Qx_len || memcmp(tc->Qx, Qx, Qx_len) != 0)
    return printf("Qx mismatch\n"), 0;

  if (tc->Qy_len != Qy_len || memcmp(tc->Qy, Qy, Qy_len) != 0)
    return printf("Qy mismatch\n"), 0;

  if (hal_ecdsa_private_key_to_der_len(key1) != tc->key_len)
    return printf("DER Key length mismatch\n"), 0;

  uint8_t der[tc->key_len];
  size_t der_len;

  err = hal_ecdsa_private_key_to_der(key1, der, &der_len, sizeof(der));

  snprintf(fn, sizeof(fn), "test-ecdsa-private-key-p%u.der", (unsigned) tc->d_len * 8);

  if ((f = fopen(fn, "wb")) == NULL)
    return printf("Couldn't open %s: %s\n", fn, strerror(errno)), 0;

  if (fwrite(der, der_len, 1, f) != 1)
    return printf("Length mismatch writing %s\n", fn), 0;

  if (fclose(f) == EOF)
    return printf("Couldn't close %s: %s\n", fn, strerror(errno)), 0;

  /* Deferred error from hal_ecdsa_private_key_to_der() */
  if (err != HAL_OK)
    return printf("hal_ecdsa_private_key_to_der() failed: %s\n", hal_error_string(err)), 0;

  uint8_t keybuf2[hal_ecdsa_key_t_size];
  hal_ecdsa_key_t *key2 = NULL;

  if ((err = hal_ecdsa_private_key_from_der(&key2, keybuf2, sizeof(keybuf2), der, der_len)) != HAL_OK)
    return printf("hal_ecdsa_private_key_from_der() failed: %s\n", hal_error_string(err)), 0;

  if (memcmp(key1, key2, hal_ecdsa_key_t_size) != 0)
    return printf("Private key mismatch after read/write cycle\n"), 0;

  set_next_random(tc->k, tc->k_len);

  uint8_t sig[tc->sig_len + 4];
  size_t  sig_len;

  if ((err = hal_ecdsa_sign(NULL, key1, tc->H, tc->H_len, sig, &sig_len, sizeof(sig))) != HAL_OK)
    return printf("hal_ecdsa_sign() failed: %s\n", hal_error_string(err)), 0;

  if (sig_len != tc->sig_len || memcmp(sig, tc->sig, tc->sig_len) != 0)
    return printf("Signature mismatch\n"), 0;

  if ((err = hal_ecdsa_verify(NULL, key2, tc->H, tc->H_len, sig, sig_len)) != HAL_OK)
    return printf("hal_ecdsa_verify(private) failed: %s\n", hal_error_string(err)), 0;

  hal_ecdsa_key_clear(key2);
  key2 = NULL;

  if ((err = hal_ecdsa_key_load_private(&key2, keybuf2, sizeof(keybuf2), tc->curve,
                                        tc->Qx, tc->Qx_len, tc->Qy, tc->Qy_len, tc->d, tc->d_len)) != HAL_OK)
    return printf("hal_ecdsa_load_private() failed: %s\n", hal_error_string(err)), 0;

  if (memcmp(key1, key2, hal_ecdsa_key_t_size) != 0)
    return printf("Key mismatch after hal_ecdsa_load_private_key()\n"), 0;

  hal_ecdsa_key_clear(key2);
  key2 = NULL;

  if ((err = hal_ecdsa_key_load_public(&key2, keybuf2, sizeof(keybuf2), tc->curve,
                                       tc->Qx, tc->Qx_len, tc->Qy, tc->Qy_len)) != HAL_OK)
    return printf("hal_ecdsa_load_public() failed: %s\n", hal_error_string(err)), 0;

  if ((err = hal_ecdsa_verify(NULL, key2, tc->H, tc->H_len, sig, sig_len)) != HAL_OK)
    return printf("hal_ecdsa_verify(public) failed: %s\n", hal_error_string(err)), 0;

  uint8_t point[hal_ecdsa_key_to_ecpoint_len(key1)];
  size_t  point_len;

  if ((err = hal_ecdsa_key_to_ecpoint(key1, point, &point_len, sizeof(point))) != HAL_OK)
    return printf("hal_ecdsa_key_to_point() failed: %s\n", hal_error_string(err)), 0;

  hal_ecdsa_key_clear(key1);
  key1 = NULL;

  if ((err = hal_ecdsa_key_from_ecpoint(&key1, keybuf1, sizeof(keybuf1), point, point_len, tc->curve)) != HAL_OK)
    return printf("hal_ecdsa_key_from_point() failed: %s\n", hal_error_string(err)), 0;

  if (memcmp(key1, key2, hal_ecdsa_key_t_size) != 0)
    return printf("Public key mismatch after first read/write cycle\n"), 0;

  hal_ecdsa_key_clear(key2);
  key2 = NULL;

  err = hal_ecdsa_public_key_to_der(key1, der, &der_len, sizeof(der));

  snprintf(fn, sizeof(fn), "test-ecdsa-public-key-p%u.der", (unsigned) tc->d_len * 8);

  if ((f = fopen(fn, "wb")) == NULL)
    return printf("Couldn't open %s: %s\n", fn, strerror(errno)), 0;

  if (fwrite(der, der_len, 1, f) != 1)
    return printf("Length mismatch writing %s\n", fn), 0;

  if (fclose(f) == EOF)
    return printf("Couldn't close %s: %s\n", fn, strerror(errno)), 0;

  /* Deferred error from hal_ecdsa_public_key_to_der() */
  if (err != HAL_OK)
    return printf("hal_ecdsa_public_key_to_der() failed: %s\n", hal_error_string(err)), 0;

  if ((err = hal_ecdsa_public_key_from_der(&key2, keybuf2, sizeof(keybuf2), der, der_len)) != HAL_OK)
    return printf("hal_ecdsa_public_key_from_der() failed: %s\n", hal_error_string(err)), 0;

  if (memcmp(key1, key2, hal_ecdsa_key_t_size) != 0)
    return printf("Public key mismatch after second read/write cycle\n"), 0;

  hal_ecdsa_key_clear(key1);
  hal_ecdsa_key_clear(key2);

  return 1;
}

#endif /* HAL_ECDSA_DEBUG_ONLY_STATIC_TEST_VECTOR_RANDOM */

/*
 * Run one keygen/sign/verify test with a newly generated key.
 */

static int test_keygen_sign_verify(const hal_curve_name_t curve)

{
  const hal_hash_descriptor_t *hash_descriptor = NULL;
  uint8_t keybuf[hal_ecdsa_key_t_size];
  hal_ecdsa_key_t *key = NULL;
  hal_error_t err;

  switch (curve) {

  case HAL_CURVE_P256:
    printf("ECDSA P-256 key generation / signature / verification test\n");
    hash_descriptor = hal_hash_sha256;
    break;

  case HAL_CURVE_P384:
    printf("ECDSA P-384 key generation / signature / verification test\n");
    hash_descriptor = hal_hash_sha384;
    break;

  case HAL_CURVE_P521:
    printf("ECDSA P-521 key generation / signature / verification test\n");
    hash_descriptor = hal_hash_sha512;
    break;

  default:
    printf("Unsupported ECDSA curve type\n");
    return 0;
  }

  printf("Generating key\n");

  if ((err =  hal_ecdsa_key_gen(NULL, &key, keybuf, sizeof(keybuf), curve)) != HAL_OK)
    return printf("hal_ecdsa_key_gen() failed: %s\n", hal_error_string(err)), 0;

  printf("Generating digest\n");

  uint8_t hashbuf[hash_descriptor->digest_length];

  {
    const uint8_t plaintext[] = "So long, and thanks...";
    uint8_t statebuf[hash_descriptor->hash_state_length];
    hal_hash_state_t *state = NULL;

    if ((err = hal_hash_initialize(NULL, hash_descriptor, &state, statebuf, sizeof(statebuf))) != HAL_OK ||
        (err = hal_hash_update(state, plaintext, strlen((const char *) plaintext))) != HAL_OK ||
        (err = hal_hash_finalize(state, hashbuf, sizeof(hashbuf))) != HAL_OK)
      return printf("Couldn't hash plaintext: %s\n", hal_error_string(err)), 0;
  }

  /*
   * Lazy but probably-good-enough guess on signature size -- want
   * explicit number in ecdsa_curve_t?
   */
  uint8_t sigbuf[hash_descriptor->digest_length * 3];
  size_t  siglen;

  printf("Signing\n");

  if ((err = hal_ecdsa_sign(NULL, key, hashbuf, sizeof(hashbuf),
                            sigbuf, &siglen, sizeof(sigbuf))) != HAL_OK)
    return printf("hal_ecdsa_sign() failed: %s\n", hal_error_string(err)), 0;

  printf("Verifying\n");

  if ((err = hal_ecdsa_verify(NULL, key, hashbuf, sizeof(hashbuf), sigbuf, siglen)) != HAL_OK)
    return printf("hal_ecdsa_verify() failed: %s\n", hal_error_string(err)), 0;

  return 1;
}

/*
 * Time a test.
 */

static void _time_check(const struct timeval t0, const int ok)
{
  struct timeval t;
  gettimeofday(&t, NULL);
  t.tv_sec -= t0.tv_sec;
  t.tv_usec -= t0.tv_usec;
  if (t.tv_usec < 0) {
    t.tv_usec += 1000000;
    t.tv_sec  -= 1;
  }
  printf("Elapsed time %lu.%06lu seconds, %s\n",
         (unsigned long) t.tv_sec,
         (unsigned long) t.tv_usec,
         ok ? "OK" : "FAILED");
}

#define time_check(_expr_)                      \
  do {                                          \
    struct timeval _t;                          \
    gettimeofday(&_t, NULL);                    \
    int _ok = (_expr_);                         \
    _time_check(_t, _ok);                       \
    ok &= _ok;                                  \
  } while (0)


static void show_core(const hal_core_t *core, const char *whinge)
{
  const hal_core_info_t *core_info = hal_core_info(core);
  if (core_info != NULL)
    printf("\"%8.8s\"  \"%4.4s\"\n", core_info->name, core_info->version);
  else if (whinge != NULL)
    printf("%s core not present\n", whinge);
}

int main(void)
{
  const hal_core_t *sha256_core = hal_core_find(SHA256_NAME, NULL);
  const hal_core_t *sha512_core = hal_core_find(SHA512_NAME, NULL);
  const hal_core_t *csprng_core = hal_core_find(CSPRNG_NAME, NULL);

  show_core(sha256_core, "sha-256");
  show_core(sha512_core, "sha-512");
  show_core(csprng_core, "csprng");

  int ok = 1;

#if HAL_ECDSA_DEBUG_ONLY_STATIC_TEST_VECTOR_RANDOM
  /*
   * Test vectors (where we have them).
   */
  for (int i = 0; i < sizeof(ecdsa_tc)/sizeof(*ecdsa_tc); i++)
    time_check(test_against_static_vectors(&ecdsa_tc[i]));
#endif

  /*
   * Generate/sign/verify test for each curve.
   */

  if (csprng_core != NULL && sha256_core != NULL) {
    time_check(test_keygen_sign_verify(HAL_CURVE_P256));
  }

  if (csprng_core != NULL && sha512_core != NULL) {
    time_check(test_keygen_sign_verify(HAL_CURVE_P384));
    time_check(test_keygen_sign_verify(HAL_CURVE_P521));
  }

  return !ok;
}

/*
 * Local variables:
 * indent-tabs-mode: nil
 * End:
 */
