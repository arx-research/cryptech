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

static void hexdump(uint8_t *buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; ++i)
        printf("%02x%c", buf[i], ((i & 0x07) == 0x07) ? '\n' : ' ');
    if ((len & 0x07) != 0)
        printf("\n");
}

int main(int argc, char *argv[])
{
    uint32_t i;
    uint8_t buf[256] = {0};
    uint8_t *bufptr = buf;
    const uint8_t *readptr;
    uint8_t *limit = buf + sizeof(buf);
    hal_error_t ret;
    uint8_t alphabet[] = "abcdefghijklmnopqrstuvwxyz";
    uint8_t readbuf[256] = {0};

    printf("hal_xdr_encode_int: work to failure\n");
    for (i = 1; i < 100; ++i) {
        if ((ret = hal_xdr_encode_int(&bufptr, limit, i)) != HAL_OK) {
            printf("%d: %s\n", i, hal_error_string(ret));
            break;
        }
    }
    hexdump(buf, ((uint8_t *)bufptr - buf));

    printf("\nhal_xdr_decode_int:\n");
    readptr = buf;
    while (readptr < bufptr) {
        if ((ret = hal_xdr_decode_int(&readptr, limit, &i)) != HAL_OK) {
            printf("%s\n", hal_error_string(ret));
            break;
        }
        printf("%u ", i);
    }
    printf("\n");

    printf("\nhal_xdr_encode_variable_opaque: work to failure\n");
    memset(buf, 0, sizeof(buf));
    bufptr = buf;
     for (i = 1; ; ++i) {
        if ((ret = hal_xdr_encode_variable_opaque(&bufptr, limit, alphabet, i)) != HAL_OK) {
            printf("%d: %s\n", i, hal_error_string(ret));
            break;
        }
    }
    hexdump(buf, ((uint8_t *)bufptr - buf));

    printf("\nhal_xdr_decode_variable_opaque:\n");
    readptr = buf;
    while (readptr < bufptr) {
        size_t len = bufptr - readptr;
        if ((ret = hal_xdr_decode_variable_opaque(&readptr, limit, readbuf, &len)) != HAL_OK) {
            printf("%s\n", hal_error_string(ret));
            break;
        }
        printf("%lu: ", len);
        for (size_t j = 0; j < len; ++j)
            putchar(readbuf[j]);
        putchar('\n');
        memset(readbuf, 0, sizeof(readbuf));
    }

    return 0;
}
