/*
 * sha3_driver_sample.c
 * -------------------------------------------
 * Demo program to test SHA-3 core in hardware
 *
 * Authors: Pavel Shatov
 * Copyright (c) 2017, NORDUnet A/S
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
		 * Note, that the test program needs a custom bitstream without
		 * the core selector, where the DUT is at offset 0.
		 */

		// stm32 headers
#include <string.h>
#include "stm-init.h"
#include "stm-led.h"
#include "stm-fmc.h"

		// locations of core registers
#define CORE_ADDR_NAME0						(0x00 << 2)
#define CORE_ADDR_NAME1						(0x01 << 2)
#define CORE_ADDR_VERSION					(0x02 << 2)
#define CORE_ADDR_CONTROL					(0x08 << 2)
#define CORE_ADDR_STATUS					(0x09 << 2)

		// control and status register bit maps
#define CORE_CONTROL_BIT_INIT		0x00000001
#define CORE_CONTROL_BIT_NEXT		0x00000002

#define CORE_STATUS_BIT_READY		0x00000001
#define CORE_STATUS_BIT_VALID		0x00000002

		// locations of banks (operand buffers)
#define CORE_ADDR_BANK_BLOCK		0x200
#define CORE_ADDR_BANK_STATE		0x300

		// sha-3 parameters
#define SHA3_STATE_BITS				1600
#define SHA3_STATE_BYTES			(SHA3_STATE_BITS / 8)
		
#define SHA3_PADDING_SUFFIX		0x06
#define SHA3_PADDING_FINAL		0x80
		
#define SHA3_224_BLOCK_BITS		1152
#define SHA3_256_BLOCK_BITS		1088
#define SHA3_384_BLOCK_BITS		 832
#define SHA3_512_BLOCK_BITS		 576

#define SHA3_224_OUTPUT_BITS	 224
#define SHA3_256_OUTPUT_BITS	 256
#define SHA3_384_OUTPUT_BITS	 384
#define SHA3_512_OUTPUT_BITS	 512

		/*
		 * test vectors - hashes of empty message
		 *
		 * https://en.wikipedia.org/wiki/SHA-3#Examples_of_SHA-3_variants
		 *
		 */
#define SHA3_224_HASH_EMPTY_MSG											\
	{0x6b, 0x4e, 0x03, 0x42, 0x36, 0x67, 0xdb, 0xb7,	\
	 0x3b, 0x6e, 0x15, 0x45, 0x4f, 0x0e, 0xb1, 0xab,	\
	 0xd4, 0x59, 0x7f, 0x9a, 0x1b, 0x07, 0x8e, 0x3f,	\
	 0x5b, 0x5a, 0x6b, 0xc7}

#define SHA3_256_HASH_EMPTY_MSG											\
	{0xa7, 0xff, 0xc6, 0xf8, 0xbf, 0x1e, 0xd7, 0x66,	\
	 0x51, 0xc1, 0x47, 0x56, 0xa0, 0x61, 0xd6, 0x62,	\
	 0xf5, 0x80,	0xff,0x4d, 0xe4, 0x3b, 0x49, 0xfa,	\
	 0x82, 0xd8, 0x0a, 0x4b, 0x80, 0xf8, 0x43, 0x4a}

#define SHA3_384_HASH_EMPTY_MSG											\
	{0x0c, 0x63, 0xa7, 0x5b, 0x84, 0x5e, 0x4f, 0x7d,	\
	 0x01, 0x10, 0x7d, 0x85, 0x2e, 0x4c, 0x24, 0x85,	\
	 0xc5, 0x1a, 0x50, 0xaa, 0xaa, 0x94, 0xfc, 0x61,	\
	 0x99, 0x5e, 0x71, 0xbb, 0xee, 0x98, 0x3a, 0x2a,	\
	 0xc3, 0x71, 0x38, 0x31, 0x26, 0x4a, 0xdb, 0x47,	\
	 0xfb, 0x6b, 0xd1, 0xe0, 0x58, 0xd5, 0xf0, 0x04}
	 
