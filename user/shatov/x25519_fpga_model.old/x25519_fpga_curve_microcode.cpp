//------------------------------------------------------------------------------
//
// x25519_fpga_curve_microcode.cpp
// ---------------------------------------------------
// Elliptic curve arithmetic procedures for Curve25519
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
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "x25519_fpga_model.h"


//------------------------------------------------------------------------------
// Error Handler
//------------------------------------------------------------------------------
#define uop_fatal(msg)	{(void)printf("%s\n",msg);exit(EXIT_FAILURE);}


//------------------------------------------------------------------------------
// Storage Buffers
//------------------------------------------------------------------------------
static FPGA_BUFFER BUF_LO[64];
static FPGA_BUFFER BUF_HI[64];

static bool buf_flag_lo[64];
static bool buf_flag_hi[64];


//------------------------------------------------------------------------------
enum UOP_BANK
//------------------------------------------------------------------------------
{
	BANK_LO, BANK_HI
};


//------------------------------------------------------------------------------
enum UOP_OPERAND
//------------------------------------------------------------------------------
{
	CONST_ZERO		=  0,
	CONST_ONE		=  1,
	CONST_A24		=  2,

	LADDER_R0_X		=  3,
	LADDER_R0_Z		=  4,

	LADDER_R1_X		=  5,
	LADDER_R1_Z		=  6,

	LADDER_T0_X		=  7,
	LADDER_T0_Z		=  8,

	LADDER_T1_X		=  9,
	LADDER_T1_Z		= 10,

	LADDER_S0		= 11,
	LADDER_S1		= 12,

	LADDER_D0		= 13,
	LADDER_D1		= 14,

	LADDER_QS0		= 15,
	LADDER_QD0		= 16,

	LADDER_S0D1		= 17,
	LADDER_S1D0		= 18,

	LADDER_TS		= 19,
	LADDER_TD		= 20,

	LADDER_QTD		= 21,

	LADDER_T0		= 22,
	LADDER_TA		= 23,
	LADDER_T1		= 24,

	LADDER_P_X		= 25,

	LADDER_DUMMY	= 26,

	REDUCE_R1		= 27,
	REDUCE_R2		= 28,

	REDUCE_T_1		= 29,
	REDUCE_T_10		= 30,
	REDUCE_T_1001	= 31,
	REDUCE_T_1011	= 32,

	REDUCE_T_X5		= 33,
	REDUCE_T_X10	= 34,
	REDUCE_T_X20	= 35,
	REDUCE_T_X40	= 36,
	REDUCE_T_X50	= 37,
	REDUCE_T_X100	= 38
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
// Prototypes
//------------------------------------------------------------------------------
static void uop_move	(UOP_BANK src, UOP_OPERAND s_op1, UOP_OPERAND s_op2,
						 UOP_BANK dst, UOP_OPERAND d_op1, UOP_OPERAND d_op2);

static void uop_calc	(UOP_MATH math,
						 UOP_BANK src, UOP_OPERAND s_op1, UOP_OPERAND s_op2,
						 UOP_BANK dst, UOP_OPERAND d_op,
						 UOP_MODULUS mod);

static void uop_load	(FPGA_BUFFER *mem, UOP_BANK dst, UOP_OPERAND d_op);
static void uop_stor	(UOP_BANK src, UOP_OPERAND s_op, FPGA_BUFFER *mem);


//------------------------------------------------------------------------------
//
// Elliptic curve point scalar multiplication routine.
//
// This uses the Montgomery ladder to do the multiplication and then
// converts the result to affine coordinates.
//
// The algorithm is based on Algorithm 3 from "How to (pre-)compute a ladder"
// https://eprint.iacr.org/2017/264.pdf
//
//------------------------------------------------------------------------------
void fpga_curve_scalar_multiply_microcode(FPGA_BUFFER *PX, FPGA_BUFFER *K, FPGA_BUFFER *QX)
//------------------------------------------------------------------------------
{
	bool k_bit, s;							// 1-bit values
	FPGA_WORD k_word;						// current word of multiplier
	int word_count, bit_count, cyc_count;	// counters

		// reset bank flags
	(void)memset(buf_flag_lo, 0, sizeof buf_flag_lo);
	(void)memset(buf_flag_hi, 0, sizeof buf_flag_hi);

		// initialize internal banks
	fpga_multiword_copy(&X25519_ZERO, &BUF_LO[CONST_ZERO]);
	fpga_multiword_copy(&X25519_ZERO, &BUF_HI[CONST_ZERO]);

	fpga_multiword_copy(&X25519_ONE, &BUF_LO[CONST_ONE]);
	fpga_multiword_copy(&X25519_ONE, &BUF_HI[CONST_ONE]);

	fpga_multiword_copy(&X25519_A24, &BUF_LO[CONST_A24]);
	fpga_multiword_copy(&X25519_A24, &BUF_HI[CONST_A24]);

	buf_flag_lo[CONST_ZERO] = true;
	buf_flag_hi[CONST_ZERO] = true;
	buf_flag_lo[CONST_ONE] = true;
	buf_flag_hi[CONST_ONE] = true;
	buf_flag_lo[CONST_A24] = true;
	buf_flag_hi[CONST_A24] = true;

		// initialization
	uop_load(PX, BANK_HI, LADDER_P_X);
	uop_move(BANK_HI, CONST_ONE,   CONST_ZERO, BANK_LO, LADDER_R0_X, LADDER_R0_Z);
	uop_move(BANK_HI, LADDER_P_X, CONST_ONE,  BANK_LO, LADDER_R1_X, LADDER_R1_Z);

		// ladder
	s = false;
	for (word_count=FPGA_OPERAND_NUM_WORDS; word_count>0; word_count--)
	{
		for (bit_count=FPGA_WORD_WIDTH; bit_count>0; bit_count--)
		{
			k_word = K->words[word_count - 1] >> (bit_count - 1);	// current word
			k_bit = (k_word & (FPGA_WORD)1) == 1;					// current bit

				// inputs are all in LO: R0_X, R0_Z, R1_X, R1_Z

				// swap if needed
			if (s == k_bit)
			{	uop_move(BANK_LO, LADDER_R0_X, LADDER_R0_Z, BANK_HI, LADDER_T0_X, LADDER_T0_Z);	// HI: T0_X, T0_Z = LO: R0_X, R0_Z
				uop_move(BANK_LO, LADDER_R1_X, LADDER_R1_Z, BANK_HI, LADDER_T1_X, LADDER_T1_Z);	// HI: T1_X, T1_Z = LO: R1_X, R1_Z
			}
			else
			{	uop_move(BANK_LO, LADDER_R1_X, LADDER_R1_Z, BANK_HI, LADDER_T0_X, LADDER_T0_Z);	// HI: T0_X, T0_Z = LO: R1_X, R1_Z
				uop_move(BANK_LO, LADDER_R0_X, LADDER_R0_Z, BANK_HI, LADDER_T1_X, LADDER_T1_Z);	// HI: T1_X, T1_Z = LO: R0_X, R0_Z
			}

				// remember whether we actually did the swap
			s = k_bit;

				// run step
			uop_calc(ADD, BANK_HI,  LADDER_T0_X, LADDER_T0_Z, BANK_LO, LADDER_S0, MOD_2P);	// LO: S0 = HI: T0_X + T0_Z
			uop_calc(ADD, BANK_HI,  LADDER_T1_X, LADDER_T1_Z, BANK_LO, LADDER_S1, MOD_2P);	// LO: S1 = HI: T1_X + T1_Z
			uop_calc(SUB, BANK_HI,  LADDER_T0_X, LADDER_T0_Z, BANK_LO, LADDER_D0, MOD_2P);	// LO: D0 = HI: T0_X - T0_Z
			uop_calc(SUB, BANK_HI,  LADDER_T1_X, LADDER_T1_Z, BANK_LO, LADDER_D1, MOD_2P);	// LO: D1 = HI: T1_X - T1_Z

			uop_calc(MUL, BANK_LO,  LADDER_S0,   LADDER_S0,   BANK_HI, LADDER_QS0, MOD_2P);				// HI: QS0  = LO: S0 * S0
			uop_calc(MUL, BANK_LO,  LADDER_D0,   LADDER_D0,   BANK_HI, LADDER_QD0, MOD_2P);				// HI: QD0  = LO: D0 * D0
			uop_calc(MUL, BANK_LO,  LADDER_S0,   LADDER_D1,   BANK_HI, LADDER_S0D1, MOD_2P);				// HI: S0D1 = LO: S0 * D1
			uop_calc(MUL, BANK_LO,  LADDER_S1,   LADDER_D0,   BANK_HI, LADDER_S1D0, MOD_2P);				// HI: S1D0 = LO: S1 * D0

			uop_calc(ADD, BANK_HI,  LADDER_S1D0, LADDER_S0D1, BANK_LO, LADDER_TS, MOD_2P);	// LO: TS = HI: S1D0 + S0D1
			uop_calc(SUB, BANK_HI,  LADDER_S1D0, LADDER_S0D1, BANK_LO, LADDER_TD, MOD_2P);	// LO: TD = HI: S1D0 - S0D1

			uop_calc(MUL, BANK_LO,  LADDER_TD,   LADDER_TD,   BANK_HI, LADDER_QTD, MOD_2P);				// HI: QTD = LO: TD * TD

			uop_calc(SUB, BANK_HI,  LADDER_QS0,  LADDER_QD0,  BANK_LO, LADDER_T0, MOD_2P);	// LO: T0 = HI: QS0 - QD0
			uop_calc(MUL, BANK_LO,  LADDER_T0,   CONST_A24,   BANK_HI, LADDER_TA, MOD_2P);				// HI: TA = LO: T0 * A24
			uop_calc(ADD, BANK_HI,  LADDER_TA,   LADDER_QD0,  BANK_LO, LADDER_T1, MOD_2P);	// LO: T1 = HI: TA * QD0
			
			uop_calc(MUL, BANK_HI,  LADDER_QS0,  LADDER_QD0,  BANK_LO, LADDER_R0_X, MOD_2P);				// LO: R0_X = HI: QS0 * QD0
			uop_calc(MUL, BANK_LO,  LADDER_T0,   LADDER_T1,   BANK_HI, LADDER_R0_Z, MOD_2P);				// HI: R0_Z = LO: T0 * T1
			uop_calc(MUL, BANK_LO,  LADDER_TS,   LADDER_TS,   BANK_HI, LADDER_R1_X, MOD_2P);				// HI: R1_X = LO: TS * TS
			uop_calc(MUL, BANK_HI,  LADDER_P_X,  LADDER_QTD,  BANK_LO, LADDER_R1_Z, MOD_2P);				// LO: R1_Z = HI: PX * QTD

			uop_move(BANK_HI, LADDER_R0_Z, LADDER_R1_X, BANK_LO, LADDER_R0_Z, LADDER_R1_X);	// LO: R0_Z, R1_X = HI: R0_Z, R1_X
		}
	}
	
		// T_1
	uop_move(BANK_HI, LADDER_R0_Z, LADDER_R0_Z, BANK_LO, REDUCE_T_1, REDUCE_T_1);
	uop_move(BANK_LO, REDUCE_T_1, REDUCE_T_1, BANK_HI, REDUCE_T_1, REDUCE_T_1);

		// T_10
	uop_calc(MUL, BANK_LO, REDUCE_T_1, REDUCE_T_1, BANK_HI, REDUCE_T_10, MOD_2P);

		// T_1001
	uop_calc(MUL, BANK_HI, REDUCE_T_10, REDUCE_T_10, BANK_LO, REDUCE_R1, MOD_2P);
	uop_calc(MUL, BANK_LO, REDUCE_R1, REDUCE_R1, BANK_HI, REDUCE_R2, MOD_2P);
	uop_calc(MUL, BANK_HI, REDUCE_R2, REDUCE_T_1, BANK_LO, REDUCE_T_1001, MOD_2P);

		// T_1011
	uop_move(BANK_HI, REDUCE_T_10, REDUCE_T_10, BANK_LO, REDUCE_T_10, REDUCE_T_10);
	uop_calc(MUL, BANK_LO, REDUCE_T_1001, REDUCE_T_10, BANK_HI, REDUCE_T_1011, MOD_2P);

		// T_X5
	uop_calc(MUL, BANK_HI, REDUCE_T_1011, REDUCE_T_1011, BANK_LO, REDUCE_R1, MOD_2P);
	uop_calc(MUL, BANK_LO, REDUCE_R1, REDUCE_T_1001, BANK_HI, REDUCE_T_X5, MOD_2P);

		// T_X10
	uop_move(BANK_HI, REDUCE_T_X5, REDUCE_T_X5, BANK_LO, REDUCE_R1, REDUCE_R1);

	for (cyc_count=0; cyc_count<4; cyc_count++)
		if (!(cyc_count % 2))	uop_calc(MUL, BANK_LO, REDUCE_R1, REDUCE_R1, BANK_HI, REDUCE_R2, MOD_2P);
		else					uop_calc(MUL, BANK_HI, REDUCE_R2, REDUCE_R2, BANK_LO, REDUCE_R1, MOD_2P);

	uop_calc(MUL, BANK_LO, REDUCE_R1, REDUCE_R1, BANK_HI, REDUCE_R2, MOD_2P);
	uop_calc(MUL, BANK_HI, REDUCE_R2, REDUCE_T_X5, BANK_LO, REDUCE_T_X10, MOD_2P);

		// T_X20
	uop_move(BANK_LO, REDUCE_T_X10, REDUCE_T_X10, BANK_HI, REDUCE_R1, REDUCE_R1);
	uop_move(BANK_LO, REDUCE_T_X10, REDUCE_T_X10, BANK_HI, REDUCE_T_X10, REDUCE_T_X10);

	for (cyc_count=0; cyc_count<10; cyc_count++)
		if (!(cyc_count % 2))	uop_calc(MUL, BANK_HI, REDUCE_R1, REDUCE_R1, BANK_LO, REDUCE_R2, MOD_2P);
		else					uop_calc(MUL, BANK_LO, REDUCE_R2, REDUCE_R2, BANK_HI, REDUCE_R1, MOD_2P);

	uop_calc(MUL, BANK_HI, REDUCE_R1, REDUCE_T_X10, BANK_LO, REDUCE_T_X20, MOD_2P);

		// T_X40
	uop_move(BANK_LO, REDUCE_T_X20, REDUCE_T_X20, BANK_HI, REDUCE_R1, REDUCE_R1);
	uop_move(BANK_LO, REDUCE_T_X20, REDUCE_T_X20, BANK_HI, REDUCE_T_X20, REDUCE_T_X20);

	for (cyc_count=0; cyc_count<20; cyc_count++)
		if (!(cyc_count % 2))	uop_calc(MUL, BANK_HI, REDUCE_R1, REDUCE_R1, BANK_LO, REDUCE_R2, MOD_2P);
		else					uop_calc(MUL, BANK_LO, REDUCE_R2, REDUCE_R2, BANK_HI, REDUCE_R1, MOD_2P);

	uop_calc(MUL, BANK_HI, REDUCE_R1, REDUCE_T_X20, BANK_LO, REDUCE_T_X40, MOD_2P);

		// T_X50		
	uop_move(BANK_LO, REDUCE_T_X40, REDUCE_T_X40, BANK_HI, REDUCE_R1, REDUCE_R1);

	for (cyc_count=0; cyc_count<10; cyc_count++)
		if (!(cyc_count % 2))	uop_calc(MUL, BANK_HI, REDUCE_R1, REDUCE_R1, BANK_LO, REDUCE_R2, MOD_2P);
		else					uop_calc(MUL, BANK_LO, REDUCE_R2, REDUCE_R2, BANK_HI, REDUCE_R1, MOD_2P);

	uop_calc(MUL, BANK_HI, REDUCE_R1, REDUCE_T_X10, BANK_LO, REDUCE_T_X50, MOD_2P);

		// T_X100
	uop_move(BANK_LO, REDUCE_T_X50, REDUCE_T_X50, BANK_HI, REDUCE_R1, REDUCE_R1);
	uop_move(BANK_LO, REDUCE_T_X50, REDUCE_T_X50, BANK_HI, REDUCE_T_X50, REDUCE_T_X50);

	for (cyc_count=0; cyc_count<50; cyc_count++)
		if (!(cyc_count % 2))	uop_calc(MUL, BANK_HI, REDUCE_R1, REDUCE_R1, BANK_LO, REDUCE_R2, MOD_2P);
		else					uop_calc(MUL, BANK_LO, REDUCE_R2, REDUCE_R2, BANK_HI, REDUCE_R1, MOD_2P);

	uop_calc(MUL, BANK_HI, REDUCE_R1, REDUCE_T_X50, BANK_LO, REDUCE_T_X100, MOD_2P);

	uop_move(BANK_LO, REDUCE_T_X100, REDUCE_T_X100, BANK_HI, REDUCE_R1, REDUCE_R1);
	uop_move(BANK_LO, REDUCE_T_X100, REDUCE_T_X100, BANK_HI, REDUCE_T_X100, REDUCE_T_X100);

	for (cyc_count=0; cyc_count<100; cyc_count++)
		if (!(cyc_count % 2))	uop_calc(MUL, BANK_HI, REDUCE_R1, REDUCE_R1, BANK_LO, REDUCE_R2, MOD_2P);
		else					uop_calc(MUL, BANK_LO, REDUCE_R2, REDUCE_R2, BANK_HI, REDUCE_R1, MOD_2P);

	uop_calc(MUL, BANK_HI, REDUCE_R1, REDUCE_T_X100, BANK_LO, REDUCE_R2, MOD_2P);

	for (cyc_count=0; cyc_count<50; cyc_count++)
		if (!(cyc_count % 2))	uop_calc(MUL, BANK_LO, REDUCE_R2, REDUCE_R2, BANK_HI, REDUCE_R1, MOD_2P);
		else					uop_calc(MUL, BANK_HI, REDUCE_R1, REDUCE_R1, BANK_LO, REDUCE_R2, MOD_2P);

	uop_calc(MUL, BANK_LO, REDUCE_R2, REDUCE_T_X50, BANK_HI, REDUCE_R1, MOD_2P);
	
	for (cyc_count=0; cyc_count<4; cyc_count++)
		if (!(cyc_count % 2))	uop_calc(MUL, BANK_HI, REDUCE_R1, REDUCE_R1, BANK_LO, REDUCE_R2, MOD_2P);
		else					uop_calc(MUL, BANK_LO, REDUCE_R2, REDUCE_R2, BANK_HI, REDUCE_R1, MOD_2P);
	
	uop_calc(MUL, BANK_HI, REDUCE_R1, REDUCE_R1, BANK_LO, REDUCE_R2, MOD_2P);
	uop_move(BANK_HI, REDUCE_T_1011, REDUCE_T_1011, BANK_LO, REDUCE_T_1011, REDUCE_T_X100);
	uop_calc(MUL, BANK_LO, REDUCE_R2, REDUCE_T_1011, BANK_HI, REDUCE_R2, MOD_2P);
	uop_move(BANK_HI, REDUCE_R2, REDUCE_R2, BANK_LO, REDUCE_R2, REDUCE_R2);

	uop_calc(MUL, BANK_LO, REDUCE_R2, LADDER_R0_X, BANK_HI, REDUCE_R1, MOD_2P);

	// finally reduce to just 1*P
	uop_calc(ADD, BANK_HI, REDUCE_R1, CONST_ZERO, BANK_LO, REDUCE_R2, MOD_1P);	// !!!

	uop_stor(BANK_LO, REDUCE_R2, QX);
}


//------------------------------------------------------------------------------
static void uop_move	(UOP_BANK src, UOP_OPERAND s_op1, UOP_OPERAND s_op2,
						 UOP_BANK dst, UOP_OPERAND d_op1, UOP_OPERAND d_op2)
//------------------------------------------------------------------------------
{
	FPGA_BUFFER *s_ptr1 = NULL;
	FPGA_BUFFER *s_ptr2 = NULL;
	FPGA_BUFFER *d_ptr1 = NULL;
	FPGA_BUFFER *d_ptr2 = NULL;

		// same bank?
	if (src == dst) uop_fatal("ERROR: uop_move(): src == dst");
	
		// same operands?
	//if (s_op1 == d_op1) uop_fatal("ERROR: uop_move(): s_op1 == s_op2");
	//if (d_op1 == d_op2) uop_fatal("ERROR: uop_move(): d_op1 == d_op2");

		// source filled?
	if (src == BANK_LO)
	{	if (!buf_flag_lo[s_op1])
			uop_fatal("ERROR: uop_move(): !buf_flag_lo[s_op1]");
		if (!buf_flag_lo[s_op2])
			uop_fatal("ERROR: uop_move(): !buf_flag_lo[s_op2]");
		s_ptr1 = &BUF_LO[s_op1];
		s_ptr2 = &BUF_LO[s_op2];
	}
	if (src == BANK_HI)
	{	if (!buf_flag_hi[s_op1])
			uop_fatal("ERROR: uop_move(): !buf_flag_hi[s_op1]");
		if (!buf_flag_hi[s_op2])
			uop_fatal("ERROR: uop_move(): !buf_flag_hi[s_op2]");
		s_ptr1 = &BUF_HI[s_op1];
		s_ptr2 = &BUF_HI[s_op2];
	}

	if (d_op1 == CONST_ZERO) uop_fatal("ERROR: uop_move(): d_op1 == CONST_ZERO");
	if (d_op2 == CONST_ZERO) uop_fatal("ERROR: uop_move(): d_op2 == CONST_ZERO");
	if (d_op1 == CONST_ONE) uop_fatal("ERROR: uop_move(): d_op1 == CONST_ONE");
	if (d_op2 == CONST_ONE) uop_fatal("ERROR: uop_move(): d_op2 == CONST_ONE");
	if (d_op1 == CONST_A24) uop_fatal("ERROR: uop_move(): d_op1 == CONST_A24");
	if (d_op2 == CONST_A24) uop_fatal("ERROR: uop_move(): d_op2 == CONST_A24");

	if (dst == BANK_LO)
	{	buf_flag_lo[d_op1] = true;
		buf_flag_lo[d_op2] = true;
		d_ptr1 = &BUF_LO[d_op1];
		d_ptr2 = &BUF_LO[d_op2];
	}
	if (dst == BANK_HI)
	{	buf_flag_hi[d_op1] = true;
		buf_flag_hi[d_op2] = true;
		d_ptr1 = &BUF_HI[d_op1];
		d_ptr2 = &BUF_HI[d_op2];
	}

	fpga_multiword_copy(s_ptr1, d_ptr1);
	fpga_multiword_copy(s_ptr2, d_ptr2);
}


//------------------------------------------------------------------------------
static void uop_calc	(UOP_MATH math,
						 UOP_BANK src, UOP_OPERAND s_op1, UOP_OPERAND s_op2,
						 UOP_BANK dst, UOP_OPERAND d_op,
						 UOP_MODULUS mod)
//------------------------------------------------------------------------------
{
	FPGA_BUFFER *s_ptr1 = NULL;
	FPGA_BUFFER *s_ptr2 = NULL;
	FPGA_BUFFER *d_ptr = NULL;
	FPGA_BUFFER *n_ptr = NULL;

		// same bank?
	if (src == dst)
		uop_fatal("ERROR: uop_calc(): src == dst");
	
		// same operands?
	//if (s_op1 == s_op2)
		//uop_fatal("ERROR: uop_calc(): s_op1 == s_op2");

		// sources filled?
	if (src == BANK_LO)
	{	if (!buf_flag_lo[s_op1])
			uop_fatal("ERROR: uop_calc(): !buf_flag_lo[s_op1]");
		if (!buf_flag_lo[s_op2])
			uop_fatal("ERROR: uop_calc(): !buf_flag_lo[s_op2]");
		s_ptr1 = &BUF_LO[s_op1];
		s_ptr2 = &BUF_LO[s_op2];
	}
	if (src == BANK_HI)
	{	if (!buf_flag_hi[s_op1])
			uop_fatal("ERROR: uop_calc(): !buf_flag_hi[s_op1]");
		if (!buf_flag_hi[s_op2])
			uop_fatal("ERROR: uop_calc(): !buf_flag_hi[s_op2]");
		s_ptr1 = &BUF_HI[s_op1];
		s_ptr2 = &BUF_HI[s_op2];
	}

	if (d_op == CONST_ZERO) uop_fatal("ERROR: uop_calc(): d_op == CONST_ZERO");
	if (d_op == CONST_ONE) uop_fatal("ERROR: uop_calc(): d_op == CONST_ONE");
	if (d_op == CONST_A24) uop_fatal("ERROR: uop_calc(): d_op == CONST_A24");

	if (dst == BANK_LO)
	{	buf_flag_lo[d_op] = true;
		d_ptr = &BUF_LO[d_op];
	}
	if (dst == BANK_HI)
	{	buf_flag_hi[d_op] = true;
		d_ptr = &BUF_HI[d_op];
	}

	if (mod == MOD_1P) n_ptr = &X25519_1P;
	if (mod == MOD_2P) n_ptr = &X25519_2P;

	if (math == ADD) fpga_modular_add(s_ptr1, s_ptr2, d_ptr, n_ptr);
	if (math == SUB) fpga_modular_sub(s_ptr1, s_ptr2, d_ptr, n_ptr);
	if (math == MUL) fpga_modular_mul(s_ptr1, s_ptr2, d_ptr, n_ptr);
}


//------------------------------------------------------------------------------
static void uop_load(FPGA_BUFFER *mem, UOP_BANK dst, UOP_OPERAND d_op)
//------------------------------------------------------------------------------
{
	if (d_op == CONST_ZERO) uop_fatal("ERROR: uop_load(): d_op1 == CONST_ZERO");
	if (d_op == CONST_ONE) uop_fatal("ERROR: uop_load(): d_op1 == CONST_ONE");
	if (d_op == CONST_A24) uop_fatal("ERROR: uop_load(): d_op1 == CONST_A24");

	FPGA_BUFFER *d_ptr = NULL;
	if (dst == BANK_LO)
	{	d_ptr = &BUF_LO[d_op];
		buf_flag_lo[d_op] = true;
	}
	if (dst == BANK_HI)
	{	d_ptr = &BUF_HI[d_op];
		buf_flag_hi[d_op] = true;
	}

	fpga_multiword_copy(mem, d_ptr);
}


//------------------------------------------------------------------------------
static void uop_stor(UOP_BANK src, UOP_OPERAND s_op, FPGA_BUFFER *mem)
//------------------------------------------------------------------------------
{
	FPGA_BUFFER *s_ptr = NULL;
	if (src == BANK_LO)
	{	if (!buf_flag_lo[s_op])
			uop_fatal("ERROR: uop_stor(): !buf_flag_lo[s_op]");
		s_ptr = &BUF_LO[s_op];
		buf_flag_lo[s_op] = true;
	}
	if (src == BANK_HI)
	{	if (!buf_flag_hi[s_op])
			uop_fatal("ERROR: uop_stor(): !buf_flag_hi[s_op]");
		s_ptr = &BUF_HI[s_op];
		buf_flag_hi[s_op] = true;
	}

	fpga_multiword_copy(s_ptr, mem);
}


//------------------------------------------------------------------------------
// End-of-File
//------------------------------------------------------------------------------
