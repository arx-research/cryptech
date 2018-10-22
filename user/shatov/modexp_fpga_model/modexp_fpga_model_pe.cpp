//
// modexp_fpga_model_pe.cpp
// -----------------------------
// Low-level processing elements
//
// Authors: Pavel Shatov
// Copyright (c) 2017, NORDUnet A/S
// All rights reserved.
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


//----------------------------------------------------------------
// Headers
//----------------------------------------------------------------
#include "modexp_fpga_model.h"


//----------------------------------------------------------------
// Low-level 32-bit multiplier with pre-adder
//----------------------------------------------------------------
void pe_mul(	FPGA_WORD  a, FPGA_WORD  b,
				FPGA_WORD  t, FPGA_WORD  c_in,
				FPGA_WORD *p, FPGA_WORD *c_out)
//----------------------------------------------------------------
//
// {c_out, p} = a * b + t + c_in
//
// Note, that the output will never overflow, because 0xFFFFFFFF^2 +
// 2*0xFFFFFFFF is exactly 0xFFFFFFFFFFFFFFFF. Doh, we're lucky...
//
//----------------------------------------------------------------
{
	_WIDE_WORD s = (_WIDE_WORD)a * (_WIDE_WORD)b;				// get product from multiplier
	
	s += (_WIDE_WORD)t;											// trigger pre-adder
	s += (_WIDE_WORD)c_in;										// take carry into account
	
	*p = (FPGA_WORD)s;											// return the lower part of result
	*c_out = (FPGA_WORD)(s >> (CHAR_BIT * sizeof(FPGA_WORD)));	// return the higher part of result
}


//----------------------------------------------------------------
// Low-level 32-bit adder
//----------------------------------------------------------------
void pe_add(	FPGA_WORD  a,
				FPGA_WORD  b,
				FPGA_WORD  c_in,
				FPGA_WORD *s,
				FPGA_WORD *c_out)
//----------------------------------------------------------------
//
// {c_out, s} = a + b + c_in
//
//----------------------------------------------------------------
{
	_WIDE_WORD t = (_WIDE_WORD)a + (_WIDE_WORD)b;					// get sum from adder
	
	t += (_WIDE_WORD)(c_in & 1);									// take carry into account
	
	*s = (FPGA_WORD)t;												// return the lower part of resukt
	*c_out = (FPGA_WORD)(t >> (CHAR_BIT * sizeof(FPGA_WORD))) & 1;	// return the higher part of result

}


//----------------------------------------------------------------
// Low-level 32-bit subtractor
//----------------------------------------------------------------
void pe_sub(	FPGA_WORD  a,
				FPGA_WORD  b,
				FPGA_WORD  b_in,
				FPGA_WORD *d,
				FPGA_WORD *b_out)
//----------------------------------------------------------------
//
// {b_out, d} = a - b - b_in
//
//----------------------------------------------------------------
{
	_WIDE_WORD t = (_WIDE_WORD)a - (_WIDE_WORD)b;					// get difference from subtractor
	
	t -= (_WIDE_WORD)(b_in & 1);									// take borrow into account
	
	*d = (FPGA_WORD)t;												// return the lower part of result
	*b_out = (FPGA_WORD)(t >> (CHAR_BIT * sizeof(FPGA_WORD))) & 1;	// return the higher part of result
}


//----------------------------------------------------------------
// End of file
//----------------------------------------------------------------