#define SHA3_512_HASH_EMPTY_MSG											\
	{0xa6, 0x9f, 0x73, 0xcc, 0xa2, 0x3a, 0x9a, 0xc5,	\
	 0xc8, 0xb5, 0x67, 0xdc, 0x18, 0x5a, 0x75, 0x6e,	\
	 0x97, 0xc9, 0x82, 0x16, 0x4f, 0xe2, 0x58, 0x59,	\
	 0xe0, 0xd1, 0xdc, 0xc1, 0x47, 0x5c, 0x80, 0xa6,	\
	 0x15, 0xb2, 0x12, 0x3a, 0xf1, 0xf5, 0xf9, 0x4c,	\
	 0x11, 0xe3, 0xe9, 0x40, 0x2c, 0x3a, 0xc5, 0x58,	\
	 0xf5, 0x00, 0x19, 0x9d, 0x95, 0xb6, 0xd3, 0xe3,	\
	 0x01, 0x75, 0x85, 0x86, 0x28, 0x1d, 0xcd, 0x26}
	 
	 /*
	  * test vectors - hashes of short message "abc"
	  *
	  * https://www.di-mgt.com.au/sha_testvectors.html
	  *
	  */
#define SHA3_224_HASH_SHORT_MSG											\
	{0xe6, 0x42, 0x82, 0x4c, 0x3f, 0x8c, 0xf2, 0x4a,	\
	 0xd0, 0x92, 0x34, 0xee, 0x7d, 0x3c, 0x76, 0x6f,	\
	 0xc9, 0xa3, 0xa5, 0x16, 0x8d, 0x0c, 0x94, 0xad,	\
	 0x73, 0xb4, 0x6f, 0xdf}
	
#define SHA3_256_HASH_SHORT_MSG											\
	{0x3a, 0x98, 0x5d, 0xa7, 0x4f, 0xe2, 0x25, 0xb2,	\
	 0x04, 0x5c, 0x17, 0x2d, 0x6b, 0xd3, 0x90, 0xbd,	\
	 0x85, 0x5f, 0x08, 0x6e, 0x3e, 0x9d, 0x52, 0x5b,	\
	 0x46, 0xbf, 0xe2, 0x45, 0x11, 0x43, 0x15, 0x32}
	 
#define SHA3_384_HASH_SHORT_MSG											\
	{0xec, 0x01, 0x49, 0x82, 0x88, 0x51, 0x6f, 0xc9,	\
	 0x26, 0x45, 0x9f, 0x58, 0xe2, 0xc6, 0xad, 0x8d,	\
	 0xf9, 0xb4, 0x73, 0xcb, 0x0f, 0xc0, 0x8c, 0x25,	\
	 0x96, 0xda, 0x7c, 0xf0, 0xe4, 0x9b, 0xe4, 0xb2,	\
	 0x98, 0xd8, 0x8c, 0xea, 0x92, 0x7a, 0xc7, 0xf5,	\
	 0x39, 0xf1, 0xed, 0xf2, 0x28, 0x37, 0x6d, 0x25}
	 
#define SHA3_512_HASH_SHORT_MSG											\
	{0xb7, 0x51, 0x85, 0x0b, 0x1a, 0x57, 0x16, 0x8a,	\
	 0x56, 0x93, 0xcd, 0x92, 0x4b, 0x6b, 0x09, 0x6e,	\
	 0x08, 0xf6, 0x21, 0x82, 0x74, 0x44, 0xf7, 0x0d,	\
	 0x88, 0x4f, 0x5d, 0x02, 0x40, 0xd2, 0x71, 0x2e,	\
	 0x10, 0xe1, 0x16, 0xe9, 0x19, 0x2a, 0xf3, 0xc9,	\
	 0x1a, 0x7e, 0xc5, 0x76, 0x47, 0xe3, 0x93, 0x40,	\
	 0x57, 0x34, 0x0b, 0x4c, 0xf4, 0x08, 0xd5, 0xa5,	\
	 0x65, 0x92, 0xf8, 0x27, 0x4e, 0xec, 0x53, 0xf0}

	 /*
	  * test vectors - hashes of long message (see below)
	  *
	  * https://csrc.nist.gov/Projects/Cryptographic-Standards-and-Guidelines/example-values
	  *
	  */
	 
#define SHA3_224_HASH_LONG_MSG											\
	{0x93, 0x76, 0x81, 0x6A, 0xBA, 0x50, 0x3F, 0x72,	\
	 0xF9, 0x6C, 0xE7, 0xEB, 0x65, 0xAC, 0x09, 0x5D,	\
	 0xEE, 0xE3, 0xBE, 0x4B, 0xF9, 0xBB, 0xC2, 0xA1,	\
	 0xCB, 0x7E, 0x11, 0xE0}
	 
#define SHA3_256_HASH_LONG_MSG											\
	{0x79, 0xF3, 0x8A, 0xDE, 0xC5, 0xC2, 0x03, 0x07,	\
	 0xA9, 0x8E, 0xF7, 0x6E, 0x83, 0x24, 0xAF, 0xBF,	\
	 0xD4, 0x6C, 0xFD, 0x81, 0xB2, 0x2E, 0x39, 0x73,	\
	 0xC6, 0x5F, 0xA1, 0xBD, 0x9D, 0xE3, 0x17, 0x87}
	 
