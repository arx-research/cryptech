//------------------------------------------------------------------------------
//
// x25519_fpga_modular.cpp
// ------------------------------------------
// Modular arithmetic routines for Curve25519
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
#include "x25519_fpga_model.h"


//------------------------------------------------------------------------------
// Globals
//------------------------------------------------------------------------------
FPGA_BUFFER X25519_1P;
FPGA_BUFFER X25519_2P;


//------------------------------------------------------------------------------
void fpga_modular_init()
//------------------------------------------------------------------------------
{
	int w_src, w_dst;	// word counters

		// temporary things
	FPGA_WORD TMP_1P[FPGA_OPERAND_NUM_WORDS]	= X25519_1P_INIT;
	FPGA_WORD TMP_2P[FPGA_OPERAND_NUM_WORDS]	= X25519_2P_INIT;

		/* fill buffers for large multi-word integers, we need to fill them in
		 * reverse order because of the way C arrays are initialized
		 */
	for (	w_src = 0, w_dst = FPGA_OPERAND_NUM_WORDS - 1;
			w_src < FPGA_OPERAND_NUM_WORDS;
			w_src++, w_dst--)
	{
		X25519_1P.words[w_dst]	= TMP_1P[w_src];
		X25519_2P.words[w_dst]	= TMP_2P[w_src];
	}
}


//------------------------------------------------------------------------------
//
// Modular addition.
//
// This routine implements algorithm 3. from "Ultra High Performance ECC over
// NIST Primes on Commercial FPGAs".
//
// s = (a + b) mod q
//
// The naive approach is like the following:
//
// 1. s = a + b
// 2. if (s >= q) s -= q
//
// The speed-up trick is to simultaneously calculate (a + b) and (a + b - q)
// and then select the right variant.
//
//------------------------------------------------------------------------------
void fpga_modular_add(FPGA_BUFFER *A, FPGA_BUFFER *B, FPGA_BUFFER *S, FPGA_BUFFER *N)
//------------------------------------------------------------------------------
{
	int w;					// word counter
	FPGA_BUFFER AB, AB_N;	// intermediate buffers
	bool c_in, c_out;		// carries
	bool b_in, b_out;		// borrows

	c_in = false;			// first word has no carry
	b_in = false;			// first word has no borrow
	
		// run parallel addition and subtraction
	for (w=0; w<FPGA_OPERAND_NUM_WORDS; w++)
	{
		fpga_lowlevel_add32(A->words[w], B->words[w], c_in, &AB.words[w],   &c_out);
		fpga_lowlevel_sub32(AB.words[w], N->words[w], b_in, &AB_N.words[w], &b_out);

		c_in = c_out;	// propagate carry
		b_in = b_out;	// propagate borrow
	}

		// now select the right buffer

		/*
		 * We select the right variant based on borrow and carry flags after
		 * addition and subtraction of the very last pair of words. Note, that
		 * we only need to select the first variant (a + b) when (a + b) < q.
		 * This way if we get negative number after subtraction, we discard it
		 * and use the output of the adder instead. The subtractor output is
		 * negative when borrow flag is set *and* carry flag is not set. When
		 * both borrow and carry are set, the number is non-negative, because
		 * borrow and carry cancel each other out.
		 */
	for (w=0; w<FPGA_OPERAND_NUM_WORDS; w++)
		S->words[w] = (b_out && !c_out) ? AB.words[w] : AB_N.words[w];
}


