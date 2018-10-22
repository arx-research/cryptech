/*
 * ks_index.c
 * ----------
 * Keystore index API.  This is internal within libhal.
 *
 * Copyright (c) 2016-2017, NORDUnet A/S All rights reserved.
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

#include <stddef.h>
#include <string.h>

#include "hal.h"
#include "hal_internal.h"
#include "ks.h"

/*
 * Find a block in the index, return true (found) or false (not found).
 * "where" indicates the name's position, or the position of the first free block.
 *
 * NB: This does NOT return a block number, it returns an index into
 * ks->index[].
 */

static int ks_find(const hal_ks_t * const ks,
                   const hal_uuid_t * const uuid,
                   const int * const hint,
                   int *where)
{
  if (ks == NULL || ks->index == NULL || ks->names == NULL || uuid == NULL || where == NULL)
    return 0;

  if (hint != NULL && *hint >= 0 && *hint < (int)ks->used &&
      hal_uuid_cmp(uuid, &ks->names[ks->index[*hint]]) == 0) {
    *where = *hint;
    return 1;
  }

  int lo = -1;
  int hi = ks->used;

  for (;;) {
    int m = (lo + hi) / 2;
    if (hi == 0 || m == lo) {
      *where = hi;
      return 0;
    }
    const int cmp = hal_uuid_cmp(uuid, &ks->names[ks->index[m]]);
    if (cmp < 0)
      hi = m;
    else if (cmp > 0)
      lo = m;
    else {
      *where = m;
      return 1;
    }
  }
}

/*
 * Heapsort the index.  We only need to do this on setup, for other
 * operations we're just inserting or deleting a single entry in an
 * already-ordered array, which is just a search problem.  If we were
 * really crunched for space, we could use an insertion sort here, but
 * heapsort is easy and works well with data already in place.
 */

static inline hal_error_t ks_heapsift(hal_ks_t *ks, int parent, const int end)
{
  if (ks == NULL || ks->index == NULL || ks->names == NULL || parent < 0 || end < parent)
    return HAL_ERROR_IMPOSSIBLE;

  for (;;) {
    const int left_child  = parent * 2 + 1;
    const int right_child = parent * 2 + 2;
    int biggest = parent;
    if (left_child  <= end && hal_uuid_cmp(&ks->names[ks->index[biggest]],
                                           &ks->names[ks->index[left_child]])  < 0)
      biggest = left_child;
    if (right_child <= end && hal_uuid_cmp(&ks->names[ks->index[biggest]],
                                           &ks->names[ks->index[right_child]]) < 0)
      biggest = right_child;
    if (biggest == parent)
      return HAL_OK;
    const uint16_t tmp = ks->index[biggest];
    ks->index[biggest] = ks->index[parent];
    ks->index[parent]  = tmp;
    parent = biggest;
  }
}

hal_error_t hal_ks_index_heapsort(hal_ks_t *ks)
{
  if (ks == NULL || ks->index == NULL || ks->names == NULL)
    return HAL_ERROR_IMPOSSIBLE;

  if (ks->used < 2)
    return HAL_OK;

  hal_error_t err;

  for (int i = (ks->used - 2) / 2; i >= 0; i--)
    if ((err = ks_heapsift(ks, i, ks->used - 1)) != HAL_OK)
      return err;

  for (int i = ks->used - 1; i > 0; i--) {
    const uint16_t tmp = ks->index[i];
    ks->index[i]       = ks->index[0];
    ks->index[0]       = tmp;
    if ((err = ks_heapsift(ks, 0, i - 1)) != HAL_OK)
      return err;
  }

  return HAL_OK;
}

/*
 * Perform a consistency check on the index.
 */

#define fsck(_ks) \
  do { hal_error_t _err = hal_ks_index_fsck(_ks); if (_err != HAL_OK) return _err; } while (0)


hal_error_t hal_ks_index_fsck(hal_ks_t *ks)
{
  if (ks == NULL || ks->index == NULL || ks->names == NULL ||
      ks->size == 0 || ks->used > ks->size)
    return HAL_ERROR_BAD_ARGUMENTS;

  for (unsigned i = 1; i < ks->used; i++)
    if (hal_uuid_cmp(&ks->names[ks->index[i - 1]], &ks->names[ks->index[i]]) >= 0)
      return HAL_ERROR_KS_INDEX_UUID_MISORDERED;

  return HAL_OK;
}

