/*
 * ks.h
 * ----
 * Keystore, generic parts anyway.  This is internal within libhal.
 *
 * Copyright (c) 2015-2017, NORDUnet A/S All rights reserved.
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

#ifndef _KS_H_
#define _KS_H_

#include "hal.h"
#include "hal_internal.h"

/*
 * Size of a keystore "block".
 *
 * This must be an integer multiple of the flash subsector size, among
 * other reasons because that's the minimum erasable unit.
 */

#ifndef HAL_KS_BLOCK_SIZE
#define HAL_KS_BLOCK_SIZE       (4096 * 2)
#endif

#if HAL_KS_WRAPPED_KEYSIZE + 8 > HAL_KS_BLOCK_SIZE
#warning HAL_KS_WRAPPED_KEYSIZE is too big for to fit in a keystore block
#endif

/*
 * PIN block gets the all-zeros UUID, which will never be returned by
 * the UUID generation code (by definition -- it's not a version 4 UUID).
 */

extern const hal_uuid_t hal_ks_pin_uuid;

/*
 * Known block states.
 *
 * C does not guarantee any particular representation for enums, so
 * including enums directly in the block header isn't safe.  Instead,
 * we use an access method which casts when reading from the header.
 * Writing to the header isn't a problem, because C does guarantee
 * that enum is compatible with *some* integer type, it just doesn't
 * specify which one.
 */

typedef enum {
  HAL_KS_BLOCK_TYPE_ERASED  = 0xFF, /* Pristine erased block (candidate for reuse) */
  HAL_KS_BLOCK_TYPE_ZEROED  = 0x00, /* Zeroed block (recently used) */
  HAL_KS_BLOCK_TYPE_KEY     = 0x55, /* Block contains key material */
  HAL_KS_BLOCK_TYPE_PIN     = 0xAA, /* Block contains PINs */
  HAL_KS_BLOCK_TYPE_UNKNOWN = -1,   /* Internal code for "I have no clue what this is" */
} hal_ks_block_type_t;

/*
 * Block status.
 */

typedef enum {
  HAL_KS_BLOCK_STATUS_LIVE      = 0x66, /* This is a live block */
  HAL_KS_BLOCK_STATUS_TOMBSTONE = 0x44, /* This is a tombstone left behind during an update  */
  HAL_KS_BLOCK_STATUS_UNKNOWN   = -1,   /* Internal code for "I have no clue what this is" */
} hal_ks_block_status_t;

/*
 * Common header for all keystore block types.  A few of these fields
 * are deliberately omitted from the CRC.
 *
 * The legacy_1 and legacy_2 fields were used in the more complex
 * "chunked" layout used in an earlier iteration of this keystore
 * design, which proved more complex than it was worth.  At the
 * moment, the only thing we do with these fields is include them in
 * the CRC and check them for allowed values, to avoid gratuitously
 * breaking backwards compatability with the earlier design.
 */

typedef struct {
  uint8_t               block_type;
  uint8_t               block_status;
  uint8_t               legacy_1;
  uint8_t               legacy_2;
  hal_crc32_t           crc;
} hal_ks_block_header_t;

/*
 * Key block.  Tail end of "der" field (after der_len) used for attributes.
 */

typedef struct {
  hal_ks_block_header_t header;
  hal_uuid_t            name;
  hal_key_type_t        type;
  hal_curve_name_t      curve;
  hal_key_flags_t       flags;
  size_t                der_len;
  unsigned              attributes_len;
  uint8_t               der[];  /* Must be last field -- C99 "flexible array member" */
} hal_ks_key_block_t;

#define SIZEOF_KS_KEY_BLOCK_DER                                 \
  (HAL_KS_BLOCK_SIZE - offsetof(hal_ks_key_block_t, der))

/*
 * PIN block.  Also includes space for backing up the KEK when
 * HAL_MKM_FLASH_BACKUP_KLUDGE is enabled.
 */

