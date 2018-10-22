/*
 * ks_attribute.c
 * --------------
 * Keystore attribute API.  This is internal within libhal.
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

#include <string.h>

#include "hal.h"
#include "hal_internal.h"

/*
 * Read and write attribute headers (type and length).  We could do
 * this with a structure type and casts, but that has portability
 * issues, and doing it this way just isn't expensive enough to worry about.
 */

const size_t hal_ks_attribute_header_size = 6;

static inline hal_error_t read_header(const uint8_t * const bytes, const size_t bytes_len,
                                      uint32_t *attribute_type, size_t *attribute_len)
{
  if (bytes == NULL || bytes_len < hal_ks_attribute_header_size ||
      attribute_type == NULL || attribute_len == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  *attribute_type = ((bytes[0] << 24) |
                     (bytes[1] << 16) |
                     (bytes[2] <<  8) |
                     (bytes[3] <<  0));
  *attribute_len  = ((bytes[4] <<  8) |
                     (bytes[5] <<  0));

  return HAL_OK;
}

static inline hal_error_t write_header(uint8_t *bytes, const size_t bytes_len,
                                       const uint32_t attribute_type, const size_t attribute_len)
{
  if (bytes == NULL || bytes_len < hal_ks_attribute_header_size)
    return HAL_ERROR_BAD_ARGUMENTS;

  bytes[0] = (attribute_type >> 24) & 0xFF;
  bytes[1] = (attribute_type >> 16) & 0xFF;
  bytes[2] = (attribute_type >>  8) & 0xFF;
  bytes[3] = (attribute_type >>  0) & 0xFF;
  bytes[4] = (attribute_len  >>  8) & 0xFF;
  bytes[5] = (attribute_len  >>  0) & 0xFF;

  return HAL_OK;
}

hal_error_t hal_ks_attribute_scan(const uint8_t * const bytes, const size_t bytes_len,
                                  hal_pkey_attribute_t *attributes, const unsigned attributes_len,
                                  size_t *total_len)
{
  if (bytes == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  const uint8_t *b = bytes;
  const uint8_t * const end = bytes + bytes_len;

  for (unsigned i = 0; i < attributes_len; i++) {
    uint32_t type;
    size_t length;
    hal_error_t err = read_header(b, end - b, &type, &length);
    if (err != HAL_OK)
      return err;
    if (b + hal_ks_attribute_header_size + length  > end)
      return HAL_ERROR_BAD_ATTRIBUTE_LENGTH;
    b += hal_ks_attribute_header_size;
    if (attributes != NULL) {
      attributes[i].type   = type;
      attributes[i].length = length;
      attributes[i].value  = b;
    }
    b += length;
  }

  if (total_len != NULL)
    *total_len = b - bytes;

  return HAL_OK;
}

hal_error_t hal_ks_attribute_delete(uint8_t *bytes, const size_t bytes_len,
                                    hal_pkey_attribute_t *attributes, unsigned *attributes_len,
                                    size_t *total_len,
                                    const uint32_t type)
{
  if (bytes == NULL || attributes == NULL || attributes_len == NULL || total_len == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  /*
   * Search for attribute by type. Note that there can be only one
   * attribute of any given type.
   */

  unsigned i = 0;

  while (i < *attributes_len && attributes[i].type != type)
    i++;

  /* If not found, great, it's already deleted from the key. */

  if (i == *attributes_len)
    return HAL_OK;

  const size_t delete_length = hal_ks_attribute_header_size + attributes[i].length;
  const size_t delete_offset = (uint8_t*) attributes[i].value - hal_ks_attribute_header_size - bytes;

  if (delete_offset + delete_length > *total_len)
    return HAL_ERROR_IMPOSSIBLE;

  memmove(bytes + delete_offset,
          bytes + delete_offset + delete_length,
          *total_len - delete_length - delete_offset);

  return hal_ks_attribute_scan(bytes, bytes_len, attributes, --*attributes_len, total_len);
}

hal_error_t hal_ks_attribute_insert(uint8_t *bytes, const size_t bytes_len,
                                   hal_pkey_attribute_t *attributes, unsigned *attributes_len,
                                   size_t *total_len,
                                   const uint32_t type,
                                   const uint8_t * const value, const size_t value_len)

{
  if (bytes == NULL || attributes == NULL || attributes_len == NULL ||
      total_len == NULL || value == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  /* Delete the existing attribute value (if present), then write the new value. */

  hal_error_t err
    = hal_ks_attribute_delete(bytes, bytes_len, attributes, attributes_len, total_len, type);

  if (err != HAL_OK)
    return err;

  if (*total_len + hal_ks_attribute_header_size + value_len > bytes_len)
    return HAL_ERROR_RESULT_TOO_LONG;

  uint8_t *b = bytes + *total_len;

  if ((err = write_header(b, bytes_len - *total_len, type, value_len)) != HAL_OK)
    return err;

  memcpy(b + hal_ks_attribute_header_size, value, value_len);

  return hal_ks_attribute_scan(bytes, bytes_len, attributes, ++*attributes_len, total_len);
}

/*
 * Local variables:
 * indent-tabs-mode: nil
 * End:
 */
