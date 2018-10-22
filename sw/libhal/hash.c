/*
 * hash.c
 * ------
 * HAL interface to Cryptech hash cores.
 *
 * Authors: Joachim Strömbergson, Paul Selkirk, Rob Austein
 * Copyright (c) 2014-2018, NORDUnet A/S
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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "hal.h"
#include "hal_internal.h"

/*
 * Whether to include software implementations of the hash cores,
 * for use when the Verilog cores aren't available.
 */

#ifndef HAL_ENABLE_SOFTWARE_HASH_CORES
#define HAL_ENABLE_SOFTWARE_HASH_CORES 0
#endif

/*
 * Use only the software hash cores when running on remote host, without
 * access to the Verilog cores.
 */

#ifndef HAL_ONLY_USE_SOFTWARE_HASH_CORES
#define HAL_ONLY_USE_SOFTWARE_HASH_CORES 0
#endif

#if HAL_ONLY_USE_SOFTWARE_HASH_CORES && ! HAL_ENABLE_SOFTWARE_HASH_CORES
#error HAL_ONLY_USE_SOFTWARE_HASH_CORES && ! HAL_ENABLE_SOFTWARE_HASH_CORES
#endif

typedef hal_error_t (*sw_hash_core_t)(hal_hash_state_t *);

#if HAL_ENABLE_SOFTWARE_HASH_CORES

static hal_error_t sw_hash_core_sha1(  hal_hash_state_t *);
static hal_error_t sw_hash_core_sha256(hal_hash_state_t *);
static hal_error_t sw_hash_core_sha512(hal_hash_state_t *);

#else /* HAL_ENABLE_SOFTWARE_HASH_CORES */

#define sw_hash_core_sha1       ((sw_hash_core_t) 0)
#define sw_hash_core_sha256     ((sw_hash_core_t) 0)
#define sw_hash_core_sha512     ((sw_hash_core_t) 0)

#endif /* HAL_ENABLE_SOFTWARE_HASH_CORES */

#if HAL_ONLY_USE_SOFTWARE_HASH_CORES
#define hal_core_alloc(x, y, z) HAL_ERROR_CORE_NOT_FOUND
#define hal_core_free(x)
#endif

/*
 * HMAC magic numbers.
 */

#define HMAC_IPAD 0x36
#define HMAC_OPAD 0x5c

/*
 * Driver.  This encapsulates whatever per-algorithm voodoo we need
 * this week.  At the moment, this is mostly Cryptech core addresses,
 * but this is subject to change without notice.
 */

struct hal_hash_driver {
  size_t length_length;                 /* Length of the length field */
  hal_addr_t block_addr;                /* Where to write hash blocks */
  hal_addr_t digest_addr;               /* Where to read digest */
  uint8_t ctrl_mode;                    /* Digest mode, for cores that have modes */
  sw_hash_core_t sw_core;               /* Software implementation, when enabled */
  size_t sw_word_size;                  /* Word size for software implementation */
};

/*
 * Hash state.  For now we assume that the only core state we need to
 * save and restore is the current digest value.
 */

struct hal_hash_state {
  hal_core_t *core;
  const hal_hash_descriptor_t *descriptor;
  const hal_hash_driver_t *driver;
  uint64_t msg_length_high;                     /* Total data hashed in this message */
  uint64_t msg_length_low;                      /* (128 bits in SHA-512 cases) */
  uint8_t block[HAL_MAX_HASH_BLOCK_LENGTH],     /* Block we're accumulating */
    core_state[HAL_MAX_HASH_DIGEST_LENGTH];     /* Saved core state */
  size_t block_used;                            /* How much of the block we've used */
  unsigned block_count;                         /* Blocks sent */
  unsigned flags;
  hal_core_lru_t pomace;                        /* Private data for hal_core_alloc() */
};

#define STATE_FLAG_STATE_ALLOCATED      0x1     /* State buffer in use */
#define STATE_FLAG_SOFTWARE_CORE        0x2     /* Use software rather than hardware core */
#define STATE_FLAG_FREE_CORE            0x4     /* Free core after use */

/*
 * HMAC state.  Right now this just holds the key block and a hash
 * context; if and when we figure out how PCLSR the hash cores, we
 * might want to save a lot more than that, and may also want to
 * reorder certain operations during HMAC initialization to get a
 * performance boost for things like PBKDF2.
 */

struct hal_hmac_state {
  hal_hash_state_t hash_state;               /* Hash state */
  uint8_t keybuf[HAL_MAX_HASH_BLOCK_LENGTH]; /* HMAC key */
};

/*
 * Drivers for known digest algorithms.
 */

static const hal_hash_driver_t sha1_driver = {
  SHA1_LENGTH_LEN, SHA1_ADDR_BLOCK, SHA1_ADDR_DIGEST, 0, sw_hash_core_sha1, sizeof(uint32_t)
};

static const hal_hash_driver_t sha224_driver = {
  SHA256_LENGTH_LEN, SHA256_ADDR_BLOCK, SHA256_ADDR_DIGEST, SHA256_MODE_SHA_224, sw_hash_core_sha256, sizeof(uint32_t)
};

static const hal_hash_driver_t sha256_driver = {
  SHA256_LENGTH_LEN, SHA256_ADDR_BLOCK, SHA256_ADDR_DIGEST, SHA256_MODE_SHA_256, sw_hash_core_sha256, sizeof(uint32_t)
};

static const hal_hash_driver_t sha512_224_driver = {
  SHA512_LENGTH_LEN, SHA512_ADDR_BLOCK, SHA512_ADDR_DIGEST, SHA512_MODE_SHA_512_224, sw_hash_core_sha512, sizeof(uint64_t)
};

static const hal_hash_driver_t sha512_256_driver = {
  SHA512_LENGTH_LEN, SHA512_ADDR_BLOCK, SHA512_ADDR_DIGEST, SHA512_MODE_SHA_512_256, sw_hash_core_sha512, sizeof(uint64_t)
};

static const hal_hash_driver_t sha384_driver = {
  SHA512_LENGTH_LEN, SHA512_ADDR_BLOCK, SHA512_ADDR_DIGEST, SHA512_MODE_SHA_384, sw_hash_core_sha512, sizeof(uint64_t)
};

static const hal_hash_driver_t sha512_driver = {
  SHA512_LENGTH_LEN, SHA512_ADDR_BLOCK, SHA512_ADDR_DIGEST, SHA512_MODE_SHA_512, sw_hash_core_sha512, sizeof(uint64_t)
};

