//------------------------------------------------------------------------------
//
// ed25519_fpga_model.cpp
// ----------------------
// Ed25519 FPGA Model
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
// Mode Switch
//------------------------------------------------------------------------------
#define USE_MICROCODE


//------------------------------------------------------------------------------
// Headers
//------------------------------------------------------------------------------
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "ed25519_fpga_model.h"


//------------------------------------------------------------------------------
// Prototypes
//------------------------------------------------------------------------------
static	void		fpga_model_ed25519_init				();
static	bool		test_ed25519_base_point_multiplier	(const FPGA_BUFFER *k, const FPGA_BUFFER *qy);
static	FPGA_WORD	fpga_model_ed25519_bswap			(FPGA_WORD w);


//------------------------------------------------------------------------------
// Locals
//------------------------------------------------------------------------------
static FPGA_BUFFER ed25519_d_1, ed25519_q_y_1;
static FPGA_BUFFER ed25519_d_2, ed25519_q_y_2;
static FPGA_BUFFER ed25519_d_3, ed25519_q_y_3;
static FPGA_BUFFER ed25519_d_4, ed25519_q_y_4;
static FPGA_BUFFER ed25519_d_5, ed25519_q_y_5;


//------------------------------------------------------------------------------
int main()
//------------------------------------------------------------------------------
{
	bool	ok;		// flag


		//
		// initialize buffers
		//
	fpga_multiword_init();
	fpga_modular_init();
	fpga_curve_ed25519_init();
	fpga_model_ed25519_init();


		//
		// test base point multiplier: Q = d * G
		//
	printf("Trying to derive public key from private key...\n\n");
	ok = test_ed25519_base_point_multiplier(&ed25519_d_1, &ed25519_q_y_1);
	if (!ok) return EXIT_FAILURE;

	
		//
		// test base point multiplier: Q = d * G
		//
	printf("Trying to derive public key from private key...\n\n");
	ok = test_ed25519_base_point_multiplier(&ed25519_d_2, &ed25519_q_y_2);
	if (!ok) return EXIT_FAILURE;
		
	
		//
		// test base point multiplier: Q = d * G
		//
	printf("Trying to derive public key from private key...\n\n");
	ok = test_ed25519_base_point_multiplier(&ed25519_d_3, &ed25519_q_y_3);
	if (!ok) return EXIT_FAILURE;
		
	
		//
		// test base point multiplier: Q = d * G
		//
	printf("Trying to derive public key from private key...\n\n");
	ok = test_ed25519_base_point_multiplier(&ed25519_d_4, &ed25519_q_y_4);
	if (!ok) return EXIT_FAILURE;

	
		//
		// test base point multiplier: Q = d * G
		//
	printf("Trying to derive public key from private key...\n\n");
	ok = test_ed25519_base_point_multiplier(&ed25519_d_5, &ed25519_q_y_5);
	if (!ok) return EXIT_FAILURE;


		//
		// everything went just fine
		//
	return EXIT_SUCCESS;
}


//------------------------------------------------------------------------------
static void fpga_model_ed25519_init()
//------------------------------------------------------------------------------
{
	int w_src, w_dst;	// word counters

	FPGA_WORD tmp_d_1[FPGA_OPERAND_NUM_WORDS]	= ED25519_D_HASHED_LSB_1;
	FPGA_WORD tmp_d_2[FPGA_OPERAND_NUM_WORDS]	= ED25519_D_HASHED_LSB_2;
	FPGA_WORD tmp_d_3[FPGA_OPERAND_NUM_WORDS]	= ED25519_D_HASHED_LSB_3;
	FPGA_WORD tmp_d_4[FPGA_OPERAND_NUM_WORDS]	= ED25519_D_HASHED_LSB_4;
	FPGA_WORD tmp_d_5[FPGA_OPERAND_NUM_WORDS]	= ED25519_D_HASHED_LSB_5;

	FPGA_WORD tmp_q_y_1[FPGA_OPERAND_NUM_WORDS]	= ED25519_Q_Y_1;
	FPGA_WORD tmp_q_y_2[FPGA_OPERAND_NUM_WORDS]	= ED25519_Q_Y_2;
	FPGA_WORD tmp_q_y_3[FPGA_OPERAND_NUM_WORDS]	= ED25519_Q_Y_3;
	FPGA_WORD tmp_q_y_4[FPGA_OPERAND_NUM_WORDS]	= ED25519_Q_Y_4;
	FPGA_WORD tmp_q_y_5[FPGA_OPERAND_NUM_WORDS]	= ED25519_Q_Y_5;

		/* fill buffers for large multi-word integers */
	for (	w_src = 0, w_dst = FPGA_OPERAND_NUM_WORDS - 1;
			w_src < FPGA_OPERAND_NUM_WORDS;
			w_src++, w_dst--)
	{	
		ed25519_d_1.words[w_dst] = tmp_d_1[w_src];
		ed25519_d_2.words[w_dst] = tmp_d_2[w_src];
		ed25519_d_3.words[w_dst] = tmp_d_3[w_src];
		ed25519_d_4.words[w_dst] = tmp_d_4[w_src];
		ed25519_d_5.words[w_dst] = tmp_d_5[w_src];

		// public key is in reverse order
		ed25519_q_y_1.words[w_dst] = fpga_model_ed25519_bswap(tmp_q_y_1[w_dst]);
		ed25519_q_y_2.words[w_dst] = fpga_model_ed25519_bswap(tmp_q_y_2[w_dst]);
		ed25519_q_y_3.words[w_dst] = fpga_model_ed25519_bswap(tmp_q_y_3[w_dst]);
		ed25519_q_y_4.words[w_dst] = fpga_model_ed25519_bswap(tmp_q_y_4[w_dst]);
		ed25519_q_y_5.words[w_dst] = fpga_model_ed25519_bswap(tmp_q_y_5[w_dst]);
	}
}


//------------------------------------------------------------------------------
static bool test_ed25519_base_point_multiplier(const FPGA_BUFFER *k, const FPGA_BUFFER *qy)
//------------------------------------------------------------------------------
//
// k - multiplier
//
// (..., qy) - expected coordinates of product
//
// Returns true when point (..., ry) = k * G matches the point (..., qy).
//
//------------------------------------------------------------------------------
{
	bool ok;			// flag
	FPGA_BUFFER ry;		// result

		/* run the model */
	fpga_curve_ed25519_base_scalar_multiply(k, &ry);

		/* handle result */
	ok = compare_fpga_buffers(qy, &ry);
	if (!ok)
	{	printf("\n    ERROR\n\n");
		return false;
	}
	else printf("\n    OK\n\n");

		// everything went just fine
	return true;
}


//------------------------------------------------------------------------------
FPGA_WORD fpga_model_ed25519_bswap(FPGA_WORD w)
//------------------------------------------------------------------------------
{
	FPGA_WORD ret = 0;

	ret |= (uint8_t)w, ret <<= 8;
	ret |= (uint8_t)(w >> 8), ret <<= 8;
	ret |= (uint8_t)(w >> 16), ret <<= 8;
	ret |= (uint8_t)(w >> 24);

	return ret;
}


//------------------------------------------------------------------------------
// End-of-File
//------------------------------------------------------------------------------
