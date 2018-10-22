//
// modexp_fpga_model.h
// -----------------------------------------------
// Model of fast modular exponentiation on an FPGA
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
#include <stdint.h>		// uintNN_t
#include <stddef.h>		// size_t
#include <limits.h>		// CHAR_BIT


//----------------------------------------------------------------
// Data types
//----------------------------------------------------------------
typedef uint32_t  FPGA_WORD;		// FPGA data bus width
typedef uint64_t _WIDE_WORD;		// only used internally to mimic DSP slice operation


//----------------------------------------------------------------
// Model settings
//----------------------------------------------------------------
#define MAX_OPERAND_WIDTH	512		// largest supported operand width in bits
#define SYSTOLIC_WIDTH		128		// width of systolic array in bits


//----------------------------------------------------------------
// Power Consumption Masking Constant
//----------------------------------------------------------------
#define POWER_MASK	0x5A5A5A5A


//----------------------------------------------------------------
// Handy values
//----------------------------------------------------------------

	// largest possible number of 32-bit words in an operand
#define MAX_OPERAND_WORDS	(MAX_OPERAND_WIDTH / (CHAR_BIT * sizeof(FPGA_WORD)))

	// number of words systolic array processes at once
#define SYSTOLIC_NUM_WORDS		(SYSTOLIC_WIDTH    / (CHAR_BIT * sizeof(FPGA_WORD)))

	// largest possible number of consequtive systolic cycles
	// that are necessary to process entire operand
#define MAX_SYSTOLIC_CYCLES		(MAX_OPERAND_WIDTH / SYSTOLIC_WIDTH)


//----------------------------------------------------------------
// End of file
//----------------------------------------------------------------
