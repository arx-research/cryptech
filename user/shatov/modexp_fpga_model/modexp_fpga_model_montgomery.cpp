//
// modexp_fpga_model_montgomery.cpp
// -------------------------------------------------------------
// Montgomery modular multiplication and exponentiation routines
//
// Authors: Pavel Shatov
// Copyright (c) 2017, NORDUnet A/S
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// - Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
//
// - Neither the name of the NORDUnet nor the names of its contributors may
//   be used to endorse or promote products derived from this software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//


//----------------------------------------------------------------
// Headers
//----------------------------------------------------------------
#include "modexp_fpga_model.h"
#include "modexp_fpga_model_pe.h"
#include "modexp_fpga_systolic.h"
#include "modexp_fpga_model_montgomery.h"


//----------------------------------------------------------------
// Montgomery modular multiplier
//----------------------------------------------------------------
void montgomery_multiply(const FPGA_WORD *A, const FPGA_WORD *B, const FPGA_WORD *N, const FPGA_WORD *N_COEFF, FPGA_WORD *R, size_t len, bool reduce_only)
//----------------------------------------------------------------
//
// R = A * B * 2^-len mod N
//
// The high-level algorithm is:
//
// 1. AB =  A * B
// 2. Q  = AB * N_COEFF
// 3. QN =  Q * N
// 4. S  = AB + QN
// 5. SN =  S - N
// 6. R  = (SN < 0) ? S : SN
// 7. R  = R >> len
//
//----------------------------------------------------------------
{
	size_t i;												// counters

	FPGA_WORD AB[2 * MAX_OPERAND_WORDS];					// products
	FPGA_WORD Q [    MAX_OPERAND_WORDS];					//
	FPGA_WORD QN[2 * MAX_OPERAND_WORDS];					//

	bool select_s;											// flag

	FPGA_WORD c_in_s;										// 1-bit carry and borrow
	FPGA_WORD b_in_sn;										//
	FPGA_WORD c_out_s;										//
	FPGA_WORD b_out_sn;										//

	FPGA_WORD S [2 * MAX_OPERAND_WORDS];					// final sum
	FPGA_WORD SN[2 * MAX_OPERAND_WORDS];					// final difference

		// copy twice larger A into AB
	if (reduce_only)
		for (i=0; i<(2*len); i++)
			AB[i] = A[i];

	if (!reduce_only)	multiply_systolic(A,       B,  AB, len, 2 * len);		// AB = A  * B
						multiply_systolic(N_COEFF, AB, Q,  len,     len);		// Q  = AB * N_COEFF
						multiply_systolic(Q,       N,  QN, len, 2 * len);		// QN = Q * N

		// initialize 1-bit carry and borrow
	c_in_s = 0, b_in_sn = 0;

		// now it's time to simultaneously add and subtract
	for (i = 0; i < (2 * len); i++)
	{	
			// current operand words
		FPGA_WORD QNi = QN[i];
		FPGA_WORD Ni  = (i < len) ? 0 : N[i-len];

			// add, subtract
		pe_add(AB[i], QNi, c_in_s,  &S[i],  &c_out_s);
		pe_sub(S [i], Ni,  b_in_sn, &SN[i], &b_out_sn);

			// propagate carry and borrow
		c_in_s  = c_out_s;
		b_in_sn = b_out_sn;
	}

		// flag select the right result
	select_s = b_out_sn && !c_out_s;

		// copy product into output buffer
	for (i=0; i<len; i++)
		R[i] = select_s ? S[i+len] : SN[i+len];
}


