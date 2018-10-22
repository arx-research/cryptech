//------------------------------------------------------------------------------
//
// ecdsa_model_fpga.cpp
// --------------------------------------------
// Base point scalar multiplier model for ECDSA
//
// Authors: Pavel Shatov
//
// Copyright (c) 2015-2016, NORDUnet A/S
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
#include "ecdsa_model.h"
#include "fpga_lowlevel.h"
#include "fpga_modular.h"
#include "fpga_curve.h"
#include "fpga_util.h"


//------------------------------------------------------------------------------
// Prototypes
//------------------------------------------------------------------------------
void	fpga_model_init					();
bool	test_base_point_multiplier		(FPGA_BUFFER *k, FPGA_BUFFER *qx, FPGA_BUFFER *qy);

bool	abuse_internal_point_adder		();
bool	abuse_internal_point_doubler	();

void	print_fpga_buffer				(const char *s, FPGA_BUFFER *v);

bool	compare_fpga_buffers			(FPGA_BUFFER *ax, FPGA_BUFFER *ay, FPGA_BUFFER *bx, FPGA_BUFFER *by);
bool	compare_fpga_buffers			(FPGA_BUFFER *ax, FPGA_BUFFER *ay, FPGA_BUFFER *az, FPGA_BUFFER *bx, FPGA_BUFFER *by, FPGA_BUFFER *bz);


//------------------------------------------------------------------------------
// Locals
//------------------------------------------------------------------------------
static FPGA_BUFFER ecdsa_d;
static FPGA_BUFFER ecdsa_k;
static FPGA_BUFFER ecdsa_n;


//------------------------------------------------------------------------------
int main()
//------------------------------------------------------------------------------
{
	bool	ok;		// flag

		//
		// initialize buffers
		//
	fpga_model_init();
	fpga_modular_init();
	fpga_curve_init();

	
		//
		// test base point multiplier: Q = d * G
		//
	printf("Trying to derive public key from private key...\n\n");
	ok = test_base_point_multiplier(&ecdsa_d, &ecdsa_q_x, &ecdsa_q_y);
	if (!ok) return EXIT_FAILURE;


		//
		// test base point multiplier: R = k * G
		//
	printf("Trying to sign something...\n\n");
	ok = test_base_point_multiplier(&ecdsa_k, &ecdsa_r_x, &ecdsa_r_y);
	if (!ok) return EXIT_FAILURE;


		//
		// test base point multiplier: O = n * G
		//
	printf("Trying to multiply the base point by its order...\n\n");
	ok = test_base_point_multiplier(&ecdsa_n, &ecdsa_zero, &ecdsa_zero);
	if (!ok) return EXIT_FAILURE;


		//
		// try to abuse internal point doubler
		//
	ok = abuse_internal_point_doubler();
	if (!ok) return EXIT_FAILURE;
	

		//
		// try to abuse internal point adder
		//
	ok = abuse_internal_point_adder();
	if (!ok) return EXIT_FAILURE;
	

		//
		// everything went just fine
		//
	return EXIT_SUCCESS;
}


//------------------------------------------------------------------------------
void fpga_model_init()
//------------------------------------------------------------------------------
{
	int w;	// word counter

	FPGA_BUFFER tmp_d = ECDSA_D;
	FPGA_BUFFER tmp_k = ECDSA_K;
	FPGA_BUFFER tmp_n = ECDSA_N;

		/* fill buffers for large multi-word integers */
	for (w=0; w<OPERAND_NUM_WORDS; w++)
	{	ecdsa_d.words[w] = tmp_d.words[OPERAND_NUM_WORDS - (w+1)];
		ecdsa_k.words[w] = tmp_k.words[OPERAND_NUM_WORDS - (w+1)];
		ecdsa_n.words[w] = tmp_n.words[OPERAND_NUM_WORDS - (w+1)];
	}
}


