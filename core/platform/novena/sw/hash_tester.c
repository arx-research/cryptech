/*
 * hash_tester.c
 * --------------
 * This program sends several commands to the coretest_hashes subsystem
 * in order to verify the SHA-1, SHA-256 and SHA-512/x hash function
 * cores.
 *
 * Note: This version of the program talks to the FPGA over an EIM bus.
 *
 * The single and dual block test cases are taken from the
 * NIST KAT document:
 * http://csrc.nist.gov/groups/ST/toolkit/documents/Examples/SHA_All.pdf
 *
 *
 * Authors: Joachim Strömbergson, Paul Selkirk
 * Copyright (c) 2014-2015, NORDUnet A/S All rights reserved.
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
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <stdint.h>
#include <ctype.h>
#include <signal.h>

#include "cryptech.h"

int quiet = 0;
int repeat = 0;


/* SHA-1/SHA-256 One Block Message Sample
   Input Message: "abc" */
const uint8_t NIST_512_SINGLE[] =
{ 0x61, 0x62, 0x63, 0x80, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18 };

const uint8_t SHA1_SINGLE_DIGEST[] =
{ 0xa9, 0x99, 0x3e, 0x36, 0x47, 0x06, 0x81, 0x6a,
  0xba, 0x3e, 0x25, 0x71, 0x78, 0x50, 0xc2, 0x6c,
  0x9c, 0xd0, 0xd8, 0x9d };

const uint8_t SHA256_SINGLE_DIGEST[] =
{ 0xBA, 0x78, 0x16, 0xBF, 0x8F, 0x01, 0xCF, 0xEA,
  0x41, 0x41, 0x40, 0xDE, 0x5D, 0xAE, 0x22, 0x23,
  0xB0, 0x03, 0x61, 0xA3, 0x96, 0x17, 0x7A, 0x9C,
  0xB4, 0x10, 0xFF, 0x61, 0xF2, 0x00, 0x15, 0xAD };

/* SHA-1/SHA-256 Two Block Message Sample
   Input Message: "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq" */
const uint8_t NIST_512_DOUBLE0[] =
{ 0x61, 0x62, 0x63, 0x64, 0x62, 0x63, 0x64, 0x65,
  0x63, 0x64, 0x65, 0x66, 0x64, 0x65, 0x66, 0x67,
  0x65, 0x66, 0x67, 0x68, 0x66, 0x67, 0x68, 0x69,
  0x67, 0x68, 0x69, 0x6A, 0x68, 0x69, 0x6A, 0x6B,
  0x69, 0x6A, 0x6B, 0x6C, 0x6A, 0x6B, 0x6C, 0x6D,
  0x6B, 0x6C, 0x6D, 0x6E, 0x6C, 0x6D, 0x6E, 0x6F,
  0x6D, 0x6E, 0x6F, 0x70, 0x6E, 0x6F, 0x70, 0x71,
  0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const uint8_t NIST_512_DOUBLE1[] =
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xC0 };

const uint8_t SHA1_DOUBLE_DIGEST[] =
{ 0x84, 0x98, 0x3E, 0x44, 0x1C, 0x3B, 0xD2, 0x6E,
  0xBA, 0xAE, 0x4A, 0xA1, 0xF9, 0x51, 0x29, 0xE5,
  0xE5, 0x46, 0x70, 0xF1 };

const uint8_t SHA256_DOUBLE_DIGEST[] =
{ 0x24, 0x8D, 0x6A, 0x61, 0xD2, 0x06, 0x38, 0xB8,
  0xE5, 0xC0, 0x26, 0x93, 0x0C, 0x3E, 0x60, 0x39,
  0xA3, 0x3C, 0xE4, 0x59, 0x64, 0xFF, 0x21, 0x67,
  0xF6, 0xEC, 0xED, 0xD4, 0x19, 0xDB, 0x06, 0xC1 };

/* SHA-512 One Block Message Sample
   Input Message: "abc" */
const uint8_t NIST_1024_SINGLE[] =
{ 0x61, 0x62, 0x63, 0x80, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18 };