/*
 * Digest algorithm identifiers: DER encoded full TLV of an
 * DigestAlgorithmIdentifier SEQUENCE including OID for the algorithm in
 * question and a NULL parameters value.
 *
 * See RFC 2313 and the NIST algorithm registry:
 * http://csrc.nist.gov/groups/ST/crypto_apps_infra/csor/algorithms.html
 *
 * The DER encoding is too complex to generate in the C preprocessor,
 * and we want these as compile-time constants, so we just supply the
 * raw hex encoding here.  If this gets seriously out of control we'll
 * write a script to generate a header file we can include.
 */

static const uint8_t
  dalgid_sha1[]       = { 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0e, 0x03, 0x02, 0x1a, 0x05, 0x00 },
  dalgid_sha256[]     = { 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05, 0x00 },
  dalgid_sha384[]     = { 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x02, 0x05, 0x00 },
  dalgid_sha512[]     = { 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x03, 0x05, 0x00 },
  dalgid_sha224[]     = { 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x04, 0x05, 0x00 },
  dalgid_sha512_224[] = { 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x05, 0x05, 0x00 },
  dalgid_sha512_256[] = { 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x06, 0x05, 0x00 };

/*
 * Descriptors.  Yes, the {hash,hmac}_state_length fields are a bit
 * repetitive given that they (currently) have the same value
 * regardless of algorithm, but we don't want to wire in that
 * assumption, so it's simplest to be explicit.
 */

const hal_hash_descriptor_t hal_hash_sha1[1] = {{
  HAL_DIGEST_ALGORITHM_SHA1,
  SHA1_BLOCK_LEN, SHA1_DIGEST_LEN,
  sizeof(hal_hash_state_t), sizeof(hal_hmac_state_t),
  dalgid_sha1, sizeof(dalgid_sha1),
  &sha1_driver, SHA1_NAME, 0
}};

const hal_hash_descriptor_t hal_hash_sha224[1] = {{
  HAL_DIGEST_ALGORITHM_SHA256,
  SHA256_BLOCK_LEN, SHA224_DIGEST_LEN,
  sizeof(hal_hash_state_t), sizeof(hal_hmac_state_t),
  dalgid_sha224, sizeof(dalgid_sha224),
  &sha224_driver, SHA256_NAME, 1
}};

const hal_hash_descriptor_t hal_hash_sha256[1] = {{
  HAL_DIGEST_ALGORITHM_SHA256,
  SHA256_BLOCK_LEN, SHA256_DIGEST_LEN,
  sizeof(hal_hash_state_t), sizeof(hal_hmac_state_t),
  dalgid_sha256, sizeof(dalgid_sha256),
  &sha256_driver, SHA256_NAME, 1
}};

const hal_hash_descriptor_t hal_hash_sha512_224[1] = {{
  HAL_DIGEST_ALGORITHM_SHA512_224,
  SHA512_BLOCK_LEN, SHA512_224_DIGEST_LEN,
  sizeof(hal_hash_state_t), sizeof(hal_hmac_state_t),
  dalgid_sha512_224, sizeof(dalgid_sha512_224),
  &sha512_224_driver, SHA512_NAME, 1
}};

const hal_hash_descriptor_t hal_hash_sha512_256[1] = {{
  HAL_DIGEST_ALGORITHM_SHA512_256,
  SHA512_BLOCK_LEN, SHA512_256_DIGEST_LEN,
  sizeof(hal_hash_state_t), sizeof(hal_hmac_state_t),
  dalgid_sha512_256, sizeof(dalgid_sha512_256),
  &sha512_256_driver, SHA512_NAME, 1
}};

const hal_hash_descriptor_t hal_hash_sha384[1] = {{
  HAL_DIGEST_ALGORITHM_SHA384,
  SHA512_BLOCK_LEN, SHA384_DIGEST_LEN,
  sizeof(hal_hash_state_t), sizeof(hal_hmac_state_t),
  dalgid_sha384, sizeof(dalgid_sha384),
  &sha384_driver, SHA512_NAME, 1
}};

const hal_hash_descriptor_t hal_hash_sha512[1] = {{
  HAL_DIGEST_ALGORITHM_SHA512,
  SHA512_BLOCK_LEN, SHA512_DIGEST_LEN,
  sizeof(hal_hash_state_t), sizeof(hal_hmac_state_t),
  dalgid_sha512, sizeof(dalgid_sha512),
  &sha512_driver, SHA512_NAME, 1
}};

/*
 * Static state blocks.  This library is intended for a style of
 * embedded programming in which one avoids heap-based allocation
 * functions such as malloc() wherever possible and instead uses
 * static variables when just allocating on the stack won't do.
 *
 * The number of each kind of state block to be allocated this way
 * must be configured at compile-time.  Sorry, that's life in the
 * deeply embedded universe.
 */

#ifndef HAL_STATIC_HASH_STATE_BLOCKS
#define HAL_STATIC_HASH_STATE_BLOCKS 0
#endif

#ifndef HAL_STATIC_HMAC_STATE_BLOCKS
#define HAL_STATIC_HMAC_STATE_BLOCKS 0
#endif

#if HAL_STATIC_HASH_STATE_BLOCKS > 0
static hal_hash_state_t static_hash_state[HAL_STATIC_HASH_STATE_BLOCKS];
#endif

#if HAL_STATIC_HMAC_STATE_BLOCKS > 0
static hal_hmac_state_t static_hmac_state[HAL_STATIC_HMAC_STATE_BLOCKS];
#endif

/*
 * Debugging control.
 */

static int debug = 0;

void hal_hash_set_debug(int onoff)
{
  debug = onoff;
}

/*
 * Internal utilities to allocate static state blocks.
 */

static inline hal_hash_state_t *alloc_static_hash_state(void)
{

#if HAL_STATIC_HASH_STATE_BLOCKS > 0

  for (size_t i = 0; i < sizeof(static_hash_state)/sizeof(*static_hash_state); i++)
    if ((static_hash_state[i].flags & STATE_FLAG_STATE_ALLOCATED) == 0)
      return &static_hash_state[i];

#endif

  return NULL;
}

static inline hal_hmac_state_t *alloc_static_hmac_state(void)
{

#if HAL_STATIC_HMAC_STATE_BLOCKS > 0

  for (size_t i = 0; i < sizeof(static_hmac_state)/sizeof(*static_hmac_state); i++)
    if ((static_hmac_state[i].hash_state.flags & STATE_FLAG_STATE_ALLOCATED) == 0)
      return &static_hmac_state[i];

#endif

  return NULL;
}

