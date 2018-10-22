//======================================================================
//
// modexp_tester.c
// ---------------
// Simple test sw for the modexp.
//
//
// Author: Joachim Strombergson, Rob Austein,
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
#define check(_expr_)			\
  do {					\
    if ((_expr_) != 0) {		\
      printf("%s failed\n", #_expr_);	\
      exit(1);				\
    }					\
  } while (0)


//------------------------------------------------------------------
// tc_w32()
//
// Write 32-bit word to given address.
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
// tc_r32
//
// Read 32-bit word from given address.
//------------------------------------------------------------------
static uint32_t tc_r32(const off_t addr)
{
  uint8_t w[4];
  check(tc_read(addr, w, 4));
  return (uint32_t)((w[0] << 24) + (w[1] << 16) + (w[2] << 8) + w[3]);
}


//------------------------------------------------------------------
// check_modexp_access
//
// Check that we can read from the modexp core by trying to
// read out the name and version.
//------------------------------------------------------------------
static off_t modexp_addr_base;

static void check_modexp_access(void)
{
    modexp_addr_base = tc_core_base("modexp");
    assert(modexp_addr_base != 0);
}


#if 0
//------------------------------------------------------------------
// check_modulus_mem()
//
// Check that we can write and read to the modulus memory.
//------------------------------------------------------------------
static void check_modulus_mem(void)
{
  uint8_t i;
  uint32_t j;

  printf("Testing modulus mem access.\n");

  tc_w32(modexp_addr_base + MODEXP_MODULUS_PTR_RST, 0x00000000);
  // Write test data to modulus mempory.
  for (i = 0 ; i < 64; i = i + 1) {
    j = ((i * 4 + 3) << 24) + ((i * 4 + 2) << 16) +
      ((i * 4 + 1) << 8) + i * 4;
    tc_w32(modexp_addr_base + MODEXP_MODULUS_DATA, j);
  }

  tc_w32(modexp_addr_base + MODEXP_MODULUS_PTR_RST, 0x00000000);
  // Read out test data from modulus mempory.
  for (i = 0 ; i < 64 ; i = i + 4) {
    printf("modulus mem: 0x%08x 0x%08x 0x%08x 0x%08x\n",
	   tc_r32(modexp_addr_base + MODEXP_MODULUS_DATA),
	   tc_r32(modexp_addr_base + MODEXP_MODULUS_DATA),
	   tc_r32(modexp_addr_base + MODEXP_MODULUS_DATA),
	   tc_r32(modexp_addr_base + MODEXP_MODULUS_DATA));
  }
}


//------------------------------------------------------------------
// check_exponent_mem()
//
// Check that we can write and read to the exponent memory.
//------------------------------------------------------------------
static void check_exponent_mem(void)
{
  uint8_t i;
  uint32_t j;

  printf("Testing exponent mem access.\n");

  tc_w32(modexp_addr_base + MODEXP_EXPONENT_PTR_RST, 0x00000000);
  // Write test data to exponent memory.
  for (i = 0 ; i < 64; i = i + 1) {
    j = ((i * 4 + 3) << 24) + ((i * 4 + 2) << 16) +
      ((i * 4 + 1) << 8) + i * 4;
    tc_w32(modexp_addr_base + MODEXP_EXPONENT_DATA, j);
  }

  tc_w32(modexp_addr_base + MODEXP_EXPONENT_PTR_RST, 0x00000000);
  // Read out test data from exponent memory.
  for (i = 0 ; i < 64 ; i = i + 4) {
    printf("exponent mem: 0x%08x 0x%08x 0x%08x 0x%08x\n",
	   tc_r32(modexp_addr_base + MODEXP_EXPONENT_DATA),
	   tc_r32(modexp_addr_base + MODEXP_EXPONENT_DATA),
	   tc_r32(modexp_addr_base + MODEXP_EXPONENT_DATA),
	   tc_r32(modexp_addr_base + MODEXP_EXPONENT_DATA));
  }
}



//------------------------------------------------------------------
// check_message_mem()
//
// Check that we can write and read to the message memory.
//------------------------------------------------------------------
static void check_message_mem(void)
{
  uint8_t i;
  uint32_t j;

  printf("Testing message mem access.\n");

  tc_w32(modexp_addr_base + MODEXP_MESSAGE_PTR_RST, 0x00000000);
  // Write test data to message memory.
  for (i = 0 ; i < 64; i = i + 1) {
    j = ((i * 4 + 3) << 24) + ((i * 4 + 2) << 16) +
      ((i * 4 + 1) << 8) + i * 4;
    tc_w32(modexp_addr_base + MODEXP_MESSAGE_DATA, j);
  }

  tc_w32(modexp_addr_base + MODEXP_MESSAGE_PTR_RST, 0x00000000);
  // Read out test data from messsage memory.
  for (i = 0 ; i < 64 ; i = i + 4) {
    printf("message mem: 0x%08x 0x%08x 0x%08x 0x%08x\n",
	   tc_r32(modexp_addr_base + MODEXP_MESSAGE_DATA),
	   tc_r32(modexp_addr_base + MODEXP_MESSAGE_DATA),
	   tc_r32(modexp_addr_base + MODEXP_MESSAGE_DATA),
	   tc_r32(modexp_addr_base + MODEXP_MESSAGE_DATA));
  }
}


//------------------------------------------------------------------
// clear_mems()
//
// Zero fill the memories.
//------------------------------------------------------------------
static void clear_mems()
{
  uint32_t i;
  tc_w32(modexp_addr_base + MODEXP_MESSAGE_PTR_RST,  0x00000000);
  tc_w32(modexp_addr_base + MODEXP_EXPONENT_PTR_RST, 0x00000000);
  tc_w32(modexp_addr_base + MODEXP_MODULUS_PTR_RST,  0x00000000);

  for (i = 0 ; i < 256 ; i++) {
    tc_w32(modexp_addr_base + MODEXP_MESSAGE_DATA,  0x00000000);
    tc_w32(modexp_addr_base + MODEXP_EXPONENT_DATA, 0x00000000);
    tc_w32(modexp_addr_base + MODEXP_MODULUS_DATA,  0x00000000);
  }

  tc_w32(modexp_addr_base + MODEXP_MESSAGE_PTR_RST,  0x00000000);
  tc_w32(modexp_addr_base + MODEXP_EXPONENT_PTR_RST, 0x00000000);
  tc_w32(modexp_addr_base + MODEXP_MODULUS_PTR_RST,  0x00000000);
}


//------------------------------------------------------------------
// dump_mems()
//
// Dump the first words from the memories.
//------------------------------------------------------------------
static void dump_mems()
{
  tc_w32(modexp_addr_base + MODEXP_MESSAGE_PTR_RST, 0x00000000);
  printf("First words in messagee mem:\n");
  printf("0x%08x 0x%08x 0x%08x 0x%08x\n",
	 tc_r32(modexp_addr_base + MODEXP_MESSAGE_DATA), tc_r32(modexp_addr_base + MODEXP_MESSAGE_DATA),
	 tc_r32(modexp_addr_base + MODEXP_MESSAGE_DATA), tc_r32(modexp_addr_base + MODEXP_MESSAGE_DATA));

  tc_w32(modexp_addr_base + MODEXP_EXPONENT_PTR_RST, 0x00000000);
  printf("First words in exponent mem:\n");
  printf("0x%08x 0x%08x 0x%08x 0x%08x\n",
	 tc_r32(modexp_addr_base + MODEXP_EXPONENT_DATA), tc_r32(modexp_addr_base + MODEXP_EXPONENT_DATA),
	 tc_r32(modexp_addr_base + MODEXP_EXPONENT_DATA), tc_r32(modexp_addr_base + MODEXP_EXPONENT_DATA));

  tc_w32(modexp_addr_base + MODEXP_MODULUS_PTR_RST, 0x00000000);
  printf("First words in modulus mem:\n");
  printf("0x%08x 0x%08x 0x%08x 0x%08x\n",
	 tc_r32(modexp_addr_base + MODEXP_MODULUS_DATA), tc_r32(modexp_addr_base + MODEXP_MODULUS_DATA),
	 tc_r32(modexp_addr_base + MODEXP_MODULUS_DATA), tc_r32(modexp_addr_base + MODEXP_MODULUS_DATA));

  tc_w32(modexp_addr_base + MODEXP_RESULT_PTR_RST, 0x00000000);
  printf("First words in result mem:\n");
  printf("0x%08x 0x%08x 0x%08x 0x%08x\n",
	 tc_r32(modexp_addr_base + MODEXP_RESULT_DATA), tc_r32(modexp_addr_base + MODEXP_RESULT_DATA),
	 tc_r32(modexp_addr_base + MODEXP_RESULT_DATA), tc_r32(modexp_addr_base + MODEXP_RESULT_DATA));

  tc_w32(modexp_addr_base + MODEXP_MESSAGE_PTR_RST, 0x00000000);
  tc_w32(modexp_addr_base + MODEXP_EXPONENT_PTR_RST, 0x00000000);
  tc_w32(modexp_addr_base + MODEXP_MODULUS_PTR_RST, 0x00000000);
  tc_w32(modexp_addr_base + MODEXP_RESULT_PTR_RST, 0x00000000);
}
#endif /* #if 0 */


//------------------------------------------------------------------
// testrunner()
//------------------------------------------------------------------
uint8_t testrunner(uint32_t exp_len, uint32_t *exponent,
                   uint32_t mod_len, uint32_t *modulus,
                   uint32_t *message, uint32_t *expected)
{
  uint32_t i;
  uint32_t result;
  uint8_t correct;

  tc_w32(modexp_addr_base + MODEXP_EXPONENT_LENGTH, exp_len);
  tc_w32(modexp_addr_base + MODEXP_EXPONENT_PTR_RST, 0x00000000);
  for (i = 0 ; i < mod_len ; i++) {
    tc_w32(modexp_addr_base + MODEXP_EXPONENT_DATA, exponent[i]);
  }

  tc_w32(modexp_addr_base + MODEXP_MODULUS_LENGTH, mod_len);
  tc_w32(modexp_addr_base + MODEXP_MESSAGE_PTR_RST, 0x00000000);
  tc_w32(modexp_addr_base + MODEXP_MODULUS_PTR_RST, 0x00000000);
  for (i = 0 ; i < mod_len ; i++) {
    tc_w32(modexp_addr_base + MODEXP_MESSAGE_DATA, message[i]);
    tc_w32(modexp_addr_base + MODEXP_MODULUS_DATA, modulus[i]);
  }

  tc_w32(modexp_addr_base + MODEXP_ADDR_CTRL, 0x00000001);
  check(tc_wait_ready(modexp_addr_base + MODEXP_ADDR_STATUS));

  correct = 1;

  tc_w32(modexp_addr_base + MODEXP_RESULT_PTR_RST, 0x00000000);
  for (i = 0 ; i < mod_len ; i++) {
    result = tc_r32(modexp_addr_base + MODEXP_RESULT_DATA);
    if (result != expected[i]) {
      printf("Error. Expected 0x%08x, got 0x%08x\n", expected[i], result);
      correct = 0;
    }
  }

  return correct;
}


//------------------------------------------------------------------
// tc1()
//
// c = m ** e % N with the following (decimal) test values:
//  m = 3
//  e = 7
//  n = 11 (0x0b)
//  c = 3 ** 7 % 11 = 9
//------------------------------------------------------------------
static void tc1()
{
  uint32_t exponent[] = {0x00000007};
  uint32_t modulus[]  = {0x0000000b};
  uint32_t message[]  = {0x00000003};
  uint32_t expected[] = {0x00000009};
  uint8_t result;

  printf("Running TC1: 0x03 ** 0x07 mod 0x0b = 0x09\n");

  result = testrunner(1, exponent, 1, modulus, message, expected);

  if (result)
    printf("TC1: OK\n");
  else
    printf("TC1: NOT OK\n");
}


//------------------------------------------------------------------
// tc2()
//
// c = m ** e % N with the following test values:
//  m = 251 (0xfb)
//  e = 251 (0xfb)
//  n = 257 (0x101)
//  c = 251 ** 251 % 257 = 183 (0xb7)
//------------------------------------------------------------------
static void tc2()
{
  uint32_t exponent[] = {0x000000fb};
  uint32_t modulus[]  = {0x00000101};
  uint32_t message[]  = {0x000000fb};
  uint32_t expected[] = {0x000000b7};
  uint8_t result;

  printf("Running TC2: 0xfb ** 0xfb mod 0x101 = 0xb7\n");


  result = testrunner(1, exponent, 1, modulus, message, expected);

  if (result)
    printf("TC2: OK\n");
  else
    printf("TC2: NOT OK\n");
}


//------------------------------------------------------------------
// tc3()
//
// c = m ** e % N with the following test values:
//  m = 0x81
//  e = 0x41
//  n = 0x87
//  c = 0x81 ** 0x41 % 0x87 = 0x36
//------------------------------------------------------------------
static void tc3()
{
  uint32_t exponent[] = {0x00000041};
  uint32_t modulus[]  = {0x00000087};
  uint32_t message[]  = {0x00000081};
  uint32_t expected[] = {0x00000036};
  uint8_t result;

  printf("Running TC3: 0x81 ** 0x41 mod 0x87 = 0x36\n");

  result = testrunner(1, exponent, 1, modulus, message, expected);

  if (result)
    printf("TC3: OK\n");
  else
    printf("TC3: NOT OK\n");
}


//------------------------------------------------------------------
// tc4()
//
// c = m ** e % N with the following test values:
//  m = 0x00000001946473e1
//  e = 0xh000000010e85e74f
//  n = 0x0000000170754797
//  c = 0x000000007761ed4f
//
// These operands spans two 32-bit words.
//------------------------------------------------------------------
static void tc4()
{
  uint32_t exponent[] = {0x00000001, 0x0e85e74f};
  uint32_t modulus[]  = {0x00000001, 0x70754797};
  uint32_t message[]  = {0x00000001, 0x946473e1};
  uint32_t expected[] = {0x00000000, 0x7761ed4f};
  uint8_t result;

  printf("Running TC4: 0x00000001946473e1 ** 0xh000000010e85e74f mod 0x0000000170754797 = 0x000000007761ed4f\n");

  result = testrunner(2, exponent, 2, modulus, message, expected);

  if (result)
    printf("TC4: OK\n");
  else
    printf("TC4: NOT OK\n");
}


//------------------------------------------------------------------
// tc5()
//
// c = m ** e % N with 128 bit operands.
//------------------------------------------------------------------
static void tc5()
{
  uint32_t exponent[] = {0x3285c343, 0x2acbcb0f, 0x4d023228, 0x2ecc73db};
  uint32_t modulus[]  = {0x267d2f2e, 0x51c216a7, 0xda752ead, 0x48d22d89};
  uint32_t message[]  = {0x29462882, 0x12caa2d5, 0xb80e1c66, 0x1006807f};
  uint32_t expected[] = {0x0ddc404d, 0x91600596, 0x7425a8d8, 0xa066ca56};
  uint8_t result;

  printf("Running TC5: 128 bit operands\n");

  result = testrunner(4, exponent, 4, modulus, message, expected);

  if (result)
    printf("TC5: OK\n");
  else
    printf("TC5: NOT OK\n");
}


//------------------------------------------------------------------
// tc6()
//
// e = 65537 and message, modulus are 64 bit oprerands.
//------------------------------------------------------------------
static void tc6()
{
  uint32_t message[] = {0x00000000, 0xdb5a7e09, 0x86b98bfb};

  uint32_t exponent[] = {0x00000000, 0x00000000, 0x00010001};

  uint32_t modulus[] = {0x00000000, 0xb3164743, 0xe1de267d};

  uint32_t expected[] = {0x00000000, 0x9fc7f328, 0x3ba0ae18};
  uint8_t result;

  printf("Running TC6: e=65537 and 64 bit operands\n");

  result = testrunner(3, exponent, 3, modulus, message, expected);

  if (result)
    printf("TC6: OK\n");
  else
    printf("TC6: NOT OK\n");
}


//------------------------------------------------------------------
// tc7()
//
// e = 65537 and message, modulus are 256 bit oprerands.
//------------------------------------------------------------------
static void tc7()
{
  uint32_t message[] = {0x00000000, 0xbd589a51, 0x2ba97013, 0xc4736649, 0xe233fd5c,
			0x39fcc5e5, 0x2d60b324, 0x1112f2d0, 0x1177c62b};

  uint32_t exponent[] = {0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
			 0x00000000, 0x00000000, 0x00000000, 0x00010001};

  uint32_t modulus[] = {0x00000000, 0xf169d36e, 0xbe2ce61d, 0xc2e87809, 0x4fed15c3,
			0x7c70eac5, 0xa123e643, 0x299b36d2, 0x788e583b};

  uint32_t expected[] = {0x00000000, 0x7c5f0fee, 0x73028fc5, 0xc4fe57c4, 0x91a6f5be,
			 0x33a5c174, 0x2d2c2bcd, 0xda80e7d6, 0xfb4c889f};
  uint8_t result;

  printf("Running TC7: e=65537 and 256 bit operands\n");

  result = testrunner(9, exponent, 9, modulus, message, expected);

  if (result)
    printf("TC7: OK\n");
  else
    printf("TC7: NOT OK\n");
}


//------------------------------------------------------------------
// tc8()
//
// Testcase with 2048 bit operands.
//------------------------------------------------------------------
static void tc8()
{
  uint32_t message[] = {0x00000000, 0xc1dded3d, 0x28434587, 0xcccdffa8,
                        0xc98a9a1c, 0x04a6eb9f, 0xcf672252, 0x3ca88273,
                        0x4fa3868a, 0xd2228ce5, 0x005f7876, 0x2abbc04b,
                        0x04d86c72, 0x8466923d, 0x41d7077b, 0x950250b9,
                        0xb0044ecd, 0x440bd649, 0x23a57ce7, 0xd5651065,
                        0xa7aab420, 0x4a6f7a81, 0x433c6761, 0xe5a44ca7,
                        0x903dfee9, 0xcf7946a7, 0x22914c75, 0xbd0204ab,
                        0x192f78ad, 0xd45811cd, 0xa1b58078, 0x3ed0a735,
                        0xd81e6402, 0x2faf947c, 0xe7b85734, 0x18ada37a,
                        0xd438e4ce, 0xb9e2a374, 0x88968bf2, 0xe2db443c,
                        0xa9e8bb02, 0x32bca770, 0xa2964ec0, 0x782d3bd5,
                        0x575dc836, 0xd57f2b1b, 0x444300b2, 0x07889868,
                        0xb6f174dc, 0x0663243e, 0x93c14967, 0x4696ffb1,
                        0xd7c9a423, 0x1168031b, 0x55577481, 0x91ed0cde,
                        0x5ba3fc60, 0x55845380, 0x21dc1d33, 0x2c5fa2e5,
                        0xbc12c97e, 0x4bcc04ea, 0x692a309d, 0x8e1c9e02,
                        0xaa1c0a3d };

  uint32_t exponent[] = {0x00000000, 0x19f18035, 0xcc60d544, 0x19d27c61,
                         0x8ed90eb3, 0x3690e87d, 0x773ca91e, 0xdade42b8,
                         0x0a3f677f, 0x7f0bf0c3, 0xad92b9fb, 0x52db2b4c,
                         0x8aa72367, 0x0a449805, 0x1b3b511c, 0x1d7e7d6b,
                         0x741a1b6a, 0x3d8800fe, 0x547dfdc2, 0xa802c31a,
                         0xfefb2a15, 0xce0ab737, 0x1fa90820, 0xdf80b4ea,
                         0x9ce78816, 0xb782861e, 0x7af81e25, 0x4343e5bf,
                         0xebe0b724, 0x6ece76ab, 0x01aa5089, 0xe4e21ba3,
                         0x248b6b0d, 0x1c091b64, 0x9c37f319, 0x22c25e57,
                         0x5a7448d1, 0x5a8300da, 0x1278cd36, 0x0cb4c6ac,
                         0x8deed224, 0xb7fdd7d0, 0x6326c04d, 0x539fff6f,
                         0x63778630, 0x85468bf5, 0x5a9c33f7, 0x160efc5c,
                         0xf8e4b6d1, 0x353bd641, 0x117508cc, 0xd1996bc5,
                         0x0a392c11, 0xb0e1ffe8, 0xe7b14a2e, 0x5013a5af,
                         0xbcce99d5, 0x8b93bd75, 0xa4e198d7, 0x4c18c142,
                         0xe51872d5, 0x7ef0cf34, 0x3ae53a47, 0xf5297694,
                         0xfd0c2275 };

  uint32_t modulus[] = {0x00000000, 0xd49c6a62, 0xae09979b, 0x5337cdad,
                        0xb457e3f7, 0x5550dd37, 0x05180d6d, 0xf5fbe3a5,
                        0xa108dbf3, 0x88629746, 0xca129de2, 0x8302471f,
                        0x15058a33, 0x97c1d786, 0xf87da044, 0x13acbbe8,
                        0x9dad545c, 0xdd778482, 0x24f3bf5b, 0x42473afd,
                        0x89b05301, 0x9299817b, 0xc1222669, 0x4ec4a193,
                        0x274889fa, 0xcd1bce7a, 0x41b5310d, 0xf86b14a4,
                        0x5673ea86, 0x521b8374, 0xd28da0ac, 0xc84464f1,
                        0x1ec80fe6, 0xe75ecc90, 0x6c34aee2, 0xa627e90f,
                        0xb7688407, 0x41833bdf, 0x411ab5da, 0x6759d67b,
                        0x182bc41a, 0x910dfa56, 0xf6e345de, 0xe1aae4d1,
                        0xa7c63ba1, 0xd65aa619, 0xd8b2c716, 0x483cdc54,
                        0x516ba960, 0xa221a1c4, 0xee39e3c3, 0x0d839205,
                        0xd6adba6a, 0xc8fa9741, 0x4434bab7, 0x0cb18c9c,
                        0x75c967d4, 0xb15febac, 0x7237454e, 0x72087e79,
                        0xd9e1acf1, 0xfc374a56, 0xa7741ed9, 0xc16ad5d8,
                        0x285d4f41 };

  uint32_t expected[] = {0x00000000, 0x0a311e48, 0x0d000a72, 0x1abe90c3,
                         0xfde69c22, 0xb68a5512, 0x9e0e3179, 0x9830556f,
                         0xb3012eaf, 0xc2e02fc5, 0x5dded2d0, 0xc5c7ad29,
                         0x9292ab12, 0x60393a6a, 0x81f2ce8a, 0xdffaf8e3,
                         0xc719e252, 0x5961a5fc, 0x6b29d3e5, 0x3421e018,
                         0xec174916, 0xa1ae3027, 0xf9bdec45, 0xe67ab6fa,
                         0x7ae109d1, 0xb840fc18, 0x1a8a17cc, 0xee81b969,
                         0x7bb5db8e, 0x5263943a, 0xa55ee6cd, 0x62c716f5,
                         0x830bfe99, 0x39f77d9d, 0x6684b8e4, 0xfae01bbd,
                         0xe04cb546, 0x7205a682, 0x7aba9d46, 0xd02a3970,
                         0x106d3dc0, 0x9ee094b5, 0xdc454b0b, 0x6661c887,
                         0x731569cb, 0xa37867cd, 0x3fe6992a, 0xed571459,
                         0x41585bf3, 0x8bc4979f, 0x1dc42dc1, 0xc44e2f03,
                         0xbd1e3599, 0xab66c76d, 0x0fac6628, 0x3eaef9fe,
                         0xaac66e77, 0x07ef4d15, 0x5f2bc8f1, 0xa8299364,
                         0xfea22998, 0xf55f7ee7, 0xdb61eef0, 0x898e8c64,
                         0xd5535329 };
  uint8_t result;

  printf("Running TC8: 2048 bit operands.s\n");

  result = testrunner(65, exponent, 65, modulus, message, expected);

  if (result)
    printf("TC8: OK\n");
  else
    printf("TC8: NOT OK\n");
}


//------------------------------------------------------------------
// tc9()
//
// Testcase with 2048 bit operands.
//------------------------------------------------------------------
static void tc9()
{
  uint32_t message[] = {0x21558179, 0x3e2914b1, 0xefe95957, 0x965fdead,
                        0xe766d8fc, 0x136eadf4, 0xa6106a2a, 0x88b2df7e,
                        0xe0b0eaae, 0x2c17946a, 0x6f5b5563, 0x228052ae,
                        0x7fc40d80, 0xf81354db, 0xfceecd1a, 0xa5e4c97d,
                        0x433ecfcd, 0xc20d1e4d, 0x2a748fe3, 0x1d9e63f0,
                        0xdc6c25d6, 0xdae5c8be, 0x1d8c5431, 0xb1d7d270,
                        0xed5b2566, 0x1463b0fd, 0xa9e26cf7, 0x3dd6fbd7,
                        0x1347c8f7, 0x76c2cc37, 0xf382b786, 0x1d5ac517,
                        0x26b96692, 0x2c1fe6f8, 0x5852dbf8, 0x4bcabda2,
                        0xbedb2f5f, 0xbfe58158, 0x8cd5d15f, 0xac7c7f4c,
                        0xf8ba47d2, 0x86c6571d, 0x06a4760b, 0xa6afa0e1,
                        0x7a819f62, 0x5cdbfe15, 0x9b2d10b5, 0xf508b1fd,
                        0xb3f0462a, 0x92f45a64, 0x69b6ec58, 0xbfad8fab,
                        0x6799260f, 0x27415db5, 0xf6ac7832, 0xe547826d,
                        0x6a9806a5, 0x36c62a88, 0x98bee14d, 0x9b8c2648,
                        0xabdbbd3d, 0xaf59eea1, 0x164eacb5, 0x3a18e427};


  uint32_t exponent[] = {0x2519837b, 0xe73a9031, 0xe241606d, 0x21e70fa2,
                         0x7881f254, 0x4e60831d, 0x266f408e, 0x4a83e6ed,
                         0xa7741995, 0x32b477ba, 0x91bdf5d0, 0x4acd7a06,
                         0x51e344b9, 0xdf376e4e, 0x8494e625, 0xa0cc9697,
                         0x817a0c93, 0x3b68cefb, 0x46de14c1, 0x52229965,
                         0x329645bd, 0xf4176adc, 0x29a8bc50, 0x44900fec,
                         0x1558d492, 0xf838a8e7, 0xea207abd, 0xcd21a28c,
                         0x91e6b02f, 0x2a490ea8, 0x5d99663b, 0x87c92fb6,
                         0x0a185325, 0x5256a7a3, 0x496b7288, 0x6688b6c8,
                         0x650e1776, 0x54cd429f, 0x90ea3b18, 0x0b72ae61,
                         0xcc8651b3, 0xa488742d, 0x93c401ef, 0x5a2220ff,
                         0xaee1f257, 0xf9d1e29a, 0xd47151fe, 0x4978342b,
                         0x0927048a, 0x404b0689, 0xdc9df8cc, 0xfba9845f,
                         0xeb8a39b0, 0xd3f24ae2, 0x5ea9ca0a, 0x0c064f94,
                         0x35368ae2, 0xeab6c035, 0x9baa39c6, 0x2ef6259d,
                         0xa2577555, 0x514c7d98, 0x0890d44f, 0xf416fbdd};


  uint32_t modulus[] = {0x2c5337a9, 0x3f2e1ca6, 0x91de65ea, 0xc3f9a3c2,
                        0xdc9099e0, 0x64ebe412, 0xf4583fae, 0x1fc8e8dd,
                        0x92dcbbfb, 0x9159239e, 0xdbbec456, 0x8735a660,
                        0x8248dbbc, 0x76f01415, 0x3cb8a897, 0x7cc09280,
                        0x6cc6db51, 0x9c2544da, 0x316564ce, 0x4b6d9b3b,
                        0x3e0e123f, 0x942a4a3c, 0x1f128873, 0x5ad14862,
                        0xdde8e6dd, 0x73da31fb, 0x1a8a2046, 0xc3ff18c6,
                        0x24e31d54, 0x7d8a1796, 0x88ab346c, 0x262bb321,
                        0x2cada5dc, 0x1fb2284c, 0x042375fd, 0xba10d309,
                        0xcda978ec, 0x229ee156, 0x8470728a, 0xa58017fd,
                        0x65727801, 0x1ea396a6, 0xbd9a4bc1, 0x8e97c08f,
                        0xd7529796, 0x2c8339e9, 0xc5340a83, 0x6f7d1f9c,
                        0xd6014fec, 0xdffa2265, 0xfa9906a9, 0xafbd424a,
                        0x631994ae, 0x73a9b3f1, 0x2284f999, 0x6f8c87f6,
                        0x93136a66, 0x47c81e45, 0xd35f0e41, 0x238d6960,
                        0x96cf337d, 0x8865e4cc, 0x15039c40, 0x65ee7211};


  uint32_t expected[] = {0x24665860, 0x4b150493, 0xc0834602, 0xc0b99ab5,
                         0xbe649545, 0xa7d8b1ca, 0x55c1b98a, 0x1dce374b,
                         0x65750415, 0x573dfed7, 0x95df9943, 0x58a4aea0,
                         0x5fb40a92, 0x1408d9c2, 0xb5e23fc9, 0x225eb60b,
                         0x41d33a41, 0xbf958f7f, 0x619f5ac1, 0x207647f3,
                         0x223e56f8, 0x26afd4ae, 0x6a297840, 0x830947db,
                         0xbc5af940, 0x4c97ebb1, 0xca38b220, 0x04c9a26d,
                         0x49a16b72, 0x0882c658, 0x2dbc50e0, 0x67e2d057,
                         0x4b8ef356, 0x4ba5eac3, 0x17237d9f, 0x27c111a8,
                         0xc1b1944e, 0xe91fd6b6, 0xa78d9747, 0x61e946d3,
                         0x0078fe23, 0x7770a088, 0x6d5762af, 0x435ac5f9,
                         0x36cde9d5, 0xc313804d, 0xa4623760, 0xb1c37572,
                         0x2b22486d, 0x8af131e3, 0x3e5fc3ea, 0x0d9c9ba0,
                         0x218bcc8f, 0x8bcdfea2, 0xcf55a599, 0x57b9fcbc,
                         0x5c087f62, 0xec130a15, 0x7e8bd1f5, 0x60eaaa51,
                         0x020dd89b, 0x890cc6ea, 0x042d0054, 0x74055863};

  uint8_t result;

  printf("Running TC9: 2048 bit operands.s\n");

  result = testrunner(64, exponent, 64, modulus, message, expected);

  if (result)
    printf("TC9: OK\n");
  else
    printf("TC9: NOT OK\n");
}


//------------------------------------------------------------------
// main()
//------------------------------------------------------------------
int main(void)
{
  check_modexp_access();
//  tc_set_debug(1);

//  check_modulus_mem();
//  check_exponent_mem();
//  check_message_mem();

  tc1();
  tc2();
  tc3();
  tc4();
  tc5();
  tc6();
  tc7();
  tc8();
  tc9();

  return 0;
}

//======================================================================
// EOF modexp_tester.c
//======================================================================