typedef struct {
  hal_ks_block_header_t header;
  hal_ks_pin_t          wheel_pin;
  hal_ks_pin_t          so_pin;
  hal_ks_pin_t          user_pin;
#if HAL_MKM_FLASH_BACKUP_KLUDGE
  uint32_t              kek_set;
  uint8_t               kek[KEK_LENGTH];
#endif
} hal_ks_pin_block_t;

#define FLASH_KEK_NOT_SET 0
#define FLASH_KEK_SET   0x33333333

/*
 * One keystore block.
 */

typedef union {
  uint8_t                   bytes[HAL_KS_BLOCK_SIZE];
  hal_ks_block_header_t     header;
  hal_ks_key_block_t   key;
  hal_ks_pin_block_t   pin;
} hal_ks_block_t;

/*
 * In-memory cache.
 */

typedef struct {
  unsigned              blockno;
  unsigned              lru;
  hal_ks_block_t        block;
} hal_ks_cache_block_t;

/*
 * Keystore object.  hal_internal.h typedefs this to hal_ks_t.
 *
 * We expect this to be a static variable, but we expect the arrays in
 * it to be allocated at runtime using hal_allocate_static_memory()
 * because they can get kind of large.
 *
 * Driver-specific stuff is handled by a form of subclassing: the
 * driver embeds the hal_ks_t structure at the head of whatever else
 * it needs, and performs (controlled, type-safe) casts as needed.
 *
 * Core of this is the keystore index.  This is intended to be usable
 * by both memory-based and flash-based keystores.  Some of the
 * features aren't necessary for memory-based keystores, but should be
 * harmless, and let us keep the drivers simple.
 *
 * General approach is multiple arrays, all but one of which are
 * indexed by "block" numbers, where a block number might be a slot in
 * yet another static array, the number of a flash sub-sector, or
 * whatever is the appropriate unit for holding one keystore record.
 *
 * The index array only contains block numbers.  This is a small data
 * structure so that moving data within it is relatively cheap.
 *
 * The index array is divided into two portions: the index proper, and
 * the free queue.  The index proper is ordered according to the names
 * (UUIDs) of the corresponding blocks; the free queue is a FIFO, to
 * support a simplistic form of wear leveling in flash-based keystores.
 *
 * Key names are kept in a separate array, indexed by block number.
 *
 * The all-zeros UUID, which (by definition) cannot be a valid key
 * UUID, is reserved for the (non-key) block used to stash PINs and
 * other small data which aren't really part of the keystore proper
 * but are kept with it because the keystore is the flash we have.
 *
 * Note that this API deliberately says nothing about how the keys
 * themselves are stored, that's up to the keystore driver.
 */

typedef struct hal_ks_driver hal_ks_driver_t;

struct hal_ks {
  const hal_ks_driver_t *driver;/* Must be first */
  unsigned size;                /* Blocks in keystore */
  unsigned used;                /* How many blocks are in use */
  uint16_t *index;              /* Index/freelist array */
  hal_uuid_t *names;            /* Keyname array */
  unsigned cache_lru;           /* Cache LRU counter */
  unsigned cache_size;          /* Size (how many blocks) in cache */
  hal_ks_cache_block_t *cache;  /* Cache */
};

/*
 * Keystore driver.
 */

struct hal_ks_driver {
  hal_error_t (*init)        (hal_ks_t *ks, const int alloc);
  hal_error_t (*read)        (hal_ks_t *ks, const unsigned blockno, hal_ks_block_t *block);
  hal_error_t (*write)       (hal_ks_t *ks, const unsigned blockno, hal_ks_block_t *block);
  hal_error_t (*deprecate)   (hal_ks_t *ks, const unsigned blockno);
  hal_error_t (*zero)        (hal_ks_t *ks, const unsigned blockno);
  hal_error_t (*erase)       (hal_ks_t *ks, const unsigned blockno);
  hal_error_t (*erase_maybe) (hal_ks_t *ks, const unsigned blockno);
  hal_error_t (*set_owner)   (hal_ks_t *ks, const unsigned blockno,
                              const hal_client_handle_t client, const hal_session_handle_t session);
  hal_error_t (*test_owner)  (hal_ks_t *ks, const unsigned blockno,
                              const hal_client_handle_t client, const hal_session_handle_t session);
  hal_error_t (*copy_owner)  (hal_ks_t *ks, const unsigned source, const unsigned target);
  hal_error_t (*logout)      (hal_ks_t *ks, const hal_client_handle_t client);
};