const uint8_t SHA512_224_SINGLE_DIGEST[] =
{ 0x46, 0x34, 0x27, 0x0f, 0x70, 0x7b, 0x6a, 0x54,
  0xda, 0xae, 0x75, 0x30, 0x46, 0x08, 0x42, 0xe2,
  0x0e, 0x37, 0xed, 0x26, 0x5c, 0xee, 0xe9, 0xa4,
  0x3e, 0x89, 0x24, 0xaa };
const uint8_t SHA512_256_SINGLE_DIGEST[] =
{ 0x53, 0x04, 0x8e, 0x26, 0x81, 0x94, 0x1e, 0xf9,
  0x9b, 0x2e, 0x29, 0xb7, 0x6b, 0x4c, 0x7d, 0xab,
  0xe4, 0xc2, 0xd0, 0xc6, 0x34, 0xfc, 0x6d, 0x46,
  0xe0, 0xe2, 0xf1, 0x31, 0x07, 0xe7, 0xaf, 0x23 };
const uint8_t SHA384_SINGLE_DIGEST[] =
{ 0xcb, 0x00, 0x75, 0x3f, 0x45, 0xa3, 0x5e, 0x8b,
  0xb5, 0xa0, 0x3d, 0x69, 0x9a, 0xc6, 0x50, 0x07,
  0x27, 0x2c, 0x32, 0xab, 0x0e, 0xde, 0xd1, 0x63,
  0x1a, 0x8b, 0x60, 0x5a, 0x43, 0xff, 0x5b, 0xed,
  0x80, 0x86, 0x07, 0x2b, 0xa1, 0xe7, 0xcc, 0x23,
  0x58, 0xba, 0xec, 0xa1, 0x34, 0xc8, 0x25, 0xa7 };
const uint8_t SHA512_SINGLE_DIGEST[] =
{ 0xdd, 0xaf, 0x35, 0xa1, 0x93, 0x61, 0x7a, 0xba,
  0xcc, 0x41, 0x73, 0x49, 0xae, 0x20, 0x41, 0x31,
  0x12, 0xe6, 0xfa, 0x4e, 0x89, 0xa9, 0x7e, 0xa2,
  0x0a, 0x9e, 0xee, 0xe6, 0x4b, 0x55, 0xd3, 0x9a,
  0x21, 0x92, 0x99, 0x2a, 0x27, 0x4f, 0xc1, 0xa8,
  0x36, 0xba, 0x3c, 0x23, 0xa3, 0xfe, 0xeb, 0xbd,
  0x45, 0x4d, 0x44, 0x23, 0x64, 0x3c, 0xe8, 0x0e,
  0x2a, 0x9a, 0xc9, 0x4f, 0xa5, 0x4c, 0xa4, 0x9f };

/* SHA-512 Two Block Message Sample
   Input Message: "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmn"
   "hijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu" */
const uint8_t NIST_1024_DOUBLE0[] =
{ 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
  0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
  0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a,
  0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b,
  0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c,
  0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d,
  0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e,
  0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
  0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70,
  0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71,
  0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72,
  0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73,
  0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74,
  0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75,
  0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const uint8_t NIST_1024_DOUBLE1[] =
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x80 };

const uint8_t SHA512_224_DOUBLE_DIGEST[] =
{ 0x23, 0xfe, 0xc5, 0xbb, 0x94, 0xd6, 0x0b, 0x23,
  0x30, 0x81, 0x92, 0x64, 0x0b, 0x0c, 0x45, 0x33,
  0x35, 0xd6, 0x64, 0x73, 0x4f, 0xe4, 0x0e, 0x72,
  0x68, 0x67, 0x4a, 0xf9 };
const uint8_t SHA512_256_DOUBLE_DIGEST[] =
{ 0x39, 0x28, 0xe1, 0x84, 0xfb, 0x86, 0x90, 0xf8,
  0x40, 0xda, 0x39, 0x88, 0x12, 0x1d, 0x31, 0xbe,
  0x65, 0xcb, 0x9d, 0x3e, 0xf8, 0x3e, 0xe6, 0x14,
  0x6f, 0xea, 0xc8, 0x61, 0xe1, 0x9b, 0x56, 0x3a };
