//------------------------------------------------------------------------------
//
// curve25519_fpga_microcode.h
// -------------------------------------
// Microcode Architecture for Curve25519
//
// Authors: Pavel Shatov
//
// Copyright (c) 2018 NORDUnet A/S
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
#include <stdlib.h>


//------------------------------------------------------------------------------
enum UOP_BANK
//------------------------------------------------------------------------------
{
	BANK_LO, BANK_HI
};


//--------------------------
enum CURVE25519_UOP_OPERAND
//--------------------------
{
	CONST_ZERO,
	CONST_ONE,

	INVERT_R1,
	INVERT_R2,

	INVERT_T_1,
	INVERT_T_10,
	INVERT_T_1001,
	INVERT_T_1011,

	INVERT_T_X5,
	INVERT_T_X10,
	INVERT_T_X20,
	INVERT_T_X40,
	INVERT_T_X50,
	INVERT_T_X100,

	CURVE25519_UOP_OPERAND_COUNT
};


//------------------------------------------------------------------------------
enum UOP_MODULUS
//------------------------------------------------------------------------------
{
	MOD_1P,
	MOD_2P
};


//------------------------------------------------------------------------------
enum UOP_MATH
//------------------------------------------------------------------------------
{
	ADD, SUB, MUL
};


//------------------------------------------------------------------------------
// Prototypes (Micro-Operations)
//------------------------------------------------------------------------------
void uop_move	(UOP_BANK src, int s_op1,
				 UOP_BANK dst, int d_op1,
				 FPGA_BUFFER *buf_lo, FPGA_BUFFER *buf_hi);

void uop_calc	(UOP_MATH math,
				 UOP_BANK src, int s_op1, int s_op2,
				 UOP_BANK dst, int d_op,
				 FPGA_BUFFER *buf_lo, FPGA_BUFFER *buf_hi,
				 UOP_MODULUS mod);

void uop_load	(const FPGA_BUFFER *mem, UOP_BANK dst, int d_op, FPGA_BUFFER *buf_lo, FPGA_BUFFER *buf_hi);
void uop_stor	(const FPGA_BUFFER *buf_lo, const FPGA_BUFFER *buf_hi, UOP_BANK src, int s_op, FPGA_BUFFER *mem);


//------------------------------------------------------------------------------
// Prototype (Macro-Operation)
//------------------------------------------------------------------------------
void	fpga_modular_inv_microcode	(FPGA_BUFFER *buf_lo, FPGA_BUFFER *buf_hi);


//------------------------------------------------------------------------------
// End-of-File
//------------------------------------------------------------------------------
