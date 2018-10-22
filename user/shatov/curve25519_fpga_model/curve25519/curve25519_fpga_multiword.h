//------------------------------------------------------------------------------
//
// curve25519_fpga_multiword.h
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
// Curve25519 Parameters
//------------------------------------------------------------------------------
#define CURVE25519_OPERAND_WIDTH	256


//------------------------------------------------------------------------------
// FPGA Pipeline Settings
//------------------------------------------------------------------------------
#define FPGA_OPERAND_NUM_WORDS		(CURVE25519_OPERAND_WIDTH / FPGA_WORD_WIDTH)


//------------------------------------------------------------------------------
// Operand Data Type
//------------------------------------------------------------------------------
typedef struct FPGA_BUFFER
{
	FPGA_WORD words[FPGA_OPERAND_NUM_WORDS];
}
FPGA_BUFFER;


//------------------------------------------------------------------------------
// Globals
//------------------------------------------------------------------------------
extern FPGA_BUFFER CURVE25519_ZERO;
extern FPGA_BUFFER CURVE25519_ONE;


//------------------------------------------------------------------------------
// Prototypes
//------------------------------------------------------------------------------
void	fpga_multiword_init		();
void	fpga_multiword_copy		(const FPGA_BUFFER *src, FPGA_BUFFER *dst);


//------------------------------------------------------------------------------
// End-of-File
//------------------------------------------------------------------------------