const uint8_t SHA384_DOUBLE_DIGEST[] =
{ 0x09, 0x33, 0x0c, 0x33, 0xf7, 0x11, 0x47, 0xe8,
  0x3d, 0x19, 0x2f, 0xc7, 0x82, 0xcd, 0x1b, 0x47,
  0x53, 0x11, 0x1b, 0x17, 0x3b, 0x3b, 0x05, 0xd2,
  0x2f, 0xa0, 0x80, 0x86, 0xe3, 0xb0, 0xf7, 0x12,
  0xfc, 0xc7, 0xc7, 0x1a, 0x55, 0x7e, 0x2d, 0xb9,
  0x66, 0xc3, 0xe9, 0xfa, 0x91, 0x74, 0x60, 0x39 };
const uint8_t SHA512_DOUBLE_DIGEST[] =
{ 0x8e, 0x95, 0x9b, 0x75, 0xda, 0xe3, 0x13, 0xda,
  0x8c, 0xf4, 0xf7, 0x28, 0x14, 0xfc, 0x14, 0x3f,
  0x8f, 0x77, 0x79, 0xc6, 0xeb, 0x9f, 0x7f, 0xa1,
  0x72, 0x99, 0xae, 0xad, 0xb6, 0x88, 0x90, 0x18,
  0x50, 0x1d, 0x28, 0x9e, 0x49, 0x00, 0xf7, 0xe4,
  0x33, 0x1b, 0x99, 0xde, 0xc4, 0xb5, 0x43, 0x3a,
  0xc7, 0xd3, 0x29, 0xee, 0xb6, 0xdd, 0x26, 0x54,
  0x5e, 0x96, 0xe5, 0x5b, 0x87, 0x4b, 0xe9, 0x09 };


/* ---------------- startup code ---------------- */

static off_t board_addr_base = 0;
static off_t sha1_addr_base, sha256_addr_base, sha512_addr_base;

static int init(void)
{
    static int inited = 0;

    if (inited)
        return 0;

    sha1_addr_base = tc_core_base("sha1");
    sha256_addr_base = tc_core_base("sha2-256");
    sha512_addr_base = tc_core_base("sha2-512");

    inited = 1;
    return 0;
}

/* ---------------- sanity test case ---------------- */

int TC0()
{
    uint8_t board_name0[4]      = NOVENA_BOARD_NAME0;
    uint8_t board_name1[4]      = NOVENA_BOARD_NAME1;
    uint8_t board_version[4]    = NOVENA_BOARD_VERSION;
    uint8_t t[4];

    if (init() != 0)
        return -1;

    if (!quiet)
        printf("TC0: Reading board type, version, and dummy reg from global registers.\n");

    /* write current time into dummy register, then try to read it back
     * to make sure that we can actually write something into EIM
     */
    (void)time((time_t *)t);
    if (tc_write(board_addr_base + BOARD_ADDR_DUMMY, t, 4) != 0)
        return 1;

    return
        tc_expected(board_addr_base + BOARD_ADDR_NAME0,   board_name0,   4) ||
        tc_expected(board_addr_base + BOARD_ADDR_NAME1,   board_name1,   4) ||
        tc_expected(board_addr_base + BOARD_ADDR_VERSION, board_version, 4) ||
        tc_expected(board_addr_base + BOARD_ADDR_DUMMY,   t, 4);
}

/* ---------------- SHA-1 test cases ---------------- */

/* TC1: Read name and version from SHA-1 core. */
int TC1(void)
{
    uint8_t name0[4]   = SHA1_NAME0;
    uint8_t name1[4]   = SHA1_NAME1;
    uint8_t version[4] = SHA1_VERSION;

    if (init() != 0)
        return -1;
    if ((sha1_addr_base == 0) && !quiet) {
        printf("TC1: SHA-1 not present\n");
        return 0;
    }

    if (!quiet)
        printf("TC1: Reading name and version words from SHA-1 core.\n");

    return
        tc_expected(sha1_addr_base + SHA1_ADDR_NAME0, name0, 4) ||
        tc_expected(sha1_addr_base + SHA1_ADDR_NAME1, name1, 4) ||
        tc_expected(sha1_addr_base + SHA1_ADDR_VERSION, version, 4);
}