#define SHA3_384_HASH_LONG_MSG											\
	{0x18, 0x81, 0xDE, 0x2C, 0xA7, 0xE4, 0x1E, 0xF9,	\
	 0x5D, 0xC4, 0x73, 0x2B, 0x8F, 0x5F, 0x00, 0x2B,	\
	 0x18, 0x9C, 0xC1, 0xE4, 0x2B, 0x74, 0x16, 0x8E,	\
	 0xD1, 0x73, 0x26, 0x49, 0xCE, 0x1D, 0xBC, 0xDD,	\
	 0x76, 0x19, 0x7A, 0x31, 0xFD, 0x55, 0xEE, 0x98,	\
	 0x9F, 0x2D, 0x70, 0x50, 0xDD, 0x47, 0x3E, 0x8F}
		
#define SHA3_512_HASH_LONG_MSG											\
	{0xE7, 0x6D, 0xFA, 0xD2, 0x20, 0x84, 0xA8, 0xB1,	\
	 0x46, 0x7F, 0xCF, 0x2F, 0xFA, 0x58, 0x36, 0x1B,	\
	 0xEC, 0x76, 0x28, 0xED, 0xF5, 0xF3, 0xFD, 0xC0,	\
	 0xE4, 0x80, 0x5D, 0xC4, 0x8C, 0xAE, 0xEC, 0xA8,	\
	 0x1B, 0x7C, 0x13, 0xC3, 0x0A, 0xDF, 0x52, 0xA3,	\
	 0x65, 0x95, 0x84, 0x73, 0x9A, 0x2D, 0xF4, 0x6B,	\
	 0xE5, 0x89, 0xC5, 0x1C, 0xA1, 0xA4, 0xA8, 0x41,	\
	 0x6D, 0xF6, 0x54, 0x5A, 0x1C, 0xE8, 0xBA, 0x00}
	 
static const uint8_t hash_224_empty_msg[SHA3_224_OUTPUT_BITS / 8] = SHA3_224_HASH_EMPTY_MSG;
static const uint8_t hash_256_empty_msg[SHA3_256_OUTPUT_BITS / 8] = SHA3_256_HASH_EMPTY_MSG;
static const uint8_t hash_384_empty_msg[SHA3_384_OUTPUT_BITS / 8] = SHA3_384_HASH_EMPTY_MSG;
static const uint8_t hash_512_empty_msg[SHA3_512_OUTPUT_BITS / 8] = SHA3_512_HASH_EMPTY_MSG;

static const uint8_t hash_224_short_msg[SHA3_224_OUTPUT_BITS / 8] = SHA3_224_HASH_SHORT_MSG;
static const uint8_t hash_256_short_msg[SHA3_256_OUTPUT_BITS / 8] = SHA3_256_HASH_SHORT_MSG;
static const uint8_t hash_384_short_msg[SHA3_384_OUTPUT_BITS / 8] = SHA3_384_HASH_SHORT_MSG;
static const uint8_t hash_512_short_msg[SHA3_512_OUTPUT_BITS / 8] = SHA3_512_HASH_SHORT_MSG;

static const uint8_t hash_224_long_msg[SHA3_224_OUTPUT_BITS / 8] = SHA3_224_HASH_LONG_MSG;
static const uint8_t hash_256_long_msg[SHA3_256_OUTPUT_BITS / 8] = SHA3_256_HASH_LONG_MSG;
static const uint8_t hash_384_long_msg[SHA3_384_OUTPUT_BITS / 8] = SHA3_384_HASH_LONG_MSG;
static const uint8_t hash_512_long_msg[SHA3_512_OUTPUT_BITS / 8] = SHA3_512_HASH_LONG_MSG;
	 
	 /* short message, will always fit in single block */
static const char msg_short[] = "abc";
	 
	 /* long message, guaranteed to _not_ fit in one block */
static const char msg_long[] =
	"\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3"
	"\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3"
	"\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3"
	"\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3"
	"\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3"
	"\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3"
	"\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3"
	"\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3"
	"\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3"
	"\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3\xA3";


		/*
		 * prototypes
		 */
void toggle_yellow_led(void);

int test_sha3(	const uint8_t		*msg,
								uint32_t 				 num_msg_bytes,
								uint32_t				 num_block_bits,
								const uint8_t		*hash,
								uint32_t				 num_hash_bits);

void sha3_absorb(	uint32_t	*state,
									uint32_t	 num_block_bytes,
									uint32_t	 block_number);

								
		/*
		 * test routine
		 */