//------------------------------------------------------------------------------
bool test_base_point_multiplier(FPGA_BUFFER *k, FPGA_BUFFER *qx, FPGA_BUFFER *qy)
//------------------------------------------------------------------------------
//
// k - multiplier
//
// qx, qy - expected coordinates of product
//
// Returns true when point (rx,ry) = k * G matches the point (qx,qy).
//
//------------------------------------------------------------------------------
{
	bool ok;				// flag
	FPGA_BUFFER rx, ry;		// result

		/* run the model */
	fpga_curve_scalar_multiply(k, &rx, &ry);

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
bool abuse_internal_point_doubler()
//------------------------------------------------------------------------------
//
// This routine tries to abuse the internal curve point doubler by forcing it
// to double point at infinity.
//
//------------------------------------------------------------------------------
{
	int w;		// word counter
	bool ok;	// flag
	
	FPGA_BUFFER px, py, pz;		// input
	FPGA_BUFFER qx, qy, qz;		// output

		// set P.X and P.Y to some "random" garbage and P.Z to zero
	for (w=0; w<OPERAND_NUM_WORDS; w++)
	{	px.words[w] = ecdsa_g_x.words[w] ^ ecdsa_h_x.words[w];
		py.words[w] = ecdsa_g_y.words[w] ^ ecdsa_h_y.words[w];
	}
	fpga_buffer_copy(&ecdsa_zero, &pz);

		// try to double point at infinity (should produce point at infinity)
	printf("Trying to double something at infinity...\n\n");
	fpga_curve_double_jacobian(&px, &py, &pz, &qx, &qy, &qz);

		// handle result
	ok = compare_fpga_buffers(&ecdsa_one, &ecdsa_one, &ecdsa_zero, &qx, &qy, &qz);
	if (! ok)
	{	printf("\n    ERROR\n\n");
		return false;
	}
	else printf("\n    OK\n\n");

		// everything went just fine
	return true;
}


//------------------------------------------------------------------------------
bool abuse_internal_point_adder()
//------------------------------------------------------------------------------
//
// This routine tries to abuse the internal curve point adder by forcing it to
// go throgh all the possible "corner cases".
//
//------------------------------------------------------------------------------
{
	int w;		// word counter	
	bool ok;	// flag

	FPGA_BUFFER px, py, pz;		// input
	FPGA_BUFFER rx, ry, rz;		// output

	//
	// try to add point at infinity to the base point
	//
	{
			// set P.X and P.Y to some "random" garbage and P.Z to zero
		for (w=0; w<OPERAND_NUM_WORDS; w++)
		{	px.words[w] = ecdsa_g_x.words[w] ^ ecdsa_h_x.words[w];
			py.words[w] = ecdsa_g_y.words[w] ^ ecdsa_h_y.words[w];
		}
		fpga_buffer_copy(&ecdsa_zero, &pz);

			// run addition proceduce
		printf("Trying to add something at infinity to the base point...\n\n");
		fpga_curve_add_jacobian(&px, &py, &pz, &rx, &ry, &rz);

			// handle result
		ok = compare_fpga_buffers(&ecdsa_g_x, &ecdsa_g_y, &ecdsa_one, &rx, &ry, &rz);
		if (! ok)
		{	printf("\n    ERROR\n\n");
			return false;
		}
		else printf("\n    OK\n\n");
	}

	//
	// try to add the base point to itself
	//
	{
			// set P to G
		fpga_buffer_copy(&ecdsa_g_x, &px);
		fpga_buffer_copy(&ecdsa_g_y, &py);
		fpga_buffer_copy(&ecdsa_one, &pz);

			// run addition proceduce
		printf("Trying to add the base point to itself...\n\n");
		fpga_curve_add_jacobian(&px, &py, &pz, &rx, &ry, &rz);

			// handle result
		ok = compare_fpga_buffers(&ecdsa_h_x, &ecdsa_h_y, &ecdsa_one, &rx, &ry, &rz);
		if (! ok)
		{	printf("\n    ERROR\n\n");
			return false;
		}
		else printf("\n    OK\n\n");
	}

	//
	// try to add the base point to its opposite
	//
	{
			// set P to (G.X, -G.Y, 1)
		fpga_buffer_copy(&ecdsa_zero, &px);
		fpga_buffer_copy(&ecdsa_zero, &py);
		fpga_buffer_copy(&ecdsa_one,  &pz);

		fpga_modular_add(&ecdsa_zero, &ecdsa_g_x, &px);
		fpga_modular_sub(&ecdsa_zero, &ecdsa_g_y, &py);

			// run addition proceduce
		printf("Trying to add the base point to its opposite...\n\n");
		fpga_curve_add_jacobian(&px, &py, &pz, &rx, &ry, &rz);

			// handle result
		ok = compare_fpga_buffers(&ecdsa_one, &ecdsa_one, &ecdsa_zero, &rx, &ry, &rz);
		if (! ok)
		{	printf("\n    ERROR\n\n");
			return false;
		}
		else printf("\n    OK\n\n");
	}

		// everything went just fine
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
bool compare_fpga_buffers(FPGA_BUFFER *ax, FPGA_BUFFER *ay, FPGA_BUFFER *az, FPGA_BUFFER *bx, FPGA_BUFFER *by, FPGA_BUFFER *bz)
//------------------------------------------------------------------------------
//
// Compare projective coordinates of two points and return true when they match.
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
	printf("\n");
	print_fpga_buffer("  Expected:   Z = ", az);
	print_fpga_buffer("  Calculated: Z = ", bz);

		// compare values
	for (w=0; w<OPERAND_NUM_WORDS; w++)
	{
			// compare x
		if (ax->words[w] != bx->words[w]) return false;

			// compare y
		if (ay->words[w] != by->words[w]) return false;

			// compare z
		if (az->words[w] != bz->words[w]) return false;
	}

		// values are the same
	return true;
}


//------------------------------------------------------------------------------
// End-of-File
//------------------------------------------------------------------------------
