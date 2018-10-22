/*
 * rpc_pkey.c
 * ----------
 * Remote procedure call server-side public key implementation.
 *
 * Authors: Rob Austein
 * Copyright (c) 2015, NORDUnet A/S All rights reserved.
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

#include "hal.h"
#include "hal_internal.h"
#include "asn1_internal.h"
#include "hashsig.h"

#ifndef HAL_STATIC_PKEY_STATE_BLOCKS
#define HAL_STATIC_PKEY_STATE_BLOCKS 0
#endif

#if HAL_STATIC_PKEY_STATE_BLOCKS > 0
static hal_pkey_slot_t pkey_slot[HAL_STATIC_PKEY_STATE_BLOCKS];
#endif

/*
 * Handle allocation is simple: look for an unused (HAL_HANDLE_NONE)
 * slot in the table, and, assuming we find one, construct a composite
 * handle consisting of the index into the table and a counter whose
 * sole purpose is to keep the same handle from reoccurring anytime
 * soon, to help identify use-after-free bugs in calling code.
 *
 * The high order bit of the pkey handle is left free for
 * HAL_PKEY_HANDLE_TOKEN_FLAG, which is used by the mixed-mode
 * handlers to route calls to the appropriate destination.  In most
 * cases this flag is set here, but pkey_local_open() also sets it
 * directly, so that we can present a unified UUID namespace
 * regardless of which keystore holds a particular key.
 */

static inline hal_pkey_slot_t *alloc_slot(const hal_key_flags_t flags)
{
  hal_pkey_slot_t *slot = NULL;
  hal_critical_section_start();

#if HAL_STATIC_PKEY_STATE_BLOCKS > 0
  static uint16_t next_glop = 0;
  uint32_t glop = ++next_glop << 16;
  next_glop %= 0x7FFF;

  if ((flags & HAL_KEY_FLAG_TOKEN) != 0)
    glop |= HAL_PKEY_HANDLE_TOKEN_FLAG;

  for (size_t i = 0; slot == NULL && i < sizeof(pkey_slot)/sizeof(*pkey_slot); i++) {
    if (pkey_slot[i].pkey.handle != HAL_HANDLE_NONE)
      continue;
    memset(&pkey_slot[i], 0, sizeof(pkey_slot[i]));
    pkey_slot[i].pkey.handle = i | glop;
    pkey_slot[i].hint = -1;
    slot = &pkey_slot[i];
  }
#endif

  hal_critical_section_end();
  return slot;
}

/*
 * Clear a slot.  Probably not necessary to do this in a critical
 * section, but be safe.
 */

static inline void clear_slot(hal_pkey_slot_t *slot)
{
  hal_critical_section_start();

  if (slot != NULL)
    memset(slot, 0, sizeof(*slot));

  hal_critical_section_end();
}

/*
 * Check a caller-supplied handle.  Must be in range, in use, and have
 * the right glop.  Returns slot pointer on success, NULL otherwise.
 */

static inline hal_pkey_slot_t *find_handle(const hal_pkey_handle_t handle)
{
  hal_pkey_slot_t *slot = NULL;
  hal_critical_section_start();

#if HAL_STATIC_PKEY_STATE_BLOCKS > 0
  const size_t i = handle.handle & 0xFFFF;

  if (i < sizeof(pkey_slot)/sizeof(*pkey_slot) && pkey_slot[i].pkey.handle == handle.handle)
    slot = &pkey_slot[i];
#endif

  hal_critical_section_end();
  return slot;
}

/*
 * Clean up key state associated with a client when logging out.
 */

hal_error_t hal_pkey_logout(const hal_client_handle_t client)
{
  if (client.handle == HAL_HANDLE_NONE)
    return HAL_OK;

  hal_error_t err;

  if ((err = hal_ks_logout(hal_ks_volatile, client)) != HAL_OK ||
      (err = hal_ks_logout(hal_ks_token,    client)) != HAL_OK)
    return err;

  hal_critical_section_start();

  for (size_t i = 0; i < sizeof(pkey_slot)/sizeof(*pkey_slot); i++)
    if (pkey_slot[i].pkey.handle == client.handle)
      memset(&pkey_slot[i], 0, sizeof(pkey_slot[i]));

  hal_critical_section_end();

  return HAL_OK;
}

/*
 * Access rules are a bit complicated, mostly due to PKCS #11.
 *
 * The simple, obvious rule would be that one must be logged in as
 * HAL_USER_NORMAL to create, see, use, or delete a key, full stop.
 *
 * That's almost the rule that PKCS #11 follows for so-called
 * "private" objects (CKA_PRIVATE = CK_TRUE), but PKCS #11 has a more
 * complex model which not only allows wider visibility to "public"
 * objects (CKA_PRIVATE = CK_FALSE) but also allows write access to
 * "public session" (CKA_PRIVATE = CK_FALSE, CKA_TOKEN = CK_FALSE)
 * objects regardless of login state.
 *
 * PKCS #11 also has a concept of read-only sessions, which we don't
 * bother to implement at all on the HSM, since the PIN is required to
 * be the same as for the corresponding read-write session, so this
 * would just be additional compexity without adding any security on
 * the HSM; the PKCS #11 library still has to support read-only
 * sessions, but that's not our problem here.
 *
 * In general, non-PKCS #11 users of this API should probably never
 * set HAL_KEY_FLAG_PUBLIC, in which case they'll get the simple rule.
 *
 * Note that keystore drivers may need to implement additional
 * additional checks, eg, ks_volatile needs to enforce the rule that
 * session objects are only visible to the client which created them
 * (not the session, that would be too simple, thanks PKCS #11).  In
 * practice, this should not be a serious problem, since such checks
 * will likely only apply to existing objects.  The thing we really
 * want to avoid is doing all the work to create a large key only to
 * have the keystore driver reject access at the end, but since, by
 * definition, that only occurs when creating new objects, the access
 * decision doesn't depend on preexisting data, so the rules here
 * should suffice.  That's the theory, anyway, if this is wrong we may
 * need to refactor.
 */

static inline hal_error_t check_normal_or_wheel(const hal_client_handle_t client)
{
  const hal_error_t err = hal_rpc_is_logged_in(client, HAL_USER_NORMAL);
  return (err == HAL_ERROR_FORBIDDEN
          ? hal_rpc_is_logged_in(client, HAL_USER_WHEEL)
          : err);
}

static inline hal_error_t check_readable(const hal_client_handle_t client,
                                         const hal_key_flags_t flags)
{
  if ((flags & HAL_KEY_FLAG_PUBLIC) != 0)
    return HAL_OK;

  return check_normal_or_wheel(client);
}

static inline hal_error_t check_writable(const hal_client_handle_t client,
                                         const hal_key_flags_t flags)
{
  if ((flags & (HAL_KEY_FLAG_TOKEN | HAL_KEY_FLAG_PUBLIC)) == HAL_KEY_FLAG_PUBLIC)
    return HAL_OK;

  return check_normal_or_wheel(client);
}