/*
 * Internal utility to do a sort of byte-swapping memcpy() (sigh).
 * This is only used by the software hash cores, but it's simpler to define it unconditionally.
 */

static inline hal_error_t swytebop(void *out_, const void * const in_, const size_t n, const size_t w)
{
  const uint8_t  order[] = { 0x01, 0x02, 0x03, 0x04 };

  const uint8_t * const in = in_;
  uint8_t *out = out_;

  /* w must be a power of two */
  hal_assert(in != out && in != NULL && out != NULL && w && !(w & (w - 1)));

  switch (* (uint32_t *) order) {

  case 0x01020304:
    memcpy(out, in, n);
    break;

  case 0x04030201:
    for (size_t i = 0; i < n; i += w)
      for (size_t j = 0; j < w && i + j < n; j++)
        out[i + j] = in[i + w - j - 1];
    break;

  default:
    hal_assert((* (uint32_t *) order) == 0x01020304 || (* (uint32_t *) order) == 0x04030201);
  }
  return HAL_OK;
}

/*
 * Internal utility to check core against descriptor, including
 * attempting to locate an appropriate core if we weren't given one.
 */

static inline hal_error_t check_core(hal_core_t **core,
                                     const hal_hash_descriptor_t * const descriptor,
                                     unsigned *flags,
                                     hal_core_lru_t *pomace)
{
  if (core == NULL || descriptor == NULL || descriptor->driver == NULL || flags == NULL)
    return HAL_ERROR_IMPOSSIBLE;

  hal_error_t err = HAL_ERROR_CORE_NOT_FOUND;

#if !HAL_ONLY_USE_SOFTWARE_HASH_CORES

  if (*core != NULL)
    return HAL_OK;

  if ((err = hal_core_alloc(descriptor->core_name, core, pomace)) == HAL_OK) {
    *flags |= STATE_FLAG_FREE_CORE;
    return HAL_OK;
  }

#endif

  if (*core != NULL)
    return HAL_ERROR_IMPOSSIBLE;

#if HAL_ENABLE_SOFTWARE_HASH_CORES

  if (descriptor->driver->sw_core && err == HAL_ERROR_CORE_NOT_FOUND) {
    *flags |= STATE_FLAG_SOFTWARE_CORE;
    return HAL_OK;
  }

#endif

  return err;
}

/*
 * Internal utility to do whatever checking we need of a descriptor,
 * then extract the driver pointer in a way that works nicely with
 * initialization of an automatic const pointer.
 *
 * Returns the driver pointer on success, NULL on failure.
 */

static inline const hal_hash_driver_t *check_driver(const hal_hash_descriptor_t * const descriptor)
{
  return descriptor == NULL ? NULL : descriptor->driver;
}

/*
 * Initialize hash state.
 */

