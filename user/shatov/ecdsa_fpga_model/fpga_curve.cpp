//------------------------------------------------------------------------------
//
// fpga_curve.cpp
// ------------------------------------
// Elliptic curve arithmetic procedures
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
#include <stdint.h>
#include "ecdsa_model.h"
#include "fpga_lowlevel.h"
#include "fpga_modular.h"
#include "fpga_curve.h"
#include "fpga_util.h"


//------------------------------------------------------------------------------
// Globals
//------------------------------------------------------------------------------
FPGA_BUFFER ecdsa_g_x, ecdsa_g_y;
FPGA_BUFFER ecdsa_h_x, ecdsa_h_y;
FPGA_BUFFER ecdsa_q_x, ecdsa_q_y;
FPGA_BUFFER ecdsa_r_x, ecdsa_r_y;


//------------------------------------------------------------------------------
void fpga_curve_init()
//------------------------------------------------------------------------------
{
	int w;	// word counter

	FPGA_BUFFER tmp_g_x = ECDSA_G_X, tmp_g_y = ECDSA_G_Y;
	FPGA_BUFFER tmp_h_x = ECDSA_H_X, tmp_h_y = ECDSA_H_Y;
	FPGA_BUFFER tmp_q_x = ECDSA_Q_X, tmp_q_y = ECDSA_Q_Y;
	FPGA_BUFFER tmp_r_x = ECDSA_R_X, tmp_r_y = ECDSA_R_Y;

		/* fill buffers for large multi-word integers */
	for (w=0; w<OPERAND_NUM_WORDS; w++)
	{	ecdsa_g_x.words[w]	= tmp_g_x.words[OPERAND_NUM_WORDS - (w+1)];
		ecdsa_g_y.words[w]	= tmp_g_y.words[OPERAND_NUM_WORDS - (w+1)];
		ecdsa_h_x.words[w]	= tmp_h_x.words[OPERAND_NUM_WORDS - (w+1)];
		ecdsa_h_y.words[w]	= tmp_h_y.words[OPERAND_NUM_WORDS - (w+1)];
		ecdsa_r_x.words[w]	= tmp_r_x.words[OPERAND_NUM_WORDS - (w+1)];
		ecdsa_r_y.words[w]	= tmp_r_y.words[OPERAND_NUM_WORDS - (w+1)];
		ecdsa_q_x.words[w]	= tmp_q_x.words[OPERAND_NUM_WORDS - (w+1)];
		ecdsa_q_y.words[w]	= tmp_q_y.words[OPERAND_NUM_WORDS - (w+1)];
	}
}


//------------------------------------------------------------------------------
//
// Elliptic curve point doubling routine.
//
// R(rx,ry,rz) = 2 * P(px,py,pz)
//
// Note, that P(px,py,pz) is supposed to be in projective Jacobian coordinates,
// R will be in projective Jacobian coordinates.
//
// This routine implements algorithm 3.21 from "Guide to Elliptic Curve
// Cryptography", the only difference is that step 6. does T1 = T2 + T2 and
// then T2 = T2 + T1 instead of T2 = 3 * T2, because our addition is much
// faster, than multiplication.
//
// Note, that this routine also handles one special case, namely when P is at
// infinity.
//
// Instead of actual modular division, multiplication by pre-computed constant
// (2^-1 mod q) is done.
//
// Note, that FPGA modular multiplier can't multiply a given buffer by itself,
// this way it's impossible to do eg. fpga_modular_mul(pz, pz, &t1). To overcome
// the problem the algorithm was modified to do fpga_buffer_copy(pz, &t1) and
// then fpga_modular_mul(pz, &t1, &t1) instead.
//
// WARNING: Though this procedure always does doubling steps, it does not take
// any active measures to keep run-time constant. The main purpose of this
// model is to help debug Verilog code for FPGA, so *DO NOT* use is anywhere
// near production!
//
//------------------------------------------------------------------------------
void fpga_curve_double_jacobian(FPGA_BUFFER *px, FPGA_BUFFER *py, FPGA_BUFFER *pz, FPGA_BUFFER *rx, FPGA_BUFFER *ry, FPGA_BUFFER *rz)
//------------------------------------------------------------------------------
{
	FPGA_BUFFER t1, t2, t3;	// temporary variables

		// check, whether P is at infinity
	bool pz_is_zero = fpga_buffer_is_zero(pz);

	/*  2. */ fpga_buffer_copy(pz,  &t1);
              fpga_modular_mul(pz,  &t1,          &t1);
	/*  3. */ fpga_modular_sub(px,  &t1,          &t2);
	/*  4. */ fpga_modular_add(px,  &t1,          &t1);
	/*  5. */ fpga_modular_mul(&t1, &t2,          &t2);
	/*  6. */ fpga_modular_add(&t2, &t2,          &t1);
	/*     */ fpga_modular_add(&t1, &t2,          &t2);
	/*  7. */ fpga_modular_add(py,  py,           ry);
	/*  8. */ fpga_modular_mul(pz,  ry,           rz);
	/*  9. */ fpga_buffer_copy(ry,  &t1);
	          fpga_buffer_copy(ry,  &t3);
	          fpga_modular_mul(&t1, &t3,          ry);
	/* 10. */ fpga_modular_mul(px,  ry,           &t3);
	/* 11. */ fpga_buffer_copy(ry,  &t1);
	          fpga_modular_mul(ry,  &t1,          &t1);
	/* 12. */ fpga_modular_mul(&t1, &ecdsa_delta, ry);
	/* 13. */ fpga_buffer_copy(&t2, &t1);
	          fpga_modular_mul(&t1, &t2,          rx);
	/* 14. */ fpga_modular_add(&t3, &t3,          &t1);
	/* 15. */ fpga_modular_sub(rx,  &t1,          rx);
	/* 16. */ fpga_modular_sub(&t3, rx,           &t1);	
	/* 17. */ fpga_modular_mul(&t1, &t2,          &t1);
	/* 18. */ fpga_modular_sub(&t1, ry,           ry);	

		// handle special case (input point is at infinity)
	if (pz_is_zero)
	{	fpga_buffer_copy(&ecdsa_one,  rx);
		fpga_buffer_copy(&ecdsa_one,  ry);
		fpga_buffer_copy(&ecdsa_zero, rz);
	}
}