int main()
{
		int ok;
	
    stm_init();
    fmc_init();
	
				// turn on the green led
    led_on(LED_GREEN);
    led_off(LED_RED);
    led_off(LED_YELLOW);
    led_off(LED_BLUE);

				// check, that core is present
		uint32_t core_name0;
		uint32_t core_name1;
		uint32_t core_version;
	
		fmc_read_32(CORE_ADDR_NAME0,   &core_name0);
		fmc_read_32(CORE_ADDR_NAME1,   &core_name1);
		fmc_read_32(CORE_ADDR_VERSION, &core_version);
			
				// must be "sha3", "    " [four spaces], "0.10"
		if (	(core_name0   != 0x73686133) ||
					(core_name1   != 0x20202020) ||
					(core_version != 0x302E3130))
		{
				led_off(LED_GREEN);
				led_on(LED_RED);
				while (1);
		}
				
			// repeat forever
		while (1)
		{			
						// fresh start
				ok = 1;

						// test with empty message
				ok = ok && test_sha3(NULL, 0, SHA3_224_BLOCK_BITS, hash_224_empty_msg, SHA3_224_OUTPUT_BITS);
				ok = ok && test_sha3(NULL, 0, SHA3_256_BLOCK_BITS, hash_256_empty_msg, SHA3_256_OUTPUT_BITS);
				ok = ok && test_sha3(NULL, 0, SHA3_384_BLOCK_BITS, hash_384_empty_msg, SHA3_384_OUTPUT_BITS);
				ok = ok && test_sha3(NULL, 0, SHA3_512_BLOCK_BITS, hash_512_empty_msg, SHA3_512_OUTPUT_BITS);
			
						// test with the short message ("abc")
				ok = ok && test_sha3((uint8_t *)msg_short, strlen(msg_short), SHA3_224_BLOCK_BITS, hash_224_short_msg, SHA3_224_OUTPUT_BITS);
				ok = ok && test_sha3((uint8_t *)msg_short, strlen(msg_short), SHA3_256_BLOCK_BITS, hash_256_short_msg, SHA3_256_OUTPUT_BITS);
				ok = ok && test_sha3((uint8_t *)msg_short, strlen(msg_short), SHA3_384_BLOCK_BITS, hash_384_short_msg, SHA3_384_OUTPUT_BITS);
				ok = ok && test_sha3((uint8_t *)msg_short, strlen(msg_short), SHA3_512_BLOCK_BITS, hash_512_short_msg, SHA3_512_OUTPUT_BITS);
			
						// test with the long message
				ok = ok && test_sha3((uint8_t *)msg_long, strlen(msg_long), SHA3_224_BLOCK_BITS, hash_224_long_msg, SHA3_224_OUTPUT_BITS);
				ok = ok && test_sha3((uint8_t *)msg_long, strlen(msg_long), SHA3_256_BLOCK_BITS, hash_256_long_msg, SHA3_256_OUTPUT_BITS);
				ok = ok && test_sha3((uint8_t *)msg_long, strlen(msg_long), SHA3_384_BLOCK_BITS, hash_384_long_msg, SHA3_384_OUTPUT_BITS);
				ok = ok && test_sha3((uint8_t *)msg_long, strlen(msg_long), SHA3_512_BLOCK_BITS, hash_512_long_msg, SHA3_512_OUTPUT_BITS);
			
						// turn on the red led to indicate something went wrong
				if (!ok)
				{		led_off(LED_GREEN);
						led_on(LED_RED);
				}
				
						// indicate, that we're alive doing something...
				toggle_yellow_led();
		}
}