//------------------------------------------------------------------------------
//
// Modular subtraction.
//
// This routine implements algorithm 3. from "Ultra High Performance ECC over
// NIST Primes on Commercial FPGAs".
//
// d = (a - b) mod q
//
// The naive approach is like the following:
//
// 1. d = a - b
// 2. if (a < b) d += q
//
// The speed-up trick is to simultaneously calculate (a - b) and (a - b + q)
// and then select the right variant.
//
//------------------------------------------------------------------------------
void fpga_modular_sub(FPGA_BUFFER *A, FPGA_BUFFER *B, FPGA_BUFFER *D, FPGA_BUFFER *N)
//------------------------------------------------------------------------------
{
	int w;					// word counter
	FPGA_BUFFER AB, AB_N;	// intermediate buffers
	bool c_in, c_out;		// carries
	bool b_in, b_out;		// borrows

	c_in = false;			// first word has no carry
	b_in = false;			// first word has no borrow
	
		// run parallel subtraction and addition
	for (w=0; w<FPGA_OPERAND_NUM_WORDS; w++)
	{
		fpga_lowlevel_sub32(A->words[w], B->words[w], b_in, &AB.words[w],   &b_out);
		fpga_lowlevel_add32(AB.words[w], N->words[w], c_in, &AB_N.words[w], &c_out);

		b_in = b_out;	// propagate borrow
		c_in = c_out;	// propagate carry
	}

		// now select the right buffer

		/*
		 * We select the right variant based on borrow flag after subtraction
		 * and addition of the very last pair of words. Note, that we only
		 * need to select the second variant (a - b + q) when a < b. This way
		 * if we get negative number after subtraction, we discard it
		 * and use the output of the adder instead. The Subtractor output is
		 * negative when borrow flag is set.
		 */
	for (w=0; w<FPGA_OPERAND_NUM_WORDS; w++)
		D->words[w] = b_out ? AB_N.words[w] : AB.words[w];
}


//------------------------------------------------------------------------------
//
// Modular multiplication for Curve25519.
//
// p = (a * b) mod q
//
// The complex algorithm is split into three parts:
//
// 1. Calculation of partial words
// 2. Acccumulation of partial words into full-size product
// 3. Modular reduction of the full-size product
//
// See comments for corresponding helper routines for more information.
//
//------------------------------------------------------------------------------
void fpga_modular_mul(FPGA_BUFFER *A, FPGA_BUFFER *B, FPGA_BUFFER *P, FPGA_BUFFER *N)
//------------------------------------------------------------------------------
{
	FPGA_WORD_EXTENDED SI[4*FPGA_OPERAND_NUM_WORDS-1];	// parts of intermediate product
	FPGA_WORD C[2*FPGA_OPERAND_NUM_WORDS];				// full-size intermediate product

		/* multiply to get partial words */
	fpga_modular_mul_helper_multiply(A, B, SI);

		/* accumulate partial words into full-size product */
	fpga_modular_mul_helper_accumulate(SI, C);

		/* reduce full-size product using special routine */
	fpga_modular_mul_helper_reduce(C, P, N);
}


