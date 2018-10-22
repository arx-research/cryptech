/*
 * test-rpc_bighash.c
 * ------------------
 * Test code for RPC interface to Cryptech hash cores.
 *
 * Copyright (c) 2016, NORDUnet A/S
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

/*
 * Throw a large hashing operation at the RPC server. This was originally
 * written to flush out an interaction between RPC and the CLI login
 * process (which uses PBKDF2, which uses HMAC-256). It might be useful
 * for other puposes?
 */

#include <stdio.h>
#include <string.h>

#include <hal.h>

static uint8_t block[] = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";

/* some common numbers of iterations, and their digests */
static uint8_t expected_5k[] = {
    0x28, 0xe6, 0x00, 0x2d, 0x7f, 0x18, 0x05, 0x42,
    0xdb, 0x89, 0xc9, 0x9f, 0xc1, 0x5f, 0x83, 0x16,
    0xe4, 0xc2, 0x15, 0x75, 0xad, 0xe5, 0x9f, 0xe7,
    0x22, 0x0a, 0x59, 0x72, 0x56, 0x28, 0x1f, 0xe8,
};

static uint8_t expected_10k[] = {
    0x2d, 0xb1, 0x9b, 0x83, 0x14, 0x86, 0x48, 0x18,
    0x76, 0x54, 0xec, 0xe0, 0xfc, 0x1a, 0x56, 0xfe,
    0xdc, 0xfa, 0x8f, 0x46, 0xfd, 0x9d, 0x88, 0x3a,
    0xcd, 0x59, 0x51, 0x92, 0x44, 0x89, 0xc8, 0x51,
};

static uint8_t expected_25k[] = {
    0xcb, 0xf2, 0x5c, 0x1d, 0x0a, 0xee, 0xfc, 0xf7,
    0xe7, 0x7f, 0xda, 0x9a, 0x81, 0x1f, 0x6c, 0xa9,
    0x80, 0x95, 0x04, 0x75, 0xdc, 0x3a, 0xc1, 0x18,
    0x68, 0x7b, 0xe7, 0x9e, 0xb4, 0x2e, 0x43, 0xe5,
};

static void hexdump(uint8_t *buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; ++i)
        printf("%02x%c", buf[i], ((i & 0x07) == 0x07) ? '\n' : ' ');
    if ((len & 0x07) != 0)
        printf("\n");
}

#define check(op)                                               \
    do {                                                        \
        hal_error_t err = (op);                                 \
        if (err) {                                              \
            printf("%s: %s\n", #op, hal_error_string(err));     \
            return err;                                         \
        }                                                       \
    } while (0)

int main (int argc, char *argv[])
{
    hal_client_handle_t client = {0};
    hal_session_handle_t session = {0};
    hal_hash_handle_t hash;
    uint8_t digest[32];
    int iterations = 5000;
    uint8_t *expected;

    if (argc > 1)
        iterations = atoi(argv[1]);

    if (iterations == 5000)
        expected = expected_5k;
    else if (iterations == 10000)
        expected = expected_10k;
    else if (iterations == 25000)
        expected = expected_25k;
    else
        expected = NULL;

    check(hal_rpc_client_init());
    check(hal_rpc_hash_initialize(client, session, &hash, HAL_DIGEST_ALGORITHM_SHA256, NULL, 0));

    for (int i = 0; i < iterations; ++i) {
        check(hal_rpc_hash_update(hash, block, sizeof(block)));
    }

    check(hal_rpc_hash_finalize(hash, digest, sizeof(digest)));

    if (expected) {
        if (memcmp(digest, expected, sizeof(digest)) != 0) {
            printf("received:\n"); hexdump(digest, sizeof(digest));
            printf("\nexpected:\n"); hexdump(expected, sizeof(digest));
        }
    }
    else {
        hexdump(digest, sizeof(digest));
    }

    check(hal_rpc_client_close());
    return 0;
}
