//------------------------------------------------------------------------------
//
// ed25519_fpga_curve_microcode.cpp
// -----------------------------------------------
// Elliptic curve arithmetic procedures for Ed25519
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
#include "ed25519_fpga_model.h"


//------------------------------------------------------------------------------
enum ED25519_UOP_OPERAND
//------------------------------------------------------------------------------
{
	CONST_G_X = CURVE25519_UOP_OPERAND_COUNT + 1,
	CONST_G_Y,
	CONST_G_T,
	
	CYCLE_R0_X,
	CYCLE_R0_Y,
	CYCLE_R0_Z,
	CYCLE_R0_T,
	
	CYCLE_R1_X,
	CYCLE_R1_Y,
	CYCLE_R1_Z,
	CYCLE_R1_T,
	
	CYCLE_S_X,
	CYCLE_S_Y,
	CYCLE_S_Z,
	CYCLE_S_T,
	
	CYCLE_T_X,
	CYCLE_T_Y,
	CYCLE_T_Z,
	CYCLE_T_T,
	
	CYCLE_U_X,
	CYCLE_U_Y,
	CYCLE_U_Z,
	CYCLE_U_T,
	
	CYCLE_V_X,
	CYCLE_V_Y,
	CYCLE_V_Z,
	CYCLE_V_T,
	
	PROC_A,
	PROC_B,
	PROC_C,
	PROC_D,
	PROC_E,
	PROC_F,
	PROC_G,
	PROC_H,
	PROC_I,
	PROC_J,

	ED25519_UOP_OPERAND_COUNT
};


//------------------------------------------------------------------------------
// Storage Buffers
//------------------------------------------------------------------------------
static FPGA_BUFFER BUF_LO[ED25519_UOP_OPERAND_COUNT];
static FPGA_BUFFER BUF_HI[ED25519_UOP_OPERAND_COUNT];