hal_error_t hal_hash_initialize(hal_core_t *core,
                                const hal_hash_descriptor_t * const descriptor,
                                hal_hash_state_t **state_,
                                void *state_buffer, const size_t state_length)
{
  const hal_hash_driver_t * const driver = check_driver(descriptor);
  hal_hash_state_t *state = state_buffer;
  hal_core_lru_t pomace = 0;
  unsigned flags = 0;
  hal_error_t err;

  if (driver == NULL || state_ == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  if (state_buffer != NULL && state_length < descriptor->hash_state_length)
    return HAL_ERROR_BAD_ARGUMENTS;

  if ((err = check_core(&core, descriptor, &flags, &pomace)) != HAL_OK)
    return err;

  if ((flags & STATE_FLAG_FREE_CORE) != 0)
    hal_core_free(core);

  /* A dynamically allocated core that can't restore state isn't going to work. */
  if (!descriptor->can_restore_state && (flags & STATE_FLAG_FREE_CORE) != 0)
    return HAL_ERROR_BAD_ARGUMENTS;

  if (state_buffer == NULL && (state = alloc_static_hash_state()) == NULL)
      return HAL_ERROR_ALLOCATION_FAILURE;

  memset(state, 0, sizeof(*state));
  state->descriptor = descriptor;
  state->driver = driver;
  state->core = core;
  state->flags = flags | STATE_FLAG_STATE_ALLOCATED;
  state->pomace = pomace;

  *state_ = state;

  return HAL_OK;
}

/*
 * Clean up hash state.
 */

void hal_hash_cleanup(hal_hash_state_t **state)
{
  if (state == NULL || *state == NULL)
    return;

  memset(*state, 0, (*state)->descriptor->hash_state_length);

  *state = NULL;
}

#if ! HAL_ONLY_USE_SOFTWARE_HASH_CORES

/*
 * Read hash result from core.  At least for now, this also serves to
 * read current hash state from core.
 */

static hal_error_t hash_read_digest(const hal_core_t *core,
                                    const hal_hash_driver_t * const driver,
                                    uint8_t *digest,
                                    const size_t digest_length)
{
  hal_error_t err;

  hal_assert(digest != NULL && digest_length % 4 == 0);

  if ((err = hal_io_wait_valid(core)) != HAL_OK)
    return err;

  return hal_io_read(core, driver->digest_addr, digest, digest_length);
}

/*
 * Write hash state back to core.
 */

static hal_error_t hash_write_digest(const hal_core_t *core,
                                     const hal_hash_driver_t * const driver,
                                     const uint8_t * const digest,
                                     const size_t digest_length)
{
  hal_error_t err;

  hal_assert(digest != NULL && digest_length % 4 == 0);

  if ((err = hal_io_wait_ready(core)) != HAL_OK)
    return err;

  return hal_io_write(core, driver->digest_addr, digest, digest_length);
}

#endif

/*
 * Send one block to a core.
 */

static hal_error_t hash_write_block(hal_hash_state_t * const state)
{
  hal_assert(state != NULL && state->descriptor != NULL && state->driver != NULL);
  hal_assert(state->descriptor->block_length % 4 == 0);

  hal_assert(state->descriptor->digest_length <= sizeof(state->core_state) ||
             !state->descriptor->can_restore_state);

  if (debug)
    hal_log(HAL_LOG_DEBUG, "[ %s ]\n", state->block_count == 0 ? "init" : "next");

#if HAL_ENABLE_SOFTWARE_HASH_CORES
  if ((state->flags & STATE_FLAG_SOFTWARE_CORE) != 0)
    return state->driver->sw_core(state);
#endif

#if ! HAL_ONLY_USE_SOFTWARE_HASH_CORES
  uint8_t ctrl_cmd[4];
  hal_error_t err;

  if ((err = hal_io_wait_ready(state->core)) != HAL_OK)
    return err;

  if (state->descriptor->can_restore_state &&
      state->block_count != 0 &&
      (err = hash_write_digest(state->core, state->driver, state->core_state,
                               state->descriptor->digest_length)) != HAL_OK)
    return err;

  if ((err = hal_io_write(state->core, state->driver->block_addr, state->block,
                          state->descriptor->block_length)) != HAL_OK)
    return err;

  ctrl_cmd[0] = ctrl_cmd[1] = ctrl_cmd[2] = 0;
  ctrl_cmd[3] = state->block_count == 0 ? CTRL_INIT : CTRL_NEXT;
  ctrl_cmd[3] |= state->driver->ctrl_mode;

  if ((err = hal_io_write(state->core, ADDR_CTRL, ctrl_cmd, sizeof(ctrl_cmd))) != HAL_OK)
    return err;

  if (state->descriptor->can_restore_state &&
      (err = hash_read_digest(state->core, state->driver, state->core_state,
                              state->descriptor->digest_length)) != HAL_OK)
    return err;

  return hal_io_wait_valid(state->core);
#endif

  /*NOTREACHED*/
  return HAL_ERROR_IMPOSSIBLE;
}

/*
 * Add data to hash.
 */

hal_error_t hal_hash_update(hal_hash_state_t *state,            /* Opaque state block */
                            const uint8_t * const data_buffer,  /* Data to be hashed */
                            size_t data_buffer_length)          /* Length of data_buffer */
{
  const uint8_t *p = data_buffer;
  hal_error_t err = HAL_OK;
  size_t n;

  if (state == NULL || data_buffer == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  if (data_buffer_length == 0)
    return HAL_OK;

  hal_assert(state->descriptor != NULL && state->driver != NULL);
  hal_assert(state->descriptor->block_length <= sizeof(state->block));

  if ((state->flags & STATE_FLAG_FREE_CORE) != 0) {
    err = hal_core_alloc(state->descriptor->core_name, &state->core, &state->pomace);
    if (err == HAL_ERROR_CORE_REASSIGNED) {
      state->core = NULL;
      err = hal_core_alloc(state->descriptor->core_name, &state->core, &state->pomace);
    }
    if (err != HAL_OK)
      return err;
  }

  while ((n = state->descriptor->block_length - state->block_used) <= data_buffer_length) {
    /*
     * We have enough data for another complete block.
     */
    if (debug)
      hal_log(HAL_LOG_DEBUG, "[ Full block, data_buffer_length %lu, used %lu, n %lu, msg_length %llu ]\n",
              (unsigned long) data_buffer_length, (unsigned long) state->block_used, (unsigned long) n, (unsigned long long)state->msg_length_low);
    memcpy(state->block + state->block_used, p, n);
    if ((state->msg_length_low += n) < n)
      state->msg_length_high++;
    state->block_used = 0;
    data_buffer_length -= n;
    p += n;
    if ((err = hash_write_block(state)) != HAL_OK)
      goto out;
    state->block_count++;
  }

  if (data_buffer_length > 0) {
    /*
     * Data left over, but not enough for a full block, stash it.
     */
    if (debug)
      hal_log(HAL_LOG_DEBUG, "[ Partial block, data_buffer_length %lu, used %lu, n %lu, msg_length %llu ]\n",
              (unsigned long) data_buffer_length, (unsigned long) state->block_used, (unsigned long) n, (unsigned long long)state->msg_length_low);
    hal_assert(data_buffer_length < n);
    memcpy(state->block + state->block_used, p, data_buffer_length);
    if ((state->msg_length_low += data_buffer_length) < data_buffer_length)
      state->msg_length_high++;
    state->block_used += data_buffer_length;
  }

out:
  if ((state->flags & STATE_FLAG_FREE_CORE) != 0)
    hal_core_free(state->core);
  return err;
}

/*
 * Finish hash and return digest.
 */

hal_error_t hal_hash_finalize(hal_hash_state_t *state,                  /* Opaque state block */
                              uint8_t *digest_buffer,                   /* Returned digest */
                              const size_t digest_buffer_length)        /* Length of digest_buffer */
{
  uint64_t bit_length_high, bit_length_low;
  hal_error_t err;
  uint8_t *p;
  size_t n;
  size_t i;

  if (state == NULL || digest_buffer == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  hal_assert(state->descriptor != NULL && state->driver != NULL);

  if (digest_buffer_length < state->descriptor->digest_length)
    return HAL_ERROR_BAD_ARGUMENTS;

  hal_assert(state->descriptor->block_length <= sizeof(state->block));

  if ((state->flags & STATE_FLAG_FREE_CORE) != 0) {
    err = hal_core_alloc(state->descriptor->core_name, &state->core, &state->pomace);
    if (err == HAL_ERROR_CORE_REASSIGNED) {
      state->core = NULL;
      err = hal_core_alloc(state->descriptor->core_name, &state->core, &state->pomace);
    }
    if (err != HAL_OK)
      return err;
  }

  /*
   * Add padding, then pull result from the core
   */

  bit_length_low  = (state->msg_length_low  << 3);
  bit_length_high = (state->msg_length_high << 3) | (state->msg_length_low >> 61);

  /* Initial pad byte */
  hal_assert(state->block_used < state->descriptor->block_length);
  state->block[state->block_used++] = 0x80;

  /* If not enough room for bit count, zero and push current block */
  if ((n = state->descriptor->block_length - state->block_used) < state->driver->length_length) {
    if (debug)
      hal_log(HAL_LOG_DEBUG, "[ Overflow block, used %lu, n %lu, msg_length %llu ]\n",
              (unsigned long) state->block_used, (unsigned long) n, (unsigned long long)state->msg_length_low);
    if (n > 0)
      memset(state->block + state->block_used, 0, n);
    if ((err = hash_write_block(state)) != HAL_OK)
      goto out;
    state->block_count++;
    state->block_used = 0;
  }

  /* Pad final block */
  n = state->descriptor->block_length - state->block_used;
  hal_assert(n >= state->driver->length_length);
  if (n > 0)
    memset(state->block + state->block_used, 0, n);
  if (debug)
    hal_log(HAL_LOG_DEBUG, "[ Final block, used %lu, n %lu, msg_length %llu ]\n",
            (unsigned long) state->block_used, (unsigned long) n, (unsigned long long)state->msg_length_low);
  p = state->block + state->descriptor->block_length;
  for (i = 0; (bit_length_low || bit_length_high) && i < state->driver->length_length; i++) {
    *--p = (uint8_t) (bit_length_low & 0xFF);
    bit_length_low >>= 8;
    if (bit_length_high) {
      bit_length_low |= ((bit_length_high & 0xFF) << 56);
      bit_length_high >>= 8;
    }
  }

  /* Push final block */
  if ((err = hash_write_block(state)) != HAL_OK)
    goto out;
  state->block_count++;

  /* All data pushed to core, now we just need to read back the result */
#if HAL_ENABLE_SOFTWARE_HASH_CORES
  if ((state->flags & STATE_FLAG_SOFTWARE_CORE) != 0)
    return swytebop(digest_buffer, state->core_state, state->descriptor->digest_length,
                    state->driver->sw_word_size);
#endif
#if ! HAL_ONLY_USE_SOFTWARE_HASH_CORES
  if ((state->flags & STATE_FLAG_SOFTWARE_CORE) == 0)
    err = hash_read_digest(state->core, state->driver, digest_buffer, state->descriptor->digest_length);
#endif

out:
  if ((state->flags & STATE_FLAG_FREE_CORE) != 0)
    hal_core_free(state->core);
  return err;
}

/*
 * Initialize HMAC state.
 */

hal_error_t hal_hmac_initialize(hal_core_t *core,
                                const hal_hash_descriptor_t * const descriptor,
                                hal_hmac_state_t **state_,
                                void *state_buffer, const size_t state_length,
                                const uint8_t * const key, const size_t key_length)
{
  const hal_hash_driver_t * const driver = check_driver(descriptor);
  hal_hmac_state_t *state = state_buffer;
  hal_error_t err;
  size_t i;

  if (descriptor == NULL || driver == NULL || state_ == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  if (state_buffer != NULL && state_length < descriptor->hmac_state_length)
    return HAL_ERROR_BAD_ARGUMENTS;

  if (state_buffer == NULL && (state = alloc_static_hmac_state()) == NULL)
    return HAL_ERROR_ALLOCATION_FAILURE;

  hal_hash_state_t *h = &state->hash_state;

  hal_assert(descriptor->block_length <= sizeof(state->keybuf));

#if 0
  /*
   * RFC 2104 frowns upon keys shorter than the digest length.
   * ... but most of the test vectors fail this test!
   */

  if (key_length < descriptor->digest_length)
    return HAL_ERROR_UNSUPPORTED_KEY;
#endif

  if ((err = hal_hash_initialize(core, descriptor, &h, &state->hash_state,
                                 sizeof(state->hash_state))) != HAL_OK)
    goto fail;

  /*
   * If the supplied HMAC key is longer than the hash block length, we
   * need to hash the supplied HMAC key to get the real HMAC key.
   * Otherwise, we just use the supplied HMAC key directly.
   */

  memset(state->keybuf, 0, sizeof(state->keybuf));

  if (key_length <= descriptor->block_length)
    memcpy(state->keybuf, key, key_length);

  else if ((err = hal_hash_update(h, key, key_length))                         != HAL_OK ||
           (err = hal_hash_finalize(h, state->keybuf, sizeof(state->keybuf)))  != HAL_OK ||
           (err = hal_hash_initialize(core, descriptor, &h, &state->hash_state,
                                      sizeof(state->hash_state)))              != HAL_OK)
    goto fail;

  /*
   * XOR the key with the IPAD value, then start the inner hash.
   */

  for (i = 0; i < descriptor->block_length; i++)
    state->keybuf[i] ^= HMAC_IPAD;

  if ((err = hal_hash_update(h, state->keybuf, descriptor->block_length)) != HAL_OK)
    goto fail;

  /*
   * Prepare the key for the final hash.  Since we just XORed key with
   * IPAD, we need to XOR with both IPAD and OPAD to get key XOR OPAD.
   */

  for (i = 0; i < descriptor->block_length; i++)
    state->keybuf[i] ^= HMAC_IPAD ^ HMAC_OPAD;

  /*
   * If we had some good way of saving all of our state (including
   * state internal to the hash core), this would be a good place to
   * do it, since it might speed up algorithms like PBKDF2 which do
   * repeated HMAC operations using the same key.  Revisit this if and
   * when the hash cores support such a thing.
   */

  *state_ = state;

  return HAL_OK;

 fail:
  if (state_buffer == NULL)
    free(state);
  return err;
}

/*
 * Clean up HMAC state.
 */

void hal_hmac_cleanup(hal_hmac_state_t **state)
{
  if (state == NULL || *state == NULL)
    return;

  memset(*state, 0, (*state)->hash_state.descriptor->hmac_state_length);

  *state = NULL;
}

/*
 * Add data to HMAC.
 */

hal_error_t hal_hmac_update(hal_hmac_state_t *state,
                            const uint8_t * data, const size_t length)
{
  if (state == NULL || data == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  return hal_hash_update(&state->hash_state, data, length);
}

/*
 * Finish and return HMAC.
 */

hal_error_t hal_hmac_finalize(hal_hmac_state_t *state,
                              uint8_t *hmac, const size_t length)
{
  if (state == NULL || hmac == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  hal_hash_state_t *h = &state->hash_state;
  const hal_hash_descriptor_t *descriptor = h->descriptor;
  uint8_t d[HAL_MAX_HASH_DIGEST_LENGTH];
  hal_error_t err;

  hal_assert(descriptor != NULL && descriptor->digest_length <= sizeof(d));

  /*
   * Finish up inner hash and extract digest, then perform outer hash
   * to get HMAC.  Key was prepared for this in hal_hmac_initialize().
   *
   * For silly reasons, reusing the core value from the hash state
   * block here would require nontrivial refactoring, so for the
   * moment pass NULL and let the core allocator deal.  Fix someday.
   */

  if ((err = hal_hash_finalize(h, d, sizeof(d)))                           != HAL_OK ||
      (err = hal_hash_initialize(NULL, descriptor, &h, &state->hash_state,
                                 sizeof(state->hash_state)))               != HAL_OK ||
      (err = hal_hash_update(h, state->keybuf, descriptor->block_length))  != HAL_OK ||
      (err = hal_hash_update(h, d, descriptor->digest_length))             != HAL_OK ||
      (err = hal_hash_finalize(h, hmac, length))                           != HAL_OK)
    return err;

  return HAL_OK;
}

/*
 * Pull descriptor pointer from state block.
 */

const hal_hash_descriptor_t *hal_hash_get_descriptor(const hal_hash_state_t * const state)
{
  return state == NULL ? NULL : state->descriptor;
}

const hal_hash_descriptor_t *hal_hmac_get_descriptor(const hal_hmac_state_t * const state)
{
  return state == NULL ? NULL : state->hash_state.descriptor;
}

#if HAL_ENABLE_SOFTWARE_HASH_CORES

/*
 * Software implementations of hash cores.
 *
 * This is based in part on a mix of Tom St Denis's libtomcrypt C
 * implementation and Joachim Strömbergson's Python models for the
 * Cryptech hash cores.
 *
 * This is not a particularly high performance implementation, as
 * we've given priority to portability and simplicity over speed.
 * We assume that any reasonable modern compiler can handle inline
 * functions, loop unrolling, and optimization of expressions which
 * become constant upon inlining and unrolling.
 */

/*
 * K constants for SHA-2.  SHA-1 only uses four K constants, which are handled inline
 * due to other peculiarities of the SHA-1 algorithm).
 */

static const uint32_t sha256_K[64] = {
    0x428A2F98UL, 0x71374491UL, 0xB5C0FBCFUL, 0xE9B5DBA5UL, 0x3956C25BUL, 0x59F111F1UL, 0x923F82A4UL, 0xAB1C5ED5UL,
    0xD807AA98UL, 0x12835B01UL, 0x243185BEUL, 0x550C7DC3UL, 0x72BE5D74UL, 0x80DEB1FEUL, 0x9BDC06A7UL, 0xC19BF174UL,
    0xE49B69C1UL, 0xEFBE4786UL, 0x0FC19DC6UL, 0x240CA1CCUL, 0x2DE92C6FUL, 0x4A7484AAUL, 0x5CB0A9DCUL, 0x76F988DAUL,
    0x983E5152UL, 0xA831C66DUL, 0xB00327C8UL, 0xBF597FC7UL, 0xC6E00BF3UL, 0xD5A79147UL, 0x06CA6351UL, 0x14292967UL,
    0x27B70A85UL, 0x2E1B2138UL, 0x4D2C6DFCUL, 0x53380D13UL, 0x650A7354UL, 0x766A0ABBUL, 0x81C2C92EUL, 0x92722C85UL,
    0xA2BFE8A1UL, 0xA81A664BUL, 0xC24B8B70UL, 0xC76C51A3UL, 0xD192E819UL, 0xD6990624UL, 0xF40E3585UL, 0x106AA070UL,
    0x19A4C116UL, 0x1E376C08UL, 0x2748774CUL, 0x34B0BCB5UL, 0x391C0CB3UL, 0x4ED8AA4AUL, 0x5B9CCA4FUL, 0x682E6FF3UL,
    0x748F82EEUL, 0x78A5636FUL, 0x84C87814UL, 0x8CC70208UL, 0x90BEFFFAUL, 0xA4506CEBUL, 0xBEF9A3F7UL, 0xC67178F2UL
};

static const uint64_t sha512_K[80] = {
  0x428A2F98D728AE22ULL, 0x7137449123EF65CDULL, 0xB5C0FBCFEC4D3B2FULL, 0xE9B5DBA58189DBBCULL,
  0x3956C25BF348B538ULL, 0x59F111F1B605D019ULL, 0x923F82A4AF194F9BULL, 0xAB1C5ED5DA6D8118ULL,
  0xD807AA98A3030242ULL, 0x12835B0145706FBEULL, 0x243185BE4EE4B28CULL, 0x550C7DC3D5FFB4E2ULL,
  0x72BE5D74F27B896FULL, 0x80DEB1FE3B1696B1ULL, 0x9BDC06A725C71235ULL, 0xC19BF174CF692694ULL,
  0xE49B69C19EF14AD2ULL, 0xEFBE4786384F25E3ULL, 0x0FC19DC68B8CD5B5ULL, 0x240CA1CC77AC9C65ULL,
  0x2DE92C6F592B0275ULL, 0x4A7484AA6EA6E483ULL, 0x5CB0A9DCBD41FBD4ULL, 0x76F988DA831153B5ULL,
  0x983E5152EE66DFABULL, 0xA831C66D2DB43210ULL, 0xB00327C898FB213FULL, 0xBF597FC7BEEF0EE4ULL,
  0xC6E00BF33DA88FC2ULL, 0xD5A79147930AA725ULL, 0x06CA6351E003826FULL, 0x142929670A0E6E70ULL,
  0x27B70A8546D22FFCULL, 0x2E1B21385C26C926ULL, 0x4D2C6DFC5AC42AEDULL, 0x53380D139D95B3DFULL,
  0x650A73548BAF63DEULL, 0x766A0ABB3C77B2A8ULL, 0x81C2C92E47EDAEE6ULL, 0x92722C851482353BULL,
  0xA2BFE8A14CF10364ULL, 0xA81A664BBC423001ULL, 0xC24B8B70D0F89791ULL, 0xC76C51A30654BE30ULL,
  0xD192E819D6EF5218ULL, 0xD69906245565A910ULL, 0xF40E35855771202AULL, 0x106AA07032BBD1B8ULL,
  0x19A4C116B8D2D0C8ULL, 0x1E376C085141AB53ULL, 0x2748774CDF8EEB99ULL, 0x34B0BCB5E19B48A8ULL,
  0x391C0CB3C5C95A63ULL, 0x4ED8AA4AE3418ACBULL, 0x5B9CCA4F7763E373ULL, 0x682E6FF3D6B2B8A3ULL,
  0x748F82EE5DEFB2FCULL, 0x78A5636F43172F60ULL, 0x84C87814A1F0AB72ULL, 0x8CC702081A6439ECULL,
  0x90BEFFFA23631E28ULL, 0xA4506CEBDE82BDE9ULL, 0xBEF9A3F7B2C67915ULL, 0xC67178F2E372532BULL,
  0xCA273ECEEA26619CULL, 0xD186B8C721C0C207ULL, 0xEADA7DD6CDE0EB1EULL, 0xF57D4F7FEE6ED178ULL,
  0x06F067AA72176FBAULL, 0x0A637DC5A2C898A6ULL, 0x113F9804BEF90DAEULL, 0x1B710B35131C471BULL,
  0x28DB77F523047D84ULL, 0x32CAAB7B40C72493ULL, 0x3C9EBE0A15C9BEBCULL, 0x431D67C49C100D4CULL,
  0x4CC5D4BECB3E42B6ULL, 0x597F299CFC657E2AULL, 0x5FCB6FAB3AD6FAECULL, 0x6C44198C4A475817ULL
};

/*
 * Various bit twiddling operations.  We use inline functions rather than macros to get better
 * data type checking, sane argument semantics, and simpler expressions (this stuff is
 * confusing enough without adding a lot of unnecessary C macro baggage).
 */

static inline uint32_t rot_l_32(uint32_t x, unsigned n) { return ((x << n) | (x >> (32 - n))); }
static inline uint32_t rot_r_32(uint32_t x, unsigned n) { return ((x >> n) | (x << (32 - n))); }
static inline uint32_t lsh_r_32(uint32_t x, unsigned n) { return (x >> n); }

static inline uint64_t rot_r_64(uint64_t x, unsigned n) { return ((x >> n) | (x << (64 - n))); }
static inline uint64_t lsh_r_64(uint64_t x, unsigned n) { return (x >> n); }

static inline uint32_t Choose_32(  uint32_t x, uint32_t y, uint32_t z) { return (z ^ (x & (y ^ z)));       }
static inline uint32_t Majority_32(uint32_t x, uint32_t y, uint32_t z) { return ((x & y) | (z & (x | y))); }
static inline uint32_t Parity_32(  uint32_t x, uint32_t y, uint32_t z) { return (x ^ y ^ z);               }

static inline uint64_t Choose_64(  uint64_t x, uint64_t y, uint64_t z) { return (z ^ (x & (y ^ z)));       }
static inline uint64_t Majority_64(uint64_t x, uint64_t y, uint64_t z) { return ((x & y) | (z & (x | y))); }

static inline uint32_t Sigma0_32(uint32_t x) { return rot_r_32(x,  2) ^ rot_r_32(x, 13) ^ rot_r_32(x, 22); }
static inline uint32_t Sigma1_32(uint32_t x) { return rot_r_32(x,  6) ^ rot_r_32(x, 11) ^ rot_r_32(x, 25); }
static inline uint32_t Gamma0_32(uint32_t x) { return rot_r_32(x,  7) ^ rot_r_32(x, 18) ^ lsh_r_32(x,  3); }
static inline uint32_t Gamma1_32(uint32_t x) { return rot_r_32(x, 17) ^ rot_r_32(x, 19) ^ lsh_r_32(x, 10); }

static inline uint64_t Sigma0_64(uint64_t x) { return rot_r_64(x, 28) ^ rot_r_64(x, 34) ^ rot_r_64(x, 39); }
static inline uint64_t Sigma1_64(uint64_t x) { return rot_r_64(x, 14) ^ rot_r_64(x, 18) ^ rot_r_64(x, 41); }
static inline uint64_t Gamma0_64(uint64_t x) { return rot_r_64(x,  1) ^ rot_r_64(x,  8) ^ lsh_r_64(x,  7); }
static inline uint64_t Gamma1_64(uint64_t x) { return rot_r_64(x, 19) ^ rot_r_64(x, 61) ^ lsh_r_64(x,  6); }

/*
 * Offset into hash state.  In theory, this should works out to compile-time constants after optimization.
 */

static inline int sha1_pos(int i, int j) { return (5 + j - (i % 5)) % 5; }
static inline int sha2_pos(int i, int j) { return (8 + j - (i % 8)) % 8; }

/*
 * Software implementation of SHA-1 block algorithm.
 */

static hal_error_t sw_hash_core_sha1(hal_hash_state_t *state)
{
  static const uint32_t iv[5] = {0x67452301UL, 0xefcdab89UL, 0x98badcfeUL, 0x10325476UL, 0xc3d2e1f0UL};
  hal_error_t err;

  if (state == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  uint32_t *H = (uint32_t *) state->core_state, S[5], W[80];

  if (state->block_count == 0)
    memcpy(H, iv, sizeof(iv));

  memcpy(S, H, sizeof(S));

  if ((err = swytebop(W, state->block, 16 * sizeof(*W), sizeof(*W))) != HAL_OK)
    return err;

  for (int i = 16; i < 80; i++)
    W[i] = rot_l_32(W[i - 3] ^ W[i - 8] ^ W[i - 14] ^ W[i - 16], 1);

  for (int i = 0; i < 80; i++) {
    const int a = sha1_pos(i, 0), b = sha1_pos(i, 1), c = sha1_pos(i, 2), d = sha1_pos(i, 3), e = sha1_pos(i, 4);

    uint32_t f, k;
    if (i < 20)         f = Choose_32(   S[b], S[c], S[d]), k = 0x5A827999UL;
    else if (i < 40)    f = Parity_32(   S[b], S[c], S[d]), k = 0x6ED9EBA1UL;
    else if (i < 60)    f = Majority_32( S[b], S[c], S[d]), k = 0x8F1BBCDCUL;
    else                f = Parity_32(   S[b], S[c], S[d]), k = 0xCA62C1D6UL;

    if (debug)
      hal_log(HAL_LOG_DEBUG,
              "[Round %02d < a = 0x%08x, b = 0x%08x, c = 0x%08x, d = 0x%08x, e = 0x%08x, f = 0x%08x, k = 0x%08x, w = 0x%08x]\n",
              i, (unsigned)S[a], (unsigned)S[b], (unsigned)S[c], (unsigned)S[d], (unsigned)S[e], (unsigned)f, (unsigned)k, (unsigned)W[i]);

    S[e] = rot_l_32(S[a], 5) + f + S[e] + k + W[i];
    S[b] = rot_l_32(S[b], 30);

    if (debug)
      hal_log(HAL_LOG_DEBUG, "[Round %02d > a = 0x%08x, b = 0x%08x, c = 0x%08x, d = 0x%08x, e = 0x%08x]\n",
              i, (unsigned)S[a], (unsigned)S[b], (unsigned)S[c], (unsigned)S[d], (unsigned)S[e]);
  }

  for (int i = 0; i < 5; i++)
    H[i] += S[i];

  return HAL_OK;
}

/*
 * Software implementation of SHA-256 block algorithm, including support for same truncated variants
 * that the Cryptech Verilog SHA-256 core supports.
 */

static hal_error_t sw_hash_core_sha256(hal_hash_state_t *state)
{
  static const uint32_t sha224_iv[8] = {0xC1059ED8UL, 0x367CD507UL, 0x3070DD17UL, 0xF70E5939UL,
                                        0xFFC00B31UL, 0x68581511UL, 0x64F98FA7UL, 0xBEFA4FA4UL};

  static const uint32_t sha256_iv[8] = {0x6A09E667UL, 0xBB67AE85UL, 0x3C6EF372UL, 0xA54FF53AUL,
                                        0x510E527FUL, 0x9B05688CUL, 0x1F83D9ABUL, 0x5BE0CD19UL};

  hal_error_t err;

  if (state == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  uint32_t *H = (uint32_t *) state->core_state, S[8], W[64];

  if (state->block_count == 0) {
    switch (state->driver->ctrl_mode & SHA256_MODE_MASK) {
    case SHA256_MODE_SHA_224:   memcpy(H, sha224_iv, sizeof(sha224_iv)); break;
    case SHA256_MODE_SHA_256:   memcpy(H, sha256_iv, sizeof(sha256_iv)); break;
    default:                    return HAL_ERROR_IMPOSSIBLE;
    }
  }

  memcpy(S, H, sizeof(S));

  if ((err = swytebop(W, state->block, 16 * sizeof(*W), sizeof(*W))) != HAL_OK)
    return err;

  for (int i = 16; i < 64; i++)
    W[i] = Gamma1_32(W[i - 2]) + W[i - 7] + Gamma0_32(W[i - 15]) + W[i - 16];

  for (int i = 0; i < 64; i++) {
    const int a = sha2_pos(i, 0), b = sha2_pos(i, 1), c = sha2_pos(i, 2), d = sha2_pos(i, 3);
    const int e = sha2_pos(i, 4), f = sha2_pos(i, 5), g = sha2_pos(i, 6), h = sha2_pos(i, 7);

    const uint32_t t0 = S[h] + Sigma1_32(S[e]) + Choose_32(S[e], S[f], S[g]) + sha256_K[i] + W[i];
    const uint32_t t1 = Sigma0_32(S[a]) + Majority_32(S[a], S[b], S[c]);

    S[d] += t0;
    S[h] = t0 + t1;
  }

  for (int i = 0; i < 8; i++)
    H[i] += S[i];

  return HAL_OK;
}

/*
 * Software implementation of SHA-512 block algorithm, including support for same truncated variants
 * that the Cryptech Verilog SHA-512 core supports.
 */

static hal_error_t sw_hash_core_sha512(hal_hash_state_t *state)
{
  static const uint64_t
    sha512_iv[8]     = {0x6A09E667F3BCC908ULL, 0xBB67AE8584CAA73BULL, 0x3C6EF372FE94F82BULL, 0xA54FF53A5F1D36F1ULL,
                        0x510E527FADE682D1ULL, 0x9B05688C2B3E6C1FULL, 0x1F83D9ABFB41BD6BULL, 0x5BE0CD19137E2179ULL};
  static const uint64_t
    sha384_iv[8]     = {0xCBBB9D5DC1059ED8ULL, 0x629A292A367CD507ULL, 0x9159015A3070DD17ULL, 0x152FECD8F70E5939ULL,
                        0x67332667FFC00B31ULL, 0x8EB44A8768581511ULL, 0xDB0C2E0D64F98FA7ULL, 0x47B5481DBEFA4FA4ULL};
  static const uint64_t
    sha512_224_iv[8] = {0x8C3D37C819544DA2ULL, 0x73E1996689DCD4D6ULL, 0x1DFAB7AE32FF9C82ULL, 0x679DD514582F9FCFULL,
                        0x0F6D2B697BD44DA8ULL, 0x77E36F7304C48942ULL, 0x3F9D85A86A1D36C8ULL, 0x1112E6AD91D692A1ULL};
  static const uint64_t
    sha512_256_iv[8] = {0x22312194FC2BF72CULL, 0x9F555FA3C84C64C2ULL, 0x2393B86B6F53B151ULL, 0x963877195940EABDULL,
                        0x96283EE2A88EFFE3ULL, 0xBE5E1E2553863992ULL, 0x2B0199FC2C85B8AAULL, 0x0EB72DDC81C52CA2ULL};

  hal_error_t err;

  if (state == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  uint64_t *H = (uint64_t *) state->core_state, S[8], W[80];

  if (state->block_count == 0) {
    switch (state->driver->ctrl_mode & SHA512_MODE_MASK) {
    case SHA512_MODE_SHA_512_224:       memcpy(H, sha512_224_iv, sizeof(sha512_224_iv)); break;
    case SHA512_MODE_SHA_512_256:       memcpy(H, sha512_256_iv, sizeof(sha512_256_iv)); break;
    case SHA512_MODE_SHA_384:           memcpy(H, sha384_iv,     sizeof(sha384_iv));     break;
    case SHA512_MODE_SHA_512:           memcpy(H, sha512_iv,     sizeof(sha512_iv));     break;
    default:                            return HAL_ERROR_IMPOSSIBLE;
    }
  }

  memcpy(S, H, sizeof(S));

  if ((err = swytebop(W, state->block, 16 * sizeof(*W), sizeof(*W))) != HAL_OK)
    return err;

  for (int i = 16; i < 80; i++)
    W[i] = Gamma1_64(W[i - 2]) + W[i - 7] + Gamma0_64(W[i - 15]) + W[i - 16];

  for (int i = 0; i < 80; i++) {
    const int a = sha2_pos(i, 0), b = sha2_pos(i, 1), c = sha2_pos(i, 2), d = sha2_pos(i, 3);
    const int e = sha2_pos(i, 4), f = sha2_pos(i, 5), g = sha2_pos(i, 6), h = sha2_pos(i, 7);

    const uint64_t t0 = S[h] + Sigma1_64(S[e]) + Choose_64(S[e], S[f], S[g]) + sha512_K[i] + W[i];
    const uint64_t t1 = Sigma0_64(S[a]) + Majority_64(S[a], S[b], S[c]);

    S[d] += t0;
    S[h] = t0 + t1;
  }

  for (int i = 0; i < 8; i++)
    H[i] += S[i];

  return HAL_OK;
}

#endif /* HAL_ENABLE_SOFTWARE_HASH_CORES */

/*
 * "Any programmer who fails to comply with the standard naming, formatting,
 *  or commenting conventions should be shot.  If it so happens that it is
 *  inconvenient to shoot him, then he is to be politely requested to recode
 *  his program in adherence to the above standard."
 *                      -- Michael Spier, Digital Equipment Corporation
 *
 * Local variables:
 * indent-tabs-mode: nil
 * End:
 */
