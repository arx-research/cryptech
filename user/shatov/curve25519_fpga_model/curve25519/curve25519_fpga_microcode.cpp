//------------------------------------------------------------------------------
//
// curve25519_fpga_microcode.cpp
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
#include "curve25519_fpga_model.h"


//------------------------------------------------------------------------------
// Macros
//------------------------------------------------------------------------------
#define uop_cycle(iters);		for (cyc_count=0; cyc_count<iters; cyc_count++) {
#define uop_repeat();			}
#define uop_calc_if_even(...)	if (!(cyc_count % 2)) uop_calc(__VA_ARGS__)
#define uop_calc_if_odd(...)	else uop_calc(__VA_ARGS__)

//------------------------------------------------------------------------------
void uop_move	(UOP_BANK src, int s_op,
				 UOP_BANK dst, int d_op,
				 FPGA_BUFFER *buf_lo, FPGA_BUFFER *buf_hi)
//------------------------------------------------------------------------------
{
	FPGA_BUFFER *s_ptr = NULL;
	FPGA_BUFFER *d_ptr = NULL;

	if (src == BANK_LO) s_ptr = &buf_lo[s_op];
	if (src == BANK_HI) s_ptr = &buf_hi[s_op];
	if (dst == BANK_LO) d_ptr = &buf_lo[d_op];
	if (dst == BANK_HI) d_ptr = &buf_hi[d_op];

	fpga_multiword_copy(s_ptr, d_ptr);
}


//------------------------------------------------------------------------------
void uop_calc	(UOP_MATH math,
				 UOP_BANK src, int s_op1, int s_op2,
				 UOP_BANK dst, int d_op,
				 FPGA_BUFFER *buf_lo, FPGA_BUFFER *buf_hi,
				 UOP_MODULUS mod)
//------------------------------------------------------------------------------
{
	FPGA_BUFFER *s_ptr1 = NULL;
	FPGA_BUFFER *s_ptr2 = NULL;
	FPGA_BUFFER *d_ptr = NULL;
	FPGA_BUFFER *n_ptr = NULL;

	if (src == BANK_LO)
	{	s_ptr1 = &buf_lo[s_op1];
		s_ptr2 = &buf_lo[s_op2];
	}
	if (src == BANK_HI)
	{	s_ptr1 = &buf_hi[s_op1];
		s_ptr2 = &buf_hi[s_op2];
	}
	if (dst == BANK_LO)
	{	d_ptr = &buf_lo[d_op];
	}
	if (dst == BANK_HI)
	{	d_ptr = &buf_hi[d_op];
	}

	if (mod == MOD_1P) n_ptr = &CURVE25519_1P;
	if (mod == MOD_2P) n_ptr = &CURVE25519_2P;

	if (math == ADD) fpga_modular_add(s_ptr1, s_ptr2, d_ptr, n_ptr);
	if (math == SUB) fpga_modular_sub(s_ptr1, s_ptr2, d_ptr, n_ptr);
	if (math == MUL) fpga_modular_mul(s_ptr1, s_ptr2, d_ptr, n_ptr);
}


//------------------------------------------------------------------------------
void uop_load(const FPGA_BUFFER *mem, UOP_BANK dst, int d_op, FPGA_BUFFER *buf_lo, FPGA_BUFFER *buf_hi)
//------------------------------------------------------------------------------
{
	FPGA_BUFFER *d_ptr = NULL;
	if (dst == BANK_LO) d_ptr = &buf_lo[d_op];
	if (dst == BANK_HI) d_ptr = &buf_hi[d_op];

	fpga_multiword_copy(mem, d_ptr);
}


//------------------------------------------------------------------------------
void uop_stor(const FPGA_BUFFER *buf_lo, const FPGA_BUFFER *buf_hi, UOP_BANK src, int s_op, FPGA_BUFFER *mem)
//------------------------------------------------------------------------------
{
	FPGA_BUFFER *s_ptr = NULL;
	if (src == BANK_LO)
	{	s_ptr = (FPGA_BUFFER *)&buf_lo[s_op];
	}
	if (src == BANK_HI)
	{	s_ptr = (FPGA_BUFFER *)&buf_hi[s_op];
	}

	fpga_multiword_copy(s_ptr, mem);
}


