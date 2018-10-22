//------------------------------------------------------------------------------
//
// fpga_util.cpp
// ---------------------
// Utility FPGA routines
//
// Authors: Pavel Shatov
//
// Copyright (c) 2015-2016, 2018 NORDUnet A/S
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
#include "ecdh_fpga_model.h"
#include "fpga_lowlevel.h"
#include "fpga_util.h"


//------------------------------------------------------------------------------
bool fpga_buffer_is_zero(FPGA_BUFFER *x)
//------------------------------------------------------------------------------
//
// Returns true when large multi-word integer x is zero.
//
//------------------------------------------------------------------------------
{
	int w;	// word counter

		// try to find non-zero words
	for (w=0; w<OPERAND_NUM_WORDS; w++)
		if (x->words[w]) return false;

		// no non-zero words found
	return true;
}


//------------------------------------------------------------------------------
void fpga_buffer_copy(FPGA_BUFFER *src, FPGA_BUFFER *dst)
//------------------------------------------------------------------------------
//
// Copies large multi-word integer from src into dst.
//
//------------------------------------------------------------------------------
{
	int w;	// word counter

		// copy all the words from src into dst
	for (w=0; w<OPERAND_NUM_WORDS; w++)
		dst->words[w] = src->words[w];
}


//------------------------------------------------------------------------------
// End-of-File
//------------------------------------------------------------------------------