//----------------------------------------------------------------
// Classic binary exponentiation
//----------------------------------------------------------------
void montgomery_exponentiate(const FPGA_WORD *A, const FPGA_WORD *B, const FPGA_WORD *N, const FPGA_WORD *N_COEFF, FPGA_WORD *R, size_t len)
//----------------------------------------------------------------
//
// R = A ** B mod N
//
//----------------------------------------------------------------
{
	size_t word_cnt,   bit_cnt;			// counters
	size_t word_index, bit_index;		// indices

	bool flag_update_r;					// flag

	FPGA_WORD T0[MAX_OPERAND_WORDS];	//
	FPGA_WORD T1[MAX_OPERAND_WORDS];	//
	FPGA_WORD T2[MAX_OPERAND_WORDS];	//

	FPGA_WORD P1[MAX_OPERAND_WORDS];	//
	FPGA_WORD P2[MAX_OPERAND_WORDS];	//
	FPGA_WORD P3[MAX_OPERAND_WORDS];	//

	FPGA_WORD mask;						//

		// R = 1, P = 1
	for (word_cnt=0; word_cnt<len; word_cnt++)
		T1[word_cnt] = (word_cnt > 0) ? 0 : 1,
		T2[word_cnt] = (word_cnt > 0) ? 0 : 1,
		P1[word_cnt] = A[word_cnt],
		P2[word_cnt] = A[word_cnt],
		P3[word_cnt] = A[word_cnt];

	FPGA_WORD PP[MAX_OPERAND_WORDS];	// intermediate buffer for next power
	FPGA_WORD TP[MAX_OPERAND_WORDS];	// intermediate buffer for next result

		// scan all bits of the exponent
	for (bit_cnt=0; bit_cnt<(len * CHAR_BIT * sizeof(FPGA_WORD)); bit_cnt++)
	{
		for (word_cnt=0; word_cnt<len; word_cnt++)
			T0[word_cnt] = T1[word_cnt] ^ POWER_MASK;

		montgomery_multiply(P1, P2, N, N_COEFF, PP, len, false);	// PP = P1 * P2
		montgomery_multiply(T2, P3, N, N_COEFF, TP, len, false);	// TP =  T * P3
		
		word_index = bit_cnt / (CHAR_BIT * sizeof(FPGA_WORD));
		bit_index = bit_cnt & ((CHAR_BIT * sizeof(FPGA_WORD)) - 1);

		mask = 1 << bit_index;	// current bit of exponent

			// whether we need to update R (non-zero exponent bit)
		flag_update_r = (B[word_index] & mask) == mask;

			// always update P
		for (word_cnt=0; word_cnt<len; word_cnt++)
			P1[word_cnt] = PP[word_cnt],
			P2[word_cnt] = PP[word_cnt],
			P3[word_cnt] = PP[word_cnt];

			// update T
		for (word_cnt=0; word_cnt<len; word_cnt++)
			T1[word_cnt] = flag_update_r ? TP[word_cnt] : T0[word_cnt] ^ POWER_MASK,
			T2[word_cnt] = flag_update_r ? TP[word_cnt] : T0[word_cnt] ^ POWER_MASK;
	}

		// store result
	for (word_cnt=0; word_cnt<len; word_cnt++)
		R[word_cnt] = T1[word_cnt];
}


//----------------------------------------------------------------
// Montgomery factor calculation
//----------------------------------------------------------------
void montgomery_calc_factor(const FPGA_WORD *N, FPGA_WORD *FACTOR, size_t len)
//----------------------------------------------------------------
//
// FACTOR = 2 ** (2*len) mod N
//
// This routine calculates the factor that is necessary to bring
// numbers into Montgomery domain. The high-level algorithm is:
//
// 1. f = 1
// 2. for i=0 to 2*len-1
// 3.   f1 = f << 1
// 4.   f2 = f1 - n
// 5.   f = (f2 < 0) ? f1 : f2
//
//----------------------------------------------------------------
{
	size_t i, j;		// counters

	bool flag_keep_f;	// flag
	
		// temporary buffer
	FPGA_WORD FACTOR_N[MAX_OPERAND_WORDS];

		// carry and borrow
	FPGA_WORD carry_in, carry_out;
	FPGA_WORD borrow_in, borrow_out;

		// FACTOR = 1
	for (i=0; i<len; i++)
		FACTOR[i] = (i > 0) ? 0 : 1;
		
		// do the math
	for (i=0; i<(2 * len * CHAR_BIT * sizeof(FPGA_WORD)); i++)
	{
			// clear carry and borrow
		carry_in = 0, borrow_in = 0;
		
			// calculate f1 = f << 1, f2 = f1 - n
		for (j=0; j<len; j++)
		{
			carry_out = FACTOR[j] >> (sizeof(FPGA_WORD) * CHAR_BIT - 1);		// | M <<= 1
			FACTOR[j] <<= 1, FACTOR[j] |= carry_in;								// |

			pe_sub(FACTOR[j], N[j], borrow_in, &FACTOR_N[j], &borrow_out);		// MN = M - N
	
			carry_in = carry_out, borrow_in = borrow_out;						// propagate carry & borrow
		}

			// obtain flag
		flag_keep_f = (borrow_out && !carry_out);

			// now select the right value
		for (j=0; j<len; j++)
			FACTOR[j] = flag_keep_f ? FACTOR[j] : FACTOR_N[j];
	}

}