//------------------------------------------------------------------------------
//
// Elliptic curve point addition routine.
//
// R(rx,ry,rz) = P(px,py,pz) + Q(qx,qy)
//
// Note, that P(px, py, pz) is supposed to be in projective Jacobian
// coordinates, while Q(qx,qy) is supposed to be in affine coordinates,
// R(rx, ry, rz) will be in projective Jacobian coordinates. Moreover, in this
// particular implementation Q is always the base point G.
//
// This routine implements algorithm 3.22 from "Guide to Elliptic Curve
// Cryptography". Differences from the original algorithm:
// 
// 1) Step 1. is omitted, because point Q is always the base point, which is
//    not at infinity by definition.
//
// 2) Step 9.1 just returns the pre-computed double of the base point instead
// of actually doubling it.
//
// Note, that this routine also handles three special cases:
//
// 1) P is at infinity
// 2) P == Q
// 3) P == -Q
//
// Note, that FPGA modular multiplier can't multiply a given buffer by itself,
// this way it's impossible to do eg. fpga_modular_mul(pz, pz, &t1). To overcome
// the problem the algorithm was modified to do fpga_buffer_copy(pz, &t1) and
// then fpga_modular_mul(pz, &t1, &t1) instead.
//
// WARNING: This procedure does not take any active measures to keep run-time
// constant. The main purpose of this model is to help debug Verilog code for
// FPGA, so *DO NOT* use is anywhere near production!
//
//------------------------------------------------------------------------------
void fpga_curve_add_jacobian(FPGA_BUFFER *px, FPGA_BUFFER *py, FPGA_BUFFER *pz, FPGA_BUFFER *rx, FPGA_BUFFER *ry, FPGA_BUFFER *rz)
//------------------------------------------------------------------------------
{
	FPGA_BUFFER t1, t2, t3, t4;		// temporary variables
	
	bool pz_is_zero = fpga_buffer_is_zero(pz);		// Step 2.

	/*  3. */ fpga_buffer_copy(pz,  &t1);
	          fpga_modular_mul(pz,  &t1,        &t1);
	/*  4. */ fpga_modular_mul(pz,  &t1,        &t2);
	/*  5. */ fpga_modular_mul(&t1, &ecdsa_g_x, &t1);
	/*  6. */ fpga_modular_mul(&t2, &ecdsa_g_y, &t2);
	/*  7. */ fpga_modular_sub(&t1, px,         &t1);
	/*  8. */ fpga_modular_sub(&t2, py,         &t2);

	bool t1_is_zero = fpga_buffer_is_zero(&t1);		// | Step 9.
	bool t2_is_zero = fpga_buffer_is_zero(&t2);		// |

	/* 10. */ fpga_modular_mul(pz,  &t1,        rz);
	/* 11. */ fpga_buffer_copy(&t1, &t3);
	          fpga_modular_mul(&t1, &t3,        &t3);
	/* 12. */ fpga_modular_mul(&t1, &t3,        &t4);
	/* 13. */ fpga_modular_mul(px,  &t3,        &t3);
	/* 14. */ fpga_modular_add(&t3, &t3,        &t1);
	/* 15. */ fpga_buffer_copy(&t2, rx);
	          fpga_modular_mul(rx,  &t2,        rx);
	/* 16. */ fpga_modular_sub(rx,  &t1,        rx);
	/* 17. */ fpga_modular_sub(rx,  &t4,        rx);
	/* 18. */ fpga_modular_sub(&t3, rx,         &t3);
	/* 19. */ fpga_modular_mul(&t2, &t3,        &t3);
	/* 20. */ fpga_modular_mul(py,  &t4,        &t4);
	/* 21. */ fpga_modular_sub(&t3, &t4,        ry);

		//
		// final selection
		//
	if (pz_is_zero)	// P at infinity ?
	{	fpga_buffer_copy(&ecdsa_g_x, rx);
		fpga_buffer_copy(&ecdsa_g_y, ry);
		fpga_buffer_copy(&ecdsa_one, rz);
	}
	else if (t1_is_zero) // same x for P and Q ?
	{
		fpga_buffer_copy(t2_is_zero ? &ecdsa_h_x : &ecdsa_one,  rx);	// | same y ? (P==Q => R=2*G) : (P==-Q => R=O)
		fpga_buffer_copy(t2_is_zero ? &ecdsa_h_y : &ecdsa_one,  ry);	// |
		fpga_buffer_copy(t2_is_zero ? &ecdsa_one : &ecdsa_zero, rz);	// |
	}
}