/*
 * PKCS #1.5 encryption requires non-zero random bytes, which is a bit
 * messy if done in place, so make it a separate function for readability.
 */

static inline hal_error_t get_nonzero_random(uint8_t *buffer, size_t n)
{
  hal_assert(buffer != NULL);

  uint32_t word = 0;
  hal_error_t err;

  while (n > 0) {

    while ((word &  0xFF) == 0)
      if  ((word & ~0xFF) != 0)
        word >>= 8;
      else if ((err = hal_get_random(NULL, &word, sizeof(word))) != HAL_OK)
        return err;

    *buffer++ = word & 0xFF;
    word >>= 8;
    n--;
  }

  return HAL_OK;
}

/*
 * Pad an octet string with PKCS #1.5 padding for use with RSA.
 *
 * This handles type 01 and type 02 encryption blocks.  The formats
 * are identical, except that the padding string is constant 0xFF
 * bytes for type 01 and non-zero random bytes for type 02.
 *
 * We use memmove() instead of memcpy() so that the caller can
 * construct the data to be padded in the same buffer.
 */

static hal_error_t pkcs1_5_pad(const uint8_t * const data, const size_t data_len,
                               uint8_t *block, const size_t block_len,
                               const uint8_t type)
{
  hal_assert(data != NULL && block != NULL && (type == 0x01 || type == 0x02));

  hal_error_t err;

  /*
   * Congregation will now please turn to RFC 2313 8.1 as we
   * construct a PKCS #1.5 type 01 or type 02 encryption block.
   */

  if (data_len > block_len - 11)
    return HAL_ERROR_RESULT_TOO_LONG;

  memmove(block + block_len - data_len, data, data_len);

  block[0] = 0x00;
  block[1] = type;

  switch (type) {

  case 0x01:                    /* Signature */
    memset(block + 2, 0xFF, block_len - 3 - data_len);
    break;

  case 0x02:                    /* Encryption */
    if ((err = get_nonzero_random(block + 2, block_len - 3 - data_len)) != HAL_OK)
      return err;
    break;

  }

  block[block_len - data_len - 1] = 0x00;

  return HAL_OK;
}

/*
 * Given key flags, return appropriate keystore.
 */

static inline hal_ks_t *ks_from_flags(const hal_key_flags_t flags)
{
  return (flags & HAL_KEY_FLAG_TOKEN) == 0 ? hal_ks_volatile : hal_ks_token;
}

/*
 * Fetch a key from keystore indicated by key flag in slot object.
 */

static inline hal_error_t ks_fetch_from_flags(hal_pkey_slot_t *slot,
                                              uint8_t *der, size_t *der_len, const size_t der_max)
{
  if (slot == NULL)
    return HAL_ERROR_IMPOSSIBLE;

  return hal_ks_fetch(ks_from_flags(slot->flags), slot, der, der_len, der_max);
}


/*
 * Receive key from application, generate a name (UUID), store it, and
 * return a key handle and the name.
 */

static hal_error_t pkey_local_load(const hal_client_handle_t client,
                                   const hal_session_handle_t session,
                                   hal_pkey_handle_t *pkey,
                                   hal_uuid_t *name,
                                   const uint8_t * const der, const size_t der_len,
                                   const hal_key_flags_t flags)
{
  hal_assert(pkey != NULL && name != NULL && der != NULL);

  hal_curve_name_t curve;
  hal_pkey_slot_t *slot;
  hal_key_type_t type;
  hal_error_t err;

  if ((err = check_writable(client, flags)) != HAL_OK)
    return err;

  if ((err = hal_asn1_guess_key_type(&type, &curve, der, der_len)) != HAL_OK)
    return err;

  if ((slot = alloc_slot(flags)) == NULL)
    return HAL_ERROR_NO_KEY_SLOTS_AVAILABLE;

  if ((err = hal_uuid_gen(&slot->name)) != HAL_OK)
    return err;

  slot->client  = client;
  slot->session = session;
  slot->type    = type;
  slot->curve   = curve;
  slot->flags   = flags;

  if ((err = hal_ks_store(ks_from_flags(flags), slot, der, der_len)) != HAL_OK) {
    slot->type = HAL_KEY_TYPE_NONE;
    return err;
  }

  *pkey = slot->pkey;
  *name = slot->name;
  return HAL_OK;
}

/*
 * Look up a key given its name, return a key handle.
 */

static hal_error_t pkey_local_open(const hal_client_handle_t client,
                                   const hal_session_handle_t session,
                                   hal_pkey_handle_t *pkey,
                                   const hal_uuid_t * const name)
{
  hal_assert(pkey != NULL && name != NULL);

  hal_pkey_slot_t *slot;
  hal_error_t err;

  if ((err = check_readable(client, 0)) != HAL_OK)
    return err;

  if ((slot = alloc_slot(0)) == NULL)
    return HAL_ERROR_NO_KEY_SLOTS_AVAILABLE;

  slot->name    = *name;
  slot->client  = client;
  slot->session = session;

  if ((err = hal_ks_fetch(hal_ks_token, slot, NULL, NULL, 0)) == HAL_OK)
    slot->pkey.handle |= HAL_PKEY_HANDLE_TOKEN_FLAG;

  else if (err == HAL_ERROR_KEY_NOT_FOUND)
    err = hal_ks_fetch(hal_ks_volatile, slot, NULL, NULL, 0);

  if (err != HAL_OK)
    goto fail;

  *pkey = slot->pkey;
  return HAL_OK;

 fail:
  clear_slot(slot);
  return err;
}

/*
 * Generate a new RSA key with supplied name, return a key handle.
 */

static hal_error_t pkey_local_generate_rsa(const hal_client_handle_t client,
                                           const hal_session_handle_t session,
                                           hal_pkey_handle_t *pkey,
                                           hal_uuid_t *name,
                                           const unsigned key_length,
                                           const uint8_t * const public_exponent, const size_t public_exponent_len,
                                           const hal_key_flags_t flags)
{
  hal_assert(pkey != NULL && name != NULL && (key_length & 7) == 0);

  uint8_t keybuf[hal_rsa_key_t_size];
  hal_rsa_key_t *key = NULL;
  hal_pkey_slot_t *slot;
  hal_error_t err;

  if ((err = check_writable(client, flags)) != HAL_OK)
    return err;

  if ((slot = alloc_slot(flags)) == NULL)
    return HAL_ERROR_NO_KEY_SLOTS_AVAILABLE;

  if ((err = hal_uuid_gen(&slot->name)) != HAL_OK)
    return err;

  slot->client  = client;
  slot->session = session;
  slot->type    = HAL_KEY_TYPE_RSA_PRIVATE;
  slot->curve   = HAL_CURVE_NONE;
  slot->flags   = flags;

  if ((err = hal_rsa_key_gen(NULL, &key, keybuf, sizeof(keybuf), key_length / 8,
                             public_exponent, public_exponent_len)) != HAL_OK) {
    slot->type = HAL_KEY_TYPE_NONE;
    return err;
  }

  uint8_t der[hal_rsa_private_key_to_der_len(key)];
  size_t der_len;

  if ((err = hal_rsa_private_key_to_der(key, der, &der_len, sizeof(der))) == HAL_OK)
    err = hal_ks_store(ks_from_flags(flags), slot, der, der_len);

  memset(keybuf, 0, sizeof(keybuf));
  memset(der, 0, sizeof(der));

  if (err != HAL_OK) {
    slot->type = HAL_KEY_TYPE_NONE;
    return err;
  }

  *pkey = slot->pkey;
  *name = slot->name;
  return HAL_OK;
}

