//------------------------------------------------------------------------------
//
// ecdh_fpga_model.cpp
// --------------------------------------------
// Curve point scalar multiplier model for ECDH
//
// Authors: Pavel Shatov
//
// Copyright (c) 2017-2018, NORDUnet A/S
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
#include "ecdh_fpga_model.h"
#include "fpga_lowlevel.h"
#include "fpga_modular.h"
#include "fpga_curve.h"
#include "fpga_util.h"


//------------------------------------------------------------------------------
// Prototypes
//------------------------------------------------------------------------------
bool	test_point_multiplier			(FPGA_BUFFER *px, FPGA_BUFFER *py, FPGA_BUFFER *k, FPGA_BUFFER *qx, FPGA_BUFFER *qy);
bool	abuse_point_multiplier			(FPGA_BUFFER *qx, FPGA_BUFFER *qy);

void	print_fpga_buffer				(const char *s, FPGA_BUFFER *v);
bool	compare_fpga_buffers			(FPGA_BUFFER *ax, FPGA_BUFFER *ay, FPGA_BUFFER *bx, FPGA_BUFFER *by);


//------------------------------------------------------------------------------
int main()
//------------------------------------------------------------------------------
{
	bool	ok_a, ok_b, ok_g;		// flags

		//
		// initialize buffers
		//
	fpga_modular_init();
	fpga_curve_init();


		//
		// test point multiplier: QA = dA * G
		//                        QB = dB * G
		//
	printf("Trying to derive public keys from private keys...\n\n");
	ok_a = test_point_multiplier(&ecdsa_g_x, &ecdsa_g_y, &ecdh_da, &ecdh_qa_x, &ecdh_qa_y);
	ok_b = test_point_multiplier(&ecdsa_g_x, &ecdsa_g_y, &ecdh_db, &ecdh_qb_x, &ecdh_qb_y);
	if (!ok_a || !ok_b) return EXIT_FAILURE;


		//
		// test point multiplier: S = dA * QB
		//						  S = dB * QA
		//
	printf("Trying to derive shared secret key...\n\n");
	ok_a = test_point_multiplier(&ecdh_qa_x, &ecdh_qa_y, &ecdh_db, &ecdh_s_x, &ecdh_s_y);
	ok_b = test_point_multiplier(&ecdh_qb_x, &ecdh_qb_y, &ecdh_da, &ecdh_s_x, &ecdh_s_y);
	if (!ok_a || !ok_b) return EXIT_FAILURE;


		//
		// test point multiplier: O = 0 * QA
		//                        O = 0 * QB
		//
	printf("Trying to multiply public keys by zero...\n\n");
	ok_a = test_point_multiplier(&ecdh_qa_x, &ecdh_qa_y, &ecdsa_zero, &ecdsa_zero, &ecdsa_zero);
	ok_b = test_point_multiplier(&ecdh_qb_x, &ecdh_qb_y, &ecdsa_zero, &ecdsa_zero, &ecdsa_zero);
	if (!ok_a || !ok_b) return EXIT_FAILURE;


		//
		// test point multiplier: QA = 1 * QA
		//                        QB = 1 * QB
		//
	printf("Trying to multiply public keys by one...\n\n");
	ok_a = test_point_multiplier(&ecdh_qa_x, &ecdh_qa_y, &ecdsa_one, &ecdh_qa_x, &ecdh_qa_y);
	ok_b = test_point_multiplier(&ecdh_qb_x, &ecdh_qb_y, &ecdsa_one, &ecdh_qb_x, &ecdh_qb_y);
	if (!ok_a || !ok_b) return EXIT_FAILURE;


		//
		// abuse point multiplier
		//
	ok_g = abuse_point_multiplier(&ecdsa_g_x, &ecdsa_g_y);
	ok_a = abuse_point_multiplier(&ecdh_qa_x, &ecdh_qa_y);
	ok_b = abuse_point_multiplier(&ecdh_qb_x, &ecdh_qb_y);
	if (!ok_g || !ok_a || !ok_b) return EXIT_FAILURE;


		//
		// everything went just fine
		//
	return EXIT_SUCCESS;
}