/* TC2: SHA-1 Single block message test as specified by NIST. */
int TC2(void)
{
    const uint8_t *block = NIST_512_SINGLE;
    const uint8_t *expected = SHA1_SINGLE_DIGEST;
    int ret;

    if (init() != 0)
        return -1;
    if ((sha1_addr_base == 0) && !quiet) {
        printf("TC2: SHA-1 not present\n");
        return 0;
    }

    if (!quiet)
        printf("TC2: Single block message test for SHA-1.\n");

    /* Write block to SHA-1. */
    tc_write(sha1_addr_base + SHA1_ADDR_BLOCK, block, SHA1_BLOCK_LEN);
    /* Start initial block hashing, wait and check status. */
    tc_init(sha1_addr_base + SHA1_ADDR_CTRL);
    tc_wait_valid(sha1_addr_base + SHA1_ADDR_STATUS);
    /* Extract the digest. */
    ret = tc_expected(sha1_addr_base + SHA1_ADDR_DIGEST, expected, SHA1_DIGEST_LEN);
    return ret;
}

/* TC3: SHA-1 Double block message test as specified by NIST. */
int TC3(void)
{
    const uint8_t *block[2] = { NIST_512_DOUBLE0, NIST_512_DOUBLE1 };
    static const uint8_t block0_expected[] =
        { 0xF4, 0x28, 0x68, 0x18, 0xC3, 0x7B, 0x27, 0xAE,
          0x04, 0x08, 0xF5, 0x81, 0x84, 0x67, 0x71, 0x48,
          0x4A, 0x56, 0x65, 0x72 };
    const uint8_t *expected = SHA1_DOUBLE_DIGEST;
    int ret;

    if (init() != 0)
        return -1;
    if ((sha1_addr_base == 0) && !quiet) {
        printf("TC3: SHA-1 not present\n");
        return 0;
    }

    if (!quiet)
        printf("TC3: Double block message test for SHA-1.\n");

    /* Write first block to SHA-1. */
    tc_write(sha1_addr_base + SHA1_ADDR_BLOCK, block[0], SHA1_BLOCK_LEN);
    /* Start initial block hashing, wait and check status. */
    tc_init(sha1_addr_base + SHA1_ADDR_CTRL);
    tc_wait_valid(sha1_addr_base + SHA1_ADDR_STATUS);
    /* Extract the first digest. */
    tc_expected(sha1_addr_base + SHA1_ADDR_DIGEST, block0_expected, SHA1_DIGEST_LEN);
    /* Write second block to SHA-1. */
    tc_write(sha1_addr_base + SHA1_ADDR_BLOCK, block[1], SHA1_BLOCK_LEN);
    /* Start next block hashing, wait and check status. */
    tc_next(sha1_addr_base + SHA1_ADDR_CTRL);
    tc_wait_valid(sha1_addr_base + SHA1_ADDR_STATUS);
    /* Extract the second digest. */
    ret = tc_expected(sha1_addr_base + SHA1_ADDR_DIGEST, expected, SHA1_DIGEST_LEN);
    return ret;
}

/* ---------------- SHA-256 test cases ---------------- */

/* TC4: Read name and version from SHA-256 core. */
int TC4(void)
{
    uint8_t name0[4]   = SHA256_NAME0;
    uint8_t name1[4]   = SHA256_NAME1;
    uint8_t version[4] = SHA256_VERSION;

    if (init() != 0)
        return -1;
    if ((sha256_addr_base == 0) && !quiet) {
        printf("TC4: SHA-256 not present\n");
        return 0;
    }

    if (!quiet)
        printf("TC4: Reading name and version words from SHA-256 core.\n");

    return
        tc_expected(sha256_addr_base + SHA256_ADDR_NAME0, name0, 4) ||
        tc_expected(sha256_addr_base + SHA256_ADDR_NAME1, name1, 4) ||
        tc_expected(sha256_addr_base + SHA256_ADDR_VERSION, version, 4);
}

