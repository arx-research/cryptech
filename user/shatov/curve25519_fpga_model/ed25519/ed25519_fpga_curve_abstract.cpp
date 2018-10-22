//------------------------------------------------------------------------------
//
// ed25519_fpga_curve_abstract.cpp
// ------------------------------------------------
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
// - Redistributions in binary form+ must reproduce the above copyright notice,
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


#include <stdio.h>


//------------------------------------------------------------------------------
// Headers
//------------------------------------------------------------------------------
#include "ed25519_fpga_model.h"


//------------------------------------------------------------------------------
// Globals
//------------------------------------------------------------------------------
FPGA_BUFFER ED25519_G_X;	// x-coordinate of the base point
FPGA_BUFFER ED25519_G_Y;	// y-coordinate of the base point
FPGA_BUFFER ED25519_G_Z;	// z-coordinate of the base point
FPGA_BUFFER ED25519_G_T;	// y-coordinate of the base point


//------------------------------------------------------------------------------
void fpga_curve_ed25519_init()
//------------------------------------------------------------------------------
{
	int w_src, w_dst;	// word counters

	FPGA_WORD TMP_G_X[FPGA_OPERAND_NUM_WORDS]	= ED25519_G_X_INIT;
	FPGA_WORD TMP_G_Y[FPGA_OPERAND_NUM_WORDS]	= ED25519_G_Y_INIT;
	FPGA_WORD TMP_G_Z[FPGA_OPERAND_NUM_WORDS]	= ED25519_G_Z_INIT;
	FPGA_WORD TMP_G_T[FPGA_OPERAND_NUM_WORDS]	= ED25519_G_T_INIT;

		/* fill buffers for large multi-word integers */
	for (	w_src = 0, w_dst = FPGA_OPERAND_NUM_WORDS - 1;
			w_src < FPGA_OPERAND_NUM_WORDS;
			w_src++, w_dst--)
	{
		ED25519_G_X.words[w_dst]	= TMP_G_X[w_src];
		ED25519_G_Y.words[w_dst]	= TMP_G_Y[w_src];
		ED25519_G_Z.words[w_dst]	= TMP_G_Z[w_src];
		ED25519_G_T.words[w_dst]	= TMP_G_T[w_src];
	}
}