int test_sha3(	const uint8_t		*msg,
								uint32_t 				 num_msg_bytes,
								uint32_t				 num_block_bits,
								const uint8_t		*hash,
								uint32_t				 num_hash_bits)
{
		/* calculate digest of 'msg' and compare it against known reference 'hash' */
	
			// counter
		uint32_t i;
	
			// buffer for input block, consists of 32-bit words to ease copying over FMC
		uint32_t block32[SHA3_STATE_BYTES / 4];
		
			// byte pointer, handy for storing one byte at a time
		uint8_t *block = (uint8_t *)&block32;
	
			// handy values
		uint32_t num_block_bytes = num_block_bits >> 3;	// /8
		uint32_t num_hash_bytes = num_hash_bits >> 3;		// /8

			// counters
		uint32_t block_number	= 0;	// number of blocks absorbed (we need this, because for the
																// very first block we toggle the 'init' control bit, for all the
																// subsequent blocks we toggle the 'next' bit)
	
		uint32_t block_offset = 0;	// current byte position in the input block (we need this to
																// apply padding properly)
		
			// first wipe entire input block...
		for (i=0; i<(SHA3_STATE_BYTES / sizeof(uint32_t)); i++)
			block32[i] = 0;
		
			// ...then absorb all the bytes...
		while (num_msg_bytes)
		{
					// store the next byte
				block[block_offset] = msg[0];
			
				msg++;						// advance pointer
				block_offset++;		// increment block offset
				num_msg_bytes--;	// reduce remaining byte count
			
					// check, whether we've already filled entire block
				if (block_offset == num_block_bytes)
				{
							// absorb part of message accumulated in block
						sha3_absorb(block32, num_block_bytes, block_number);
					
						block_number++;		// increment processed block count
						block_offset = 0;	// start filling a new block
				}
			
		}
		
			// ...and finally apply padding
		
			// Now do the required padding, block_offset points to the very first empty byte in block
			// (there should be at least 1 empty byte now, because we would have processed completely
			// filled block earlier while absorbing bytes).
		
			/* Padding involves three steps:
		   *
		   * 1. Add "011" bit string (0x06) to the message ("01" is SHA-3 domain suffix, "1" is actual padding)
		   * 2. Add zero or more "0" bits until the message is exactly 1 bit short of full block
		   * 3. Add final "1" bit (0x80) to make the message length a multiple of block size
		   *
		   */
		
			// add "011" part
		block[block_offset] = SHA3_PADDING_SUFFIX;
		
			// wipe all the remaining bytes in block (if there are any)
		while (block_offset < (num_block_bytes - 1))
		{
				block_offset++;
				block[block_offset] = 0x00;
		}
		
		
			// add the final "1" part. note, that we must use |=, not just =,
			// because we could have added no extra null bytes and state[block_offset]
			// might already contain the suffix, we should not overwrite it.
		block[block_offset] |= SHA3_PADDING_FINAL;
		
			// absorb the last block with padding
		sha3_absorb(block32, num_block_bytes, block_number);
		
			// read state from core...
		for (i=0; i<(num_hash_bytes / sizeof(uint32_t)); i++)
			fmc_read_32(CORE_ADDR_BANK_STATE + i * sizeof(uint32_t), block32 + i);
	
			// ...and now compare state to known good hash
		for (i=0; i<num_hash_bytes; i++)	
			if (block[i] != hash[i]) return 0;
		
			// everything went just fine
		return 1;
}


		//
		// absorb one block of data into the sponge
		//
void sha3_absorb(uint32_t *block, uint32_t num_block_bytes, uint32_t block_number)
{
		uint32_t i;						// word counter
		uint32_t ctrl, sts;		// control register, status register
	
			// copy 32-bit words from state into core's input block buffer
		for (i=0; i<(num_block_bytes / sizeof(uint32_t)); i++)
				fmc_write_32(CORE_ADDR_BANK_BLOCK + i * sizeof(uint32_t), block + i);
	
			// note, that the very first block needs special handling: 'init' bit copies
			// input block into core's state, 'next' bit xor's current core's state with input block
	
			// block has enough space for entire core state, lower words are filled with
			// message and upper words remain zeroes. When the very first block is absorbed
			// into the sponge, we need to initialize *all* the core's state bits, because the
			// upper part of core's state may contain leftovers from previously absorbed data.
	
			// for subsequent blocks we don't need to copy the upper null part of block into the input
			// bank, because we've already filled it with zeroes for the very first block
	
		if (block_number == 0)
		{
				for (; i<(SHA3_STATE_BYTES / sizeof(uint32_t)); i++)
					fmc_write_32(CORE_ADDR_BANK_BLOCK + i * sizeof(uint32_t), block + i);
		}
	
			// CONTROL = 0
		ctrl = 0;
		fmc_write_32(CORE_ADDR_CONTROL, &ctrl);
					
			// determine what control bit to set ('init' for the very first block,
			// 'next' for all the subsequent blocks)
		ctrl = (block_number > 0) ? CORE_CONTROL_BIT_NEXT : CORE_CONTROL_BIT_INIT;
		fmc_write_32(CORE_ADDR_CONTROL, &ctrl);
	
			// wait for 'valid' bit to be set
		sts = 0;
		while (!(sts & CORE_STATUS_BIT_VALID))
				fmc_read_32(CORE_ADDR_STATUS, &sts);
}


		//
		// toggle the yellow led to indicate that we're not stuck somewhere
		//
void toggle_yellow_led(void)
{
		static int led_state = 0;
	
		led_state = !led_state;
	
		if (led_state) led_on(LED_YELLOW);
		else           led_off(LED_YELLOW);
}


		//
		// end of file
		//