/*
 * Generate a new EC key with supplied name, return a key handle.
 * At the moment, EC key == ECDSA key, but this is subject to change.
 */

static hal_error_t pkey_local_generate_ec(const hal_client_handle_t client,
                                          const hal_session_handle_t session,
                                          hal_pkey_handle_t *pkey,
                                          hal_uuid_t *name,
                                          const hal_curve_name_t curve,
                                          const hal_key_flags_t flags)
{
  hal_assert(pkey != NULL && name != NULL);

  uint8_t keybuf[hal_ecdsa_key_t_size];
  hal_ecdsa_key_t *key = NULL;
  hal_pkey_slot_t *slot;
  hal_error_t err;

  if ((err = check_writable(client, flags)) != HAL_OK)
    return err;

  if ((slot = alloc_slot(flags)) == NULL)
    return HAL_ERROR_NO_KEY_SLOTS_AVAILABLE;

  if ((err = hal_uuid_gen(&slot->name)) != HAL_OK)
    return err;

  slot->client  = client;
  slot->session = session;
  slot->type    = HAL_KEY_TYPE_EC_PRIVATE;
  slot->curve   = curve;
  slot->flags   = flags;

  if ((err = hal_ecdsa_key_gen(NULL, &key, keybuf, sizeof(keybuf), curve)) != HAL_OK) {
    slot->type = HAL_KEY_TYPE_NONE;
    return err;
  }

  uint8_t der[hal_ecdsa_private_key_to_der_len(key)];
  size_t der_len;

  if ((err = hal_ecdsa_private_key_to_der(key, der, &der_len, sizeof(der))) == HAL_OK)
    err = hal_ks_store(ks_from_flags(flags), slot, der, der_len);

  memset(keybuf, 0, sizeof(keybuf));
  memset(der, 0, sizeof(der));

  if (err != HAL_OK) {
    slot->type = HAL_KEY_TYPE_NONE;
    return err;
  }

  *pkey = slot->pkey;
  *name = slot->name;
  return HAL_OK;
}

/*
 * Generate a new hash-tree key with supplied name, return a key handle.
 */

static hal_error_t pkey_local_generate_hashsig(const hal_client_handle_t client,
                                               const hal_session_handle_t session,
                                               hal_pkey_handle_t *pkey,
                                               hal_uuid_t *name,
                                               const size_t hss_levels,
                                               const hal_lms_algorithm_t lms_type,
                                               const hal_lmots_algorithm_t lmots_type,
                                               const hal_key_flags_t flags)
{
  hal_assert(pkey != NULL && name != NULL);

  hal_hashsig_key_t *key = NULL;
  hal_pkey_slot_t *slot;
  hal_error_t err;

  if ((err = check_writable(client, flags)) != HAL_OK)
    return err;

  if ((slot = alloc_slot(flags)) == NULL)
    return HAL_ERROR_NO_KEY_SLOTS_AVAILABLE;

  if ((err = hal_uuid_gen(&slot->name)) != HAL_OK)
    return err;

  slot->client  = client;
  slot->session = session;
  slot->type    = HAL_KEY_TYPE_HASHSIG_PRIVATE,
  slot->curve   = HAL_CURVE_NONE;
  slot->flags   = flags;

  if ((err = hal_hashsig_key_gen(NULL, &key, hss_levels, lms_type, lmots_type)) != HAL_OK) {
    slot->type = HAL_KEY_TYPE_NONE;
    return err;
  }

  uint8_t der[hal_hashsig_private_key_to_der_len(key)];
  size_t der_len;

  if ((err = hal_hashsig_private_key_to_der(key, der, &der_len, sizeof(der))) == HAL_OK)
    err = hal_ks_store(ks_from_flags(flags), slot, der, der_len);

  /* There's nothing sensitive in the top-level private key, but we wipe
   * the der anyway, for symmetry with other key types. The actual key buf
   * is allocated internally and stays in memory, because everything else
   * is linked off of it.
   */
  memset(der, 0, sizeof(der));

  if (err != HAL_OK) {
    slot->type = HAL_KEY_TYPE_NONE;
    return err;
  }

  *pkey = slot->pkey;
  *name = slot->name;
  return HAL_OK;
}

/*
 * Discard key handle, leaving key intact.
 */

static hal_error_t pkey_local_close(const hal_pkey_handle_t pkey)
{
  hal_pkey_slot_t *slot;

  if ((slot = find_handle(pkey)) == NULL)
    return HAL_ERROR_KEY_NOT_FOUND;

  clear_slot(slot);

  return HAL_OK;
}

/*
 * Delete a key from the store, given its key handle.
 */
static hal_error_t pkey_local_get_key_type(const hal_pkey_handle_t pkey, hal_key_type_t *type);

static hal_error_t pkey_local_delete(const hal_pkey_handle_t pkey)
{
  hal_pkey_slot_t *slot = find_handle(pkey);

  if (slot == NULL)
    return HAL_ERROR_KEY_NOT_FOUND;

  hal_error_t err;

  if ((err = check_writable(slot->client, slot->flags)) != HAL_OK)
    return err;

  hal_key_type_t key_type;
  if ((err = pkey_local_get_key_type(pkey, &key_type)) != HAL_OK)
      return err;

  if (key_type == HAL_KEY_TYPE_HASHSIG_PRIVATE) {
    hal_hashsig_key_t *key;
    uint8_t keybuf[hal_hashsig_key_t_size];
    uint8_t der[HAL_KS_WRAPPED_KEYSIZE];
    size_t der_len;
    if ((err = ks_fetch_from_flags(slot, der, &der_len, sizeof(der))) != HAL_OK ||
        (err = hal_hashsig_private_key_from_der(&key, keybuf, sizeof(keybuf), der, der_len)) != HAL_OK ||
        (err = hal_hashsig_key_delete(key)) != HAL_OK)
      return err;
  }

  err = hal_ks_delete(ks_from_flags(slot->flags), slot);

  if (err == HAL_OK || err == HAL_ERROR_KEY_NOT_FOUND)
    clear_slot(slot);

  return err;
}

