/*
 * pbkdf2.c
 * --------
 * PBKDF2 (RFC 2898) on top of HAL interface to Cryptech hash cores.
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

#include <string.h>
#include <stdint.h>

#include "hal.h"
#include "hal_internal.h"

/*
 * Utility to encapsulate the HMAC operations.  May need refactoring
 * if and when we get clever about reusing HMAC state for speed.
 */

static hal_error_t do_hmac(hal_core_t *core,
                           const hal_hash_descriptor_t * const d,
                           const uint8_t * const pw,   const size_t pw_len,
                           const uint8_t * const data, const size_t data_len,
                           const uint32_t  block,
                                 uint8_t * mac,        const size_t mac_len)
{
  hal_assert(d != NULL && pw != NULL && data != NULL && mac != NULL);

  uint8_t sb[d->hmac_state_length];
  hal_hmac_state_t *s;
  hal_error_t err;

  if ((err = hal_hmac_initialize(core, d, &s, sb, sizeof(sb), pw, pw_len)) != HAL_OK)
    return err;

  if ((err = hal_hmac_update(s, data, data_len)) != HAL_OK)
    return err;

  if (block > 0) {
    uint8_t b[4] = { (block >> 24) & 0xFF, (block >> 16) & 0xFF, (block >> 8) & 0xFF, (block >> 0) & 0xFF };
    if ((err = hal_hmac_update(s, b, sizeof(b))) != HAL_OK)
      return err;
  }

  return hal_hmac_finalize(s, mac, mac_len);
}

/*
 * Derive a key from a passphrase using the PBKDF2 algorithm.
 */

hal_error_t hal_pbkdf2(hal_core_t *core,
                       const hal_hash_descriptor_t * const descriptor,
                       const uint8_t * const password, const size_t password_length,
                       const uint8_t * const salt,     const size_t salt_length,
                       uint8_t       * derived_key,          size_t derived_key_length,
                       unsigned iterations_desired)
{
  uint8_t result[HAL_MAX_HASH_DIGEST_LENGTH], mac[HAL_MAX_HASH_DIGEST_LENGTH];
  uint8_t statebuf[1024];
  unsigned iteration;
  hal_error_t err;
  uint32_t block;

  if (descriptor == NULL || password == NULL || salt == NULL ||
      derived_key == NULL || derived_key_length == 0 ||
      iterations_desired == 0)
    return HAL_ERROR_BAD_ARGUMENTS;

  hal_assert(sizeof(statebuf) >= descriptor->hmac_state_length);
  hal_assert(sizeof(result)   >= descriptor->digest_length);
  hal_assert(sizeof(mac)      >= descriptor->digest_length);

  /* Output length check per RFC 2989 5.2. */
  if ((uint64_t) derived_key_length > ((uint64_t) 0xFFFFFFFF) * descriptor->block_length)
    return HAL_ERROR_UNSUPPORTED_KEY;

  memset(result, 0, sizeof(result));
  memset(mac,    0, sizeof(mac));

  /*
   * We probably should check here to see whether the password is
   * longer than the HMAC block size, and, if so, we should hash the
   * password here to avoid having recomputing that every time through
   * the loops below.  There are other optimizations we'd like to
   * make, but this one doesn't require being able to save and restore
   * the hash state.
   */

  /*
   * Generate output blocks until we reach the requested length.
   */

  for (block = 1; ; block++) {

    /*
     * Initial HMAC is of the salt concatenated with the block count.
     * This seeds the result, and constitutes iteration one.
     */

    if ((err = do_hmac(core, descriptor, password, password_length,
                       salt, salt_length, block, mac, sizeof(mac))) != HAL_OK)
      return err;

    memcpy(result, mac, descriptor->digest_length);

    /*
     * Now iterate however many times the caller requested, XORing the
     * HMAC back into the result on each iteration.
     */

    for (iteration = 2; iteration <= iterations_desired; iteration++) {

      hal_task_yield_maybe();

      if ((err = do_hmac(core, descriptor, password, password_length,
                         mac, descriptor->digest_length,
                         0, mac, sizeof(mac))) != HAL_OK)
        return err;

      for (size_t i = 0; i < descriptor->digest_length; i++)
        result[i] ^= mac[i];
    }

    /*
     * Save result block, then exit or loop for another block.
     */

    if (derived_key_length > descriptor->digest_length) {
      memcpy(derived_key, result, descriptor->digest_length);
      derived_key              += descriptor->digest_length;
      derived_key_length       -= descriptor->digest_length;
    }
    else {
      memcpy(derived_key, result, derived_key_length);
      return HAL_OK;
    }
  }
}

/*
 * Local variables:
 * indent-tabs-mode: nil
 * End:
 */
