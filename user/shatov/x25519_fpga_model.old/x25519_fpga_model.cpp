//------------------------------------------------------------------------------
//
// x25519_fpga_model.cpp
// --------------------------------------------
// X25519 Model
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
#include "x25519_fpga_model.h"


//------------------------------------------------------------------------------
// Prototypes
//------------------------------------------------------------------------------
void	fpga_model_init			();

bool	test_point_multiplier	(FPGA_BUFFER *px, FPGA_BUFFER *k, FPGA_BUFFER *qx);

void	print_fpga_buffer		(const char *s, FPGA_BUFFER *v);
bool	compare_fpga_buffers	(FPGA_BUFFER *ax, FPGA_BUFFER *bx);


//------------------------------------------------------------------------------
// Locals
//------------------------------------------------------------------------------
static FPGA_BUFFER x25519_da, x25519_db;
static FPGA_BUFFER x25519_qa_x, x25519_qb_x;
static FPGA_BUFFER x25519_qab_x;


//------------------------------------------------------------------------------
int main()
//------------------------------------------------------------------------------
{
	bool	ok_a, ok_b;		// flags


		//
		// initialize buffers
		//
	fpga_multiword_init();
	fpga_modular_init();
	fpga_curve_init();
	fpga_model_init();


		//
		// test point multiplier: QA = dA * G
		//                        QB = dB * G
		//
	printf("Trying to derive public keys from private keys...\n\n");
	ok_a = test_point_multiplier(&X25519_G_X, &x25519_da, &x25519_qa_x);
	ok_b = test_point_multiplier(&X25519_G_X, &x25519_db, &x25519_qb_x);
	if (!ok_a || !ok_b) return EXIT_FAILURE;


		//
		// test point multiplier: QAB = dA * QB
		//						  QAB = dB * QA
		//
	printf("Trying to derive shared secret key...\n\n");
	ok_a = test_point_multiplier(&x25519_qb_x, &x25519_da, &x25519_qab_x);
	ok_b = test_point_multiplier(&x25519_qa_x, &x25519_db, &x25519_qab_x);
	if (!ok_a || !ok_b) return EXIT_FAILURE;


		//
		// everything went just fine
		//
	return EXIT_SUCCESS;
}


//------------------------------------------------------------------------------
void fpga_model_init()
//------------------------------------------------------------------------------
{
	int w_src, w_dst;	// word counters

	FPGA_WORD tmp_da[FPGA_OPERAND_NUM_WORDS]	= X25519_DA;
	FPGA_WORD tmp_db[FPGA_OPERAND_NUM_WORDS]	= X25519_DB;

	FPGA_WORD tmp_qa_x[FPGA_OPERAND_NUM_WORDS]	= X25519_QA_X;
	FPGA_WORD tmp_qb_x[FPGA_OPERAND_NUM_WORDS]	= X25519_QB_X;

	FPGA_WORD tmp_qab_x[FPGA_OPERAND_NUM_WORDS]	= X25519_QAB_X;

		/* fill buffers for large multi-word integers */
	for (	w_src = 0, w_dst = FPGA_OPERAND_NUM_WORDS - 1;
			w_src < FPGA_OPERAND_NUM_WORDS;
			w_src++, w_dst--)
	{	
		x25519_da.words[w_dst]	= tmp_da[w_src];
		x25519_db.words[w_dst]	= tmp_db[w_src];

		x25519_qa_x.words[w_dst] = tmp_qa_x[w_src];
		x25519_qb_x.words[w_dst] = tmp_qb_x[w_src];

		x25519_qab_x.words[w_dst] = tmp_qab_x[w_src];
	}
}


//------------------------------------------------------------------------------
bool test_point_multiplier(FPGA_BUFFER *px, FPGA_BUFFER *k, FPGA_BUFFER *qx)
//------------------------------------------------------------------------------
//
// (px, ...) - multiplicand
// k - multiplier
//
// (qx, ...) - expected coordinates of product
//
// Returns true when point (rx, ...) = k * P matches the point (qx, ...).
//
//------------------------------------------------------------------------------
{
	bool ok;			// flag
	FPGA_BUFFER rx;		// result

		/* run the model */
	fpga_curve_scalar_multiply(px, k, &rx);

		/* handle result */
	ok = compare_fpga_buffers(qx, &rx);
	if (!ok)
	{	printf("\n    ERROR\n\n");
		return false;
	}
	else printf("\n    OK\n\n");

		// everything went just fine
	return true;
}


//------------------------------------------------------------------------------
bool compare_fpga_buffers(FPGA_BUFFER *ax, FPGA_BUFFER *bx)
//------------------------------------------------------------------------------
//
// Compare affine coordinates of two points and return true when they match.
//
//------------------------------------------------------------------------------
{
	int w;	// word counter

		// print all the values
	print_fpga_buffer("  Expected:   X = ", ax);
	print_fpga_buffer("  Calculated: X = ", bx);

		// compare values
	for (w=0; w<FPGA_OPERAND_NUM_WORDS; w++)
	{
			// compare x
		if (ax->words[w] != bx->words[w]) return false;
	}

		// values are the same
	return true;
}


//------------------------------------------------------------------------------
void print_fpga_buffer(const char *s, FPGA_BUFFER *buf)
//------------------------------------------------------------------------------
//
// Pretty print large multi-word integer.
//
//------------------------------------------------------------------------------
{
	int w;	// word counter

		// print header
	printf("%s", s);

		// print all bytes
	for (w=FPGA_OPERAND_NUM_WORDS; w>0; w--)
	{	
		printf("%08x", buf->words[w-1]);

			// insert space after every group of 4 bytes
		if (w > 1) printf(" ");
	}

		// print footer
	printf("\n");
}


//------------------------------------------------------------------------------
// End-of-File
//------------------------------------------------------------------------------