/* TC5: SHA-256 Single block message test as specified by NIST. */
int TC5()
{
    const uint8_t *block = NIST_512_SINGLE;
    const uint8_t *expected = SHA256_SINGLE_DIGEST;

    if (init() != 0)
        return -1;
    if ((sha256_addr_base == 0) && !quiet) {
        printf("TC5: SHA-256 not present\n");
        return 0;
    }

    if (!quiet)
        printf("TC5: Single block message test for SHA-256.\n");

    return
        /* Write block to SHA-256. */
        tc_write(sha256_addr_base + SHA256_ADDR_BLOCK, block, SHA256_BLOCK_LEN) ||
        /* Start initial block hashing, wait and check status. */
        tc_init(sha256_addr_base + SHA256_ADDR_CTRL) ||
        tc_wait_valid(sha256_addr_base + SHA256_ADDR_STATUS) ||
        /* Extract the digest. */
        tc_expected(sha256_addr_base + SHA256_ADDR_DIGEST, expected, SHA256_DIGEST_LEN);
}

/* TC6: SHA-256 Double block message test as specified by NIST. */
int TC6()
{
    const uint8_t *block[2] = { NIST_512_DOUBLE0, NIST_512_DOUBLE1 };
    static const uint8_t block0_expected[] =
        { 0x85, 0xE6, 0x55, 0xD6, 0x41, 0x7A, 0x17, 0x95,
          0x33, 0x63, 0x37, 0x6A, 0x62, 0x4C, 0xDE, 0x5C,
          0x76, 0xE0, 0x95, 0x89, 0xCA, 0xC5, 0xF8, 0x11,
          0xCC, 0x4B, 0x32, 0xC1, 0xF2, 0x0E, 0x53, 0x3A };
    const uint8_t *expected = SHA256_DOUBLE_DIGEST;

    if (init() != 0)
        return -1;
    if ((sha256_addr_base == 0) && !quiet) {
        printf("TC6: SHA-256 not present\n");
        return 0;
    }

    if (!quiet)
        printf("TC6: Double block message test for SHA-256.\n");

    return
        /* Write first block to SHA-256. */
        tc_write(sha256_addr_base + SHA256_ADDR_BLOCK, block[0], SHA256_BLOCK_LEN) ||
        /* Start initial block hashing, wait and check status. */
        tc_init(sha256_addr_base + SHA256_ADDR_CTRL) ||
        tc_wait_valid(sha256_addr_base + SHA256_ADDR_STATUS) ||
        /* Extract the first digest. */
        tc_expected(sha256_addr_base + SHA256_ADDR_DIGEST, block0_expected, SHA256_DIGEST_LEN) ||
        /* Write second block to SHA-256. */
        tc_write(sha256_addr_base + SHA256_ADDR_BLOCK, block[1], SHA256_BLOCK_LEN) ||
        /* Start next block hashing, wait and check status. */
        tc_next(sha256_addr_base + SHA256_ADDR_CTRL) ||
        tc_wait_valid(sha256_addr_base + SHA256_ADDR_STATUS) ||
        /* Extract the second digest. */
        tc_expected(sha256_addr_base + SHA256_ADDR_DIGEST, expected, SHA256_DIGEST_LEN);
}