//------------------------------------------------------------------------------
//
// Modular reduction for Curve25519.
//
// Note, that this routine reduces 512-bit product modulo 2*P, i.e.
// 2 * (2^255 -19) = 2^256 - 19. It is computationally more effective to not
// fully reduce the result until the very end of X25519 calculation.
//
// See the "Special Reduction" section of "High-Performance Modular
// Multiplication on the Cell Processor" by Joppe W. Bos for more information
// about the math behind reduction: http://joppebos.com/files/waifi09.pdf
//
//------------------------------------------------------------------------------
void fpga_modular_mul_helper_reduce(FPGA_WORD *C, FPGA_BUFFER *P, FPGA_BUFFER *N)
//------------------------------------------------------------------------------
{
	int w;	// word counter

		// handy vars
	FPGA_WORD y;
	FPGA_WORD x1_msb, x1_lsb;
	FPGA_WORD x2_msb, x2_lsb;
	FPGA_WORD x5_msb, x5_lsb;
	FPGA_WORD n_word;

		// S1 is 262-bit result after the first reduction attempt
		// S2 is 257-bit result after the second reduction attempt
	FPGA_WORD S1[FPGA_OPERAND_NUM_WORDS + 1];
	FPGA_WORD S2[FPGA_OPERAND_NUM_WORDS + 1];

		// carries during the first and the second stages
	FPGA_WORD_REDUCED t_carry1;
	FPGA_WORD_EXTENDED t_carry2;

		// borrows for the final stage
	bool b_in, b_out;

		// temporary result of the final stage
	FPGA_WORD S2_N[FPGA_OPERAND_NUM_WORDS + 1];

		// outputs of adders
	FPGA_WORD_EXTENDED T1[FPGA_OPERAND_NUM_WORDS + 1];
	FPGA_WORD_EXTENDED T2[FPGA_OPERAND_NUM_WORDS + 1];
	FPGA_WORD_EXTENDED T3[FPGA_OPERAND_NUM_WORDS + 1];
	FPGA_WORD_EXTENDED T4[FPGA_OPERAND_NUM_WORDS + 1];

		// parts of full input product
	FPGA_BUFFER P_LO, P_HI;

		// split 512-bit input C into two 256-bit parts
	for (w=0; w<FPGA_OPERAND_NUM_WORDS; w++)
	{
		P_LO.words[w] = C[w];
		P_HI.words[w] = C[FPGA_OPERAND_NUM_WORDS + w];
	}

		/* We need to calculate S1 = P_HI * 38 + P_LO, this is done using
		 * additions instead of multiplications, because our low-level
		 * multiplier can only process 16 bits at a time, while an adder
		 * can do 47. This is done by replacing 38 with 32 + 4 + 2 = 
		 * 2^5 + 2^2 + 2^1, this way:
		 * S1 = P_LO + (P_HI << 5) + (P_HI << 2) + (P_HI << 1)
		 */

		/* for every word we need to calculate a sum of five values: three
		 * shifted copies of P_HI[w], P_LO[w] and carry from the previous word.
		 * This is done using four adders in a pipelined fashion.
		 */

	t_carry1 = 0;	// no carry in the very first word
	for (w=0; w<(FPGA_OPERAND_NUM_WORDS+1); w++)
	{
			// upper parts of shifted copies of P_HI[w-i] (from the previous cycle)
		x1_msb = (w > 0) ? (FPGA_WORD)(P_HI.words[w-1] >> (32 - 1)) : 0;
		x2_msb = (w > 0) ? (FPGA_WORD)(P_HI.words[w-1] >> (32 - 2)) : 0;
		x5_msb = (w > 0) ? (FPGA_WORD)(P_HI.words[w-1] >> (32 - 5)) : 0;

			// lower parts of shifted copies of P_HI[w]
		x1_lsb = (w < FPGA_OPERAND_NUM_WORDS) ? (FPGA_WORD)(P_HI.words[w] << 1) : 0;
		x2_lsb = (w < FPGA_OPERAND_NUM_WORDS) ? (FPGA_WORD)(P_HI.words[w] << 2) : 0;
		x5_lsb = (w < FPGA_OPERAND_NUM_WORDS) ? (FPGA_WORD)(P_HI.words[w] << 5) : 0;

			// take care of uppers parts from the previous cycle
		x1_lsb |= x1_msb;
		x2_lsb |= x2_msb;
		x5_lsb |= x5_msb;

			// current word of P_LO
		y = (w < FPGA_OPERAND_NUM_WORDS) ? P_LO.words[w] : 0;

			// run addition
		fpga_lowlevel_add47(x1_lsb, x2_lsb,   &T1[w]);	// T1 obtained after clock cycle w
		fpga_lowlevel_add47(x5_lsb, y,        &T2[w]);	// T2 obtained after clock cycle w
		fpga_lowlevel_add47(T1[w],  T2[w],    &T3[w]);	// T3 obtained after clock cycle w+1 (when T1 and T2 are available)
		fpga_lowlevel_add47(T3[w],  t_carry1, &T4[w]);	// T4 obtained after clock cycle w+2 (when T3 is available)

			// store carry
		t_carry1 = (FPGA_WORD_REDUCED)(T4[w] >> FPGA_WORD_WIDTH);

			// store word of sum
		S1[w] = (FPGA_WORD)T4[w];
	}

		/* now repeat what we've just done once again with S1, but this time S1_HI
		 * is 6-bit wide at most, so we can calculate S1_HI * 38 beforehand,
		 * add it to the lowest word of S1_LO and then propagate the carry
		 */

	t_carry2 = 0;
	t_carry2 += (FPGA_WORD_EXTENDED)S1[FPGA_OPERAND_NUM_WORDS] << 1;
	t_carry2 += (FPGA_WORD_EXTENDED)S1[FPGA_OPERAND_NUM_WORDS] << 2;
	t_carry2 += (FPGA_WORD_EXTENDED)S1[FPGA_OPERAND_NUM_WORDS] << 5;

	for (w=0; w<(FPGA_OPERAND_NUM_WORDS+1); w++)
	{
			// current word of S1_LO
		y = (w < FPGA_OPERAND_NUM_WORDS) ? S1[w] : 0;

			// do addition
		fpga_lowlevel_add47(t_carry2, y, &T1[w]);

			// store carry
		t_carry2 = (FPGA_WORD_REDUCED)(T1[w] >> FPGA_WORD_WIDTH);

			// store word of sum
		S2[w] = (FPGA_WORD)T1[w];
	}

		/* So we've ended up with 257-bit result in S2. Note that there can only be
		 * two situations, given our modulus N is 2^256 - 38:
		 *
		 * a) 0 <= S < N
		 * b) N <= S < 2*N
		 *
		 * This is because we've obtained S2 by adding 256-bit quantity S1_LO and
		 * 12-bit quantity 38 * S_HI. S1_LO is at most 2^256 - 1 or N + 37, while
		 * the 12-bit quantity is at most 4095, so the largest possible value of
		 * S2 is N + 4132, which is obviously less than 2*N.
		 *
		 * What we now do is we try subtracting N from S2 to obtain S2_N, if we end up
		 * with a negative number, we return S2 (reduction was not necessary), otherwise
		 * we return S2_N.
		 */

	b_in = false;	// first word has no borrow
	
		// run parallel subtraction
	for (w=0; w<=FPGA_OPERAND_NUM_WORDS; w++)
	{
			// current word of N
		n_word = (w < FPGA_OPERAND_NUM_WORDS) ? N->words[w] : 0;

			// do subtraction
		fpga_lowlevel_sub32(S2[w], n_word, b_in, &S2_N[w], &b_out);

			// propagate borrow
		b_in = b_out;
	}
	
		// copy result to output buffer
	for (w=0; w<FPGA_OPERAND_NUM_WORDS; w++)
	{
			// if subtraction of the highest words produced carry, we ended up
			// with a negative number and S2 must be returned, not S2_N
		P->words[w] = b_out ? S2[w] : S2_N[w];
	}

}