//------------------------------------------------------------------------------
//
// Conversion from projective Jacobian to affine coordinates.
//
// P(px,py,pz) -> Q(qx,qy)
//
// Note, that qx = px / Z^2 and qy = py / Z^3. Division in modular arithmetic
// is equivalent to multiplication by the inverse value of divisor, so
// qx = px * (pz^-1)^2 and qy = py * (pz^-1)^3.
//
// Note, that this procedure does *NOT* handle points at infinity correctly. It
// can only be called from the base point multiplication routine, that
// specifically makes sure that P is not at infinity, so pz will always be
// non-zero value.
//
//------------------------------------------------------------------------------
void fpga_curve_point_to_affine(FPGA_BUFFER *px, FPGA_BUFFER *py, FPGA_BUFFER *pz, FPGA_BUFFER *qx, FPGA_BUFFER *qy)
//------------------------------------------------------------------------------
{
	FPGA_BUFFER pz1;					// inverse value of pz
	FPGA_BUFFER t2, t3;					// intermediate values

	fpga_modular_inv(pz, &pz1);			// pz1 = pz^-1 (mod q)

	fpga_modular_mul(&pz1, &pz1, &t2);	// t2 = pz1 ^ 2 (mod q)
	fpga_modular_mul(&pz1, &t2,  &t3);	// t3 = tz1 ^ 3 (mod q)

	fpga_modular_mul(px, &t2, qx);		// qx = px * (pz^-1)^2 (mod q)
	fpga_modular_mul(py, &t3, qy);		// qy = py * (pz^-1)^3 (mod q)
}


//------------------------------------------------------------------------------
//
// Elliptic curve base point scalar multiplication routine.
//
// Q(qx,qy) = k * G(px,py)
//
// Note, that Q is supposed to be in affine coordinates. Multiplication is done
// using the double-and-add algorithm 3.27 from "Guide to Elliptic Curve
// Cryptography".
//
// WARNING: Though this procedure always does the addition step, it only
// updates the result when current bit of k is set. It does not take any
// active measures to keep run-time constant. The main purpose of this model
// is to help debug Verilog code for FPGA, so *DO NOT* use it anywhere near
// production!
//
//------------------------------------------------------------------------------
void fpga_curve_scalar_multiply(FPGA_BUFFER *k, FPGA_BUFFER *qx, FPGA_BUFFER *qy)
//------------------------------------------------------------------------------
{
	int word_count, bit_count;	// counters

	FPGA_BUFFER rx, ry, rz;		// intermediate result
	FPGA_BUFFER tx, ty, tz;		// temporary variable

		/* set initial value of R to point at infinity */
	fpga_buffer_copy(&ecdsa_one,  &rx);
	fpga_buffer_copy(&ecdsa_one,  &ry);
	fpga_buffer_copy(&ecdsa_zero, &rz);

		/* process bits of k left-to-right */
	for (word_count=OPERAND_NUM_WORDS; word_count>0; word_count--)
		for (bit_count=FPGA_WORD_WIDTH; bit_count>0; bit_count--)
		{
				/* calculate T = 2 * R */
			fpga_curve_double_jacobian(&rx, &ry, &rz, &tx, &ty, &tz);

				/* always calculate R = T + P for constant-time */
			fpga_curve_add_jacobian(&tx, &ty, &tz, &rx, &ry, &rz);

				/* revert to the value of T before addition if the current bit of k is not set */
			if (!((k->words[word_count-1] >> (bit_count-1)) & 1))
			{	fpga_buffer_copy(&tx, &rx);
				fpga_buffer_copy(&ty, &ry);
				fpga_buffer_copy(&tz, &rz);
			}

		}

		// convert result to affine coordinates anyway
	fpga_curve_point_to_affine(&rx, &ry, &rz, qx, qy);

		// check, that rz is non-zero (not point at infinity)
	bool rz_is_zero = fpga_buffer_is_zero(&rz);

		// handle special case (result is point at infinity)
	if (rz_is_zero)
	{	fpga_buffer_copy(&ecdsa_zero, qx);
		fpga_buffer_copy(&ecdsa_zero, qy);
	}
}


//------------------------------------------------------------------------------
// End-of-File
//------------------------------------------------------------------------------
