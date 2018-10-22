//------------------------------------------------------------------------------
//
// curve25519_fpga_model.cpp
// -------------------------
// Curve25519 FPGA Model 
//
// Authors: Pavel Shatov
//
// Copyright (c) 2018, NORDUnet A/S
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
#include "curve25519_fpga_model.h"


//------------------------------------------------------------------------------
bool compare_fpga_buffers(const FPGA_BUFFER *ax, const FPGA_BUFFER *bx)
//------------------------------------------------------------------------------
//
// Compare affine coordinates of two points and return true when they match.
//
//------------------------------------------------------------------------------
{
	int w;	// word counter

		// print all the values
	print_fpga_buffer("  Expected:   X = ", ax);
	print_fpga_buffer("  Calculated: X = ", bx);

		// compare values
	for (w=0; w<FPGA_OPERAND_NUM_WORDS; w++)
	{
			// compare x
		if (ax->words[w] != bx->words[w]) return false;
	}

		// values are the same
	return true;
}


//------------------------------------------------------------------------------
void print_fpga_buffer(const char *s, const FPGA_BUFFER *buf)
//------------------------------------------------------------------------------
//
// Pretty print large multi-word integer.
//
//------------------------------------------------------------------------------
{
	int w;	// word counter

		// print header
	printf("%s", s);

		// print all bytes
	for (w=FPGA_OPERAND_NUM_WORDS; w>0; w--)
	{	
		printf("%08x", buf->words[w-1]);

			// insert space after every group of 4 bytes
		if (w > 1) printf(" ");
	}

		// print footer
	printf("\n");
}


//------------------------------------------------------------------------------
// End-of-File
//------------------------------------------------------------------------------