/*
 * Find a single block by name.
 */

hal_error_t hal_ks_index_find(hal_ks_t *ks,
                              const hal_uuid_t * const name,
                              unsigned *blockno,
                              int *hint)
{
  if (ks == NULL || ks->index == NULL || ks->names == NULL ||
      ks->size == 0 || ks->used > ks->size || name == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  int where;

  fsck(ks);

  int ok = ks_find(ks, name, hint, &where);

  if (blockno != NULL)
    *blockno = ks->index[where];

  if (hint != NULL)
    *hint = where;

  return ok ? HAL_OK : HAL_ERROR_KEY_NOT_FOUND;
}

/*
 * Add a single block to the index.
 */

hal_error_t hal_ks_index_add(hal_ks_t *ks,
                             const hal_uuid_t * const name,
                             unsigned *blockno,
                             int *hint)
{
  if (ks == NULL || ks->index == NULL || ks->names == NULL ||
      ks->size == 0 || ks->used > ks->size || name == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  if (ks->used == ks->size)
    return HAL_ERROR_NO_KEY_INDEX_SLOTS;

  int where;

  fsck(ks);

  if (ks_find(ks, name, hint, &where))
    return HAL_ERROR_KEY_NAME_IN_USE;

  /*
   * Grab first block on free list, which makes room to slide the
   * index up by one slot so we can insert the new block number.
   */

  const size_t len = (ks->used - where) * sizeof(*ks->index);
  const uint16_t b = ks->index[ks->used++];
  memmove(&ks->index[where + 1], &ks->index[where], len);
  ks->index[where] = b;
  ks->names[b] = *name;

  if (blockno != NULL)
    *blockno = b;

  if (hint != NULL)
    *hint = where;

  fsck(ks);

  return HAL_OK;
}

/*
 * Delete a single block from the index.
 */

hal_error_t hal_ks_index_delete(hal_ks_t *ks,
                                const hal_uuid_t * const name,
                                unsigned *blockno,
                                int *hint)
{
  if (ks == NULL || ks->index == NULL || ks->names == NULL ||
      ks->size == 0 || ks->used > ks->size || name == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  int where;

  fsck(ks);

  if (ks->used == 0 || !ks_find(ks, name, hint, &where))
    return HAL_ERROR_KEY_NOT_FOUND;

  /*
   * Free the block and stuff it at the end of the free list.
   */

  const size_t len = (ks->size - where - 1) * sizeof(*ks->index);
  const uint16_t b = ks->index[where];
  memmove(&ks->index[where], &ks->index[where + 1], len);
  ks->index[ks->size - 1] = b;
  ks->used--;
  memset(&ks->names[b], 0, sizeof(ks->names[b]));

  if (blockno != NULL)
    *blockno = b;

  if (hint != NULL)
    *hint = where;

  fsck(ks);

  return HAL_OK;
}

/*
 * Replace a single block with a new one, return new block number.
 * Name of block does not change.  This is an optimization of a delete
 * immediately followed by an add for the same name.
 */

hal_error_t hal_ks_index_replace(hal_ks_t *ks,
                                 const hal_uuid_t * const name,
                                 unsigned *blockno,
                                 int *hint)
{
  if (ks == NULL || ks->index == NULL || ks->names == NULL ||
      ks->size == 0 || ks->used > ks->size || name == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  if (ks->used == ks->size)
    return HAL_ERROR_NO_KEY_INDEX_SLOTS;

  int where;

  fsck(ks);

  if (ks->used == 0 || !ks_find(ks, name, hint, &where))
    return HAL_ERROR_KEY_NOT_FOUND;

  /*
   * Grab first block from free list, slide free list down, put old
   * block at end of free list and replace old block with new block.
   */

  const size_t len = (ks->size - ks->used - 1) * sizeof(*ks->index);
  const uint16_t b1 = ks->index[where];
  const uint16_t b2 = ks->index[ks->used];
  memmove(&ks->index[ks->used], &ks->index[ks->used + 1], len);
  ks->index[ks->size - 1] = b1;
  ks->index[where] = b2;
  ks->names[b2] = *name;
  memset(&ks->names[b1], 0, sizeof(ks->names[b1]));

  if (blockno != NULL)
    *blockno = b2;

  if (hint != NULL)
    *hint = where;

  fsck(ks);

  return HAL_OK;
}

/*
 * Local variables:
 * indent-tabs-mode: nil
 * End:
 */
