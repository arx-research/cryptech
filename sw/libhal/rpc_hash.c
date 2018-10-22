/*
 * rpc_hash.c
 * ----------
 * Remote procedure call server-side hash implementation.
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

#include <string.h>

#include "hal.h"
#include "hal_internal.h"

/*
 * Need table and handle allocation, including some kind of in_use
 * flag (perhaps just handle == none).
 *
 * Hash and HMAC aren't really things for which we need permission
 * bits, so not sure we even care about login stuff here.
 */

typedef struct {
  hal_client_handle_t client_handle;
  hal_session_handle_t session_handle;
  hal_hash_handle_t hash_handle;
  union {
    hal_hash_state_t *hash;
    hal_hmac_state_t *hmac;
  } state;
} handle_slot_t;

#ifndef	HAL_STATIC_HASH_STATE_BLOCKS
#define	HAL_STATIC_HASH_STATE_BLOCKS 0
#endif

#ifndef	HAL_STATIC_HMAC_STATE_BLOCKS
#define	HAL_STATIC_HMAC_STATE_BLOCKS 0
#endif

#if HAL_STATIC_HASH_STATE_BLOCKS > 0
static handle_slot_t hash_handle[HAL_STATIC_HASH_STATE_BLOCKS];
#endif

#if HAL_STATIC_HMAC_STATE_BLOCKS > 0
static handle_slot_t hmac_handle[HAL_STATIC_HMAC_STATE_BLOCKS];
#endif

/*
 * Handle allocation is simple: we look for an unused (state == NULL)
 * slot in the appropriate table, and, assuming we find one, construct
 * a composite handle consisting of a flag telling us which table this
 * is, the index into the table, and a counter whose sole purpose is
 * to keep the same handle from reoccurring anytime soon, to help
 * identify use-after-free bugs in calling code.
 */

#define	HANDLE_FLAG_HMAC	0x80000000

static inline handle_slot_t *alloc_handle(const int is_hmac)
{
#if HAL_STATIC_HASH_STATE_BLOCKS > 0 || HAL_STATIC_HMAC_STATE_BLOCKS > 0
  static uint16_t next_glop = 0;
  uint32_t glop = ++next_glop << 16;
  next_glop %= 0x7FFF;
#endif

#if HAL_STATIC_HASH_STATE_BLOCKS > 0
  if (!is_hmac) {
    for (size_t i = 0; i < sizeof(hash_handle)/sizeof(*hash_handle); i++) {
      if (hash_handle[i].state.hash != NULL)
        continue;
      hash_handle[i].hash_handle.handle = i | glop;
      return &hash_handle[i];
    }
  }
#endif

#if HAL_STATIC_HMAC_STATE_BLOCKS > 0
  if (is_hmac) {
    for (size_t i = 0; i < sizeof(hmac_handle)/sizeof(*hmac_handle); i++) {
      if (hmac_handle[i].state.hmac != NULL)
        continue;
      hmac_handle[i].hash_handle.handle = i | glop | HANDLE_FLAG_HMAC;
      return &hmac_handle[i];
    }
  }
#endif

  return NULL;
}

/*
 * Check a caller-supplied handle.  Must be in range, in use, and have
 * the right glop.  Returns slot pointer on success, NULL otherwise.
 */

static inline handle_slot_t *find_handle(const hal_hash_handle_t handle)
{
#if HAL_STATIC_HASH_STATE_BLOCKS > 0 || HAL_STATIC_HMAC_STATE_BLOCKS > 0
  const size_t i = (size_t) (handle.handle & 0xFFFF);
  const int is_hmac = (handle.handle & HANDLE_FLAG_HMAC) != 0;
#endif

#if HAL_STATIC_HASH_STATE_BLOCKS > 0
  if (!is_hmac && i < sizeof(hash_handle)/sizeof(*hash_handle) &&
      hash_handle[i].hash_handle.handle == handle.handle && hash_handle[i].state.hash != NULL)
    return &hash_handle[i];
#endif

#if HAL_STATIC_HMAC_STATE_BLOCKS > 0
  if (is_hmac && i < sizeof(hmac_handle)/sizeof(*hmac_handle) &&
      hmac_handle[i].hash_handle.handle == handle.handle && hmac_handle[i].state.hmac != NULL)
    return &hmac_handle[i];
#endif

  return NULL;
}

static inline void free_handle(handle_slot_t *slot)
{
  if (slot != NULL)
    /* state is a union, so this this works for hash and hmac */
    slot->state.hash = NULL;
}

/*
 * Translate an algorithm number to a descriptor.
 */