//------------------------------------------------------------------------------
void fpga_modular_inv_microcode(FPGA_BUFFER *buf_lo, FPGA_BUFFER *buf_hi)
//------------------------------------------------------------------------------
{
	int cyc_count;	// counters

	/* BEGIN_MICROCODE: DURING_INVERSION */

		// T_1
	uop_move(BANK_LO, INVERT_T_1, BANK_HI, INVERT_T_1, buf_lo, buf_hi);

		// T_10
	uop_calc(MUL, BANK_LO, INVERT_T_1, INVERT_T_1, BANK_HI, INVERT_T_10, buf_lo, buf_hi, MOD_2P);

		// T_1001
	uop_calc(MUL, BANK_HI, INVERT_T_10, INVERT_T_10, BANK_LO, INVERT_R1, buf_lo, buf_hi, MOD_2P);
	uop_calc(MUL, BANK_LO, INVERT_R1, INVERT_R1, BANK_HI, INVERT_R2, buf_lo, buf_hi, MOD_2P);
	uop_calc(MUL, BANK_HI, INVERT_R2, INVERT_T_1, BANK_LO, INVERT_T_1001, buf_lo, buf_hi, MOD_2P);

		// T_1011
	uop_move(BANK_HI, INVERT_T_10, BANK_LO, INVERT_T_10, buf_lo, buf_hi);
	uop_calc(MUL, BANK_LO, INVERT_T_1001, INVERT_T_10, BANK_HI, INVERT_T_1011, buf_lo, buf_hi, MOD_2P);

		// T_X5
	uop_calc(MUL, BANK_HI, INVERT_T_1011, INVERT_T_1011, BANK_LO, INVERT_R1, buf_lo, buf_hi, MOD_2P);
	uop_calc(MUL, BANK_LO, INVERT_R1, INVERT_T_1001, BANK_HI, INVERT_T_X5, buf_lo, buf_hi, MOD_2P);

		// T_X10
	uop_move(BANK_HI, INVERT_T_X5, BANK_LO, INVERT_R1, buf_lo, buf_hi);

	uop_cycle(4);
		uop_calc_if_even(MUL, BANK_LO, INVERT_R1, INVERT_R1, BANK_HI, INVERT_R2, buf_lo, buf_hi, MOD_2P);
		uop_calc_if_odd (MUL, BANK_HI, INVERT_R2, INVERT_R2, BANK_LO, INVERT_R1, buf_lo, buf_hi, MOD_2P);
	uop_repeat();

	uop_calc(MUL, BANK_LO, INVERT_R1, INVERT_R1, BANK_HI, INVERT_R2, buf_lo, buf_hi, MOD_2P);
	uop_calc(MUL, BANK_HI, INVERT_R2, INVERT_T_X5, BANK_LO, INVERT_T_X10, buf_lo, buf_hi, MOD_2P);

		// T_X20
	uop_move(BANK_LO, INVERT_T_X10, BANK_HI, INVERT_R1, buf_lo, buf_hi);
	uop_move(BANK_LO, INVERT_T_X10, BANK_HI, INVERT_T_X10, buf_lo, buf_hi);

	uop_cycle(10);
		uop_calc_if_even(MUL, BANK_HI, INVERT_R1, INVERT_R1, BANK_LO, INVERT_R2, buf_lo, buf_hi, MOD_2P);
		uop_calc_if_odd (MUL, BANK_LO, INVERT_R2, INVERT_R2, BANK_HI, INVERT_R1, buf_lo, buf_hi, MOD_2P);
	uop_repeat();

	uop_calc(MUL, BANK_HI, INVERT_R1, INVERT_T_X10, BANK_LO, INVERT_T_X20, buf_lo, buf_hi, MOD_2P);

		// T_X40
	uop_move(BANK_LO, INVERT_T_X20, BANK_HI, INVERT_R1, buf_lo, buf_hi);
	uop_move(BANK_LO, INVERT_T_X20, BANK_HI, INVERT_T_X20, buf_lo, buf_hi);

	uop_cycle(20);
		uop_calc_if_even(MUL, BANK_HI, INVERT_R1, INVERT_R1, BANK_LO, INVERT_R2, buf_lo, buf_hi, MOD_2P);
		uop_calc_if_odd(MUL, BANK_LO, INVERT_R2, INVERT_R2, BANK_HI, INVERT_R1, buf_lo, buf_hi, MOD_2P);
	uop_repeat();

	uop_calc(MUL, BANK_HI, INVERT_R1, INVERT_T_X20, BANK_LO, INVERT_T_X40, buf_lo, buf_hi, MOD_2P);

		// T_X50		
	uop_move(BANK_LO, INVERT_T_X40, BANK_HI, INVERT_R1, buf_lo, buf_hi);

	uop_cycle(10);
		uop_calc_if_even(MUL, BANK_HI, INVERT_R1, INVERT_R1, BANK_LO, INVERT_R2, buf_lo, buf_hi, MOD_2P);
		uop_calc_if_odd(MUL, BANK_LO, INVERT_R2, INVERT_R2, BANK_HI, INVERT_R1, buf_lo, buf_hi, MOD_2P);
	uop_repeat();

	uop_calc(MUL, BANK_HI, INVERT_R1, INVERT_T_X10, BANK_LO, INVERT_T_X50, buf_lo, buf_hi, MOD_2P);

		// T_X100
	uop_move(BANK_LO, INVERT_T_X50, BANK_HI, INVERT_R1, buf_lo, buf_hi);
	uop_move(BANK_LO, INVERT_T_X50, BANK_HI, INVERT_T_X50, buf_lo, buf_hi);

	uop_cycle(50);
		uop_calc_if_even(MUL, BANK_HI, INVERT_R1, INVERT_R1, BANK_LO, INVERT_R2, buf_lo, buf_hi, MOD_2P);
		uop_calc_if_odd(MUL, BANK_LO, INVERT_R2, INVERT_R2, BANK_HI, INVERT_R1, buf_lo, buf_hi, MOD_2P);
	uop_repeat();

	uop_calc(MUL, BANK_HI, INVERT_R1, INVERT_T_X50, BANK_LO, INVERT_T_X100, buf_lo, buf_hi, MOD_2P);

	uop_move(BANK_LO, INVERT_T_X100, BANK_HI, INVERT_R1, buf_lo, buf_hi);
	uop_move(BANK_LO, INVERT_T_X100, BANK_HI, INVERT_T_X100, buf_lo, buf_hi);

	uop_cycle(100);
		uop_calc_if_even(MUL, BANK_HI, INVERT_R1, INVERT_R1, BANK_LO, INVERT_R2, buf_lo, buf_hi, MOD_2P);
		uop_calc_if_odd(MUL, BANK_LO, INVERT_R2, INVERT_R2, BANK_HI, INVERT_R1, buf_lo, buf_hi, MOD_2P);
	uop_repeat();

	uop_calc(MUL, BANK_HI, INVERT_R1, INVERT_T_X100, BANK_LO, INVERT_R2, buf_lo, buf_hi, MOD_2P);

	uop_cycle(50);
		uop_calc_if_even(MUL, BANK_LO, INVERT_R2, INVERT_R2, BANK_HI, INVERT_R1, buf_lo, buf_hi, MOD_2P);
		uop_calc_if_odd(MUL, BANK_HI, INVERT_R1, INVERT_R1, BANK_LO, INVERT_R2, buf_lo, buf_hi, MOD_2P);
	uop_repeat();

	uop_calc(MUL, BANK_LO, INVERT_R2, INVERT_T_X50, BANK_HI, INVERT_R1, buf_lo, buf_hi, MOD_2P);
	
	uop_cycle(4);
		uop_calc_if_even(MUL, BANK_HI, INVERT_R1, INVERT_R1, BANK_LO, INVERT_R2, buf_lo, buf_hi, MOD_2P);
		uop_calc_if_odd (MUL, BANK_LO, INVERT_R2, INVERT_R2, BANK_HI, INVERT_R1, buf_lo, buf_hi, MOD_2P);
	uop_repeat();
	
	uop_calc(MUL, BANK_HI, INVERT_R1, INVERT_R1, BANK_LO, INVERT_R2, buf_lo, buf_hi, MOD_2P);

	uop_move(BANK_HI, INVERT_T_1011, BANK_LO, INVERT_T_1011, buf_lo, buf_hi);

	uop_calc(MUL, BANK_LO, INVERT_R2, INVERT_T_1011, BANK_HI, INVERT_R1, buf_lo, buf_hi, MOD_2P);

	/* END_MICROCODE */
}


//------------------------------------------------------------------------------
// End-of-File
//------------------------------------------------------------------------------
