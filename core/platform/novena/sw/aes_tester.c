//======================================================================
//
// aes_tester.c
// ------------
// Simple test sw for the aes core. Based on the NIST test cases.
// Test cases taken from NIST SP 800-38A:
// http://csrc.nist.gov/publications/nistpubs/800-38a/sp800-38a.pdf
//
//
// Author: Joachim Strombergson
// Copyright (c) 2014-2015, NORDUnet A/S All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// - Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
//
// - Neither the name of the NORDUnet nor the names of its contributors may
//   be used to endorse or promote products derived from this software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//
//======================================================================

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "cryptech.h"


//------------------------------------------------------------------
// Global defines.
//------------------------------------------------------------------
#define VERBOSE 0
#define CHECK_WRITE 0


//------------------------------------------------------------------
// Robs macros. Scary scary.
//------------------------------------------------------------------
#define check(_expr_)				\
  do {						\
    if ((_expr_) != 0) {			\
      printf("%s failed\n", #_expr_);		\
      exit(1);					\
    }						\
  } while (0)


//------------------------------------------------------------------
//------------------------------------------------------------------
static off_t aes_addr_base;

static void check_aes_access(void)
{
    aes_addr_base = tc_core_base("aes");
    assert(aes_addr_base != 0);
}


//------------------------------------------------------------------
//------------------------------------------------------------------
static void tc_w32(const off_t addr, const uint32_t data)
{
  uint8_t w[4];
  w[0] = data >> 24 & 0xff;
  w[1] = data >> 16 & 0xff;
  w[2] = data >> 8  & 0xff;
  w[3] = data       & 0xff;
  check(tc_write(addr, w, 4));
}


//------------------------------------------------------------------
//------------------------------------------------------------------
static uint32_t tc_r32(const off_t addr)
{
  uint8_t w[4];
  check(tc_read(addr, w, 4));
  return (uint32_t)((w[0] << 24) + (w[1] << 16) + (w[2] << 8) + w[3]);
}


//------------------------------------------------------------------
// single_block_test
//
// Perform single block tests.
//------------------------------------------------------------------
static void single_block_test(const uint32_t keylength, const uint32_t *key,
			      const uint32_t *block, const uint32_t *expected)
{
  uint32_t enc_result[4];
  uint32_t dec_result[4];

  if (VERBOSE) {
    if (keylength == 256)
      printf("Writing key 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",
	     key[0], key[1], key[2], key[3],
	     key[4], key[5], key[6], key[7]);
    else
      printf("Writing key 0x%08x 0x%08x 0x%08x 0x%08x\n",
	     key[0], key[1], key[2], key[3]);
  }

  tc_w32(aes_addr_base + AES_ADDR_KEY0, key[0]);
  tc_w32(aes_addr_base + AES_ADDR_KEY1, key[1]);
  tc_w32(aes_addr_base + AES_ADDR_KEY2, key[2]);
  tc_w32(aes_addr_base + AES_ADDR_KEY3, key[3]);

  if (keylength == 256) {
    tc_w32(aes_addr_base + AES_ADDR_KEY4, key[4]);
    tc_w32(aes_addr_base + AES_ADDR_KEY5, key[5]);
    tc_w32(aes_addr_base + AES_ADDR_KEY6, key[6]);
    tc_w32(aes_addr_base + AES_ADDR_KEY7, key[7]);
  }

  if (CHECK_WRITE) {
    const uint32_t
      k0 = tc_r32(aes_addr_base + AES_ADDR_KEY0), k1 = tc_r32(aes_addr_base + AES_ADDR_KEY1),
      k2 = tc_r32(aes_addr_base + AES_ADDR_KEY2), k3 = tc_r32(aes_addr_base + AES_ADDR_KEY3),
      k4 = tc_r32(aes_addr_base + AES_ADDR_KEY4), k5 = tc_r32(aes_addr_base + AES_ADDR_KEY5),
      k6 = tc_r32(aes_addr_base + AES_ADDR_KEY6), k7 = tc_r32(aes_addr_base + AES_ADDR_KEY7);
    const int
      ok1 = k0 == key[0] && k1 == key[1] && k2 == key[2] && k3 == key[3],
      ok2 = k4 == key[4] && k5 == key[5] && k6 == key[6] && k7 == key[7];
    if (keylength == 256)
      printf("Reading back key: 0x%08x 0x%08x 0x%08x 0x%08x  0x%08x 0x%08x 0x%08x 0x%08x %s\n",
	     k0, k1, k2, k3, k4, k5, k6, k7, (ok1 && ok2) ? "OK" : "BAD");
    else
      printf("Reading back key: 0x%08x 0x%08x 0x%08x 0x%08x %s\n",
	     k0, k1, k2, k3, ok1 ? "OK" : "BAD");
  }

  // Performing init i.e. key expansion,
  printf("Doing key init\n");
  if (keylength == 256)
    tc_w32(aes_addr_base + AES_ADDR_CONFIG, 0x00000002);
  else
    tc_w32(aes_addr_base + AES_ADDR_CONFIG, 0x00000000);

  tc_w32(aes_addr_base + AES_ADDR_CTRL,   0x00000001);


  if (VERBOSE)
    printf("Writing block 0x%08x 0x%08x 0x%08x 0x%08x\n",
	   block[0], block[1], block[2], block[3]);

  tc_w32(aes_addr_base + AES_ADDR_BLOCK0, block[0]);
  tc_w32(aes_addr_base + AES_ADDR_BLOCK1, block[1]);
  tc_w32(aes_addr_base + AES_ADDR_BLOCK2, block[2]);
  tc_w32(aes_addr_base + AES_ADDR_BLOCK3, block[3]);

  if (CHECK_WRITE) {
    const uint32_t
      b0 = tc_r32(aes_addr_base + AES_ADDR_BLOCK0), b1 = tc_r32(aes_addr_base + AES_ADDR_BLOCK1),
      b2 = tc_r32(aes_addr_base + AES_ADDR_BLOCK2), b3 = tc_r32(aes_addr_base + AES_ADDR_BLOCK3);
    const int
      ok = b0 == block[0] && b1 == block[1] && b2 == block[2] && b3 == block[3];
    printf("Reading back block: 0x%08x  0x%08x 0x%08x 0x%08x %s\n",
	   b0, b1, b2, b3, ok ? "OK" : "BAD");
  }

  if (VERBOSE)
    printf("Starting single block encipher operation\n");

  if (keylength == 256)
    tc_w32(aes_addr_base + AES_ADDR_CONFIG, 0x00000003);
  else
    tc_w32(aes_addr_base + AES_ADDR_CONFIG, 0x00000001);

  tc_w32(aes_addr_base + AES_ADDR_CTRL,   0x00000002);

  if (VERBOSE)
      printf("Checking ready: 0x%08x\n",
	     tc_r32(aes_addr_base + AES_ADDR_STATUS));

  check(tc_wait_ready(aes_addr_base + AES_ADDR_STATUS));

  if (VERBOSE)
    printf("Ready seen. Result: 0x%08x 0x%08x 0x%08x 0x%08x\n",
	   tc_r32(aes_addr_base + AES_ADDR_RESULT0), tc_r32(aes_addr_base + AES_ADDR_RESULT1),
	   tc_r32(aes_addr_base + AES_ADDR_RESULT2), tc_r32(aes_addr_base + AES_ADDR_RESULT3));

  enc_result[0] = tc_r32(aes_addr_base + AES_ADDR_RESULT0);
  enc_result[1] = tc_r32(aes_addr_base + AES_ADDR_RESULT1);
  enc_result[2] = tc_r32(aes_addr_base + AES_ADDR_RESULT2);
  enc_result[3] = tc_r32(aes_addr_base + AES_ADDR_RESULT3);

  tc_w32(aes_addr_base + AES_ADDR_BLOCK0, enc_result[0]);
  tc_w32(aes_addr_base + AES_ADDR_BLOCK1, enc_result[1]);
  tc_w32(aes_addr_base + AES_ADDR_BLOCK2, enc_result[2]);
  tc_w32(aes_addr_base + AES_ADDR_BLOCK3, enc_result[3]);

  // Single block decipher operation.
  if (keylength == 256)
    tc_w32(aes_addr_base + AES_ADDR_CONFIG, 0x00000002);
  else
    tc_w32(aes_addr_base + AES_ADDR_CONFIG, 0x00000000);
  tc_w32(aes_addr_base + AES_ADDR_CTRL,   0x00000002);

  check(tc_wait_ready(aes_addr_base + AES_ADDR_STATUS));

  dec_result[0] = tc_r32(aes_addr_base + AES_ADDR_RESULT0);
  dec_result[1] = tc_r32(aes_addr_base + AES_ADDR_RESULT1);
  dec_result[2] = tc_r32(aes_addr_base + AES_ADDR_RESULT2);
  dec_result[3] = tc_r32(aes_addr_base + AES_ADDR_RESULT3);

  printf("Generated cipher block: 0x%08x 0x%08x 0x%08x 0x%08x\n",
         enc_result[0], enc_result[1], enc_result[2], enc_result[3]);
  printf("Expected cipher block:  0x%08x 0x%08x 0x%08x 0x%08x\n",
         expected[0], expected[1], expected[2], expected[3]);
  if (enc_result[0] == expected[0] && enc_result[1] == expected[1] &&
      enc_result[2] == expected[2] && enc_result[3] == expected[3])
    printf("OK\n");
  else
    printf("BAD\n");
  printf("\n");

  printf("Generated decipher block: 0x%08x 0x%08x 0x%08x 0x%08x\n",
         dec_result[0], dec_result[1], dec_result[2], dec_result[3]);
  printf("Expected decipher block:  0x%08x 0x%08x 0x%08x 0x%08x\n",
         block[0], block[1], block[2], block[3]);
  if (dec_result[0] == block[0] && dec_result[1] == block[1] &&
      dec_result[2] == block[2] && dec_result[3] == block[3])
    printf("OK\n");
  else
    printf("BAD\n");
  printf("\n");
}


//------------------------------------------------------------------
// run_nist_tests()
//------------------------------------------------------------------
static void run_nist_tests()
{
  static const uint32_t nist_aes128_key[4] = {0x2b7e1516, 0x28aed2a6, 0xabf71588, 0x09cf4f3c};
  static const uint32_t nist_aes256_key[8] = {0x603deb10, 0x15ca71be, 0x2b73aef0, 0x857d7781,
					      0x1f352c07, 0x3b6108d7, 0x2d9810a3, 0x0914dff4};

  static const uint32_t nist_plaintext0[4] = {0x6bc1bee2, 0x2e409f96, 0xe93d7e11, 0x7393172a};
  static const uint32_t nist_plaintext1[4] = {0xae2d8a57, 0x1e03ac9c, 0x9eb76fac, 0x45af8e51};
  static const uint32_t nist_plaintext2[4] = {0x30c81c46, 0xa35ce411, 0xe5fbc119, 0x1a0a52ef};
  static const uint32_t nist_plaintext3[4] = {0xf69f2445, 0xdf4f9b17, 0xad2b417b, 0xe66c3710};

  static const uint32_t nist_ecb_128_enc_expected0[4] = {0x3ad77bb4, 0x0d7a3660, 0xa89ecaf3, 0x2466ef97};
  static const uint32_t nist_ecb_128_enc_expected1[4] = {0xf5d3d585, 0x03b9699d, 0xe785895a, 0x96fdbaaf};
  static const uint32_t nist_ecb_128_enc_expected2[4] = {0x43b1cd7f, 0x598ece23, 0x881b00e3, 0xed030688};
  static const uint32_t nist_ecb_128_enc_expected3[4] = {0x7b0c785e, 0x27e8ad3f, 0x82232071, 0x04725dd4};

  static const uint32_t nist_ecb_256_enc_expected0[4] = {0xf3eed1bd, 0xb5d2a03c, 0x064b5a7e, 0x3db181f8};
  static const uint32_t nist_ecb_256_enc_expected1[4] = {0x591ccb10, 0xd410ed26, 0xdc5ba74a, 0x31362870};
  static const uint32_t nist_ecb_256_enc_expected2[4] = {0xb6ed21b9, 0x9ca6f4f9, 0xf153e7b1, 0xbeafed1d};
  static const uint32_t nist_ecb_256_enc_expected3[4] = {0x23304b7a, 0x39f9f3ff, 0x067d8d8f, 0x9e24ecc7};

  printf("Running NIST single block test.\n");

  single_block_test(128, &nist_aes128_key[0], &nist_plaintext0[0], &nist_ecb_128_enc_expected0[0]);
  single_block_test(128, &nist_aes128_key[0], &nist_plaintext1[0], &nist_ecb_128_enc_expected1[0]);

  single_block_test(128, &nist_aes128_key[0], &nist_plaintext2[0], &nist_ecb_128_enc_expected2[0]);
  single_block_test(128, &nist_aes128_key[0], &nist_plaintext3[0], &nist_ecb_128_enc_expected3[0]);

  single_block_test(256, &nist_aes256_key[0], &nist_plaintext0[0], &nist_ecb_256_enc_expected0[0]);
  single_block_test(256, &nist_aes256_key[0], &nist_plaintext1[0], &nist_ecb_256_enc_expected1[0]);
  single_block_test(256, &nist_aes256_key[0], &nist_plaintext2[0], &nist_ecb_256_enc_expected2[0]);
  single_block_test(256, &nist_aes256_key[0], &nist_plaintext3[0], &nist_ecb_256_enc_expected3[0]);

}


//------------------------------------------------------------------
//------------------------------------------------------------------
int main(int argc, char *argv[])
{
  check_aes_access();
//  tc_set_debug(1);
  run_nist_tests();

  return 0;
}

//======================================================================
// EOF aes_tester.c
//======================================================================