//------------------------------------------------------------------------------
bool test_point_multiplier(FPGA_BUFFER *px, FPGA_BUFFER *py, FPGA_BUFFER *k, FPGA_BUFFER *qx, FPGA_BUFFER *qy)
//------------------------------------------------------------------------------
//
// (px,py) - multiplicand
// k - multiplier
//
// qx, qy - expected coordinates of product
//
// Returns true when point (rx,ry) = k * P matches the point (qx,qy).
//
//------------------------------------------------------------------------------
{
	bool ok;				// flag
	FPGA_BUFFER rx, ry;		// result

		/* run the model */
	fpga_curve_scalar_multiply(px, py, k, &rx, &ry);

		/* handle result */
	ok = compare_fpga_buffers(qx, qy, &rx, &ry);
	if (!ok)
	{	printf("\n    ERROR\n\n");
		return false;
	}
	else printf("\n    OK\n\n");

		// everything went just fine
	return true;
}


//------------------------------------------------------------------------------
bool abuse_point_multiplier(FPGA_BUFFER *qx, FPGA_BUFFER *qy)
//------------------------------------------------------------------------------
//
// This routine tries to abuse the curve point multiplier by triggering the
// rarely used code path where the internal adder has to add two identical
// points.
//
//------------------------------------------------------------------------------
{
	bool ok;	// flag

		// obtain quantity n + 1, n + 2
	FPGA_BUFFER two, n1, n2;
	fpga_modular_add(&ecdsa_one, &ecdsa_one, &two);	// n1 = n + 1
	fpga_modular_add(&ecdsa_n,   &ecdsa_one, &n1);	// n1 = n + 1
	fpga_modular_add(&n1,        &ecdsa_one, &n2);	// n2 = n1 + 1 = n + 2

	printf("Trying to abuse point multiplier...\n\n");

		// make sure, that (n + 1) * Q = Q
	FPGA_BUFFER qn1_x, qn1_y;
	fpga_curve_scalar_multiply(qx, qy, &n1, &qn1_x, &qn1_y);
	ok = compare_fpga_buffers(qx, qy, &qn1_x, &qn1_y);
	if (! ok)
	{	printf("\n    ERROR\n\n");
		return false;
	}
	else printf("\n    OK\n\n");

		// we first calculate 2 * Q
	FPGA_BUFFER q2a_x, q2a_y;
	fpga_curve_scalar_multiply(qx, qy, &two, &q2a_x, &q2a_y);

		// we now calculate (n + 2) * Q
	FPGA_BUFFER q2b_x, q2b_y;
	fpga_curve_scalar_multiply(qx, qy, &n2, &q2b_x, &q2b_y);

		// both calculations should produce the same point (Q2a == Q2b)
	ok = compare_fpga_buffers(&q2a_x, &q2a_y, &q2b_x, &q2b_y);
	if (! ok)
	{	printf("\n    ERROR\n\n");
		return false;
	}
	else printf("\n    OK\n\n");

		// everything went just fine
	return true;
}


//------------------------------------------------------------------------------
bool compare_fpga_buffers(FPGA_BUFFER *ax, FPGA_BUFFER *ay, FPGA_BUFFER *bx, FPGA_BUFFER *by)
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
	printf("\n");
	print_fpga_buffer("  Expected:   Y = ", ay);
	print_fpga_buffer("  Calculated: Y = ", by);

		// compare values
	for (w=0; w<OPERAND_NUM_WORDS; w++)
	{
			// compare x
		if (ax->words[w] != bx->words[w]) return false;

			// compare y
		if (ay->words[w] != by->words[w]) return false;
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
	for (w=OPERAND_NUM_WORDS; w>0; w--)
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