//------------------------------------------------------------------------------
//
// Parallelized multiplication.
//
// This routine implements the algorithm in Fig. 3. from "Ultra High
// Performance ECC over NIST Primes on Commercial FPGAs".
//
// Inputs A and B are split into 2*OPERAND_NUM_WORDS words of FPGA_WORD_WIDTH/2
// bits each, because FPGA multipliers can't handle full FPGA_WORD_WIDTH-wide
// inputs. These smaller words are multiplied by an array of 2*OPERAND_NUM_WORDS
// multiplers and accumulated into an array of 4*OPERAND_NUM_WORDS-1 partial
// output words SI[].
//
// The order of loading A and B into the multipliers is a bit complicated,
// during the first 2*OPERAND_NUM_WORDS-1 cycles one SI word per cycle is
// obtained, during the very last 2*OPERAND_NUM_WORDS'th cycle all the
// remaining 2*OPERAND_NUM_WORDS partial words are obtained simultaneously.
//
//------------------------------------------------------------------------------
void fpga_modular_mul_helper_multiply(FPGA_BUFFER *A, FPGA_BUFFER *B, FPGA_WORD_EXTENDED *SI)
//------------------------------------------------------------------------------
{
	int w;			// counter
	int t, x;		// more counters
	int j, i;		// word indices
	FPGA_WORD p;	// product

		// buffers for smaller words that multipliers can handle
	FPGA_WORD_REDUCED AI[2*FPGA_OPERAND_NUM_WORDS];
	FPGA_WORD_REDUCED BJ[2*FPGA_OPERAND_NUM_WORDS];
	
		// split a and b into smaller words
	for (w=0; w<FPGA_OPERAND_NUM_WORDS; w++)
		AI[2*w] = (FPGA_WORD_REDUCED)A->words[w], AI[2*w + 1] = (FPGA_WORD_REDUCED)(A->words[w] >> (FPGA_WORD_WIDTH / 2)),
		BJ[2*w] = (FPGA_WORD_REDUCED)B->words[w], BJ[2*w + 1] = (FPGA_WORD_REDUCED)(B->words[w] >> (FPGA_WORD_WIDTH / 2));

		// accumulators
	FPGA_WORD_EXTENDED mac[2*FPGA_OPERAND_NUM_WORDS];
	
		// clear accumulators
	for (w=0; w<(2*FPGA_OPERAND_NUM_WORDS); w++) mac[w] = 0;

		// run the crazy algorithm :)
	for (t=0; t<(2*FPGA_OPERAND_NUM_WORDS); t++)
	{
			// save upper half of si[] (one word per cycle)
		if (t > 0)
		{	SI[4*FPGA_OPERAND_NUM_WORDS - (t+1)] = mac[t];
			mac[t] = 0;
		}

			// update index
		j = 2*FPGA_OPERAND_NUM_WORDS - (t+1);

			// parallel multiplication
		for (x=0; x<(2*FPGA_OPERAND_NUM_WORDS); x++)
		{
				// update index
			i = t - x;
			if (i < 0) i += 2*FPGA_OPERAND_NUM_WORDS;

				// multiply...
			fpga_lowlevel_mul16(AI[i], BJ[j], &p);

				// ...accumulate
			mac[x] += p;
		}

	}

		// now finally save lower half of SI[] (2*OPERAND_NUM_WORDS words at once)
	for (w=0; w<(2*FPGA_OPERAND_NUM_WORDS); w++)
		SI[w] = mac[2*FPGA_OPERAND_NUM_WORDS - (w+1)];
}