/* TC7: SHA-256 Huge message test. */
int TC7()
{
    static const uint8_t block[] =
        { 0xaa, 0x55, 0xaa, 0x55, 0xde, 0xad, 0xbe, 0xef,
          0x55, 0xaa, 0x55, 0xaa, 0xf0, 0x0f, 0xf0, 0x0f,
          0xaa, 0x55, 0xaa, 0x55, 0xde, 0xad, 0xbe, 0xef,
          0x55, 0xaa, 0x55, 0xaa, 0xf0, 0x0f, 0xf0, 0x0f,
          0xaa, 0x55, 0xaa, 0x55, 0xde, 0xad, 0xbe, 0xef,
          0x55, 0xaa, 0x55, 0xaa, 0xf0, 0x0f, 0xf0, 0x0f,
          0xaa, 0x55, 0xaa, 0x55, 0xde, 0xad, 0xbe, 0xef,
          0x55, 0xaa, 0x55, 0xaa, 0xf0, 0x0f, 0xf0, 0x0f };

    /* final digest after 1000 iterations */
    static const uint8_t expected[] =
        { 0x76, 0x38, 0xf3, 0xbc, 0x50, 0x0d, 0xd1, 0xa6,
          0x58, 0x6d, 0xd4, 0xd0, 0x1a, 0x15, 0x51, 0xaf,
          0xd8, 0x21, 0xd2, 0x35, 0x2f, 0x91, 0x9e, 0x28,
          0xd5, 0x84, 0x2f, 0xab, 0x03, 0xa4, 0x0f, 0x2a };

    int i, n = 1000;

    if (init() != 0)
        return -1;
    if ((sha256_addr_base == 0) && !quiet) {
        printf("TC7: SHA-256 not present\n");
        return 0;
    }

    if (!quiet)
        printf("TC7: Message with %d blocks test for SHA-256.\n", n);

    /* Write block data to SHA-256. */
    if (tc_write(sha256_addr_base + SHA256_ADDR_BLOCK, block, SHA256_BLOCK_LEN))
        return 1;

    /* Start initial block hashing, wait and check status. */
    if (tc_init(sha256_addr_base + SHA256_ADDR_CTRL) ||
        tc_wait_ready(sha256_addr_base + SHA256_ADDR_STATUS))
        return 1;

    /* First block done. Do the rest. */
    for (i = 1; i < n; ++i) {
        /* Start next block hashing, wait and check status. */
        if (tc_next(sha256_addr_base + SHA256_ADDR_CTRL) ||
            tc_wait_ready(sha256_addr_base + SHA256_ADDR_STATUS))
            return 1;
    }

    /* XXX valid is probably set at the same time as ready */
    if (tc_wait_valid(sha256_addr_base + SHA256_ADDR_STATUS))
        return 1;
    /* Extract the final digest. */
    return tc_expected(sha256_addr_base + SHA256_ADDR_DIGEST, expected, SHA256_DIGEST_LEN);
}

/* ---------------- SHA-512 test cases ---------------- */

/* TC8: Read name and version from SHA-512 core. */
int TC8()
{
    uint8_t name0[4]   = SHA512_NAME0;
    uint8_t name1[4]   = SHA512_NAME1;
    uint8_t version[4] = SHA512_VERSION;

    if (init() != 0)
        return -1;
    if ((sha512_addr_base == 0) && !quiet) {
        printf("TC8: SHA-512 not present\n");
        return 0;
    }

    if (!quiet)
        printf("TC8: Reading name and version words from SHA-512 core.\n");

    return
        tc_expected(sha512_addr_base + SHA512_ADDR_NAME0, name0, 4) ||
        tc_expected(sha512_addr_base + SHA512_ADDR_NAME1, name1, 4) ||
        tc_expected(sha512_addr_base + SHA512_ADDR_VERSION, version, 4);
}

/* TC9: SHA-512 Single block message test as specified by NIST.
   We do this for all modes. */
int tc9(int mode, const uint8_t *expected, int digest_len)
{
    const uint8_t *block = NIST_1024_SINGLE;
    uint8_t init[4] = { 0, 0, 0, CTRL_INIT + mode };

    return
        /* Write block to SHA-512. */
        tc_write(sha512_addr_base + SHA512_ADDR_BLOCK, block, SHA512_BLOCK_LEN) ||
        /* Start initial block hashing, wait and check status. */
        tc_write(sha512_addr_base + SHA512_ADDR_CTRL, init, 4) ||
        tc_wait_valid(sha512_addr_base + SHA512_ADDR_STATUS) ||
        /* Extract the digest. */
        tc_expected(sha512_addr_base + SHA512_ADDR_DIGEST, expected, digest_len);
}

