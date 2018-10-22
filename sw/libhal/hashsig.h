/*
 * hashsig.h
 * ---------
 * Implementation of draft-mcgrew-hash-sigs-08.txt
 *
 * Copyright (c) 2018, NORDUnet A/S All rights reserved.
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

#ifndef _HAL_HASHSIG_H_
#define _HAL_HASHSIG_H_

typedef struct hal_hashsig_key hal_hashsig_key_t;

extern const size_t hal_hashsig_key_t_size;

extern hal_error_t hal_hashsig_key_gen(hal_core_t *core,
                                       hal_hashsig_key_t **key_,
                                       const size_t hss_levels,
                                       const hal_lms_algorithm_t lms_type,
                                       const hal_lmots_algorithm_t lmots_type);

extern hal_error_t hal_hashsig_key_delete(const hal_hashsig_key_t * const key);

extern hal_error_t hal_hashsig_private_key_to_der(const hal_hashsig_key_t * const key,
                                                  uint8_t *der, size_t *der_len, const size_t der_max);

extern size_t hal_hashsig_private_key_to_der_len(const hal_hashsig_key_t * const key);

extern hal_error_t hal_hashsig_private_key_from_der(hal_hashsig_key_t **key_,
                                                    void *keybuf, const size_t keybuf_len,
                                                    const uint8_t *der, const size_t der_len);

extern hal_error_t hal_hashsig_public_key_to_der(const hal_hashsig_key_t * const key,
                                                 uint8_t *der, size_t *der_len, const size_t der_max);

extern size_t hal_hashsig_public_key_to_der_len(const hal_hashsig_key_t * const key);

extern hal_error_t hal_hashsig_public_key_from_der(hal_hashsig_key_t **key,
                                                   void *keybuf, const size_t keybuf_len,
                                                   const uint8_t * const der, const size_t der_len);

extern hal_error_t hal_hashsig_sign(hal_core_t *core,
                                    const hal_hashsig_key_t * const key,
                                    const uint8_t * const hash, const size_t hash_len,
                                    uint8_t *signature, size_t *signature_len, const size_t signature_max);

extern hal_error_t hal_hashsig_verify(hal_core_t *core,
                                      const hal_hashsig_key_t * const key,
                                      const uint8_t * const hash, const size_t hash_len,
                                      const uint8_t * const signature, const size_t signature_len);

extern hal_error_t hal_hashsig_key_load_public(hal_hashsig_key_t **key_,
                                               void *keybuf, const size_t keybuf_len,
                                               const size_t L,
                                               const hal_lms_algorithm_t lms_type,
                                               const hal_lmots_algorithm_t lmots_type,
                                               const uint8_t * const I, const size_t I_len,
                                               const uint8_t * const T1, const size_t T1_len);

extern hal_error_t hal_hashsig_key_load_public_xdr(hal_hashsig_key_t **key_,
                                                   void *keybuf, const size_t keybuf_len,
                                                   const uint8_t * const xdr, const size_t xdr_len);

extern size_t hal_hashsig_signature_len(const size_t L,
                                        const hal_lms_algorithm_t lms_type,
                                        const hal_lmots_algorithm_t lmots_type);

extern size_t hal_hashsig_lmots_private_key_len(const hal_lmots_algorithm_t lmots_type);

extern hal_error_t hal_hashsig_public_key_der_to_xdr(const uint8_t * const der, const size_t der_len,
                                                     uint8_t * const xdr, size_t * const xdr_len , const size_t xdr_max);

extern hal_error_t hal_hashsig_ks_init(void);

#endif /* _HAL_HASHSIG_H_ */