//------------------------------------------------------------------------------
//
// Elliptic curve base point scalar multiplication routine.
//
// This uses Algorithm 4 ("Joye double-and-add") from "Fast and Regular
// Algorithms for Scalar Multiplication over Elliptic Curves"
// https://eprint.iacr.org/2011/338.pdf
//
//------------------------------------------------------------------------------
void fpga_curve_ed25519_base_scalar_multiply_abstract(const FPGA_BUFFER *K, FPGA_BUFFER *Q_Y)
//------------------------------------------------------------------------------
{
	int word_count, bit_count;	// counters

		// temporary buffers
	FPGA_BUFFER R0_X;
	FPGA_BUFFER R0_Y;
	FPGA_BUFFER R0_Z;
	FPGA_BUFFER R0_T;

	FPGA_BUFFER R1_X;
	FPGA_BUFFER R1_Y;
	FPGA_BUFFER R1_Z;
	FPGA_BUFFER R1_T;

	FPGA_BUFFER T_X;
	FPGA_BUFFER T_Y;
	FPGA_BUFFER T_Z;
	FPGA_BUFFER T_T;

		// initialization
	fpga_multiword_copy(&CURVE25519_ZERO, &R0_X);
	fpga_multiword_copy(&CURVE25519_ONE,  &R0_Y);
	fpga_multiword_copy(&CURVE25519_ONE,  &R0_Z);
	fpga_multiword_copy(&CURVE25519_ZERO, &R0_T);

	fpga_multiword_copy(&ED25519_G_X, &R1_X);
	fpga_multiword_copy(&ED25519_G_Y, &R1_Y);
	fpga_multiword_copy(&ED25519_G_Z, &R1_Z);
	fpga_multiword_copy(&ED25519_G_T, &R1_T);

		// force zero bits
	FPGA_BUFFER K_INT;
	fpga_multiword_copy(K, &K_INT);

	K_INT.words[0] &= 0xFFFFFFF8;
	K_INT.words[FPGA_OPERAND_NUM_WORDS-1] &= 0x3FFFFFFF;
	K_INT.words[FPGA_OPERAND_NUM_WORDS-1] |= 0x40000000;
	

		// handy vars
	FPGA_WORD k_word;
	bool k_bit = false;

		// multiply
	for (word_count=0; word_count<FPGA_OPERAND_NUM_WORDS; word_count++)
	{
		for (bit_count=0; bit_count<FPGA_WORD_WIDTH; bit_count++)
		{
				// get current bit of K
			k_word = K_INT.words[word_count] >> bit_count;
			k_bit = (k_word & (FPGA_WORD)1) == 1;

				// symmetric processing scheme regardless of the current private bit value
			if (k_bit)
			{
				// T = double(R0)
				fpga_curve_ed25519_double(	&R0_X, &R0_Y, &R0_Z, &R0_T,
											&T_X,  &T_Y,  &T_Z,  &T_T);

				// R0 = add(T, R1)
				fpga_curve_ed25519_add(	&T_X,  &T_Y,  &T_Z,  &T_T,
										&R1_X, &R1_Y, &R1_Z, &R1_T,
										&R0_X, &R0_Y, &R0_Z, &R0_T);
			}
			else
			{
				// T = double(R1)
				fpga_curve_ed25519_double(	&R1_X, &R1_Y, &R1_Z, &R1_T,
											&T_X,  &T_Y,  &T_Z,  &T_T);

				// R1 = add(T, R0)
				fpga_curve_ed25519_add(	&T_X,  &T_Y,  &T_Z,  &T_T,
										&R0_X, &R0_Y, &R0_Z, &R0_T,
										&R1_X, &R1_Y, &R1_Z, &R1_T);
			}
		}
	}

		// now conversion to affine coordinates
	fpga_curve_ed25519_to_affine(&R0_X, &R0_Y, &R0_Z, &R1_X, &R1_Y);

		// so far we've done everything modulo 2*P, we now need
		// to do final reduction modulo P, this can be done using
		// our modular adder this way:
	fpga_modular_add(&R1_X, &CURVE25519_ZERO, &R0_X, &CURVE25519_1P);
	fpga_modular_add(&R1_Y, &CURVE25519_ZERO, &R0_Y, &CURVE25519_1P);

		// process "sign" of x, see this Cryptography Stack Exchange
		// answer for more details:
		//
		// https://crypto.stackexchange.com/questions/58921/decoding-a-ed25519-key-per-rfc8032
		//
		// the short story is that odd values of x are negative, so we
		// just copy the lsb of x into msb of y
	R0_Y.words[FPGA_OPERAND_NUM_WORDS-1] |= (R0_X.words[0] & (FPGA_WORD)1) << 31;

		// store result
	fpga_multiword_copy(&R0_Y, Q_Y);
}


//------------------------------------------------------------------------------
//
// Elliptic curve point doubling routine.
//
// This implements the "dbl-2008-hwcd" formulae set from
// https://hyperelliptic.org/EFD/g1p/auto-twisted-extended-1.html
//
// The only difference is that E, F, G and H have opposite signs, this is
// equivalent to the original algorithm, because the end result depends on
// (E * F) and (G * H). If both variables have opposite signs, then the
// sign of the product doesn't change.
//
//------------------------------------------------------------------------------
void fpga_curve_ed25519_double(	const FPGA_BUFFER *P_X, const FPGA_BUFFER *P_Y, const FPGA_BUFFER *P_Z, const FPGA_BUFFER *P_T,
								FPGA_BUFFER *Q_X, FPGA_BUFFER *Q_Y, FPGA_BUFFER *Q_Z, FPGA_BUFFER *Q_T)
{
	FPGA_BUFFER A, B, C, D, E, F, G, H, I;

	fpga_modular_mul(P_X, P_X,  &A, &CURVE25519_2P);	// A  = (qx * qx) % mod
	fpga_modular_mul(P_Y, P_Y,  &B, &CURVE25519_2P);	// B  = (qy * qy) % mod

	fpga_modular_mul(P_Z, P_Z,  &I, &CURVE25519_2P);	// I  = (qz * qz) % mod
	fpga_modular_add( &I,  &I,  &C, &CURVE25519_2P);	// C  = ( I +  I) % mod
	fpga_modular_add(P_X, P_Y,  &I, &CURVE25519_2P);	// I  = (qx + qy) % mod
	fpga_modular_mul( &I,  &I,  &D, &CURVE25519_2P);	// D  = ( I *  I) % mod

	fpga_modular_add( &A,  &B,  &H, &CURVE25519_2P);	// H  = ( A +  B) % mod
	fpga_modular_sub( &H,  &D,  &E, &CURVE25519_2P);	// E  = ( H -  D) % mod
	fpga_modular_sub( &A,  &B,  &G, &CURVE25519_2P);	// G  = ( A -  B) % mod
	fpga_modular_add( &C,  &G,  &F, &CURVE25519_2P);	// F  = ( C +  G) % mod
	
	fpga_modular_mul( &E,  &F, Q_X, &CURVE25519_2P);	// rx = ( E *  F) % mod
	fpga_modular_mul( &G,  &H, Q_Y, &CURVE25519_2P);	// ry = ( G *  H) % mod
	fpga_modular_mul( &E,  &H, Q_T, &CURVE25519_2P);	// rt = ( E *  H) % mod
	fpga_modular_mul( &F,  &G, Q_Z, &CURVE25519_2P);	// rz = ( F *  G) % mod
}