int TC9()
{
    if (init() != 0)
        return -1;
    if ((sha512_addr_base == 0) && !quiet) {
        printf("TC9: SHA-512 not present\n");
        return 0;
    }

    if (!quiet)
        printf("TC9-1: Single block message test for SHA-512/224.\n");
    if (tc9(MODE_SHA_512_224, SHA512_224_SINGLE_DIGEST, SHA512_224_DIGEST_LEN) != 0)
        return 1;

    if (!quiet)
        printf("TC9-2: Single block message test for SHA-512/256.\n");
    if (tc9(MODE_SHA_512_256, SHA512_256_SINGLE_DIGEST, SHA512_256_DIGEST_LEN) != 0)
        return 1;

    if (!quiet)
        printf("TC9-3: Single block message test for SHA-384.\n");
    if (tc9(MODE_SHA_384, SHA384_SINGLE_DIGEST, SHA384_DIGEST_LEN) != 0)
        return 1;

    if (!quiet)
        printf("TC9-4: Single block message test for SHA-512.\n");
    if (tc9(MODE_SHA_512, SHA512_SINGLE_DIGEST, SHA512_DIGEST_LEN) != 0)
        return 1;

    return 0;
}

/* TC10: SHA-512 Double block message test as specified by NIST.
   We do this for all modes. */
int tc10(int mode, const uint8_t *expected, int digest_len)
{
    const uint8_t *block[2] = { NIST_1024_DOUBLE0, NIST_1024_DOUBLE1 };
    uint8_t init[4] = { 0, 0, 0, CTRL_INIT + mode };
    uint8_t next[4] = { 0, 0, 0, CTRL_NEXT + mode };

    return
        /* Write first block to SHA-512. */
        tc_write(sha512_addr_base + SHA512_ADDR_BLOCK, block[0], SHA512_BLOCK_LEN) ||
        /* Start initial block hashing, wait and check status. */
        tc_write(sha512_addr_base + SHA512_ADDR_CTRL, init, 4) ||
        tc_wait_ready(sha512_addr_base + SHA512_ADDR_STATUS) ||
        /* Write second block to SHA-512. */
        tc_write(sha512_addr_base + SHA512_ADDR_BLOCK, block[1], SHA512_BLOCK_LEN) ||
        /* Start next block hashing, wait and check status. */
        tc_write(sha512_addr_base + SHA512_ADDR_CTRL, next, 4) ||
        tc_wait_valid(sha512_addr_base + SHA512_ADDR_STATUS) ||
        /* Extract the digest. */
        tc_expected(sha512_addr_base + SHA512_ADDR_DIGEST, expected, digest_len);
}

int TC10()
{
    if (init() != 0)
        return -1;
    if ((sha512_addr_base == 0) && !quiet) {
        printf("TC10: SHA-512 not present\n");
        return 0;
    }

    if (!quiet)
        printf("TC10-1: Double block message test for SHA-512/224.\n");
    if (tc10(MODE_SHA_512_224, SHA512_224_DOUBLE_DIGEST, SHA512_224_DIGEST_LEN) != 0)
        return 1;

    if (!quiet)
        printf("TC10-2: Double block message test for SHA-512/256.\n");
    if (tc10(MODE_SHA_512_256, SHA512_256_DOUBLE_DIGEST, SHA512_256_DIGEST_LEN) != 0)
        return 1;

    if (!quiet)
        printf("TC10-3: Double block message test for SHA-384.\n");
    if (tc10(MODE_SHA_384, SHA384_DOUBLE_DIGEST, SHA384_DIGEST_LEN) != 0)
        return 1;

    if (!quiet)
        printf("TC10-4: Double block message test for SHA-512.\n");
    if (tc10(MODE_SHA_512, SHA512_DOUBLE_DIGEST, SHA512_DIGEST_LEN) != 0)
        return 1;

    return 0;
}

/* ---------------- main ---------------- */

