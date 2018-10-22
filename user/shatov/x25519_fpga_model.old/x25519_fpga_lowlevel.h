//------------------------------------------------------------------------------
//
// x25519_fpga_lowlevel.h
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
// FPGA Pipeline Settings
//------------------------------------------------------------------------------
#define FPGA_WORD_WIDTH				(32)


//------------------------------------------------------------------------------
// Word Types (Normal 32-bit, Extended 47-bit, Reduced 16-bit)
//------------------------------------------------------------------------------
typedef uint32_t FPGA_WORD;
typedef uint64_t FPGA_WORD_EXTENDED;
typedef uint16_t FPGA_WORD_REDUCED;


//------------------------------------------------------------------------------
// Wide Adder Mask
//------------------------------------------------------------------------------
#define FPGA_MASK_ADDER47	((FPGA_WORD_EXTENDED)0x00007FFFFFFFFFFF)


//------------------------------------------------------------------------------
// Prototypes
//------------------------------------------------------------------------------
void	fpga_lowlevel_add32		(FPGA_WORD x, FPGA_WORD y, bool c_in, FPGA_WORD *s, bool *c_out);
void	fpga_lowlevel_sub32		(FPGA_WORD x, FPGA_WORD y, bool b_in, FPGA_WORD *d, bool *b_out);

void	fpga_lowlevel_mul16		(FPGA_WORD_REDUCED x, FPGA_WORD_REDUCED y, FPGA_WORD *p);

void	fpga_lowlevel_add47		(FPGA_WORD_EXTENDED x, FPGA_WORD_EXTENDED y, FPGA_WORD_EXTENDED *s);


//------------------------------------------------------------------------------
// End-of-File
//------------------------------------------------------------------------------