static inline const hal_hash_descriptor_t *alg_to_descriptor(const hal_digest_algorithm_t alg)
{
  switch (alg) {
  case HAL_DIGEST_ALGORITHM_SHA1:       return hal_hash_sha1;
  case HAL_DIGEST_ALGORITHM_SHA256:     return hal_hash_sha256;
  case HAL_DIGEST_ALGORITHM_SHA512_224: return hal_hash_sha512_224;
  case HAL_DIGEST_ALGORITHM_SHA512_256: return hal_hash_sha512_256;
  case HAL_DIGEST_ALGORITHM_SHA384:     return hal_hash_sha384;
  case HAL_DIGEST_ALGORITHM_SHA512:     return hal_hash_sha512;
  default:                              return NULL;
  }
}

/*
 * Given a slot pointer, fetch the descriptor.
 */

static inline const hal_hash_descriptor_t *slot_to_descriptor(const handle_slot_t * const slot)
{
  if (slot == NULL)
    return NULL;

  if ((slot->hash_handle.handle & HANDLE_FLAG_HMAC) == 0)
    return hal_hash_get_descriptor(slot->state.hash);
  else
    return hal_hmac_get_descriptor(slot->state.hmac);
}

/*
 * Public API
 */

static hal_error_t get_digest_length(const hal_digest_algorithm_t alg, size_t *length)
{
  const hal_hash_descriptor_t * const d = alg_to_descriptor(alg);

  if (d == NULL || length == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  *length = d->digest_length;
  return HAL_OK;
}

static hal_error_t get_digest_algorithm_id(const hal_digest_algorithm_t alg,
                                           uint8_t *id, size_t *len, const size_t len_max)
{
  const hal_hash_descriptor_t * const d = alg_to_descriptor(alg);

  if (d == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  if (len != NULL)
    *len = d->digest_algorithm_id_length;

  if (id == NULL)
    return HAL_OK;

  if (len_max < d->digest_algorithm_id_length)
    return HAL_ERROR_RESULT_TOO_LONG;

  memcpy(id, d->digest_algorithm_id, d->digest_algorithm_id_length);
  return HAL_OK;
}

static hal_error_t get_algorithm(const hal_hash_handle_t handle, hal_digest_algorithm_t *alg)
{
  handle_slot_t *slot = find_handle(handle);
  const hal_hash_descriptor_t *descriptor = slot_to_descriptor(slot);

  if (slot == NULL || alg == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  if (descriptor == NULL)
    return HAL_ERROR_IMPOSSIBLE;

  *alg = descriptor->digest_algorithm;
  return HAL_OK;
}

static hal_error_t initialize(const hal_client_handle_t client,
                              const hal_session_handle_t session,
                              hal_hash_handle_t *hash,
                              const hal_digest_algorithm_t alg,
                              const uint8_t * const key, const size_t key_len)
{
  const hal_hash_descriptor_t *descriptor;
  handle_slot_t *slot;
  hal_error_t err;

  if (hash == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  if ((descriptor = alg_to_descriptor(alg)) == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  if ((slot = alloc_handle(key_len != 0)) == NULL)
    return HAL_ERROR_ALLOCATION_FAILURE;

  slot->client_handle  = client;
  slot->session_handle = session;
  *hash = slot->hash_handle;

  if (key_len == 0)
    err = hal_hash_initialize(NULL, descriptor, &slot->state.hash, NULL, 0);
  else
    err = hal_hmac_initialize(NULL, descriptor, &slot->state.hmac, NULL, 0, key, key_len);
  if (err != HAL_OK)
    free_handle(slot);
  return err;
}

static hal_error_t update(const hal_hash_handle_t handle,
                          const uint8_t * data, const size_t length)
{
  handle_slot_t *slot = find_handle(handle);

  if (slot == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  if ((handle.handle & HANDLE_FLAG_HMAC) == 0)
    return hal_hash_update(slot->state.hash, data, length);
  else
    return hal_hmac_update(slot->state.hmac, data, length);
}

static hal_error_t finalize(const hal_hash_handle_t handle,
                            uint8_t *digest, const size_t length)
{
  handle_slot_t *slot = find_handle(handle);
  hal_error_t err;

  if (slot == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  if ((handle.handle & HANDLE_FLAG_HMAC) == 0) {
    err = hal_hash_finalize(slot->state.hash, digest, length);
    hal_hash_cleanup(&slot->state.hash);
  }

  else {
    err = hal_hmac_finalize(slot->state.hmac, digest, length);
    hal_hmac_cleanup(&slot->state.hmac);
  }

  free_handle(slot);
  return err;
}

const hal_rpc_hash_dispatch_t hal_rpc_local_hash_dispatch = {
  .get_digest_length            = get_digest_length,
  .get_digest_algorithm_id      = get_digest_algorithm_id,
  .get_algorithm                = get_algorithm,
  .initialize                   = initialize,
  .update                       = update,
  .finalize                     = finalize
};

/*
 * Local variables:
 * indent-tabs-mode: nil
 * End:
 */
