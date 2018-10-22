/*
 * hal_rpc.c
 * ---------
 * Remote procedure call public API implementation.
 *
 * Authors: Rob Austein
 * Copyright (c) 2015-2016, NORDUnet A/S All rights reserved.
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
#include "hashsig.h"

const hal_hash_handle_t hal_hash_handle_none = {HAL_HANDLE_NONE};

/*
 * PIN lengths.  These are somewhat arbitrary, and the current values
 * are really placeholders until we figure out something better.
 * Minimum length here is almost certainly too short for production
 * use, we allow it because most test programs fail if we insist on a
 * PIN long enough to have any real security.
 */

#ifndef HAL_PIN_MINIMUM_LENGTH
#define HAL_PIN_MINIMUM_LENGTH          4
#endif

#ifndef HAL_PIN_MAXIMUM_LENGTH
#define HAL_PIN_MAXIMUM_LENGTH          4096
#endif

const size_t hal_rpc_min_pin_length = HAL_PIN_MINIMUM_LENGTH;
const size_t hal_rpc_max_pin_length = HAL_PIN_MAXIMUM_LENGTH;

static inline int check_pkey_type(const hal_key_type_t type)
{
  switch (type) {
  case HAL_KEY_TYPE_RSA_PRIVATE:
  case HAL_KEY_TYPE_RSA_PUBLIC:
  case HAL_KEY_TYPE_EC_PRIVATE:
  case HAL_KEY_TYPE_EC_PUBLIC:
  case HAL_KEY_TYPE_HASHSIG_PRIVATE:
  case HAL_KEY_TYPE_HASHSIG_PUBLIC:
  case HAL_KEY_TYPE_HASHSIG_LMS:
  case HAL_KEY_TYPE_HASHSIG_LMOTS:
    return 1;
  default:
    return 0;
  }
}

static inline int check_pkey_flags(const hal_key_flags_t flags)
{
  return (flags &~ (HAL_KEY_FLAG_USAGE_DIGITALSIGNATURE |
                    HAL_KEY_FLAG_USAGE_KEYENCIPHERMENT  |
                    HAL_KEY_FLAG_USAGE_DATAENCIPHERMENT |
                    HAL_KEY_FLAG_TOKEN                  |
                    HAL_KEY_FLAG_PUBLIC                 |
                    HAL_KEY_FLAG_EXPORTABLE)) == 0;
}

static inline int check_pkey_type_curve_flags(const hal_key_type_t type,
                                              const hal_curve_name_t curve,
                                              const hal_key_flags_t flags)
{
  if (!check_pkey_flags(flags))
    return 0;

  switch (type) {

  case HAL_KEY_TYPE_RSA_PRIVATE:
  case HAL_KEY_TYPE_RSA_PUBLIC:
  case HAL_KEY_TYPE_HASHSIG_PRIVATE:
  case HAL_KEY_TYPE_HASHSIG_PUBLIC:
  case HAL_KEY_TYPE_HASHSIG_LMS:
  case HAL_KEY_TYPE_HASHSIG_LMOTS:
    return curve == HAL_CURVE_NONE;

  case HAL_KEY_TYPE_EC_PRIVATE:
  case HAL_KEY_TYPE_EC_PUBLIC:
    switch (curve) {
    case HAL_CURVE_P256:
    case HAL_CURVE_P384:
    case HAL_CURVE_P521:
      return 1;
    default:
      return 0;
    }

  default:
    return 0;
  }
}


hal_error_t hal_rpc_get_version(uint32_t *version)
{
  return hal_rpc_misc_dispatch->get_version(version);
}