/*
 * Get type of key associated with handle.
 */

static hal_error_t pkey_local_get_key_type(const hal_pkey_handle_t pkey,
                                           hal_key_type_t *type)
{
  if (type == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  hal_pkey_slot_t *slot = find_handle(pkey);

  if (slot == NULL)
    return HAL_ERROR_KEY_NOT_FOUND;

  *type = slot->type;

  return HAL_OK;
}

/*
 * Get curve of key associated with handle.
 */

static hal_error_t pkey_local_get_key_curve(const hal_pkey_handle_t pkey,
                                           hal_curve_name_t *curve)
{
  if (curve == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  hal_pkey_slot_t *slot = find_handle(pkey);

  if (slot == NULL)
    return HAL_ERROR_KEY_NOT_FOUND;

  *curve = slot->curve;

  return HAL_OK;
}

/*
 * Get flags of key associated with handle.
 */

static hal_error_t pkey_local_get_key_flags(const hal_pkey_handle_t pkey,
                                            hal_key_flags_t *flags)
{
  if (flags == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  hal_pkey_slot_t *slot = find_handle(pkey);

  if (slot == NULL)
    return HAL_ERROR_KEY_NOT_FOUND;

  *flags = slot->flags;

  return HAL_OK;
}

/*
 * Get length of public key associated with handle.
 */

static size_t pkey_local_get_public_key_len(const hal_pkey_handle_t pkey)
{
  hal_pkey_slot_t *slot = find_handle(pkey);

  if (slot == NULL)
    return 0;

  size_t result = 0;

#ifndef max
#define max(a, b) ((a) >= (b) ? (a) : (b))
#endif
  size_t keybuf_size = max(hal_rsa_key_t_size, hal_ecdsa_key_t_size);
  keybuf_size = max(keybuf_size, hal_hashsig_key_t_size);
  uint8_t keybuf[keybuf_size];
  hal_rsa_key_t     *rsa_key   = NULL;
  hal_ecdsa_key_t   *ecdsa_key = NULL;
  hal_hashsig_key_t *hashsig_key = NULL;
  uint8_t der[HAL_KS_WRAPPED_KEYSIZE];
  size_t der_len;
  hal_error_t err;

  if ((err = ks_fetch_from_flags(slot, der, &der_len, sizeof(der))) == HAL_OK) {
    switch (slot->type) {

    case HAL_KEY_TYPE_RSA_PUBLIC:
    case HAL_KEY_TYPE_EC_PUBLIC:
    case HAL_KEY_TYPE_HASHSIG_PUBLIC:
      result = der_len;
      break;

    case HAL_KEY_TYPE_RSA_PRIVATE:
      if (hal_rsa_private_key_from_der(&rsa_key, keybuf, sizeof(keybuf), der, der_len) == HAL_OK)
        result = hal_rsa_public_key_to_der_len(rsa_key);
      break;

    case HAL_KEY_TYPE_EC_PRIVATE:
      if (hal_ecdsa_private_key_from_der(&ecdsa_key, keybuf, sizeof(keybuf), der, der_len) == HAL_OK)
        result = hal_ecdsa_public_key_to_der_len(ecdsa_key);
      break;

    case HAL_KEY_TYPE_HASHSIG_PRIVATE:
      if (hal_hashsig_private_key_from_der(&hashsig_key, keybuf, sizeof(keybuf), der, der_len) == HAL_OK)
        result = hal_hashsig_public_key_to_der_len(hashsig_key);
      break;

    default:
      break;
    }
  }

  memset(keybuf, 0, sizeof(keybuf));
  memset(der,    0, sizeof(der));

  return result;
}

/*
 * Get public key associated with handle.
 */

static hal_error_t pkey_local_get_public_key(const hal_pkey_handle_t pkey,
                                             uint8_t *der, size_t *der_len, const size_t der_max)
{
  hal_pkey_slot_t *slot = find_handle(pkey);

  if (slot == NULL)
    return HAL_ERROR_KEY_NOT_FOUND;

  size_t keybuf_size = max(hal_rsa_key_t_size, hal_ecdsa_key_t_size);
  keybuf_size = max(keybuf_size, hal_hashsig_key_t_size);
  uint8_t keybuf[keybuf_size];
  hal_rsa_key_t     *rsa_key   = NULL;
  hal_ecdsa_key_t   *ecdsa_key = NULL;
  hal_hashsig_key_t *hashsig_key = NULL;
  uint8_t buf[HAL_KS_WRAPPED_KEYSIZE];
  size_t buf_len;
  hal_error_t err;

  if ((err = ks_fetch_from_flags(slot, buf, &buf_len, sizeof(buf))) == HAL_OK) {
    switch (slot->type) {

    case HAL_KEY_TYPE_RSA_PUBLIC:
    case HAL_KEY_TYPE_EC_PUBLIC:
    case HAL_KEY_TYPE_HASHSIG_PUBLIC:
      if (der_len != NULL)
        *der_len = buf_len;
      if (der != NULL && der_max < buf_len)
        err = HAL_ERROR_RESULT_TOO_LONG;
      else if (der != NULL)
        memcpy(der, buf, buf_len);
      break;

    case HAL_KEY_TYPE_RSA_PRIVATE:
      if ((err = hal_rsa_private_key_from_der(&rsa_key, keybuf, sizeof(keybuf), buf, buf_len)) == HAL_OK)
        err = hal_rsa_public_key_to_der(rsa_key, der, der_len, der_max);
      break;

    case HAL_KEY_TYPE_EC_PRIVATE:
      if ((err = hal_ecdsa_private_key_from_der(&ecdsa_key, keybuf, sizeof(keybuf), buf, buf_len)) == HAL_OK)
        err = hal_ecdsa_public_key_to_der(ecdsa_key, der, der_len, der_max);
      break;

    case HAL_KEY_TYPE_HASHSIG_PRIVATE:
      if ((err = hal_hashsig_private_key_from_der(&hashsig_key, keybuf, sizeof(keybuf), buf, buf_len)) == HAL_OK)
        err = hal_hashsig_public_key_to_der(hashsig_key, der, der_len, der_max);
      break;

    default:
      err = HAL_ERROR_UNSUPPORTED_KEY;
      break;
    }
  }

  memset(keybuf, 0, sizeof(keybuf));
  memset(buf,    0, sizeof(buf));

  return err;
}

/*
 * Sign something using private key associated with handle.
 *
 * RSA has enough quirks that it's simplest to split this out into
 * algorithm-specific functions.
 */

static hal_error_t pkey_local_sign_rsa(hal_pkey_slot_t *slot,
                                       uint8_t *keybuf, const size_t keybuf_len,
                                       const uint8_t * const der, const size_t der_len,
                                       const hal_hash_handle_t hash,
                                       const uint8_t *input, size_t input_len,
                                       uint8_t *signature, size_t *signature_len, const size_t signature_max)
{
  hal_rsa_key_t *key = NULL;
  hal_error_t err;

  hal_assert(signature != NULL && signature_len != NULL);
  hal_assert((hash.handle == HAL_HANDLE_NONE) != (input == NULL || input_len == 0));

  if ((err = hal_rsa_private_key_from_der(&key, keybuf, keybuf_len, der, der_len)) != HAL_OK ||
      (err = hal_rsa_key_get_modulus(key, NULL, signature_len, 0))         != HAL_OK)
      return err;

  if (*signature_len > signature_max)
    return HAL_ERROR_RESULT_TOO_LONG;

  if (input == NULL || input_len == 0) {
    if ((err = hal_rpc_pkcs1_construct_digestinfo(hash, signature, &input_len, *signature_len)) != HAL_OK)
      return err;
    input = signature;
  }

  if ((err = pkcs1_5_pad(input, input_len, signature, *signature_len, 0x01)) != HAL_OK ||
      (err = hal_rsa_decrypt(NULL, NULL, key, signature, *signature_len, signature, *signature_len)) != HAL_OK)
    return err;

  if (hal_rsa_key_needs_saving(key)) {
    uint8_t pkcs8[hal_rsa_private_key_to_der_extra_len(key)];
    size_t pkcs8_len = 0;
    if ((err = hal_rsa_private_key_to_der_extra(key, pkcs8, &pkcs8_len, sizeof(pkcs8))) == HAL_OK)
      err = hal_ks_rewrite_der(ks_from_flags(slot->flags), slot, pkcs8, pkcs8_len);
    memset(pkcs8, 0, sizeof(pkcs8));
    if (err != HAL_OK)
      return err;
  }

  return HAL_OK;
}

static hal_error_t pkey_local_sign_ecdsa(hal_pkey_slot_t *slot,
                                         uint8_t *keybuf, const size_t keybuf_len,
                                         const uint8_t * const der, const size_t der_len,
                                         const hal_hash_handle_t hash,
                                         const uint8_t * input, size_t input_len,
                                         uint8_t * signature, size_t *signature_len, const size_t signature_max)
{
  hal_ecdsa_key_t *key = NULL;
  hal_error_t err;

  hal_assert(signature != NULL && signature_len != NULL);
  hal_assert((hash.handle == HAL_HANDLE_NONE) != (input == NULL || input_len == 0));

  if ((err = hal_ecdsa_private_key_from_der(&key, keybuf, keybuf_len, der, der_len)) != HAL_OK)
    return err;

  if (input == NULL || input_len == 0) {
    hal_digest_algorithm_t alg;

    if ((err = hal_rpc_hash_get_algorithm(hash, &alg))          != HAL_OK ||
        (err = hal_rpc_hash_get_digest_length(alg, &input_len)) != HAL_OK)
      return err;

    if (input_len > signature_max)
      return HAL_ERROR_RESULT_TOO_LONG;

    if ((err = hal_rpc_hash_finalize(hash, signature, input_len)) != HAL_OK)
      return err;

    input = signature;
  }

  if ((err = hal_ecdsa_sign(NULL, key, input, input_len, signature, signature_len, signature_max)) != HAL_OK)
    return err;

  return HAL_OK;
}

static hal_error_t pkey_local_sign_hashsig(hal_pkey_slot_t *slot,
                                           uint8_t *keybuf, const size_t keybuf_len,
                                           const uint8_t * const der, const size_t der_len,
                                           const hal_hash_handle_t hash,
                                           const uint8_t * input, size_t input_len,
                                           uint8_t * signature, size_t *signature_len, const size_t signature_max)
{
  hal_hashsig_key_t *key = NULL;
  hal_error_t err;

  hal_assert(signature != NULL && signature_len != NULL);
  hal_assert((hash.handle == HAL_HANDLE_NONE) != (input == NULL || input_len == 0));

  if ((err = hal_hashsig_private_key_from_der(&key, keybuf, keybuf_len, der, der_len)) != HAL_OK)
    return err;

  if (input == NULL || input_len == 0) {
    hal_digest_algorithm_t alg;

    if ((err = hal_rpc_hash_get_algorithm(hash, &alg))          != HAL_OK ||
        (err = hal_rpc_hash_get_digest_length(alg, &input_len)) != HAL_OK)
      return err;

    if (input_len > signature_max)
      return HAL_ERROR_RESULT_TOO_LONG;

    if ((err = hal_rpc_hash_finalize(hash, signature, input_len)) != HAL_OK)
      return err;

    input = signature;
  }

  if ((err = hal_hashsig_sign(NULL, key, input, input_len, signature, signature_len, signature_max)) != HAL_OK)
    return err;

  return HAL_OK;
}

static hal_error_t pkey_local_sign(const hal_pkey_handle_t pkey,
                                   const hal_hash_handle_t hash,
                                   const uint8_t * const input,  const size_t input_len,
                                   uint8_t * signature, size_t *signature_len, const size_t signature_max)
{
  hal_pkey_slot_t *slot = find_handle(pkey);

  if (slot == NULL)
    return HAL_ERROR_KEY_NOT_FOUND;

  hal_error_t (*signer)(hal_pkey_slot_t *slot,
                        uint8_t *keybuf, const size_t keybuf_len,
                        const uint8_t * const der, const size_t der_len,
                        const hal_hash_handle_t hash,
                        const uint8_t * const input,  const size_t input_len,
                        uint8_t * signature, size_t *signature_len, const size_t signature_max);
  size_t keybuf_size;

  switch (slot->type) {
  case HAL_KEY_TYPE_RSA_PRIVATE:
    signer = pkey_local_sign_rsa;
    keybuf_size = hal_rsa_key_t_size;
    break;
  case HAL_KEY_TYPE_EC_PRIVATE:
    signer = pkey_local_sign_ecdsa;
    keybuf_size = hal_ecdsa_key_t_size;
    break;
  case HAL_KEY_TYPE_HASHSIG_PRIVATE:
    signer = pkey_local_sign_hashsig;
    keybuf_size = hal_hashsig_key_t_size;
    break;
  default:
    return HAL_ERROR_UNSUPPORTED_KEY;
  }

  if ((slot->flags & HAL_KEY_FLAG_USAGE_DIGITALSIGNATURE) == 0)
    return HAL_ERROR_FORBIDDEN;

  uint8_t keybuf[keybuf_size];
  uint8_t der[HAL_KS_WRAPPED_KEYSIZE];
  size_t der_len;
  hal_error_t err;

  if ((err = ks_fetch_from_flags(slot, der, &der_len, sizeof(der))) == HAL_OK)
    err = signer(slot, keybuf, sizeof(keybuf), der, der_len, hash, input, input_len,
                 signature, signature_len, signature_max);

  memset(keybuf, 0, sizeof(keybuf));
  memset(der,    0, sizeof(der));

  return err;
}

/*
 * Verify something using public key associated with handle.
 *
 * RSA has enough quirks that it's simplest to split this out into
 * algorithm-specific functions.
 */

static hal_error_t pkey_local_verify_rsa(uint8_t *keybuf, const size_t keybuf_len, const hal_key_type_t type,
                                         const uint8_t * const der, const size_t der_len,
                                         const hal_hash_handle_t hash,
                                         const uint8_t * input, size_t input_len,
                                         const uint8_t * const signature, const size_t signature_len)
{
  uint8_t expected[signature_len], received[(signature_len + 3) & ~3];
  hal_rsa_key_t *key = NULL;
  hal_error_t err;

  hal_assert(signature != NULL && signature_len > 0);
  hal_assert((hash.handle == HAL_HANDLE_NONE) != (input == NULL || input_len == 0));

  switch (type) {
  case HAL_KEY_TYPE_RSA_PRIVATE:
    err = hal_rsa_private_key_from_der(&key, keybuf, keybuf_len, der, der_len);
    break;
  case HAL_KEY_TYPE_RSA_PUBLIC:
    err = hal_rsa_public_key_from_der(&key, keybuf, keybuf_len, der, der_len);
    break;
  default:
    err = HAL_ERROR_IMPOSSIBLE;
  }

  if (err != HAL_OK)
    return err;

  if (input == NULL || input_len == 0) {
    if ((err = hal_rpc_pkcs1_construct_digestinfo(hash, expected, &input_len, sizeof(expected))) != HAL_OK)
      return err;
    input = expected;
  }

  if ((err = pkcs1_5_pad(input, input_len, expected, sizeof(expected), 0x01))                  != HAL_OK ||
      (err = hal_rsa_encrypt(NULL, key, signature, signature_len, received, sizeof(received))) != HAL_OK)
    return err;

  unsigned diff = 0;
  for (size_t i = 0; i < signature_len; i++)
    diff |= expected[i] ^ received[i + sizeof(received) - sizeof(expected)];

  if (diff != 0)
    return HAL_ERROR_INVALID_SIGNATURE;

  return HAL_OK;
}

static hal_error_t pkey_local_verify_ecdsa(uint8_t *keybuf, const size_t keybuf_len, const hal_key_type_t type,
                                           const uint8_t * const der, const size_t der_len,
                                           const hal_hash_handle_t hash,
                                           const uint8_t * input, size_t input_len,
                                           const uint8_t * const signature, const size_t signature_len)
{
  uint8_t digest[signature_len];
  hal_ecdsa_key_t *key = NULL;
  hal_error_t err;

  hal_assert(signature != NULL && signature_len > 0);
  hal_assert((hash.handle == HAL_HANDLE_NONE) != (input == NULL || input_len == 0));

  switch (type) {
  case HAL_KEY_TYPE_EC_PRIVATE:
    err = hal_ecdsa_private_key_from_der(&key, keybuf, keybuf_len, der, der_len);
    break;
  case HAL_KEY_TYPE_EC_PUBLIC:
    err = hal_ecdsa_public_key_from_der(&key, keybuf, keybuf_len, der, der_len);
    break;
  default:
    err = HAL_ERROR_IMPOSSIBLE;
  }

  if (err != HAL_OK)
    return err;

  if (input == NULL || input_len == 0) {
    hal_digest_algorithm_t alg;

    if ((err = hal_rpc_hash_get_algorithm(hash, &alg))              != HAL_OK ||
        (err = hal_rpc_hash_get_digest_length(alg, &input_len))     != HAL_OK ||
        (err = hal_rpc_hash_finalize(hash, digest, sizeof(digest))) != HAL_OK)
      return err;

    input = digest;
  }

  if ((err = hal_ecdsa_verify(NULL, key, input, input_len, signature, signature_len)) != HAL_OK)
    return err;

  return HAL_OK;
}

static hal_error_t pkey_local_verify_hashsig(uint8_t *keybuf, const size_t keybuf_len, const hal_key_type_t type,
                                           const uint8_t * const der, const size_t der_len,
                                           const hal_hash_handle_t hash,
                                           const uint8_t * input, size_t input_len,
                                           const uint8_t * const signature, const size_t signature_len)
{
  uint8_t digest[signature_len];
  hal_hashsig_key_t *key = NULL;
  hal_error_t err;

  hal_assert(signature != NULL && signature_len > 0);
  hal_assert((hash.handle == HAL_HANDLE_NONE) != (input == NULL || input_len == 0));

  if ((err = hal_hashsig_public_key_from_der(&key, keybuf, keybuf_len, der, der_len)) != HAL_OK)
    return err;

  if (input == NULL || input_len == 0) {
    hal_digest_algorithm_t alg;

    // ???
    if ((err = hal_rpc_hash_get_algorithm(hash, &alg))              != HAL_OK ||
        (err = hal_rpc_hash_get_digest_length(alg, &input_len))     != HAL_OK ||
        (err = hal_rpc_hash_finalize(hash, digest, sizeof(digest))) != HAL_OK)
      return err;

    input = digest;
  }

  if ((err = hal_hashsig_verify(NULL, key, input, input_len, signature, signature_len)) != HAL_OK)
    return err;

  return HAL_OK;
}

static hal_error_t pkey_local_verify(const hal_pkey_handle_t pkey,
                                     const hal_hash_handle_t hash,
                                     const uint8_t * const input, const size_t input_len,
                                     const uint8_t * const signature, const size_t signature_len)
{
  hal_pkey_slot_t *slot = find_handle(pkey);

  if (slot == NULL)
    return HAL_ERROR_KEY_NOT_FOUND;

  hal_error_t (*verifier)(uint8_t *keybuf, const size_t keybuf_len, const hal_key_type_t type,
                          const uint8_t * const der, const size_t der_len,
                          const hal_hash_handle_t hash,
                          const uint8_t * const input,  const size_t input_len,
                          const uint8_t * const signature, const size_t signature_len);
  size_t keybuf_size;

  switch (slot->type) {
  case HAL_KEY_TYPE_RSA_PRIVATE:
  case HAL_KEY_TYPE_RSA_PUBLIC:
    verifier = pkey_local_verify_rsa;
    keybuf_size = hal_rsa_key_t_size;
    break;
  case HAL_KEY_TYPE_EC_PRIVATE:
  case HAL_KEY_TYPE_EC_PUBLIC:
    verifier = pkey_local_verify_ecdsa;
    keybuf_size = hal_ecdsa_key_t_size;
    break;
  case HAL_KEY_TYPE_HASHSIG_PUBLIC:
    verifier = pkey_local_verify_hashsig;
    keybuf_size = hal_hashsig_key_t_size;
    break;
  default:
    return HAL_ERROR_UNSUPPORTED_KEY;
  }

  if ((slot->flags & HAL_KEY_FLAG_USAGE_DIGITALSIGNATURE) == 0)
    return HAL_ERROR_FORBIDDEN;

  uint8_t keybuf[keybuf_size];
  uint8_t der[HAL_KS_WRAPPED_KEYSIZE];
  size_t der_len;
  hal_error_t err;

  if ((err = ks_fetch_from_flags(slot, der, &der_len, sizeof(der))) == HAL_OK)
    err = verifier(keybuf, sizeof(keybuf), slot->type, der, der_len, hash,
                   input, input_len, signature, signature_len);

  memset(keybuf, 0, sizeof(keybuf));
  memset(der,    0, sizeof(der));

  return err;
}

static inline hal_error_t match_one_keystore(hal_ks_t *ks,
                                             const hal_client_handle_t client,
                                             const hal_session_handle_t session,
                                             const hal_key_type_t type,
                                             const hal_curve_name_t curve,
                                             const hal_key_flags_t mask,
                                             const hal_key_flags_t flags,
                                             const hal_pkey_attribute_t *attributes,
                                             const unsigned attributes_len,
                                             hal_uuid_t **result,
                                             unsigned *result_len,
                                             const unsigned result_max,
                                             const hal_uuid_t * const previous_uuid)
{
  hal_error_t err;
  unsigned len;

  if ((err = hal_ks_match(ks, client, session, type, curve,
                          mask, flags, attributes, attributes_len,
                          *result, &len, result_max, previous_uuid)) != HAL_OK)
    return err;

  *result     += len;
  *result_len += len;

  return HAL_OK;
}

typedef enum {
  MATCH_STATE_START,
  MATCH_STATE_TOKEN,
  MATCH_STATE_VOLATILE,
  MATCH_STATE_DONE
} match_state_t;

static hal_error_t pkey_local_match(const hal_client_handle_t client,
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
  hal_assert(state != NULL && result_len != NULL);

  static const hal_uuid_t uuid_zero[1] = {{{0}}};
  const hal_uuid_t *prev = previous_uuid;
  hal_error_t err;

  *result_len = 0;

  if ((err = check_readable(client, flags)) == HAL_ERROR_FORBIDDEN)
    return HAL_OK;
  else if (err != HAL_OK)
    return err;

  switch ((match_state_t) *state) {

  case MATCH_STATE_START:
    prev = uuid_zero;
    ++*state;

  case MATCH_STATE_TOKEN:
    if (((mask & HAL_KEY_FLAG_TOKEN) == 0 || (mask & flags & HAL_KEY_FLAG_TOKEN) != 0) &&
        (err = match_one_keystore(hal_ks_token, client, session, type, curve,
                                  mask, flags, attributes, attributes_len,
                                  &result, result_len, result_max - *result_len, prev)) != HAL_OK)
      return err;
    if (*result_len == result_max)
      return HAL_OK;
    prev = uuid_zero;
    ++*state;

  case MATCH_STATE_VOLATILE:
    if (((mask & HAL_KEY_FLAG_TOKEN) == 0 || (mask & flags & HAL_KEY_FLAG_TOKEN) == 0) &&
        (err = match_one_keystore(hal_ks_volatile, client, session, type, curve,
                                  mask, flags, attributes, attributes_len,
                                  &result, result_len, result_max - *result_len, prev)) != HAL_OK)
      return err;
    if (*result_len == result_max)
      return HAL_OK;
    ++*state;

  case MATCH_STATE_DONE:
    return HAL_OK;

  default:
    return HAL_ERROR_BAD_ARGUMENTS;
  }
}

static hal_error_t pkey_local_set_attributes(const hal_pkey_handle_t pkey,
                                             const hal_pkey_attribute_t *attributes,
                                             const unsigned attributes_len)
{
  hal_pkey_slot_t *slot = find_handle(pkey);

  if (slot == NULL)
    return HAL_ERROR_KEY_NOT_FOUND;

  hal_error_t err;

  if ((err = check_writable(slot->client, slot->flags)) != HAL_OK)
    return err;

  return hal_ks_set_attributes(ks_from_flags(slot->flags), slot, attributes, attributes_len);
}

static hal_error_t pkey_local_get_attributes(const hal_pkey_handle_t pkey,
                                             hal_pkey_attribute_t *attributes,
                                             const unsigned attributes_len,
                                             uint8_t *attributes_buffer,
                                             const size_t attributes_buffer_len)
{
  hal_pkey_slot_t *slot = find_handle(pkey);

  if (slot == NULL)
    return HAL_ERROR_KEY_NOT_FOUND;

  return hal_ks_get_attributes(ks_from_flags(slot->flags), slot, attributes, attributes_len,
                               attributes_buffer, attributes_buffer_len);
}

static hal_error_t pkey_local_export(const hal_pkey_handle_t pkey_handle,
                                     const hal_pkey_handle_t kekek_handle,
                                     uint8_t *pkcs8, size_t *pkcs8_len, const size_t pkcs8_max,
                                     uint8_t *kek,   size_t *kek_len,   const size_t kek_max)
{
  hal_assert(pkcs8 != NULL && pkcs8_len != NULL && kek != NULL && kek_len != NULL && kek_max > KEK_LENGTH);

  uint8_t rsabuf[hal_rsa_key_t_size];
  hal_rsa_key_t *rsa = NULL;
  hal_error_t err;
  size_t len;

  hal_pkey_slot_t * const pkey  = find_handle(pkey_handle);
  hal_pkey_slot_t * const kekek = find_handle(kekek_handle);

  if (pkey == NULL || kekek == NULL)
    return HAL_ERROR_KEY_NOT_FOUND;

  if ((pkey->flags & HAL_KEY_FLAG_EXPORTABLE) == 0)
    return HAL_ERROR_FORBIDDEN;

  if ((kekek->flags & HAL_KEY_FLAG_USAGE_KEYENCIPHERMENT) == 0)
    return HAL_ERROR_FORBIDDEN;

  if (kekek->type != HAL_KEY_TYPE_RSA_PRIVATE && kekek->type != HAL_KEY_TYPE_RSA_PUBLIC)
    return HAL_ERROR_UNSUPPORTED_KEY;

  if (pkcs8_max < HAL_KS_WRAPPED_KEYSIZE)
    return HAL_ERROR_RESULT_TOO_LONG;

  if ((err = ks_fetch_from_flags(kekek, pkcs8, &len, pkcs8_max)) != HAL_OK)
    goto fail;

  switch (kekek->type) {
  case HAL_KEY_TYPE_RSA_PRIVATE:
    err = hal_rsa_private_key_from_der(&rsa, rsabuf, sizeof(rsabuf), pkcs8, len);
    break;
  case HAL_KEY_TYPE_RSA_PUBLIC:
    err = hal_rsa_public_key_from_der(&rsa, rsabuf, sizeof(rsabuf), pkcs8, len);
    break;
  default:
    err = HAL_ERROR_IMPOSSIBLE;
  }
  if (err != HAL_OK)
    goto fail;

  if ((err = hal_rsa_key_get_modulus(rsa, NULL, kek_len, 0)) != HAL_OK)
    goto fail;

  if (*kek_len > kek_max) {
    err = HAL_ERROR_RESULT_TOO_LONG;
    goto fail;
  }

  if ((err = ks_fetch_from_flags(pkey, pkcs8, &len, pkcs8_max)) != HAL_OK)
    goto fail;

  if ((err = hal_get_random(NULL, kek, KEK_LENGTH)) != HAL_OK)
    goto fail;

  *pkcs8_len = pkcs8_max;
  if ((err = hal_aes_keywrap(NULL, kek, KEK_LENGTH, pkcs8, len, pkcs8, pkcs8_len)) != HAL_OK)
    goto fail;

  if ((err = hal_asn1_encode_pkcs8_encryptedprivatekeyinfo(hal_asn1_oid_aesKeyWrap,
                                                           hal_asn1_oid_aesKeyWrap_len,
                                                           pkcs8, *pkcs8_len,
                                                           pkcs8, pkcs8_len, pkcs8_max)) != HAL_OK)
    goto fail;

  if ((err = pkcs1_5_pad(kek, KEK_LENGTH, kek, *kek_len, 0x02)) != HAL_OK)
    goto fail;

  if ((err = hal_rsa_encrypt(NULL, rsa, kek, *kek_len, kek, *kek_len)) != HAL_OK)
    goto fail;

  if ((err = hal_asn1_encode_pkcs8_encryptedprivatekeyinfo(hal_asn1_oid_rsaEncryption,
                                                           hal_asn1_oid_rsaEncryption_len,
                                                           kek, *kek_len,
                                                           kek, kek_len, kek_max)) != HAL_OK)
    goto fail;

  memset(rsabuf,  0, sizeof(rsabuf));
  return HAL_OK;

 fail:
  memset(pkcs8,   0, pkcs8_max);
  memset(kek, 0, kek_max);
  memset(rsabuf,  0, sizeof(rsabuf));
  *pkcs8_len = *kek_len = 0;
  return err;
}

static hal_error_t pkey_local_import(const hal_client_handle_t client,
                                     const hal_session_handle_t session,
                                     hal_pkey_handle_t *pkey,
                                     hal_uuid_t *name,
                                     const hal_pkey_handle_t kekek_handle,
                                     const uint8_t * const pkcs8, const size_t pkcs8_len,
                                     const uint8_t * const kek_,  const size_t kek_len,
                                     const hal_key_flags_t flags)
{
  hal_assert(pkey != NULL && name != NULL && pkcs8 != NULL && kek_ != NULL && kek_len > 2);

  uint8_t kek[KEK_LENGTH], rsabuf[hal_rsa_key_t_size], der[HAL_KS_WRAPPED_KEYSIZE], *d;
  size_t der_len, oid_len, data_len;
  const uint8_t *oid, *data;
  hal_rsa_key_t *rsa = NULL;
  hal_error_t err;

  hal_pkey_slot_t * const kekek = find_handle(kekek_handle);

  if (kekek == NULL)
    return HAL_ERROR_KEY_NOT_FOUND;

  if ((kekek->flags & HAL_KEY_FLAG_USAGE_KEYENCIPHERMENT) == 0)
    return HAL_ERROR_FORBIDDEN;

  if (kekek->type != HAL_KEY_TYPE_RSA_PRIVATE)
    return HAL_ERROR_UNSUPPORTED_KEY;

  if ((err = ks_fetch_from_flags(kekek, der, &der_len, sizeof(der))) != HAL_OK)
    goto fail;

  if ((err = hal_rsa_private_key_from_der(&rsa, rsabuf, sizeof(rsabuf), der, der_len)) != HAL_OK)
    goto fail;

  if ((err = hal_asn1_decode_pkcs8_encryptedprivatekeyinfo(&oid, &oid_len, &data, &data_len,
                                                           kek_, kek_len)) != HAL_OK)
    goto fail;

  if (oid_len != hal_asn1_oid_rsaEncryption_len ||
      memcmp(oid, hal_asn1_oid_rsaEncryption, oid_len) != 0 ||
      data_len > sizeof(der) ||
      data_len < 2) {
    err = HAL_ERROR_ASN1_PARSE_FAILED;
    goto fail;
  }

  if ((err = hal_rsa_decrypt(NULL, NULL, rsa, data, data_len, der, data_len)) != HAL_OK)
    goto fail;

  if ((err = hal_get_random(NULL, kek, sizeof(kek))) != HAL_OK)
    goto fail;

  d = memchr(der + 2, 0x00, data_len - 2);

  if (der[0] == 0x00 && der[1] == 0x02 && d != NULL && d - der > 10 &&
      der + data_len == d + 1 + KEK_LENGTH)
    memcpy(kek, d + 1, sizeof(kek));

  if ((err = hal_asn1_decode_pkcs8_encryptedprivatekeyinfo(&oid, &oid_len, &data, &data_len,
                                                           pkcs8, pkcs8_len)) != HAL_OK)
    goto fail;

  if (oid_len != hal_asn1_oid_aesKeyWrap_len ||
      memcmp(oid, hal_asn1_oid_aesKeyWrap, oid_len) != 0 ||
      data_len > sizeof(der)) {
    err = HAL_ERROR_ASN1_PARSE_FAILED;
    goto fail;
  }

  der_len = sizeof(der);
  if ((err = hal_aes_keyunwrap(NULL, kek, sizeof(kek), data, data_len, der, &der_len)) != HAL_OK)
    goto fail;

  err = hal_rpc_pkey_load(client, session, pkey, name, der, der_len, flags);

 fail:
  memset(rsabuf, 0, sizeof(rsabuf));
  memset(kek, 0, sizeof(kek));
  memset(der, 0, sizeof(der));
  return err;
}

const hal_rpc_pkey_dispatch_t hal_rpc_local_pkey_dispatch = {
  .load                 = pkey_local_load,
  .open                 = pkey_local_open,
  .generate_rsa         = pkey_local_generate_rsa,
  .generate_ec          = pkey_local_generate_ec,
  .generate_hashsig     = pkey_local_generate_hashsig,
  .close                = pkey_local_close,
  .delete               = pkey_local_delete,
  .get_key_type         = pkey_local_get_key_type,
  .get_key_curve        = pkey_local_get_key_curve,
  .get_key_flags        = pkey_local_get_key_flags,
  .get_public_key_len   = pkey_local_get_public_key_len,
  .get_public_key       = pkey_local_get_public_key,
  .sign                 = pkey_local_sign,
  .verify               = pkey_local_verify,
  .match                = pkey_local_match,
  .set_attributes       = pkey_local_set_attributes,
  .get_attributes       = pkey_local_get_attributes,
  .export               = pkey_local_export,
  .import               = pkey_local_import
};

/*
 * Local variables:
 * indent-tabs-mode: nil
 * End:
 */
