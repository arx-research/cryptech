/*
 * xdr.c
 * -----
 * Serialization/deserialization routines, using XDR (RFC 4506) encoding.
 * These functions are not part of the public libhal API.
 *
 * Copyright (c) 2016-2018, NORDUnet A/S All rights reserved.
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

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>		/* ptrdiff_t */
#include <string.h>             /* memcpy, memset */

#include "hal.h"
#include "hal_internal.h"	/* htonl/ntohl */
#include "xdr_internal.h"

/* encode/decode_int. This covers int, unsigned int, enum, and bool types,
 * which are all encoded as 32-bit big-endian fields. Signed integers are
 * defined to use two's complement, but that's universal these days, yes?
 */

hal_error_t hal_xdr_encode_int(uint8_t ** const outbuf, const uint8_t * const limit, const uint32_t value)
{
    /* arg checks */
    if (outbuf == NULL || *outbuf == NULL || limit == NULL)
        return HAL_ERROR_BAD_ARGUMENTS;

    /* buffer overflow check */
    if (limit - *outbuf < (ptrdiff_t)sizeof(value))
        return HAL_ERROR_XDR_BUFFER_OVERFLOW;

    **(uint32_t **)outbuf = htonl(value);
    *outbuf += sizeof(value);
    return HAL_OK;
}

/* decode an integer value without advancing the input pointer */
hal_error_t hal_xdr_decode_int_peek(const uint8_t ** const inbuf, const uint8_t * const limit, uint32_t *value)
{
    /* arg checks */
    if (inbuf == NULL || *inbuf == NULL || limit == NULL || value == NULL)
        return HAL_ERROR_BAD_ARGUMENTS;

    /* buffer overflow check */
    if (limit - *inbuf < (ptrdiff_t)sizeof(*value))
        return HAL_ERROR_XDR_BUFFER_OVERFLOW;

    *value = ntohl(**(uint32_t **)inbuf);
    return HAL_OK;
}

/* decode an integer value the regular way */
hal_error_t hal_xdr_decode_int(const uint8_t ** const inbuf, const uint8_t * const limit, uint32_t *value)
{
    hal_error_t err;

    if ((err = hal_xdr_decode_int_peek(inbuf, limit, value)) == HAL_OK)
        *inbuf += sizeof(*value);

    return err;
}

/* encode/decode_fixed_opaque. This covers fixed-length string and opaque types.
 * The data is padded to a multiple of 4 bytes as necessary.
 */

hal_error_t hal_xdr_encode_fixed_opaque(uint8_t ** const outbuf, const uint8_t * const limit, const uint8_t * const value, const size_t len)
{
    if (len == 0)
        return HAL_OK;

    /* arg checks */
    if (outbuf == NULL || *outbuf == NULL || limit == NULL || value == NULL)
        return HAL_ERROR_BAD_ARGUMENTS;

    /* buffer overflow check */
    if (limit - *outbuf < (ptrdiff_t)((len + 3) & ~3))
        return HAL_ERROR_XDR_BUFFER_OVERFLOW;

    /* write the data */
    memcpy(*outbuf, value, len);
    *outbuf += len;

    /* pad if necessary */
    if (len & 3) {
        size_t n = 4 - (len & 3);
        memset(*outbuf, 0, n);
        *outbuf += n;
    }

    return HAL_OK;
}

hal_error_t hal_xdr_decode_fixed_opaque_ptr(const uint8_t ** const inbuf, const uint8_t * const limit, const uint8_t ** const value, const size_t len)
{
    /* arg checks */
    if (inbuf == NULL || *inbuf == NULL || limit == NULL || value == NULL)
        return HAL_ERROR_BAD_ARGUMENTS;

    /* buffer overflow check */
    if (limit - *inbuf < (ptrdiff_t)len)
        return HAL_ERROR_XDR_BUFFER_OVERFLOW;

    /* return a pointer to the data */
    *value = *inbuf;

    /* update the buffer pointer, skipping any padding bytes */
    *inbuf += ((len + 3) & ~3);

    return HAL_OK;
}

hal_error_t hal_xdr_decode_fixed_opaque(const uint8_t ** const inbuf, const uint8_t * const limit, uint8_t * const value, const size_t len)
{
    const uint8_t *p;
    hal_error_t err;

    /* get and advance the input data pointer */
    if ((err = hal_xdr_decode_fixed_opaque_ptr(inbuf, limit, &p, len)) == HAL_OK)
        /* read the data */
        memcpy(value, p, len);

    return err;
}

/* encode/decode_variable_opaque. This covers variable-length string and opaque types.
 * The data is preceded by a 4-byte length word (encoded as above), and padded
 * to a multiple of 4 bytes as necessary.
 */

hal_error_t hal_xdr_encode_variable_opaque(uint8_t ** const outbuf, const uint8_t * const limit, const uint8_t * const value, const size_t len)
{
    hal_error_t err;

    /* encode length */
    if ((err = hal_xdr_encode_int(outbuf, limit, (uint32_t)len)) == HAL_OK) {
        /* encode data */
        if ((err = hal_xdr_encode_fixed_opaque(outbuf, limit, value, len)) != HAL_OK)
            /* undo write of length */
            *outbuf -= 4;
    }

    return err;
}

/* This version returns a pointer to the data in the input buffer.
 * It is used in the rpc server.
 */
hal_error_t hal_xdr_decode_variable_opaque_ptr(const uint8_t ** const inbuf, const uint8_t * const limit, const uint8_t ** const value, size_t * const len)
{
    hal_error_t err;
    uint32_t xdr_len;

    /* arg checks */
    if (len == NULL)
        return HAL_ERROR_BAD_ARGUMENTS;

    /* read length */
    if ((err = hal_xdr_decode_int(inbuf, limit, &xdr_len)) == HAL_OK) {
        /* get the data pointer */
        if ((err = hal_xdr_decode_fixed_opaque_ptr(inbuf, limit, value, xdr_len)) == HAL_OK)
            *len = xdr_len;
        else
            /* undo read of length */
            *inbuf -= 4;
    }

    return err;
}

/* This version copies the data to the user-supplied buffer.
 * It is used in the rpc client.
 */
hal_error_t hal_xdr_decode_variable_opaque(const uint8_t ** const inbuf, const uint8_t * const limit, uint8_t * const value, size_t * const len)
{
    hal_error_t err;
    size_t xdr_len;
    const uint8_t *p;

    /* read data pointer and length */
    if ((err = hal_xdr_decode_variable_opaque_ptr(inbuf, limit, &p, &xdr_len)) == HAL_OK) {
        /* user buffer overflow check */
        if (*len < xdr_len)
            return HAL_ERROR_XDR_BUFFER_OVERFLOW;
        /* read the data */
        memcpy(value, p, xdr_len);
        *len = xdr_len;
    }

    return err;
}
