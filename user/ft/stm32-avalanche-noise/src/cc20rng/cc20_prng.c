/*
 * Simple implementation of chacha20 used as a CSPRNG.
 *
 * Copyright (c) 2016 NORDUnet A/S
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *    3. Neither the name of the NORDUnet nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "cc20_prng.h"

/* "expand 32-byte k" */
#define CHACHA20_CONSTANT0 0x61707865
#define CHACHA20_CONSTANT1 0x3320646e
#define CHACHA20_CONSTANT2 0x79622d32
#define CHACHA20_CONSTANT3 0x6b206574

#ifdef CHACHA20_PRNG_DEBUG
void _dump(struct cc20_state *cc, char *str);
#endif

inline uint32_t rotl32 (uint32_t x, uint32_t n)
{
    return (x << n) | (x >> (32 - n));
}

inline void _qr(struct cc20_state *cc, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    cc->i[a] += cc->i[b];
    cc->i[d] ^= cc->i[a];
    cc->i[d] = rotl32(cc->i[d], 16);

    cc->i[c] += cc->i[d];
    cc->i[b] ^= cc->i[c];
    cc->i[b] = rotl32(cc->i[b], 12);

    cc->i[a] += cc->i[b];
    cc->i[d] ^= cc->i[a];
    cc->i[d] = rotl32(cc->i[d], 8);

    cc->i[c] += cc->i[d];
    cc->i[b] ^= cc->i[c];
    cc->i[b] = rotl32(cc->i[b], 7);
}

void chacha20_prng_reseed(struct cc20_state *cc, uint32_t *entropy)
{
    uint32_t i = 256 / 8 / 4;
    while (i--) {
	cc->i[i] = entropy[i];
    }
}

void chacha20_prng_block(struct cc20_state *cc, uint32_t block_counter, struct cc20_state *out)
{
    uint32_t i;

    out->i[0] = CHACHA20_CONSTANT0;
    out->i[1] = CHACHA20_CONSTANT1;
    out->i[2] = CHACHA20_CONSTANT2;
    out->i[3] = CHACHA20_CONSTANT3;

    cc->i[12] = block_counter;

    for (i = 4; i < CHACHA20_NUM_WORDS; i++) {
	out->i[i] = cc->i[i];
    }

    for (i = 10; i; i--) {
	_qr(out, 0, 4, 8,12);
	_qr(out, 1, 5, 9,13);
	_qr(out, 2, 6,10,14);
	_qr(out, 3, 7,11,15);
	_qr(out, 0, 5,10,15);
	_qr(out, 1, 6,11,12);
	_qr(out, 2, 7, 8,13);
	_qr(out, 3, 4, 9,14);
    }

    for (i = 0; i < CHACHA20_NUM_WORDS; i++) {
	out->i[i] += cc->i[i];
    }
}

int chacha20_prng_self_test1()
{
    /* Test vector from RFC7539, section 2.3.2.
     *   https://tools.ietf.org/html/rfc7539#section-2.3.2
     */
    struct cc20_state test = {
        {0x61707865, 0x3320646e, 0x79622d32, 0x6b206574,
         0x03020100, 0x07060504, 0x0b0a0908, 0x0f0e0d0c,
         0x13121110, 0x17161514, 0x1b1a1918, 0x1f1e1d1c,
         0x00000001, 0x09000000, 0x4a000000, 0x00000000,
        }};
    struct cc20_state expected = {
	{0xe4e7f110, 0x15593bd1, 0x1fdd0f50, 0xc47120a3,
	 0xc7f4d1c7, 0x0368c033, 0x9aaa2204, 0x4e6cd4c3,
	 0x466482d2, 0x09aa9f07, 0x05d7c214, 0xa2028bd9,
	 0xd19c12b5, 0xb94e16de, 0xe883d0cb, 0x4e3c50a2,
	}};
    uint32_t i;
    struct cc20_state out;

#ifdef CHACHA20_PRNG_DEBUG
    _dump(&test, "Test vector from RFC7539, section 2.3.2. Input:");
#endif

    chacha20_prng_block(&test, 1, &out);
#ifdef CHACHA20_PRNG_DEBUG
    _dump(&out, "Test vector from RFC7539, section 2.3.2. Output:");
#endif

    for (i = 0; i < CHACHA20_NUM_WORDS; i++) {
        if (out.i[i] != expected.i[i]) return 0;
    }

    return 1;
}

int chacha20_prng_self_test2()
{
    /* Two-block test vector from RFC7539, section 2.4.2.
     *   https://tools.ietf.org/html/rfc7539#section-2.4.2
     */
    struct cc20_state test = {
	{0x61707865, 0x3320646e, 0x79622d32, 0x6b206574,
	 0x03020100, 0x07060504, 0x0b0a0908, 0x0f0e0d0c,
	 0x13121110, 0x17161514, 0x1b1a1918, 0x1f1e1d1c,
	 0x00000001, 0x00000000, 0x4a000000, 0x00000000,
	}};
    struct cc20_state expected1 = {
	{0xf3514f22, 0xe1d91b40, 0x6f27de2f, 0xed1d63b8,
	 0x821f138c, 0xe2062c3d, 0xecca4f7e, 0x78cff39e,
	 0xa30a3b8a, 0x920a6072, 0xcd7479b5, 0x34932bed,
	 0x40ba4c79, 0xcd343ec6, 0x4c2c21ea, 0xb7417df0,
	}};
    struct cc20_state expected2 = {
	{0x9f74a669, 0x410f633f, 0x28feca22, 0x7ec44dec,
	 0x6d34d426, 0x738cb970, 0x3ac5e9f3, 0x45590cc4,
	 0xda6e8b39, 0x892c831a, 0xcdea67c1, 0x2b7e1d90,
	 0x037463f3, 0xa11a2073, 0xe8bcfb88, 0xedc49139,
	}};
    struct cc20_state out;
    uint32_t i;

#ifdef CHACHA20_PRNG_DEBUG
    _dump(&test, "Test vector from RFC7539, section 2.4.2. Input:");
#endif

    chacha20_prng_block(&test, 1, &out);
#ifdef CHACHA20_PRNG_DEBUG
    _dump(&out, "First block");
#endif
    for (i = 0; i < CHACHA20_NUM_WORDS; i++) {
	if (out.i[i] != expected1.i[i]) return 0;
    }

    chacha20_prng_block(&test, 2, &out);
#ifdef CHACHA20_PRNG_DEBUG
    _dump(&out, "Second block");
#endif
    for (i = 0; i < CHACHA20_NUM_WORDS; i++) {
	if (out.i[i] != expected2.i[i]) return 0;
    }

    return 1;
}

#ifdef CHACHA20_PRNG_DEBUG
#include <stdio.h>
void _dump(struct cc20_state *cc, char *str)
{
    uint32_t i;

    printf("%s\n", str);

    for (i = 0; i < 4; i++) {
	printf("%02i  %08x %08x %08x %08x\n", i * 4, cc->i[i * 4 + 0],
	       cc->i[i * 4 + 1], cc->i[i * 4 + 2], cc->i[i * 4 + 3]);
    }
    printf("\n");
}
#endif

/* Test vector from RFC, used as simple power-on self-test of ability to compute
 * a block correctly.
 */
int chacha20_prng_self_test()
{
    return \
	chacha20_prng_self_test1() && \
	chacha20_prng_self_test2();
}