//------------------------------------------------------------------------------
//
// Elliptic curve point scalar multiplication routine.
//
//------------------------------------------------------------------------------
void fpga_curve_ed25519_base_scalar_multiply_microcode(const FPGA_BUFFER *K, FPGA_BUFFER *QY)
//------------------------------------------------------------------------------
{
	bool k_bit;					// 1-bit values
	FPGA_WORD k_word;			// current word of multiplier
	int word_count, bit_count;	// counters

		// initialize internal banks
	fpga_multiword_copy(&CURVE25519_ZERO, &BUF_LO[CONST_ZERO]);
	fpga_multiword_copy(&CURVE25519_ZERO, &BUF_HI[CONST_ZERO]);

	fpga_multiword_copy(&CURVE25519_ONE, &BUF_LO[CONST_ONE]);
	fpga_multiword_copy(&CURVE25519_ONE, &BUF_HI[CONST_ONE]);

	fpga_multiword_copy(&ED25519_G_X, &BUF_LO[CONST_G_X]);
	fpga_multiword_copy(&ED25519_G_X, &BUF_HI[CONST_G_X]);

	fpga_multiword_copy(&ED25519_G_Y, &BUF_LO[CONST_G_Y]);
	fpga_multiword_copy(&ED25519_G_Y, &BUF_HI[CONST_G_Y]);

	fpga_multiword_copy(&ED25519_G_T, &BUF_LO[CONST_G_T]);
	fpga_multiword_copy(&ED25519_G_T, &BUF_HI[CONST_G_T]);

		// force certain bit values
	FPGA_BUFFER K_INT;
	fpga_multiword_copy(K, &K_INT);

	K_INT.words[0] &= 0xFFFFFFF8;
	K_INT.words[FPGA_OPERAND_NUM_WORDS-1] &= 0x3FFFFFFF;
	K_INT.words[FPGA_OPERAND_NUM_WORDS-1] |= 0x40000000;


	/* BEGIN_MICROCODE: PREPARE */

		// initialize
	uop_move(BANK_HI, CONST_ZERO, BANK_LO, CYCLE_R0_X, BUF_LO, BUF_HI);
	uop_move(BANK_HI, CONST_ONE,  BANK_LO, CYCLE_R0_Y, BUF_LO, BUF_HI);
	uop_move(BANK_HI, CONST_ONE,  BANK_LO, CYCLE_R0_Z, BUF_LO, BUF_HI);
	uop_move(BANK_HI, CONST_ZERO, BANK_LO, CYCLE_R0_T, BUF_LO, BUF_HI);

	uop_move(BANK_HI, CONST_G_X, BANK_LO, CYCLE_R1_X, BUF_LO, BUF_HI);
	uop_move(BANK_HI, CONST_G_Y, BANK_LO, CYCLE_R1_Y, BUF_LO, BUF_HI);
	uop_move(BANK_HI, CONST_ONE, BANK_LO, CYCLE_R1_Z, BUF_LO, BUF_HI);
	uop_move(BANK_HI, CONST_G_T, BANK_LO, CYCLE_R1_T, BUF_LO, BUF_HI);

	/* END_MICROCODE */

		// multiply
	for (word_count=0; word_count<FPGA_OPERAND_NUM_WORDS; word_count++)
	{
		for (bit_count=0; bit_count<FPGA_WORD_WIDTH; bit_count++)
		{
				// get current bit of K
			k_word = K_INT.words[word_count] >> bit_count;
			k_bit = (k_word & (FPGA_WORD)1) == 1;

			if (k_bit)
			{
				// U = R0
				// V = R1
                    
				/* BEGIN_MICROCODE: BEFORE_ROUND_K1 */

				uop_move(BANK_LO, CYCLE_R0_X, BANK_HI, CYCLE_U_X, BUF_LO, BUF_HI);
				uop_move(BANK_LO, CYCLE_R0_Y, BANK_HI, CYCLE_U_Y, BUF_LO, BUF_HI);
				uop_move(BANK_LO, CYCLE_R0_Z, BANK_HI, CYCLE_U_Z, BUF_LO, BUF_HI);
				uop_move(BANK_LO, CYCLE_R0_T, BANK_HI, CYCLE_U_T, BUF_LO, BUF_HI);
				uop_move(BANK_LO, CYCLE_R1_X, BANK_HI, CYCLE_V_X, BUF_LO, BUF_HI);
				uop_move(BANK_LO, CYCLE_R1_Y, BANK_HI, CYCLE_V_Y, BUF_LO, BUF_HI);
				uop_move(BANK_LO, CYCLE_R1_Z, BANK_HI, CYCLE_V_Z, BUF_LO, BUF_HI);
				uop_move(BANK_LO, CYCLE_R1_T, BANK_HI, CYCLE_V_T, BUF_LO, BUF_HI);

				/* END_MICROCODE */
			}
			else
			{
				// U = R1
				// V = R0

				/* BEGIN_MICROCODE: BEFORE_ROUND_K0 */

				uop_move(BANK_LO, CYCLE_R0_X, BANK_HI, CYCLE_V_X, BUF_LO, BUF_HI);
				uop_move(BANK_LO, CYCLE_R0_Y, BANK_HI, CYCLE_V_Y, BUF_LO, BUF_HI);
				uop_move(BANK_LO, CYCLE_R0_Z, BANK_HI, CYCLE_V_Z, BUF_LO, BUF_HI);
				uop_move(BANK_LO, CYCLE_R0_T, BANK_HI, CYCLE_V_T, BUF_LO, BUF_HI);
				uop_move(BANK_LO, CYCLE_R1_X, BANK_HI, CYCLE_U_X, BUF_LO, BUF_HI);
				uop_move(BANK_LO, CYCLE_R1_Y, BANK_HI, CYCLE_U_Y, BUF_LO, BUF_HI);
				uop_move(BANK_LO, CYCLE_R1_Z, BANK_HI, CYCLE_U_Z, BUF_LO, BUF_HI);
				uop_move(BANK_LO, CYCLE_R1_T, BANK_HI, CYCLE_U_T, BUF_LO, BUF_HI);

				/* END_MICROCODE */
			}

			/* BEGIN_MICROCODE: DURING_ROUND */

				// S = double(U)
			uop_calc(MUL, BANK_HI,  CYCLE_U_X, CYCLE_U_X, BANK_LO, PROC_A, BUF_LO, BUF_HI,     MOD_2P);
			uop_calc(MUL, BANK_HI,  CYCLE_U_Y, CYCLE_U_Y, BANK_LO, PROC_B, BUF_LO, BUF_HI,     MOD_2P); 
			uop_calc(MUL, BANK_HI,  CYCLE_U_Z, CYCLE_U_Z, BANK_LO, PROC_I, BUF_LO, BUF_HI,     MOD_2P);
			uop_calc(ADD, BANK_LO,  PROC_I,    PROC_I,    BANK_HI, PROC_C, BUF_LO, BUF_HI,     MOD_2P);
			uop_calc(ADD, BANK_HI,  CYCLE_U_X, CYCLE_U_Y, BANK_LO, PROC_I, BUF_LO, BUF_HI,     MOD_2P);
			uop_calc(MUL, BANK_LO,  PROC_I,    PROC_I,    BANK_HI, PROC_D, BUF_LO, BUF_HI,     MOD_2P);

			uop_calc(ADD, BANK_LO,  PROC_A,    PROC_B,    BANK_HI, PROC_H, BUF_LO, BUF_HI,     MOD_2P);
			uop_calc(SUB, BANK_HI,  PROC_H,    PROC_D,    BANK_LO, PROC_E, BUF_LO, BUF_HI,     MOD_2P);
			uop_calc(SUB, BANK_LO,  PROC_A,    PROC_B,    BANK_HI, PROC_G, BUF_LO, BUF_HI,     MOD_2P);
			uop_calc(ADD, BANK_HI,  PROC_C,    PROC_G,    BANK_LO, PROC_F, BUF_LO, BUF_HI,     MOD_2P);

			uop_move(BANK_HI, PROC_G, BANK_LO, PROC_G, BUF_LO, BUF_HI);
			uop_move(BANK_HI, PROC_H, BANK_LO, PROC_H, BUF_LO, BUF_HI);

			uop_calc(MUL, BANK_LO,  PROC_E,    PROC_F,    BANK_HI, CYCLE_S_X, BUF_LO, BUF_HI,  MOD_2P);
			uop_calc(MUL, BANK_LO,  PROC_G,    PROC_H,    BANK_HI, CYCLE_S_Y, BUF_LO, BUF_HI,  MOD_2P);

			uop_calc(MUL, BANK_LO,  PROC_E,    PROC_H,    BANK_HI, CYCLE_S_T, BUF_LO, BUF_HI,  MOD_2P);
			uop_calc(MUL, BANK_LO,  PROC_F,    PROC_G,    BANK_HI, CYCLE_S_Z, BUF_LO, BUF_HI,  MOD_2P);

				// T = add(S, V)
			uop_calc(SUB, BANK_HI, CYCLE_S_Y, CYCLE_S_X, BANK_LO, PROC_I, BUF_LO, BUF_HI,  MOD_2P);
			uop_calc(ADD, BANK_HI, CYCLE_V_Y, CYCLE_V_X, BANK_LO, PROC_J, BUF_LO, BUF_HI,  MOD_2P);
			uop_calc(MUL, BANK_LO, PROC_I,    PROC_J,    BANK_HI, PROC_A, BUF_LO, BUF_HI,  MOD_2P);

			uop_calc(ADD, BANK_HI, CYCLE_S_Y, CYCLE_S_X, BANK_LO, PROC_I, BUF_LO, BUF_HI,  MOD_2P);
			uop_calc(SUB, BANK_HI, CYCLE_V_Y, CYCLE_V_X, BANK_LO, PROC_J, BUF_LO, BUF_HI,  MOD_2P);
			uop_calc(MUL, BANK_LO, PROC_I,    PROC_J,    BANK_HI, PROC_B, BUF_LO, BUF_HI,  MOD_2P);

			uop_calc(MUL, BANK_HI, CYCLE_S_Z, CYCLE_V_T, BANK_LO, PROC_I, BUF_LO, BUF_HI,  MOD_2P);
			uop_calc(ADD, BANK_LO, PROC_I,    PROC_I,    BANK_HI, PROC_C, BUF_LO, BUF_HI,  MOD_2P);
			uop_calc(MUL, BANK_HI, CYCLE_S_T, CYCLE_V_Z, BANK_LO, PROC_I, BUF_LO, BUF_HI,  MOD_2P);
			uop_calc(ADD, BANK_LO, PROC_I,    PROC_I,    BANK_HI, PROC_D, BUF_LO, BUF_HI,  MOD_2P);

			uop_calc(ADD, BANK_HI, PROC_C,    PROC_D,    BANK_LO, PROC_E, BUF_LO, BUF_HI,  MOD_2P);
			uop_calc(SUB, BANK_HI, PROC_B,    PROC_A,    BANK_LO, PROC_F, BUF_LO, BUF_HI,  MOD_2P);
			uop_calc(ADD, BANK_HI, PROC_B,    PROC_A,    BANK_LO, PROC_G, BUF_LO, BUF_HI,  MOD_2P);
			uop_calc(SUB, BANK_HI, PROC_D,    PROC_C,    BANK_LO, PROC_H, BUF_LO, BUF_HI,  MOD_2P);

			uop_calc(MUL, BANK_LO, PROC_E,    PROC_F,    BANK_HI, CYCLE_T_X, BUF_LO, BUF_HI,  MOD_2P);
			uop_calc(MUL, BANK_LO, PROC_G,    PROC_H,    BANK_HI, CYCLE_T_Y, BUF_LO, BUF_HI,  MOD_2P);
			uop_calc(MUL, BANK_LO, PROC_E,    PROC_H,    BANK_HI, CYCLE_T_T, BUF_LO, BUF_HI,  MOD_2P);
			uop_calc(MUL, BANK_LO, PROC_F,    PROC_G,    BANK_HI, CYCLE_T_Z, BUF_LO, BUF_HI,  MOD_2P);

			/* END_MICROCODE */
			
			if (k_bit)
			{
				// R0 = T

				/* BEGIN_MICROCODE: AFTER_ROUND_K1 */

				uop_move(BANK_HI, CYCLE_T_X, BANK_LO, CYCLE_R0_X, BUF_LO, BUF_HI);
				uop_move(BANK_HI, CYCLE_T_Y, BANK_LO, CYCLE_R0_Y, BUF_LO, BUF_HI);
				uop_move(BANK_HI, CYCLE_T_Z, BANK_LO, CYCLE_R0_Z, BUF_LO, BUF_HI);
				uop_move(BANK_HI, CYCLE_T_T, BANK_LO, CYCLE_R0_T, BUF_LO, BUF_HI);

				/* END_MICROCODE */
			}
			else
			{
				// R1 = T

				/* BEGIN_MICROCODE: AFTER_ROUND_K0 */

				uop_move(BANK_HI, CYCLE_T_X, BANK_LO, CYCLE_R1_X, BUF_LO, BUF_HI);
				uop_move(BANK_HI, CYCLE_T_Y, BANK_LO, CYCLE_R1_Y, BUF_LO, BUF_HI);
				uop_move(BANK_HI, CYCLE_T_Z, BANK_LO, CYCLE_R1_Z, BUF_LO, BUF_HI);
				uop_move(BANK_HI, CYCLE_T_T, BANK_LO, CYCLE_R1_T, BUF_LO, BUF_HI);

				/* END_MICROCODE */
			}
		}
	}

	/* BEGIN_MICROCODE: BEFORE_INVERSION */

		// inversion expects result to be in LO: T1
	uop_move(BANK_LO, CYCLE_R0_Z, BANK_HI, CYCLE_R0_Z, BUF_LO, BUF_HI);
	uop_move(BANK_HI, CYCLE_R0_Z, BANK_LO, INVERT_T_1, BUF_LO, BUF_HI);

	/* END_MICROCODE */

		// just call piece of microcode
	fpga_modular_inv_microcode(BUF_LO, BUF_HI);

	/* BEGIN_MICROCODE: AFTER_INVERSION */

		// inversion places result in HI: R1
		// coordinates are in LO: R0_X, R0_Y
	uop_move(BANK_HI, INVERT_R1, BANK_LO, INVERT_R1, BUF_LO, BUF_HI);
	uop_calc(MUL, BANK_LO, INVERT_R1, CYCLE_R0_X, BANK_HI, CYCLE_R0_X, BUF_LO, BUF_HI, MOD_2P);
	uop_calc(MUL, BANK_LO, INVERT_R1, CYCLE_R0_Y, BANK_HI, CYCLE_R0_Y, BUF_LO, BUF_HI, MOD_2P);

	/* END_MICROCODE */

	/* BEGIN_MICROCODE: FINAL_REDUCTION */

		// finally reduce to just 1*P
	uop_calc(ADD, BANK_HI, CYCLE_R0_X, CONST_ZERO, BANK_LO, CYCLE_R0_X, BUF_LO, BUF_HI, MOD_1P);
	uop_calc(ADD, BANK_HI, CYCLE_R0_Y, CONST_ZERO, BANK_LO, CYCLE_R0_Y, BUF_LO, BUF_HI, MOD_1P);

	/* END_MICROCODE */

	// poke sign bit
	BUF_LO[CYCLE_R0_Y].words[FPGA_OPERAND_NUM_WORDS-1] |=
		(FPGA_WORD)((BUF_LO[CYCLE_R0_X].words[0] & (FPGA_WORD)1) << 31);

	/* BEGIN_MICROCODE: HANDLE_SIGN */

	uop_move(BANK_LO, CYCLE_R0_X, BANK_HI, CYCLE_R0_X, BUF_LO, BUF_HI);

	/* END_MICROCODE */

	/* BEGIN_MICROCODE: OUTPUT */

	uop_move(BANK_LO, CYCLE_R0_Y, BANK_HI, CYCLE_R0_Y, BUF_LO, BUF_HI);

	/* END_MICROCODE */

		// store result
	uop_stor(BUF_LO, BUF_HI, BANK_LO, CYCLE_R0_Y, QY);
}


//------------------------------------------------------------------------------
// End-of-File
//------------------------------------------------------------------------------
