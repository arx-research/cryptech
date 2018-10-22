//------------------------------------------------------------------------------
//
// fpga_curve.cpp
// ------------------------------------
// Elliptic curve arithmetic procedures
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
#include <stdint.h>
#include "ecdh_fpga_model.h"
#include "fpga_lowlevel.h"
#include "fpga_modular.h"
#include "fpga_curve.h"
#include "fpga_util.h"


//------------------------------------------------------------------------------
// Globals
//------------------------------------------------------------------------------
FPGA_BUFFER ecdsa_n;
FPGA_BUFFER ecdsa_g_x, ecdsa_g_y;
FPGA_BUFFER ecdh_d_x,  ecdh_d_y;
FPGA_BUFFER ecdh_da,   ecdh_db;
FPGA_BUFFER ecdh_qa_x, ecdh_qa_y;
FPGA_BUFFER ecdh_qb_x, ecdh_qb_y;
FPGA_BUFFER ecdh_s_x,  ecdh_s_y;


//------------------------------------------------------------------------------
void fpga_curve_init()
//------------------------------------------------------------------------------
{
	int w;	// word counter

	FPGA_BUFFER tmp_n = ECDSA_N;
	FPGA_BUFFER tmp_g_x  = ECDSA_G_X, tmp_g_y  = ECDSA_G_Y;
	FPGA_BUFFER tmp_da   = ECDH_DA,   tmp_db   = ECDH_DB;
	FPGA_BUFFER tmp_qa_x = ECDH_QA_X, tmp_qa_y = ECDH_QA_Y;
	FPGA_BUFFER tmp_qb_x = ECDH_QB_X, tmp_qb_y = ECDH_QB_Y;
	FPGA_BUFFER tmp_s_x  = ECDH_S_X,  tmp_s_y =  ECDH_S_Y;

		/* fill buffers for large multi-word integers */
	for (w=0; w<OPERAND_NUM_WORDS; w++)
	{	ecdsa_n.words[w]	= tmp_n.words[OPERAND_NUM_WORDS - (w+1)];
		ecdsa_g_x.words[w]	= tmp_g_x.words[OPERAND_NUM_WORDS - (w+1)];
		ecdsa_g_y.words[w]	= tmp_g_y.words[OPERAND_NUM_WORDS - (w+1)];
		ecdh_da.words[w]    = tmp_da.words[OPERAND_NUM_WORDS - (w+1)];
		ecdh_db.words[w]    = tmp_db.words[OPERAND_NUM_WORDS - (w+1)];
		ecdh_qa_x.words[w]	= tmp_qa_x.words[OPERAND_NUM_WORDS - (w+1)];
		ecdh_qa_y.words[w]	= tmp_qa_y.words[OPERAND_NUM_WORDS - (w+1)];
		ecdh_qb_x.words[w]	= tmp_qb_x.words[OPERAND_NUM_WORDS - (w+1)];
		ecdh_qb_y.words[w]	= tmp_qb_y.words[OPERAND_NUM_WORDS - (w+1)];
		ecdh_s_x.words[w]   = tmp_s_x.words[OPERAND_NUM_WORDS - (w+1)];
		ecdh_s_y.words[w]   = tmp_s_y.words[OPERAND_NUM_WORDS - (w+1)];
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
// R(rx, ry, rz) will be in projective Jacobian coordinates.
//
// This routine implements algorithm 3.22 from "Guide to Elliptic Curve
// Cryptography". Differences from the original algorithm:
// 
// 1) Step 1. is omitted, because the user-supplied point Q is supposed
//    to be not at infinity.
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
void fpga_curve_add_jacobian(FPGA_BUFFER *px, FPGA_BUFFER *py, FPGA_BUFFER *pz, FPGA_BUFFER *qx, FPGA_BUFFER *qy, FPGA_BUFFER *rx, FPGA_BUFFER *ry, FPGA_BUFFER *rz)
//------------------------------------------------------------------------------
{
	FPGA_BUFFER t1, t2, t3, t4;		// temporary variables
	
	bool pz_is_zero = fpga_buffer_is_zero(pz);		// Step 2.

	/*  3. */ fpga_buffer_copy(pz,  &t1);
	          fpga_modular_mul(pz,  &t1, &t1);
	/*  4. */ fpga_modular_mul(pz,  &t1, &t2);
	/*  5. */ fpga_modular_mul(&t1, qx,  &t1);
	/*  6. */ fpga_modular_mul(&t2, qy,  &t2);
	/*  7. */ fpga_modular_sub(&t1, px,  &t1);
	/*  8. */ fpga_modular_sub(&t2, py,  &t2);

	bool t1_is_zero = fpga_buffer_is_zero(&t1);		// | Step 9.
	bool t2_is_zero = fpga_buffer_is_zero(&t2);		// |

	/* 10. */ fpga_modular_mul(pz,  &t1, rz);
	/* 11. */ fpga_buffer_copy(&t1, &t3);
	          fpga_modular_mul(&t1, &t3, &t3);
	/* 12. */ fpga_modular_mul(&t1, &t3, &t4);
	/* 13. */ fpga_modular_mul(px,  &t3, &t3);
	/* 14. */ fpga_modular_add(&t3, &t3, &t1);
	/* 15. */ fpga_buffer_copy(&t2, rx);
	          fpga_modular_mul(rx,  &t2, rx);
	/* 16. */ fpga_modular_sub(rx,  &t1, rx);
	/* 17. */ fpga_modular_sub(rx,  &t4, rx);
	/* 18. */ fpga_modular_sub(&t3, rx,  &t3);
	/* 19. */ fpga_modular_mul(&t2, &t3, &t3);
	/* 20. */ fpga_modular_mul(py,  &t4, &t4);
	/* 21. */ fpga_modular_sub(&t3, &t4, ry);

		//
		// final selection
		//
	if (pz_is_zero)	// P at infinity ?
	{	fpga_buffer_copy(qx, rx);
		fpga_buffer_copy(qy, ry);
		fpga_buffer_copy(&ecdsa_one, rz);
	}
	else if (t1_is_zero) // same x for P and Q ?
	{
		fpga_buffer_copy(t2_is_zero ? &ecdh_d_x  : &ecdsa_one,  rx);	// | same y ? (P==Q => R=2*G) : (P==-Q => R=O)
		fpga_buffer_copy(t2_is_zero ? &ecdh_d_y  : &ecdsa_one,  ry);	// |
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
// Elliptic curve point scalar multiplication routine.
//
// Q(qx,qy) = k * P(px,py)
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
void fpga_curve_scalar_multiply(FPGA_BUFFER *px, FPGA_BUFFER *py, FPGA_BUFFER *k, FPGA_BUFFER *qx, FPGA_BUFFER *qy)
//------------------------------------------------------------------------------
{
	int word_count, bit_count;	// counters

	FPGA_BUFFER rx, ry, rz;		// intermediate result
	FPGA_BUFFER tx, ty, tz;		// temporary variable

		/* prepare for computation */
	fpga_buffer_copy(px,         &rx);
	fpga_buffer_copy(py,         &ry);
	fpga_buffer_copy(&ecdsa_one, &rz);

		/* obtain quantity 2 * P */
	fpga_curve_double_jacobian(&rx, &ry, &rz, &tx, &ty, &tz);

		/* copy again */
	fpga_buffer_copy(&tx, &rx);
	fpga_buffer_copy(&ty, &ry);
	fpga_buffer_copy(&tz, &rz);

		/* convert to affine coordinates */
	fpga_curve_point_to_affine(&rx, &ry, &rz, qx, qy);

		/* store for later reuse */
	fpga_buffer_copy(qx, &ecdh_d_x);
	fpga_buffer_copy(qy, &ecdh_d_y);

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
			fpga_curve_add_jacobian(&tx, &ty, &tz, px, py, &rx, &ry, &rz);

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
