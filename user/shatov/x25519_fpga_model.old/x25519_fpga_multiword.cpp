//------------------------------------------------------------------------------
//
// x25519_fpga_multiword.cpp
// -----------------------------
// Multi-precision FPGA routines
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
// Globals
//------------------------------------------------------------------------------
FPGA_BUFFER X25519_ZERO;
FPGA_BUFFER X25519_ONE;


//------------------------------------------------------------------------------
void fpga_multiword_init()
//------------------------------------------------------------------------------
{
	int w;	// word counter

		/* fill buffers for large multi-word integers */
	for (	w = FPGA_OPERAND_NUM_WORDS - 1;
			w >= 0;
			w -= 1)
	{
		X25519_ZERO.words[w]	= 0;			// all words are zero
		X25519_ONE.words[w]		= w ? 0 : 1;	// only the lowest word is 1
	}
}


//------------------------------------------------------------------------------
void fpga_multiword_copy(FPGA_BUFFER *src, FPGA_BUFFER *dst)
//------------------------------------------------------------------------------
//
// Copies large multi-word integer from src into dst.
//
//------------------------------------------------------------------------------
{
	int w;	// word counter

		// copy all the words from src into dst
	for (w=0; w<FPGA_OPERAND_NUM_WORDS; w++)
		dst->words[w] = src->words[w];
}


//------------------------------------------------------------------------------
// End-of-File
//------------------------------------------------------------------------------
