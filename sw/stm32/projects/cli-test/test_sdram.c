/*
 * test_sdram.c
 * ------------
 * Test code for the 2x512 MBit SDRAM working memory.
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
#include "stm-led.h"
#include "stm-sdram.h"
#include "test_sdram.h"


uint32_t lfsr1;
uint32_t lfsr2;


int test_sdram_sequential(uint32_t *base_addr)
{
    // memory offset
    int offset;

    // readback value
    uint32_t sdram_readback;


    /* This test fills entire memory chip with some pseudo-random pattern
       starting from the very first cell and going in linear fashion. It then
       reads entire memory and compares read values with what was written. */


    // turn on yellow led to indicate, that we're writing
    led_on(LED_YELLOW);


    //
    // Note, that SDRAM_SIZE is in BYTES, and since we write using
    // 32-bit words, total number of words is SDRAM_SIZE / 4.
    //

    // fill entire memory with "random" values
    for (offset=0; offset<(SDRAM_SIZE >> 2); offset++)	{
	// generate next "random" value to write
	lfsr1 = lfsr_next_32(lfsr1);

	// write to memory
	base_addr[offset] = lfsr1;
    }


    // turn off yellow led to indicate, that we're going to read
    led_off(LED_YELLOW);


    // read entire memory and compare values
    for (offset=0; offset<(SDRAM_SIZE >> 2); offset++)	{
	// generate next "random" value (we use the second LFSR to catch up)
	lfsr2 = lfsr_next_32(lfsr2);

	// read from memory
	sdram_readback = base_addr[offset];

	// compare and abort test in case of mismatch
	if (sdram_readback != lfsr2) return 0;
    }

    // done
    return 1;
}


//-----------------------------------------------------------------------------
int test_sdram_random(uint32_t *base_addr)
//-----------------------------------------------------------------------------
{
    // cell counter, memory offset
    int counter, offset;

    // readback value
    uint32_t sdram_readback;


    /* This test fills entire memory chip with some pseudo-random pattern
       starting from the very first cell, but then jumping around in pseudo-
       random fashion to make sure, that SDRAM controller in STM32 handles
       bank, row and column switching correctly. It then reads entire memory
       and compares read values with what was written. */


    // turn on yellow led to indicate, that we're writing
    led_on(LED_YELLOW);


    //
    // Note, that SDRAM_SIZE is in BYTES, and since we write using
    // 32-bit words, total number of words is SDRAM_SIZE / 4.
    //

    // start with the first cell
    for (counter=0, offset=0; counter<(SDRAM_SIZE >> 2); counter++)	{
	// generate next "random" value to write
	lfsr1 = lfsr_next_32(lfsr1);

	// write to memory
	base_addr[offset] = lfsr1;

	// generate next "random" address

	//
	// Note, that for 64 MB memory with 32-bit data bus we need 24 bits
	// of address, so we use 24-bit LFSR here. Since LFSR has only 2^^24-1
	// states, i.e. all possible 24-bit values excluding 0, we have to
	// manually kick it into some arbitrary state during the first iteration.
	//

	offset = offset ? lfsr_next_24(offset) : 0x00DEC0DE;
    }


    // turn off yellow led to indicate, that we're going to read
    led_off(LED_YELLOW);


    // read entire memory and compare values
    for (counter=0, offset=0; counter<(SDRAM_SIZE >> 2); counter++) {
	// generate next "random" value (we use the second LFSR to catch up)
	lfsr2 = lfsr_next_32(lfsr2);

	// read from memory
	sdram_readback = base_addr[offset];

	// compare and abort test in case of mismatch
	if (sdram_readback != lfsr2) return 0;

	// generate next "random" address
	offset = offset ? lfsr_next_24(offset) : 0x00DEC0DE;
    }

    //
    // we should have walked exactly 2**24 iterations and returned
    // back to the arbitrary starting address...
    //

    if (offset != 0x00DEC0DE) return 0;


    // done
    return 1;
}


//-----------------------------------------------------------------------------
int test_sdrams_interleaved(uint32_t *base_addr1, uint32_t *base_addr2)
//-----------------------------------------------------------------------------
{
    // cell counter, memory offsets
    int counter, offset1, offset2;

    // readback value
    uint32_t sdram_readback;


    /* Basically this is the same as test_sdram_random() except that it
       tests both memory chips at the same time. */


    // turn on yellow led to indicate, that we're writing
    led_on(LED_YELLOW);


    //
    // Note, that SDRAM_SIZE is in BYTES, and since we write using
    // 32-bit words, total number of words is SDRAM_SIZE / 4.
    //

    // start with the first cell
    for (counter=0, offset1=0, offset2=0; counter<(SDRAM_SIZE >> 2); counter++)	{
	// generate next "random" value to write
	lfsr1 = lfsr_next_32(lfsr1);

	// write to memory
	base_addr1[offset1] = lfsr1;
	base_addr2[offset2] = lfsr1;

	// generate next "random" addresses (use different starting states!)

	offset1 = offset1 ? lfsr_next_24(offset1) : 0x00ABCDEF;
	offset2 = offset2 ? lfsr_next_24(offset2) : 0x00FEDCBA;
    }


    // turn off yellow led to indicate, that we're going to read
    led_off(LED_YELLOW);


    // read entire memory and compare values
    for (counter=0, offset1=0, offset2=0; counter<(SDRAM_SIZE >> 2); counter++)	{
	// generate next "random" value (we use the second LFSR to catch up)
	lfsr2 = lfsr_next_32(lfsr2);

	// read from the first memory and compare
	sdram_readback = base_addr1[offset1];
	if (sdram_readback != lfsr2) return 0;

	// read from the second memory and compare
	sdram_readback = base_addr2[offset2];
	if (sdram_readback != lfsr2) return 0;

	// generate next "random" addresses
	offset1 = offset1 ? lfsr_next_24(offset1) : 0x00ABCDEF;
	offset2 = offset2 ? lfsr_next_24(offset2) : 0x00FEDCBA;
    }

    //
    // we should have walked exactly 2**24 iterations and returned
    // back to the arbitrary starting address...
    //

    if (offset1 != 0x00ABCDEF) return 0;
    if (offset2 != 0x00FEDCBA) return 0;

    // done
    return 1;
}

uint32_t lfsr_next_32(uint32_t lfsr)
{
    uint32_t tap = 0;

    tap ^= (lfsr >> 31);
    tap ^= (lfsr >> 30);
    tap ^= (lfsr >> 29);
    tap ^= (lfsr >> 9);

    return (lfsr << 1) | (tap & 1);
}

uint32_t lfsr_next_24(uint32_t lfsr)
{
    unsigned int tap = 0;

    tap ^= (lfsr >> 23);
    tap ^= (lfsr >> 22);
    tap ^= (lfsr >> 21);
    tap ^= (lfsr >> 16);

    return ((lfsr << 1) | (tap & 1)) & 0x00FFFFFF;
}
