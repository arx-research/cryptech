//------------------------------------------------------------------------------
//
// x25519_fpga_curve_microcode.cpp
// -----------------------------------------------
// Elliptic curve arithmetic procedures for X25519
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
#include "x25519_fpga_model.h"


//------------------------------------------------------------------------------
enum X25519_UOP_OPERAND
//------------------------------------------------------------------------------
{
	CONST_A24 = CURVE25519_UOP_OPERAND_COUNT + 1,

	LADDER_R0_X,
	LADDER_R0_Z,

	LADDER_R1_X,
	LADDER_R1_Z,

	LADDER_T0_X,
	LADDER_T0_Z,

	LADDER_T1_X,
	LADDER_T1_Z,

	LADDER_S0,
	LADDER_S1,

	LADDER_D0,
	LADDER_D1,

	LADDER_QS0,
	LADDER_QD0,

	LADDER_S0D1,
	LADDER_S1D0,

	LADDER_TS,
	LADDER_TD,

	LADDER_QTD,

	LADDER_T0,
	LADDER_TA,
	LADDER_T1,

	LADDER_P_X,

	X25519_UOP_OPERAND_COUNT
};


//------------------------------------------------------------------------------
// Storage Buffers
//------------------------------------------------------------------------------
static FPGA_BUFFER BUF_LO[X25519_UOP_OPERAND_COUNT];
static FPGA_BUFFER BUF_HI[X25519_UOP_OPERAND_COUNT];


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
void fpga_curve_x25519_scalar_multiply_microcode(const FPGA_BUFFER *PX, const FPGA_BUFFER *K, FPGA_BUFFER *QX)
//------------------------------------------------------------------------------
{
	bool k_bit, s;				// 1-bit values
	FPGA_WORD k_word;			// current word of multiplier
	int word_count, bit_count;	// counters

		// initialize constant operands
	fpga_multiword_copy(&CURVE25519_ZERO, &BUF_LO[CONST_ZERO]);
	fpga_multiword_copy(&CURVE25519_ZERO, &BUF_HI[CONST_ZERO]);

	fpga_multiword_copy(&CURVE25519_ONE, &BUF_LO[CONST_ONE]);
	fpga_multiword_copy(&CURVE25519_ONE, &BUF_HI[CONST_ONE]);

	fpga_multiword_copy(&X25519_A24, &BUF_LO[CONST_A24]);
	fpga_multiword_copy(&X25519_A24, &BUF_HI[CONST_A24]);

		//
		// BEGIN MICROCODE
		//

		// initialization
	uop_load(PX, BANK_HI, LADDER_P_X, BUF_LO, BUF_HI);
	uop_move(BANK_HI, CONST_ONE,  CONST_ZERO, BANK_LO, LADDER_R0_X, LADDER_R0_Z, BUF_LO, BUF_HI);
	uop_move(BANK_HI, LADDER_P_X, CONST_ONE,  BANK_LO, LADDER_R1_X, LADDER_R1_Z, BUF_LO, BUF_HI);

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
			{	uop_move(BANK_LO, LADDER_R0_X, LADDER_R0_Z, BANK_HI, LADDER_T0_X, LADDER_T0_Z, BUF_LO, BUF_HI);	// HI: T0_X, T0_Z = LO: R0_X, R0_Z
				uop_move(BANK_LO, LADDER_R1_X, LADDER_R1_Z, BANK_HI, LADDER_T1_X, LADDER_T1_Z, BUF_LO, BUF_HI);	// HI: T1_X, T1_Z = LO: R1_X, R1_Z
			}
			else
			{	uop_move(BANK_LO, LADDER_R1_X, LADDER_R1_Z, BANK_HI, LADDER_T0_X, LADDER_T0_Z, BUF_LO, BUF_HI);	// HI: T0_X, T0_Z = LO: R1_X, R1_Z
				uop_move(BANK_LO, LADDER_R0_X, LADDER_R0_Z, BANK_HI, LADDER_T1_X, LADDER_T1_Z, BUF_LO, BUF_HI);	// HI: T1_X, T1_Z = LO: R0_X, R0_Z
			}

				// remember whether we actually did the swap
			s = k_bit;

				// run step
			uop_calc(ADD, BANK_HI, LADDER_T0_X, LADDER_T0_Z, BANK_LO, LADDER_S0,   BUF_LO, BUF_HI, MOD_2P);	// LO: S0 = HI: T0_X + T0_Z
			uop_calc(ADD, BANK_HI, LADDER_T1_X, LADDER_T1_Z, BANK_LO, LADDER_S1,   BUF_LO, BUF_HI, MOD_2P);	// LO: S1 = HI: T1_X + T1_Z
			uop_calc(SUB, BANK_HI, LADDER_T0_X, LADDER_T0_Z, BANK_LO, LADDER_D0,   BUF_LO, BUF_HI, MOD_2P);	// LO: D0 = HI: T0_X - T0_Z
			uop_calc(SUB, BANK_HI, LADDER_T1_X, LADDER_T1_Z, BANK_LO, LADDER_D1,   BUF_LO, BUF_HI, MOD_2P);	// LO: D1 = HI: T1_X - T1_Z

			uop_calc(MUL, BANK_LO, LADDER_S0,   LADDER_S0,   BANK_HI, LADDER_QS0,  BUF_LO, BUF_HI, MOD_2P);	// HI: QS0  = LO: S0 * S0
			uop_calc(MUL, BANK_LO, LADDER_D0,   LADDER_D0,   BANK_HI, LADDER_QD0,  BUF_LO, BUF_HI, MOD_2P);	// HI: QD0  = LO: D0 * D0
			uop_calc(MUL, BANK_LO, LADDER_S0,   LADDER_D1,   BANK_HI, LADDER_S0D1, BUF_LO, BUF_HI, MOD_2P);	// HI: S0D1 = LO: S0 * D1
			uop_calc(MUL, BANK_LO, LADDER_S1,   LADDER_D0,   BANK_HI, LADDER_S1D0, BUF_LO, BUF_HI, MOD_2P);	// HI: S1D0 = LO: S1 * D0

			uop_calc(ADD, BANK_HI, LADDER_S1D0, LADDER_S0D1, BANK_LO, LADDER_TS,   BUF_LO, BUF_HI, MOD_2P);	// LO: TS = HI: S1D0 + S0D1
			uop_calc(SUB, BANK_HI, LADDER_S1D0, LADDER_S0D1, BANK_LO, LADDER_TD,   BUF_LO, BUF_HI, MOD_2P);	// LO: TD = HI: S1D0 - S0D1

			uop_calc(MUL, BANK_LO, LADDER_TD,   LADDER_TD,   BANK_HI, LADDER_QTD,  BUF_LO, BUF_HI, MOD_2P);	// HI: QTD = LO: TD * TD

			uop_calc(SUB, BANK_HI, LADDER_QS0,  LADDER_QD0,  BANK_LO, LADDER_T0,   BUF_LO, BUF_HI, MOD_2P);	// LO: T0 = HI: QS0 - QD0
			uop_calc(MUL, BANK_LO, LADDER_T0,   CONST_A24,   BANK_HI, LADDER_TA,   BUF_LO, BUF_HI, MOD_2P);	// HI: TA = LO: T0 * A24
			uop_calc(ADD, BANK_HI, LADDER_TA,   LADDER_QD0,  BANK_LO, LADDER_T1,   BUF_LO, BUF_HI, MOD_2P);	// LO: T1 = HI: TA * QD0
			
			uop_calc(MUL, BANK_HI, LADDER_QS0,  LADDER_QD0,  BANK_LO, LADDER_R0_X, BUF_LO, BUF_HI, MOD_2P);	// LO: R0_X = HI: QS0 * QD0
			uop_calc(MUL, BANK_LO, LADDER_T0,   LADDER_T1,   BANK_HI, LADDER_R0_Z, BUF_LO, BUF_HI, MOD_2P);	// HI: R0_Z = LO: T0 * T1
			uop_calc(MUL, BANK_LO, LADDER_TS,   LADDER_TS,   BANK_HI, LADDER_R1_X, BUF_LO, BUF_HI, MOD_2P);	// HI: R1_X = LO: TS * TS
			uop_calc(MUL, BANK_HI, LADDER_P_X,  LADDER_QTD,  BANK_LO, LADDER_R1_Z, BUF_LO, BUF_HI, MOD_2P);	// LO: R1_Z = HI: PX * QTD

			uop_move(BANK_HI, LADDER_R0_Z, LADDER_R1_X, BANK_LO, LADDER_R0_Z, LADDER_R1_X, BUF_LO, BUF_HI);	// LO: R0_Z, R1_X = HI: R0_Z, R1_X
		}
	}

		// inversion expects result to be in LO: T1
	uop_move(BANK_HI, LADDER_R0_Z, LADDER_R0_Z, BANK_LO, INVERT_T_1, INVERT_T_1, BUF_LO, BUF_HI);	

		// just call piece of microcode
	fpga_modular_inv_microcode(BUF_LO, BUF_HI);
	
		// inversion places result in HI: R1
	uop_move(BANK_HI, INVERT_R1, INVERT_R1, BANK_LO, INVERT_R1, INVERT_R1, BUF_LO, BUF_HI);
	uop_calc(MUL, BANK_LO, INVERT_R1, LADDER_R0_X, BANK_HI, INVERT_R2, BUF_LO, BUF_HI, MOD_2P);

		// finally reduce to just 1*P
	uop_calc(ADD, BANK_HI, INVERT_R2, CONST_ZERO, BANK_LO, INVERT_R1, BUF_LO, BUF_HI, MOD_1P);	// !!!

		// store result
	uop_stor(BUF_LO, BUF_HI, BANK_LO, INVERT_R1, QX);
}


//------------------------------------------------------------------------------
// End-of-File
//------------------------------------------------------------------------------