//------------------------------------------------------------------------------
//
// Elliptic curve point addition routine.
//
// This implements the "add-2008-hwcd-4" formulae set from
// https://hyperelliptic.org/EFD/g1p/auto-twisted-extended-1.html
//
//------------------------------------------------------------------------------
void fpga_curve_ed25519_add(	const FPGA_BUFFER *P_X, const FPGA_BUFFER *P_Y, const FPGA_BUFFER *P_Z, const FPGA_BUFFER *P_T,
								const FPGA_BUFFER *Q_X, const FPGA_BUFFER *Q_Y, const FPGA_BUFFER *Q_Z, const FPGA_BUFFER *Q_T,
								FPGA_BUFFER *R_X, FPGA_BUFFER *R_Y, FPGA_BUFFER *R_Z, FPGA_BUFFER *R_T)
{
	FPGA_BUFFER A, B, C, D, E, F, G, H, I, J;

	fpga_modular_sub(P_Y, P_X,  &I, &CURVE25519_2P);	// I = (qy - qx) % mod
	fpga_modular_add(Q_Y, Q_X,  &J, &CURVE25519_2P);	// J = (py + px) % mod
	fpga_modular_mul( &I,  &J,  &A, &CURVE25519_2P);	// A = ( I *  J) % mod

	fpga_modular_add(P_Y, P_X,  &I, &CURVE25519_2P);	// I = (qy + qx) % mod
	fpga_modular_sub(Q_Y, Q_X,  &J, &CURVE25519_2P);	// J = (py - px) % mod
	fpga_modular_mul( &I,  &J,  &B, &CURVE25519_2P);	// B = ( I *  J) % mod

	fpga_modular_mul(P_Z, Q_T,  &I, &CURVE25519_2P);	// I = (qz * pt) % mod
	fpga_modular_add( &I,  &I,  &C, &CURVE25519_2P);	// C = ( I +  I) % mod
	fpga_modular_mul(P_T, Q_Z,  &I, &CURVE25519_2P);	// I = (qt * pz) % mod
	fpga_modular_add( &I,  &I,  &D, &CURVE25519_2P);	// D = ( I + I) % mod

	fpga_modular_add( &D,  &C,  &E, &CURVE25519_2P);	// E = (D + C) % mod
	fpga_modular_sub( &B,  &A,  &F, &CURVE25519_2P);	// F = (B - A) % mod
	fpga_modular_add( &B,  &A,  &G, &CURVE25519_2P);	// G = (B + A) % mod
	fpga_modular_sub( &D,  &C,  &H, &CURVE25519_2P);	// H = (D - C) % mod

	fpga_modular_mul( &E,  &F, R_X, &CURVE25519_2P);	// rx = (E * F) % mod
	fpga_modular_mul( &G,  &H, R_Y, &CURVE25519_2P);	// ry = (G * H) % mod
	fpga_modular_mul( &E,  &H, R_T, &CURVE25519_2P);	// rt = (E * H) % mod
	fpga_modular_mul( &F,  &G, R_Z, &CURVE25519_2P);	// rz = (F * G) % mod	
}


//------------------------------------------------------------------------------
//
// Conversion to affine coordinates.
//
// Q_X = P_X / P_Z = P_X * P_Z ^ -1
// Q_Y = P_Y / P_Z = P_Y * P_Z ^ -1
//
//------------------------------------------------------------------------------
void	fpga_curve_ed25519_to_affine	(const FPGA_BUFFER *P_X, const FPGA_BUFFER *P_Y,
										 const FPGA_BUFFER *P_Z,
										 FPGA_BUFFER *Q_X, FPGA_BUFFER *Q_Y)
//------------------------------------------------------------------------------
{
	FPGA_BUFFER P_Z_1;

	fpga_modular_inv_abstract(P_Z, &P_Z_1, &CURVE25519_2P);

	fpga_modular_mul(P_X, &P_Z_1, Q_X, &CURVE25519_2P);
	fpga_modular_mul(P_Y, &P_Z_1, Q_Y, &CURVE25519_2P);
}


//------------------------------------------------------------------------------
// End-of-File
//------------------------------------------------------------------------------