hal_error_t hal_rpc_get_random(void *buffer, const size_t length)
{
  if (buffer == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;
  if (length == 0)
    return HAL_OK;
  return hal_rpc_misc_dispatch->get_random(buffer, length);
}

hal_error_t hal_rpc_set_pin(const hal_client_handle_t client,
                            const hal_user_t user,
                            const char * const newpin, const size_t newpin_len)
{
  if (newpin == NULL ||
      newpin_len < hal_rpc_min_pin_length ||
      newpin_len > hal_rpc_max_pin_length ||
      (user != HAL_USER_NORMAL && user != HAL_USER_SO && user != HAL_USER_WHEEL))
    return HAL_ERROR_BAD_ARGUMENTS;
  return hal_rpc_misc_dispatch->set_pin(client, user, newpin, newpin_len);
}

hal_error_t hal_rpc_login(const hal_client_handle_t client,
                          const hal_user_t user,
                          const char * const pin, const size_t pin_len)
{
  if (pin == NULL ||
      pin_len < hal_rpc_min_pin_length ||
      pin_len > hal_rpc_max_pin_length ||
      (user != HAL_USER_NORMAL && user != HAL_USER_SO && user != HAL_USER_WHEEL))
    return HAL_ERROR_BAD_ARGUMENTS;
  return hal_rpc_misc_dispatch->login(client, user, pin, pin_len);
}

hal_error_t hal_rpc_logout(const hal_client_handle_t client)
{
  return hal_rpc_misc_dispatch->logout(client);
}

hal_error_t hal_rpc_logout_all(void)
{
  return hal_rpc_misc_dispatch->logout_all();
}

hal_error_t hal_rpc_is_logged_in(const hal_client_handle_t client,
                                 const hal_user_t user)
{
  if (user != HAL_USER_NORMAL && user != HAL_USER_SO && user != HAL_USER_WHEEL)
    return HAL_ERROR_BAD_ARGUMENTS;
  return hal_rpc_misc_dispatch->is_logged_in(client, user);
}

hal_error_t hal_rpc_hash_get_digest_length(const hal_digest_algorithm_t alg, size_t *length)
{
  if (length == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;
  return hal_rpc_hash_dispatch->get_digest_length(alg, length);
}

hal_error_t hal_rpc_hash_get_digest_algorithm_id(const hal_digest_algorithm_t alg,
                                                 uint8_t *id, size_t *len, const size_t len_max)
{
  return hal_rpc_hash_dispatch->get_digest_algorithm_id(alg, id, len, len_max);
}

hal_error_t hal_rpc_hash_get_algorithm(const hal_hash_handle_t hash, hal_digest_algorithm_t *alg)
{
  if (hash.handle == HAL_HANDLE_NONE || alg == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;
  return hal_rpc_hash_dispatch->get_algorithm(hash, alg);
}

hal_error_t hal_rpc_hash_initialize(const hal_client_handle_t client,
                                    const hal_session_handle_t session,
                                    hal_hash_handle_t *hash,
                                    const hal_digest_algorithm_t alg,
                                    const uint8_t * const key, const size_t key_len)
{
  if (hash == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;
  return hal_rpc_hash_dispatch->initialize(client, session, hash, alg, key, key_len);
}

hal_error_t hal_rpc_hash_update(const hal_hash_handle_t hash,
                                const uint8_t * data, const size_t length)
{
  if (hash.handle == HAL_HANDLE_NONE || data == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;
  if (length == 0)
    return HAL_OK;
  return hal_rpc_hash_dispatch->update(hash, data, length);
}

hal_error_t hal_rpc_hash_finalize(const hal_hash_handle_t hash,
                                  uint8_t *digest, const size_t length)
{
  if (hash.handle == HAL_HANDLE_NONE || digest == NULL || length == 0)
    return HAL_ERROR_BAD_ARGUMENTS;
  return hal_rpc_hash_dispatch->finalize(hash, digest, length);
}

hal_error_t hal_rpc_pkey_load(const hal_client_handle_t client,
                              const hal_session_handle_t session,
                              hal_pkey_handle_t *pkey,
                              hal_uuid_t *name,
                              const uint8_t * const der, const size_t der_len,
                              const hal_key_flags_t flags)
{
  if (pkey == NULL || name == NULL || der == NULL || der_len == 0 || !check_pkey_flags(flags))
    return HAL_ERROR_BAD_ARGUMENTS;
  return hal_rpc_pkey_dispatch->load(client, session, pkey, name, der, der_len, flags);
}

hal_error_t hal_rpc_pkey_open(const hal_client_handle_t client,
                              const hal_session_handle_t session,
                              hal_pkey_handle_t *pkey,
                              const hal_uuid_t * const name)
{
  if (pkey == NULL || name == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;
  return hal_rpc_pkey_dispatch->open(client, session, pkey, name);
}

hal_error_t hal_rpc_pkey_generate_rsa(const hal_client_handle_t client,
                                      const hal_session_handle_t session,
                                      hal_pkey_handle_t *pkey,
                                      hal_uuid_t *name,
                                      const unsigned key_len,
                                      const uint8_t * const exp, const size_t exp_len,
                                      const hal_key_flags_t flags)
{
  if (pkey == NULL || name == NULL || key_len == 0 || (key_len & 7) != 0 ||
      exp == NULL || exp_len == 0 || !check_pkey_flags(flags))
    return HAL_ERROR_BAD_ARGUMENTS;
  return hal_rpc_pkey_dispatch->generate_rsa(client, session, pkey, name, key_len, exp, exp_len, flags);
}

hal_error_t hal_rpc_pkey_generate_ec(const hal_client_handle_t client,
                                     const hal_session_handle_t session,
                                     hal_pkey_handle_t *pkey,
                                     hal_uuid_t *name,
                                     const hal_curve_name_t curve,
                                     const hal_key_flags_t flags)
{
  if (pkey == NULL || name == NULL ||
      !check_pkey_type_curve_flags(HAL_KEY_TYPE_EC_PRIVATE, curve, flags))
    return HAL_ERROR_BAD_ARGUMENTS;
  return hal_rpc_pkey_dispatch->generate_ec(client, session, pkey, name, curve, flags);
}

hal_error_t hal_rpc_pkey_generate_hashsig(const hal_client_handle_t client,
                                          const hal_session_handle_t session,
                                          hal_pkey_handle_t *pkey,
                                          hal_uuid_t *name,
                                          const size_t hss_levels,
                                          const hal_lms_algorithm_t lms_type,
                                          const hal_lmots_algorithm_t lmots_type,
                                          const hal_key_flags_t flags)
{
  if (pkey == NULL || name == NULL || !check_pkey_flags(flags))
    return HAL_ERROR_BAD_ARGUMENTS;
  return hal_rpc_pkey_dispatch->generate_hashsig(client, session, pkey, name, hss_levels, lms_type, lmots_type, flags);
}

hal_error_t hal_rpc_pkey_close(const hal_pkey_handle_t pkey)
{
  return hal_rpc_pkey_dispatch->close(pkey);
}

hal_error_t hal_rpc_pkey_delete(const hal_pkey_handle_t pkey)
{
  return hal_rpc_pkey_dispatch->delete(pkey);
}

hal_error_t hal_rpc_pkey_get_key_type(const hal_pkey_handle_t pkey,
                                      hal_key_type_t *type)
{
  if (type == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;
  return hal_rpc_pkey_dispatch->get_key_type(pkey, type);
}

hal_error_t hal_rpc_pkey_get_key_curve(const hal_pkey_handle_t pkey,
                                       hal_curve_name_t *curve)
{
  if (curve == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;
  return hal_rpc_pkey_dispatch->get_key_curve(pkey, curve);
}

hal_error_t hal_rpc_pkey_get_key_flags(const hal_pkey_handle_t pkey,
                                       hal_key_flags_t *flags)
{
  if (flags == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;
  return hal_rpc_pkey_dispatch->get_key_flags(pkey, flags);
}

size_t hal_rpc_pkey_get_public_key_len(const hal_pkey_handle_t pkey)
{
  return hal_rpc_pkey_dispatch->get_public_key_len(pkey);
}

hal_error_t hal_rpc_pkey_get_public_key(const hal_pkey_handle_t pkey,
                                        uint8_t *der, size_t *der_len, const size_t der_max)
{
  if (der == NULL || der_len == NULL || der_max == 0)
    return HAL_ERROR_BAD_ARGUMENTS;
  return hal_rpc_pkey_dispatch->get_public_key(pkey, der, der_len, der_max);
}

hal_error_t hal_rpc_pkey_sign(const hal_pkey_handle_t pkey,
                              const hal_hash_handle_t hash,
                              const uint8_t * const input,  const size_t input_len,
                              uint8_t * signature, size_t *signature_len, const size_t signature_max)
{
  if (signature == NULL || signature_len == NULL || signature_max == 0 ||
      (hash.handle == HAL_HANDLE_NONE) == (input == NULL || input_len == 0))
    return HAL_ERROR_BAD_ARGUMENTS;
  return hal_rpc_pkey_dispatch->sign(pkey, hash, input,  input_len, signature, signature_len, signature_max);
}

hal_error_t hal_rpc_pkey_verify(const hal_pkey_handle_t pkey,
                                const hal_hash_handle_t hash,
                                const uint8_t * const input, const size_t input_len,
                                const uint8_t * const signature, const size_t signature_len)
{
  if (signature == NULL || signature_len == 0 ||
      (hash.handle == HAL_HANDLE_NONE) == (input == NULL || input_len == 0))
    return HAL_ERROR_BAD_ARGUMENTS;
  return hal_rpc_pkey_dispatch->verify(pkey, hash, input, input_len, signature, signature_len);
}

hal_error_t hal_rpc_pkey_match(const hal_client_handle_t client,
                               const hal_session_handle_t session,
                               const hal_key_type_t type,
                               const hal_curve_name_t curve,
                               const hal_key_flags_t mask,
                               const hal_key_flags_t flags,
                               const hal_pkey_attribute_t *attributes,
                               const unsigned attributes_len,
                               unsigned *state,
                               hal_uuid_t *result,
                               unsigned *result_len,
                               const unsigned result_max,
                               const hal_uuid_t * const previous_uuid)
{
  if ((attributes == NULL && attributes_len > 0) || previous_uuid == NULL ||
      state == NULL || result == NULL || result_len == NULL || result_max == 0)
    return HAL_ERROR_BAD_ARGUMENTS;

  if (attributes != NULL)
    for (size_t i = 0; i < attributes_len; i++)
      if (attributes[i].value == NULL)
        return HAL_ERROR_BAD_ARGUMENTS;

  return hal_rpc_pkey_dispatch->match(client, session, type, curve, mask, flags,
                                      attributes, attributes_len,
                                      state, result, result_len, result_max, previous_uuid);
}

hal_error_t hal_rpc_pkey_set_attributes(const hal_pkey_handle_t pkey,
                                        const hal_pkey_attribute_t *attributes,
                                        const unsigned attributes_len)
{
  if (attributes == NULL || attributes_len == 0)
    return HAL_ERROR_BAD_ARGUMENTS;
  return hal_rpc_pkey_dispatch->set_attributes(pkey, attributes, attributes_len);
}

hal_error_t hal_rpc_pkey_get_attributes(const hal_pkey_handle_t pkey,
                                        hal_pkey_attribute_t *attributes,
                                        const unsigned attributes_len,
                                        uint8_t *attributes_buffer,
                                        const size_t attributes_buffer_len)
{
  if (attributes == NULL || attributes_len == 0)
    return HAL_ERROR_BAD_ARGUMENTS;
  return hal_rpc_pkey_dispatch->get_attributes(pkey, attributes, attributes_len,
                                               attributes_buffer, attributes_buffer_len);
}

hal_error_t hal_rpc_pkey_export(const hal_pkey_handle_t pkey,
                                const hal_pkey_handle_t kekek,
                                uint8_t *pkcs8, size_t *pkcs8_len, const size_t pkcs8_max,
                                uint8_t *kek,   size_t *kek_len,   const size_t kek_max)
{
  if (pkcs8 == NULL || pkcs8_len == NULL || kek == NULL || kek_len == NULL || kek_max <= KEK_LENGTH)
    return HAL_ERROR_BAD_ARGUMENTS;
  return hal_rpc_pkey_dispatch->export(pkey, kekek, pkcs8, pkcs8_len, pkcs8_max, kek, kek_len, kek_max);
}

hal_error_t hal_rpc_pkey_import(const hal_client_handle_t client,
                                const hal_session_handle_t session,
                                hal_pkey_handle_t *pkey,
                                hal_uuid_t *name,
                                const hal_pkey_handle_t kekek,
                                const uint8_t * const pkcs8, const size_t pkcs8_len,
                                const uint8_t * const kek,   const size_t kek_len,
                                const hal_key_flags_t flags)
{
  if (pkey == NULL || name == NULL || pkcs8 == NULL || kek == NULL || kek_len <= 2)
    return HAL_ERROR_BAD_ARGUMENTS;
  return hal_rpc_pkey_dispatch->import(client, session, pkey, name, kekek, pkcs8, pkcs8_len, kek, kek_len, flags);
}

/*
 * Local variables:
 * indent-tabs-mode: nil
 * End:
 */
