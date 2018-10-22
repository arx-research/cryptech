//------------------------------------------------------------------------------
//
// x25519_fpga_lowlevel.cpp
// -----------------------------------
// Models of low-level FPGA primitives
//
// Authors: Pavel Shatov
//
// Copyright (c) 2015-2016, 2018, NORDUnet A/S
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// - Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// - Neither the name of the NORDUnet nor the names of its contributors may be
//   used to endorse or promote products derived from this software without
//   specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Headers
//------------------------------------------------------------------------------
#include <stdint.h>
#include "x25519_fpga_model.h"


//------------------------------------------------------------------------------
//
// Low-level 32-bit adder with carry input and carry output.
//
// Carries are 1 bit wide.
//
// {c_out, s} = x + y + c_in
//
//------------------------------------------------------------------------------
void fpga_lowlevel_add32(FPGA_WORD x, FPGA_WORD y, bool c_in, FPGA_WORD *s, bool *c_out)
//------------------------------------------------------------------------------
{
		// calculate sum
	FPGA_WORD_EXTENDED r = (FPGA_WORD_EXTENDED)x + (FPGA_WORD_EXTENDED)y;

		// process carry input
	if (c_in) r++;

		// store sum
	*s = (FPGA_WORD)r;

		// shift sum to obtain carry flag
	r >>= FPGA_WORD_WIDTH;

		// store carry
	*c_out = (r != 0);
}


//------------------------------------------------------------------------------
//
// Low-level 32-bit subtractor with borrow input and borrow output.
//
// Borrows are 1 bit wide.
//
// {b_out, d} = x - y - b_in
//
void fpga_lowlevel_sub32(FPGA_WORD x, FPGA_WORD y, bool b_in, FPGA_WORD *d, bool *b_out)
//------------------------------------------------------------------------------
{
		// calculate difference
	FPGA_WORD_EXTENDED r = (FPGA_WORD_EXTENDED)x - (FPGA_WORD_EXTENDED)y;

		// process borrow input
	if (b_in) r--;

		// store difference
	*d = (FPGA_WORD)r;

		// shift difference to obtain borrow flag
	r >>= FPGA_WORD_WIDTH;

		// store borrow
	*b_out = (r != 0);
}


//------------------------------------------------------------------------------
//
// Low-level 16x16-bit multiplier.
//
// Inputs are 16 bits wide, output is 32 bits wide.
//
// p = x * y
//
void fpga_lowlevel_mul16(FPGA_WORD_REDUCED x, FPGA_WORD_REDUCED y, FPGA_WORD *p)
//------------------------------------------------------------------------------
{
		// multiply and store result
	*p = (FPGA_WORD)x * (FPGA_WORD)y;
}


//------------------------------------------------------------------------------
//
// Low-level wide 47-bit adder without carry logic.
//
// s = (x + y)[47:0]
//
void fpga_lowlevel_add47(FPGA_WORD_EXTENDED x, FPGA_WORD_EXTENDED y, FPGA_WORD_EXTENDED *s)
//------------------------------------------------------------------------------
{
		// add and store result truncated to 47 bits
	*s = (x + y) & FPGA_MASK_ADDER47;
}


//------------------------------------------------------------------------------
// End-of-File
//------------------------------------------------------------------------------