//----------------------------------------------------------------
// Montgomery modulus-dependent coefficient calculation
//----------------------------------------------------------------
void montgomery_calc_n_coeff(const FPGA_WORD *N, FPGA_WORD *N_COEFF, size_t len)
//----------------------------------------------------------------
//
// N_COEFF = -N ** -1 mod 2 ** len
//
// This routine calculates the coefficient that is used during the reduction
// phase of Montgomery multiplication to zero out the lower half of product.
//
// The high-level algorithm is:
//
// 1. R = 1
// 2. B = 1
// 3. NN = ~N + 1
// 4. for i=1 to len-1
// 5.   B = B << 1
// 6.   T = R * NN mod 2 ** len
// 7.   if T[i] then
// 8.     R = R + B
//
//----------------------------------------------------------------
{
	size_t i, j, k;							// counters

	FPGA_WORD NN[MAX_OPERAND_WORDS];		// NN = ~N + 1
	FPGA_WORD T [MAX_OPERAND_WORDS];		// T = R * NN
	FPGA_WORD R [MAX_OPERAND_WORDS];		// R
	FPGA_WORD B [MAX_OPERAND_WORDS];		// B
	FPGA_WORD RR[MAX_OPERAND_WORDS];		// RR = R
	FPGA_WORD RB[MAX_OPERAND_WORDS];		// RB = R + B

	bool flag_update_r;						// flag

	FPGA_WORD nw;							//
	FPGA_WORD sum_c_in, sum_c_out;			//
	FPGA_WORD shift_c_in, shift_c_out;		//
	FPGA_WORD mul_s, mul_c_in, mul_c_out;	//

		// NN = -N mod 2 ** len = ~N + 1 mod 2 ** len
	sum_c_in = 0;
	for (i=0; i<len; i++)
	{	nw = (i > 0) ? 0 : 1;								// NW = 1
		pe_add(~N[i], nw, sum_c_in, &NN[i], &sum_c_out);	// NN = ~N + nw
		sum_c_in = sum_c_out;								// propagate carry
	}

		// R = 1
		// B = 1
	for (i=0; i<len; i++)
		R[i] = (i > 0) ? 0 : 1,
		B[i] = (i > 0) ? 0 : 1;

		// calculate T = R * NN
		// calculate B = B << 1
		// calculate RB = R + B
	for (k=1; k<(len * sizeof(FPGA_WORD) * CHAR_BIT); k++)
	{
			// T = 0
		for (i=0; i<len; i++) T[i] = 0;

			// T = NN * R
		for (i=0; i<len; i++)
		{
				// reset adder and shifter carries
			if (i == 0)
			{	shift_c_in = 0;
				sum_c_in = 0;
			}
				
				// reset multiplier carry
			mul_c_in = 0;

				// get word and index indices
			size_t word_index = k / (CHAR_BIT * sizeof(FPGA_WORD));
			size_t bit_index = k & ((CHAR_BIT * sizeof(FPGA_WORD)) - 1);

				// update bit mask
			FPGA_WORD bit_mask = (1 << bit_index);

				// main calculation loop
			for (j=0; j<(len-i); j++)
			{
					// B = B << 1
					// RB = R + B
				if (i == 0)
				{	shift_c_out = B[j] >> (sizeof(FPGA_WORD) * CHAR_BIT - 1);
					B[j] <<= 1, B[j] |= shift_c_in;
					pe_add(R[j], B[j], sum_c_in,  &RB[j], &sum_c_out);
				}

					// RR = R
				if (i == 0)
					RR[j] = R[j];
				
					// T = R * NN
				pe_mul(R[j], NN[i], T[i+j], mul_c_in, &mul_s, &mul_c_out);				
				T[i+j] = mul_s;
				
					// update flag
				if ((i + j) == word_index)
					flag_update_r = (T[i+j] & (1 << bit_index)) == (1 << bit_index);

					// propagate adder and shifter carries
				if (i == 0)
				{	shift_c_in = shift_c_out;
					sum_c_in = sum_c_out;
				}

					// propagate multiplier carry
				mul_c_in = mul_c_out;
			}
		}

			// update r
		for (i=0; i<len; i++)
			R[i] = flag_update_r ? RB[i] : RR[i];
	}

		// store output
	for (i=0; i<len; i++)
		N_COEFF[i] = R[i];
}


//----------------------------------------------------------------
// End of file
//----------------------------------------------------------------