/*
 * Wrappers around keystore driver methods.
 *
 * hal_ks_init() and hal_ks_logout() are missing here because we
 * expose them to the rest of libhal.
 */

static inline hal_error_t hal_ks_block_read(hal_ks_t *ks, const unsigned blockno, hal_ks_block_t *block)
{
  return
    ks == NULL || ks->driver == NULL  ? HAL_ERROR_BAD_ARGUMENTS   :
    ks->driver->read == NULL          ? HAL_ERROR_NOT_IMPLEMENTED :
    ks->driver->read(ks, blockno, block);
}

static inline hal_error_t hal_ks_block_write(hal_ks_t *ks, const unsigned blockno, hal_ks_block_t *block)
{
  return
    ks == NULL || ks->driver == NULL  ? HAL_ERROR_BAD_ARGUMENTS   :
    ks->driver->write == NULL         ? HAL_ERROR_NOT_IMPLEMENTED :
    ks->driver->write(ks, blockno, block);
}

static inline hal_error_t hal_ks_block_deprecate(hal_ks_t *ks, const unsigned blockno)
{
  return
    ks == NULL || ks->driver == NULL  ? HAL_ERROR_BAD_ARGUMENTS   :
    ks->driver->deprecate == NULL     ? HAL_ERROR_NOT_IMPLEMENTED :
    ks->driver->deprecate(ks, blockno);
}

static inline hal_error_t hal_ks_block_zero(hal_ks_t *ks, const unsigned blockno)
{
  return
    ks == NULL || ks->driver == NULL  ? HAL_ERROR_BAD_ARGUMENTS   :
    ks->driver->zero == NULL          ? HAL_ERROR_NOT_IMPLEMENTED :
    ks->driver->zero(ks, blockno);
}

static inline hal_error_t hal_ks_block_erase(hal_ks_t *ks, const unsigned blockno)
{
  return
    ks == NULL || ks->driver == NULL  ? HAL_ERROR_BAD_ARGUMENTS   :
    ks->driver->erase == NULL         ? HAL_ERROR_NOT_IMPLEMENTED :
    ks->driver->erase(ks, blockno);
}

static inline hal_error_t hal_ks_block_erase_maybe(hal_ks_t *ks, const unsigned blockno)
{
  return
    ks == NULL || ks->driver == NULL  ? HAL_ERROR_BAD_ARGUMENTS   :
    ks->driver->erase_maybe == NULL   ? HAL_ERROR_NOT_IMPLEMENTED :
    ks->driver->erase_maybe(ks, blockno);
}

static inline hal_error_t hal_ks_block_set_owner(hal_ks_t *ks,
                                                 const unsigned blockno,
                                                 const hal_client_handle_t  client,
                                                 const hal_session_handle_t session)
{
  return
    ks == NULL || ks->driver == NULL  ? HAL_ERROR_BAD_ARGUMENTS   :
    ks->driver->set_owner == NULL     ? HAL_ERROR_NOT_IMPLEMENTED :
    ks->driver->set_owner(ks, blockno, client, session);
}

static inline hal_error_t hal_ks_block_test_owner(hal_ks_t *ks,
                                                  const unsigned blockno,
                                                  const hal_client_handle_t  client,
                                                  const hal_session_handle_t session)
{
  return
    ks == NULL || ks->driver == NULL  ? HAL_ERROR_BAD_ARGUMENTS   :
    ks->driver->test_owner == NULL    ? HAL_ERROR_NOT_IMPLEMENTED :
    ks->driver->test_owner(ks, blockno, client, session);
}

static inline hal_error_t hal_ks_block_copy_owner(hal_ks_t *ks,
                                                  const unsigned source,
                                                  const unsigned target)
{
  return
    ks == NULL || ks->driver == NULL  ? HAL_ERROR_BAD_ARGUMENTS   :
    ks->driver->copy_owner == NULL    ? HAL_ERROR_NOT_IMPLEMENTED :
    ks->driver->copy_owner(ks, source, target);
}