//------------------------------------------------------------------------------
//
// Accumulation of partial words into full-size product.
//
// This routine implements the Algorithm 4. from "Ultra High Performance ECC
// over NIST Primes on Commercial FPGAs".
//
// Input words SI[] are accumulated into full-size product C[].
//
// The algorithm is a bit tricky, there are 4*OPERAND_NUM_WORDS-1 words in
// SI[]. Complete operation takes 2*OPERAND_NUM_WORDS cycles, even words are
// summed in full, odd words are split into two parts. During every cycle the
// upper part of the previous word and the lower part of the following word are
// summed too.
//
//------------------------------------------------------------------------------
void fpga_modular_mul_helper_accumulate(FPGA_WORD_EXTENDED *SI, FPGA_WORD *C)
//------------------------------------------------------------------------------
{
	int w;							// word counter
	FPGA_WORD_EXTENDED cw0, cw1;	// intermediate sums
	FPGA_WORD_REDUCED  cw_carry;	// wide carry

		// clear carry
	cw_carry = 0;

		// execute the algorithm
	for (w=0; w<(2*FPGA_OPERAND_NUM_WORDS); w++)
	{
			// handy flags
		bool w_is_first = (w == 0);
		bool w_is_last  = (w == (2*FPGA_OPERAND_NUM_WORDS-1));

			// accumulate full current even word...
			// ...and also the upper part of the previous odd word (if not the first word)
		fpga_lowlevel_add47(SI[2*w], w_is_first ? 0 : SI[2*w-1] >> (FPGA_WORD_WIDTH / 2), &cw0);

			// generate another word from "carry" part of the previous even word...
			// ...and also the lower part of the following odd word (if not the last word)
		cw1 = w_is_last ? 0 : (FPGA_WORD)(SI[2*w+1] << (FPGA_WORD_WIDTH / 2));
		cw1 |= (FPGA_WORD_EXTENDED)cw_carry;

			// accumulate once again
		fpga_lowlevel_add47(cw0, cw1, &cw1);

			// store current word...
		C[w] = (FPGA_WORD)cw1;

			// ...and carry
		cw_carry = (FPGA_WORD_REDUCED) (cw1 >> FPGA_WORD_WIDTH);
	}
}


//------------------------------------------------------------------------------
// End-of-File
//------------------------------------------------------------------------------
