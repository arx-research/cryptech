/*
 * rpc_pkcs1.c
 * -----------
 * PKCS #1 (RSA) support code layered on top of RPC hash API.
 *
 * Copyright (c) 2016, NORDUnet A/S All rights reserved.
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

#include "hal.h"
#include "hal_internal.h"

/*
 * Construct a PKCS #1 DigestInfo object.  This requires some (very
 * basic) ASN.1 encoding, which we perform inline.
 */

hal_error_t hal_rpc_pkcs1_construct_digestinfo(const hal_hash_handle_t handle,
					       uint8_t *digest_info, size_t *digest_info_len,
					       const size_t digest_info_max)
{
  hal_assert(digest_info != NULL && digest_info_len != NULL);

  hal_digest_algorithm_t alg;
  size_t len, alg_len;
  hal_error_t err;

  if ((err = hal_rpc_hash_get_algorithm(handle, &alg))                     != HAL_OK ||
      (err = hal_rpc_hash_get_digest_length(alg, &len))                    != HAL_OK ||
      (err = hal_rpc_hash_get_digest_algorithm_id(alg, NULL, &alg_len, 0)) != HAL_OK)
    return err;

  *digest_info_len = len + alg_len + 4;

  if (*digest_info_len >= digest_info_max)
    return HAL_ERROR_RESULT_TOO_LONG;

  hal_assert(*digest_info_len < 130);

  uint8_t *d = digest_info;

  *d++ = 0x30;                /* SEQUENCE */
  *d++ = (uint8_t) (*digest_info_len - 2);

  if ((err = hal_rpc_hash_get_digest_algorithm_id(alg, d, NULL, alg_len)) != HAL_OK)
    return err;
  d += alg_len;

  *d++ = 0x04;                /* OCTET STRING */
  *d++ = (uint8_t) len;

  hal_assert(digest_info + *digest_info_len == d + len);

  return hal_rpc_hash_finalize(handle, d, len);
}