/*
 * Type safe casts.
 */

static inline hal_ks_block_type_t hal_ks_block_get_type(const hal_ks_block_t * const block)
{
  return block == NULL ? HAL_KS_BLOCK_TYPE_UNKNOWN :
    (hal_ks_block_type_t) block->header.block_type;
}

static inline hal_ks_block_status_t hal_ks_block_get_status(const hal_ks_block_t * const block)
{
  return block == NULL ? HAL_KS_BLOCK_STATUS_UNKNOWN :
    (hal_ks_block_status_t) block->header.block_status;
}

/*
 * Keystore utilities.  Some or all of these may end up static within ks.c.
 */

extern hal_error_t hal_ks_alloc_common(hal_ks_t *ks,
                                       const unsigned ks_blocks,
                                       const unsigned cache_blocks,
                                       void **extra,
                                       const size_t extra_len);

extern hal_error_t hal_ks_init_common(hal_ks_t *ks);

extern hal_crc32_t hal_ks_block_calculate_crc(const hal_ks_block_t * const block);

extern hal_error_t hal_ks_index_heapsort(hal_ks_t *ks);

extern hal_error_t hal_ks_index_find(hal_ks_t *ks,
                                     const hal_uuid_t * const name,
                                     unsigned *blockno,
                                     int *hint);

extern hal_error_t hal_ks_index_add(hal_ks_t *ks,
                                    const hal_uuid_t * const name,
                                    unsigned *blockno,
                                    int *hint);

extern hal_error_t hal_ks_index_delete(hal_ks_t *ks,
                                       const hal_uuid_t * const name,
                                       unsigned *blockno,
                                       int *hint);

extern hal_error_t hal_ks_index_replace(hal_ks_t *ks,
                                        const hal_uuid_t * const name,
                                        unsigned *blockno,
                                        int *hint);

extern hal_error_t hal_ks_index_fsck(hal_ks_t *ks);

extern const size_t hal_ks_attribute_header_size;

extern hal_error_t hal_ks_attribute_scan(const uint8_t * const bytes,
                                         const size_t bytes_len,
                                         hal_pkey_attribute_t *attributes,
                                         const unsigned attributes_len,
                                         size_t *total_len);

extern hal_error_t hal_ks_attribute_delete(uint8_t *bytes,
                                           const size_t bytes_len,
                                           hal_pkey_attribute_t *attributes,
                                           unsigned *attributes_len,
                                           size_t *total_len,
                                           const uint32_t type);

extern hal_error_t hal_ks_attribute_insert(uint8_t *bytes, const size_t bytes_len,
                                           hal_pkey_attribute_t *attributes,
                                           unsigned *attributes_len,
                                           size_t *total_len,
                                           const uint32_t type,
                                           const uint8_t * const value,
                                           const size_t value_len);

extern hal_ks_block_t *hal_ks_cache_pick_lru(hal_ks_t *ks);

extern hal_ks_block_t *hal_ks_cache_find_block(const hal_ks_t * const ks,
                                               const unsigned blockno);

extern void hal_ks_cache_mark_used(hal_ks_t *ks,
                                   const hal_ks_block_t * const block,
                                   const unsigned blockno);

extern void hal_ks_cache_release(hal_ks_t *ks,
                                 const hal_ks_block_t * const block);

extern hal_error_t hal_ks_block_read_cached(hal_ks_t *ks,
                                            const unsigned blockno,
                                            hal_ks_block_t **block);

extern hal_error_t hal_ks_block_update(hal_ks_t *ks,
                                       const unsigned b1,
                                       hal_ks_block_t *block,
                                       const hal_uuid_t * const uuid,
                                       int *hint);

extern hal_error_t hal_ks_available(hal_ks_t *ks, size_t *count);

#endif /* _KS_H_ */

/*
 * Local variables:
 * indent-tabs-mode: nil
 * End:
 */