/* signal handler for ctrl-c to end repeat testing */
unsigned long iter = 0;
struct timeval tv_start, tv_end;
void sighandler(int unused)
{
    double tv_diff;

    gettimeofday(&tv_end, NULL);
    tv_diff = (double)(tv_end.tv_sec - tv_start.tv_sec) +
        (double)(tv_end.tv_usec - tv_start.tv_usec)/1000000;
    printf("\n%lu iterations in %.3f seconds (%.3f iterations/sec)\n",
           iter, tv_diff, (double)iter/tv_diff);
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    typedef int (*tcfp)(void);
    tcfp all_tests[] = { TC0, TC1, TC2, TC3, TC4, TC5, TC6, TC7, TC8, TC9, TC10 };
    tcfp sha1_tests[] = { TC1, TC2, TC3 };
    tcfp sha256_tests[] = { TC4, TC5, TC6, TC7 };
    tcfp sha512_tests[] = { TC8, TC9, TC10 };

    char *usage = "Usage: %s [-h] [-d] [-q] [-r] tc...\n";
    int i, j, opt;

    while ((opt = getopt(argc, argv, "h?dqr")) != -1) {
        switch (opt) {
        case 'h':
        case '?':
            printf(usage, argv[0]);
            return EXIT_SUCCESS;
        case 'd':
            tc_set_debug(1);
            break;
        case 'q':
            quiet = 1;
            break;
        case 'r':
            repeat = 1;
            break;
        default:
            fprintf(stderr, usage, argv[0]);
            return EXIT_FAILURE;
        }
    }

    /* repeat one test until interrupted */
    if (repeat) {
        tcfp tc;
        if (optind != argc - 1) {
            fprintf(stderr, "only one test case can be repeated\n");
            return EXIT_FAILURE;
        }
        j = atoi(argv[optind]);
        if (j < 0 || j >= sizeof(all_tests)/sizeof(all_tests[0])) {
            fprintf(stderr, "invalid test number %s\n", argv[optind]);
            return EXIT_FAILURE;
        }
        tc = (all_tests[j]);
        srand(time(NULL));
        signal(SIGINT, sighandler);
        gettimeofday(&tv_start, NULL);
        while (1) {
            ++iter;
            if ((iter & 0xffff) == 0) {
                printf(".");
                fflush(stdout);
            }
            if (tc() != 0)
                sighandler(0);
        }
        return EXIT_SUCCESS;    /*NOTREACHED*/
    }

    /* no args == run all tests */
    if (optind >= argc) {
        for (j = 0; j < sizeof(all_tests)/sizeof(all_tests[0]); ++j)
            if (all_tests[j]() != 0)
                return EXIT_FAILURE;
        return EXIT_SUCCESS;
    }

    /* run one or more tests (by number) or groups of tests (by name) */
    for (i = optind; i < argc; ++i) {
        if (strcmp(argv[i], "all") == 0) {
            for (j = 0; j < sizeof(all_tests)/sizeof(all_tests[0]); ++j)
                if (all_tests[j]() != 0)
                    return EXIT_FAILURE;
        }
        else if (strcmp(argv[i], "sha1") == 0) {
            for (j = 0; j < sizeof(sha1_tests)/sizeof(sha1_tests[0]); ++j)
                if (sha1_tests[j]() != 0)
                    return EXIT_FAILURE;
        }
        else if (strcmp(argv[i], "sha256") == 0) {
            for (j = 0; j < sizeof(sha256_tests)/sizeof(sha256_tests[0]); ++j)
                if (sha256_tests[j]() != 0)
                    return EXIT_FAILURE;
        }
        else if (strcmp(argv[i], "sha512") == 0) {
            for (j = 0; j < sizeof(sha512_tests)/sizeof(sha512_tests[0]); ++j)
                if (sha512_tests[j]() != 0)
                    return EXIT_FAILURE;
        }
        else if (isdigit(argv[i][0]) &&
                 (((j = atoi(argv[i])) >= 0) &&
                  (j < sizeof(all_tests)/sizeof(all_tests[0])))) {
            if (all_tests[j]() != 0)
                return EXIT_FAILURE;
        }
        else {
            fprintf(stderr, "unknown test case %s\n", argv[i]);
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
