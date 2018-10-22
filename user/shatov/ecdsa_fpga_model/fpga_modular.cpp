//------------------------------------------------------------------------------
//
// fpga_modular.cpp
// ---------------------------
// Modular arithmetic routines
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


//------------------------------------------------------------------------------
// Globals
//------------------------------------------------------------------------------
FPGA_BUFFER ecdsa_q;
FPGA_BUFFER ecdsa_zero;
FPGA_BUFFER ecdsa_one;
FPGA_BUFFER ecdsa_delta;


//------------------------------------------------------------------------------
void fpga_modular_init()
//------------------------------------------------------------------------------
{
	int w;	// word counter

	FPGA_BUFFER tmp_q		= ECDSA_Q;
	FPGA_BUFFER tmp_zero	= ECDSA_ZERO;
	FPGA_BUFFER tmp_one		= ECDSA_ONE;
	FPGA_BUFFER tmp_delta	= ECDSA_DELTA;

		/* fill buffers for large multi-word integers */
	for (w=0; w<OPERAND_NUM_WORDS; w++)
	{	ecdsa_q.words[w]		= tmp_q.words[OPERAND_NUM_WORDS - (w+1)];
		ecdsa_zero.words[w]		= tmp_zero.words[OPERAND_NUM_WORDS - (w+1)];
		ecdsa_one.words[w]		= tmp_one.words[OPERAND_NUM_WORDS - (w+1)];
		ecdsa_delta.words[w]	= tmp_delta.words[OPERAND_NUM_WORDS - (w+1)];
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
void fpga_modular_add(FPGA_BUFFER *a, FPGA_BUFFER *b, FPGA_BUFFER *s)
//------------------------------------------------------------------------------
{
	int w;					// word counter
	FPGA_BUFFER ab, ab_n;	// intermediate buffers
	bool c_in, c_out;		// carries
	bool b_in, b_out;		// borrows

	c_in = false;			// first word has no carry
	b_in = false;			// first word has no borrow
	
		// run parallel addition and subtraction
	for (w=0; w<OPERAND_NUM_WORDS; w++)
	{
		fpga_lowlevel_add32(a->words[w], b->words[w],      c_in, &ab.words[w],   &c_out);
		fpga_lowlevel_sub32(ab.words[w], ecdsa_q.words[w], b_in, &ab_n.words[w], &b_out);

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
	for (w=0; w<OPERAND_NUM_WORDS; w++)
		s->words[w] = (b_out && !c_out) ? ab.words[w] : ab_n.words[w];
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
void fpga_modular_sub(FPGA_BUFFER *a, FPGA_BUFFER *b, FPGA_BUFFER *d)
//------------------------------------------------------------------------------
{
	int w;					// word counter
	FPGA_BUFFER ab, ab_n;	// intermediate buffers
	bool c_in, c_out;		// carries
	bool b_in, b_out;		// borrows

	c_in = false;			// first word has no carry
	b_in = false;			// first word has no borrow
	
		// run parallel subtraction and addition
	for (w=0; w<OPERAND_NUM_WORDS; w++)
	{
		fpga_lowlevel_sub32(a->words[w], b->words[w],          b_in, &ab.words[w],   &b_out);
		fpga_lowlevel_add32(ab.words[w], ecdsa_q.words[w], c_in, &ab_n.words[w], &c_out);

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
	for (w=0; w<OPERAND_NUM_WORDS; w++)
		d->words[w] = b_out ? ab_n.words[w] : ab.words[w];
}


//------------------------------------------------------------------------------
//
// Modular multiplication.
//
// This routine implements modular multiplication algorithm from "Ultra High
// Performance ECC over NIST Primes on Commercial FPGAs".
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
void fpga_modular_mul(FPGA_BUFFER *a, FPGA_BUFFER *b, FPGA_BUFFER *p)
//------------------------------------------------------------------------------
{
	FPGA_WORD_EXTENDED si[4*OPERAND_NUM_WORDS-1];	// parts of intermediate product
	FPGA_WORD c[2*OPERAND_NUM_WORDS];				// full-size intermediate product

		/* multiply to get partial words */
	fpga_modular_mul_helper_multiply(a, b, si);

		/* accumulate partial words into full-size product */
	fpga_modular_mul_helper_accumulate(si, c);

		/* reduce full-size product using special routine */
	fpga_modular_mul_helper_reduce(c, p);
}


//------------------------------------------------------------------------------
//
// Modular multiplicative inversion procedure.
//
// a1 = a^-1 (mod n)
//
// This routine implements the algorithm from "Constant Time Modular
// Inversion" by Joppe W. Bos (http://www.joppebos.com/files/CTInversion.pdf)
//
// The algorithm has two phases: 1) calculation of "almost" modular inverse,
// which is a^-1*2^k and 2) removal of redundant factor 2^k.
//
// The first stage has four temporary variables: r, s, u, v; they are updated
// at every iteration. Depending on flags there can be four branches, FPGA
// will pre-calculate all possible values in parallel and then use a multiplexor
// to select the next value. This implementation also calculates all possible
// outcomes. This is done for debugging purposes, *NOT* for constant run-time!
//
// The second part only works with s and k.
//
// Note, that k is a simple counter, and it can't exceed 2*OPERAND_WIDTH.
//
// The complex inversion algorithm uses helper routines. Note, that width of the
// intermediate results can temporarily exceed OPERAND_WIDTH, so all the helper
// routines process OPERAND_NUM_WORDS+1 words.
//
//------------------------------------------------------------------------------
void fpga_modular_inv(FPGA_BUFFER *a, FPGA_BUFFER *a1)
{
	int i;	// round counter
	int w;	// word counter
	int k;	// redundant power of 2

		/* q, 1 */
	FPGA_WORD buf_q[OPERAND_NUM_WORDS+1];
	FPGA_WORD buf_1[OPERAND_NUM_WORDS+1];

		/* r, s */
	FPGA_WORD buf_r[OPERAND_NUM_WORDS+1],        buf_s[OPERAND_NUM_WORDS+1];
	FPGA_WORD buf_r_double[OPERAND_NUM_WORDS+1], buf_s_double[OPERAND_NUM_WORDS+1];
	FPGA_WORD buf_r_new[OPERAND_NUM_WORDS+1],    buf_s_new[OPERAND_NUM_WORDS+1];
	FPGA_WORD buf_r_plus_s[OPERAND_NUM_WORDS+1], buf_s_plus_r[OPERAND_NUM_WORDS+1];

		/* u, v */
	FPGA_WORD buf_u[OPERAND_NUM_WORDS+1],              buf_v[OPERAND_NUM_WORDS+1];
	FPGA_WORD buf_u_half[OPERAND_NUM_WORDS+1],         buf_v_half[OPERAND_NUM_WORDS+1];
	FPGA_WORD buf_u_minus_v[OPERAND_NUM_WORDS+1],      buf_v_minus_u[OPERAND_NUM_WORDS+1];
	FPGA_WORD buf_u_minus_v_half[OPERAND_NUM_WORDS+1], buf_v_minus_u_half[OPERAND_NUM_WORDS+1];
	FPGA_WORD buf_u_new[OPERAND_NUM_WORDS+1],          buf_v_new[OPERAND_NUM_WORDS+1];

		/* comparison */
	int cmp_v_1, cmp_u_v;

		/* clear buffers */
	for (w=0; w<=OPERAND_NUM_WORDS; w++)
		buf_r[w] = 0, buf_s[w] = 0,
		buf_u[w] = 0, buf_v[w] = 0,
		buf_q[w] = 0, buf_1[w] = 0;

		/* initialize q, 1 */
	for (w=0; w<OPERAND_NUM_WORDS; w++)
		buf_q[w] = ecdsa_q.words[w], buf_1[w] = ecdsa_one.words[w];

		/* initialize r, s */
	buf_r[0] = 0, buf_s[0] = 1;

		/* initialize u, v */
	for (w=0; w<OPERAND_NUM_WORDS; w++)
		buf_u[w] = ecdsa_q.words[w], buf_v[w] = a->words[w];

		/* initialize k */
	k = 0;

		/* flags for the first stage */
	bool v_is_1, u_is_greater_than_v, u_is_even, v_is_even;			

		/* first stage */
	for (i=0; i<(2*OPERAND_WIDTH); i++)
	{
			/* pre-calculate all possible values for r and s */
		fpga_modular_inv_helper_shl(buf_r, buf_r_double);			// r_double = 2 * r
		fpga_modular_inv_helper_shl(buf_s, buf_s_double);			// s_double = 2 * s
		fpga_modular_inv_helper_add(buf_r, buf_s, buf_r_plus_s);	// r_plus_s = r + s
		fpga_modular_inv_helper_add(buf_s, buf_r, buf_s_plus_r);	// s_plus_r = s + r

			/* pre-calculate all possible values for u and v */
		fpga_modular_inv_helper_shr(buf_u, buf_u_half);						// u_half = u / 2
		fpga_modular_inv_helper_shr(buf_v, buf_v_half);						// v_half = v / 2
		fpga_modular_inv_helper_sub(buf_u, buf_v, buf_u_minus_v);			// u_minus_v = u - v
		fpga_modular_inv_helper_sub(buf_v, buf_u, buf_v_minus_u);			// v_minus_u = v - u
		fpga_modular_inv_helper_shr(buf_u_minus_v, buf_u_minus_v_half);		// u_minus_v_half = u_minus_v / 2
		fpga_modular_inv_helper_shr(buf_v_minus_u, buf_v_minus_u_half);		// v_minus_u_half = v_minus_u / 2

			/* compare */
		fpga_modular_inv_helper_cmp(buf_v, buf_1, &cmp_v_1);
		fpga_modular_inv_helper_cmp(buf_u, buf_v, &cmp_u_v);

			/* assign flags */
		v_is_1				= (cmp_v_1 == 0);
		u_is_greater_than_v	= (cmp_u_v  > 0);
		u_is_even			= !(buf_u[0] & 1);
		v_is_even			= !(buf_v[0] & 1);

			/* select u */
		if ( u_is_even)					fpga_modular_inv_helper_cpy(buf_u_new, buf_u_half);
		if (!u_is_even &&  v_is_even)	fpga_modular_inv_helper_cpy(buf_u_new, buf_u);
		if (!u_is_even && !v_is_even)	fpga_modular_inv_helper_cpy(buf_u_new, u_is_greater_than_v ? buf_u_minus_v_half : buf_u);

			/* select v */
		if ( u_is_even)					fpga_modular_inv_helper_cpy(buf_v_new, buf_v);
		if (!u_is_even &&  v_is_even)	fpga_modular_inv_helper_cpy(buf_v_new, buf_v_half);
		if (!u_is_even && !v_is_even)	fpga_modular_inv_helper_cpy(buf_v_new, u_is_greater_than_v ? buf_v : buf_v_minus_u_half);

			/* select r */
		if ( u_is_even)					fpga_modular_inv_helper_cpy(buf_r_new, buf_r);
		if (!u_is_even &&  v_is_even)	fpga_modular_inv_helper_cpy(buf_r_new, buf_r_double);
		if (!u_is_even && !v_is_even)	fpga_modular_inv_helper_cpy(buf_r_new, u_is_greater_than_v ? buf_r_plus_s : buf_r_double);

			/* select s */
		if ( u_is_even)					fpga_modular_inv_helper_cpy(buf_s_new, buf_s_double);
		if (!u_is_even &&  v_is_even)	fpga_modular_inv_helper_cpy(buf_s_new, buf_s);
		if (!u_is_even && !v_is_even)	fpga_modular_inv_helper_cpy(buf_s_new, u_is_greater_than_v ? buf_s_double : buf_s_plus_r);

			/* update values */
		if (!v_is_1)
		{	fpga_modular_inv_helper_cpy(buf_u, buf_u_new);
			fpga_modular_inv_helper_cpy(buf_v, buf_v_new);
			fpga_modular_inv_helper_cpy(buf_r, buf_r_new);
			fpga_modular_inv_helper_cpy(buf_s, buf_s_new);
		}

			/* update k */
		if (!v_is_1) k++;
	}

		//
		// Note, that to save FPGA resources, the second stage re-uses buffers
		// used in the first stage.
		//

		/* flags for the second stage */
	bool k_is_0, s_is_odd;

		/* second stage */
	for (i=0; i<(2*OPERAND_WIDTH); i++)
	{
			/* pre-calculate all possible values */
		fpga_modular_inv_helper_shr(buf_s, buf_u);
		fpga_modular_inv_helper_add(buf_s, buf_q, buf_r);
		fpga_modular_inv_helper_shr(buf_r, buf_v);

			//
			// assign flags
			//
		s_is_odd = buf_s[0] & 1;
		k_is_0   = (k == 0);

			//
			// select new values based on flags
			//
		fpga_modular_inv_helper_cpy(buf_s_new, s_is_odd ? buf_v : buf_u);

			/* update s */
		if (! k_is_0)
			fpga_modular_inv_helper_cpy(buf_s, buf_s_new);

			/* update k */
		if (! k_is_0) k--;
	}

		/* done, copy s into output buffer */
	for (w=0; w<OPERAND_NUM_WORDS; w++)
		a1->words[w] = buf_s[w];
}


//------------------------------------------------------------------------------
//
// Parallelized multiplication.
//
// This routine implements the algorithm in Fig. 3. from "Ultra High
// Performance ECC over NIST Primes on Commercial FPGAs".
//
// Inputs a and b are split into 2*OPERAND_NUM_WORDS words of FPGA_WORD_WIDTH/2
// bits each, because FPGA multipliers can't handle full FPGA_WORD_WIDTH-wide
// inputs. These smaller words are multiplied by an array of 2*OPERAND_NUM_WORDS
// multiplers and accumulated into an array of 4*OPERAND_NUM_WORDS-1 partial
// output words si[].
//
// The order of loading a and b into the multipliers is a bit complicated,
// during the first 2*OPERAND_NUM_WORDS-1 cycles one si word per cycle is
// obtained, during the very last 2*OPERAND_NUM_WORDS'th cycle all the
// remaining 2*OPERAND_NUM_WORDS partial words are obtained simultaneously.
//
//------------------------------------------------------------------------------
void fpga_modular_mul_helper_multiply(FPGA_BUFFER *a, FPGA_BUFFER *b, FPGA_WORD_EXTENDED *si)
//------------------------------------------------------------------------------
{
	int w;			// counter
	int t, x;		// more counters
	int j, i;		// word indices
	FPGA_WORD p;	// product

		// buffers for smaller words that multipliers can handle
	FPGA_WORD_REDUCED ai[2*OPERAND_NUM_WORDS];
	FPGA_WORD_REDUCED bj[2*OPERAND_NUM_WORDS];
	
		// split a and b into smaller words
	for (w=0; w<OPERAND_NUM_WORDS; w++)
		ai[2*w] = (FPGA_WORD_REDUCED)a->words[w], ai[2*w + 1] = (FPGA_WORD_REDUCED)(a->words[w] >> (FPGA_WORD_WIDTH / 2)),
		bj[2*w] = (FPGA_WORD_REDUCED)b->words[w], bj[2*w + 1] = (FPGA_WORD_REDUCED)(b->words[w] >> (FPGA_WORD_WIDTH / 2));

		// accumulators
	FPGA_WORD_EXTENDED mac[2*OPERAND_NUM_WORDS];
	
		// clear accumulators
	for (w=0; w<(2*OPERAND_NUM_WORDS); w++) mac[w] = 0;

		// run the crazy algorithm :)
	for (t=0; t<(2*OPERAND_NUM_WORDS); t++)
	{
			// save upper half of si[] (one word per cycle)
		if (t > 0)
		{	si[4*OPERAND_NUM_WORDS - (t+1)] = mac[t];
			mac[t] = 0;
		}

			// update index
		j = 2*OPERAND_NUM_WORDS - (t+1);

			// parallel multiplication
		for (x=0; x<(2*OPERAND_NUM_WORDS); x++)
		{
				// update index
			i = t - x;
			if (i < 0) i += 2*OPERAND_NUM_WORDS;

				// multiply...
			fpga_lowlevel_mul16(ai[i], bj[j], &p);

				// ...accumulate
			mac[x] += p;
		}

	}

		// now finally save lower half of si[] (2*OPERAND_NUM_WORDS words at once)
	for (w=0; w<(2*OPERAND_NUM_WORDS); w++)
		si[w] = mac[2*OPERAND_NUM_WORDS - (w+1)];
}


//------------------------------------------------------------------------------
//
// Accumulation of partial words into full-size product.
//
// This routine implements the Algorithm 4. from "Ultra High Performance ECC
// over NIST Primes on Commercial FPGAs".
//
// Input words si[] are accumulated into full-size product c[].
//
// The algorithm is a bit tricky, there are 4*OPERAND_NUM_WORDS-1 words in
// si[]. Complete operation takes 2*OPERAND_NUM_WORDS cycles, even words are
// summed in full, odd words are split into two parts. During every cycle the
// upper part of the previous word and the lower part of the following word are
// summed too.
//
//------------------------------------------------------------------------------
void fpga_modular_mul_helper_accumulate(FPGA_WORD_EXTENDED *si, FPGA_WORD *c)
//------------------------------------------------------------------------------
{
	int w;							// word counter
	FPGA_WORD_EXTENDED cw0, cw1;	// intermediate sums
	FPGA_WORD_REDUCED  cw_carry;	// wide carry

		// clear carry
	cw_carry = 0;

		// execute the algorithm
	for (w=0; w<(2*OPERAND_NUM_WORDS); w++)
	{
			// handy flags
		bool w_is_first = (w == 0);
		bool w_is_last  = (w == (2*OPERAND_NUM_WORDS-1));

			// accumulate full current even word...
			// ...and also the upper part of the previous odd word (if not the first word)
		fpga_lowlevel_add48(si[2*w], w_is_first ? 0 : si[2*w-1] >> (FPGA_WORD_WIDTH / 2), &cw0);

			// generate another word from "carry" part of the previous even word...
			// ...and also the lower part of the following odd word (if not the last word)
		cw1 = w_is_last ? 0 : (FPGA_WORD)(si[2*w+1] << (FPGA_WORD_WIDTH / 2));
		cw1 |= (FPGA_WORD_EXTENDED)cw_carry;

			// accumulate once again
		fpga_lowlevel_add48(cw0, cw1, &cw1);

			// store current word...
		c[w] = (FPGA_WORD)cw1;

			// ...and carry
		cw_carry = (FPGA_WORD_REDUCED) (cw1 >> FPGA_WORD_WIDTH);
	}
}


//------------------------------------------------------------------------------
//
// Fast modular reduction for NIST prime P-256.
//
// p = c mod p256
//
// This routine implements the algorithm 2.29 from "Guide to Elliptic Curve
// Cryptography".
//
// Output p is OPERAND_WIDTH wide (contains OPERAND_NUM_WORDS words), input c
// on the other hand is the output of the parallelized Comba multiplier, so it
// is 2*OPERAND_WIDTH wide and has twice as many words (2*OPERAND_NUM_WORDS).
//
// To save FPGA resources, the calculation is done using only two adders and
// one subtractor. The algorithm is split into five steps.
//
//------------------------------------------------------------------------------
#if USE_CURVE == 1
void fpga_modular_mul_helper_reduce_p256(FPGA_WORD *c, FPGA_BUFFER *p)
{
		// "funny" words
	FPGA_BUFFER s1, s2, s3, s4, s5, s6, s7, s8, s9;

		// compose "funny" words out of input words
	s1.words[7] = c[ 7], s1.words[6] = c[ 6], s1.words[5] = c[ 5], s1.words[4] = c[ 4], s1.words[3] = c[ 3], s1.words[2] = c[ 2], s1.words[1] = c[ 1], s1.words[0] = c[ 0];
	s2.words[7] = c[15], s2.words[6] = c[14], s2.words[5] = c[13], s2.words[4] = c[12], s2.words[3] = c[11], s2.words[2] = 0,     s2.words[1] = 0,     s2.words[0] = 0;
	s3.words[7] = 0,     s3.words[6] = c[15], s3.words[5] = c[14], s3.words[4] = c[13], s3.words[3] = c[12], s3.words[2] = 0,     s3.words[1] = 0,     s3.words[0] = 0;
	s4.words[7] = c[15], s4.words[6] = c[14], s4.words[5] = 0,     s4.words[4] = 0,     s4.words[3] = 0,     s4.words[2] = c[10], s4.words[1] = c[ 9], s4.words[0] = c[ 8];
	s5.words[7] = c[ 8], s5.words[6] = c[13], s5.words[5] = c[15], s5.words[4] = c[14], s5.words[3] = c[13], s5.words[2] = c[11], s5.words[1] = c[10], s5.words[0] = c[ 9];
	s6.words[7] = c[10], s6.words[6] = c[ 8], s6.words[5] = 0,     s6.words[4] = 0,     s6.words[3] = 0,     s6.words[2] = c[13], s6.words[1] = c[12], s6.words[0] = c[11];
	s7.words[7] = c[11], s7.words[6] = c[ 9], s7.words[5] = 0,     s7.words[4] = 0,     s7.words[3] = c[15], s7.words[2] = c[14], s7.words[1] = c[13], s7.words[0] = c[12];
	s8.words[7] = c[12], s8.words[6] = 0,     s8.words[5] = c[10], s8.words[4] = c[ 9], s8.words[3] = c[ 8], s8.words[2] = c[15], s8.words[1] = c[14], s8.words[0] = c[13];
	s9.words[7] = c[13], s9.words[6] = 0,     s9.words[5] = c[11], s9.words[4] = c[10], s9.words[3] = c[ 9], s9.words[2] = 0,     s9.words[1] = c[15], s9.words[0] = c[14];
	
		// intermediate results
	FPGA_BUFFER sum0, sum1, difference;

		/* Step 1. */
	fpga_modular_add(&s2,         &s2,         &sum0);			// sum0 = 2*s2
	fpga_modular_add(&s3,         &s3,         &sum1);			// sum1 = 2*s3
	fpga_modular_sub(&ecdsa_zero, &s6,         &difference);	// difference = -s6

		/* Step 2. */
	fpga_modular_add(&sum0,       &s1,         &sum0);			// sum0 = s1 + 2*s2
	fpga_modular_add(&sum1,       &s4,         &sum1);			// sum1 = s4 + 2*s3
	fpga_modular_sub(&difference, &s7,         &difference);	// difference = -(s6 + s7)

		/* Step 3. */
	fpga_modular_add(&sum0,       &s5,         &sum0);			// sum0 = s1 + 2*s2 + s5
	fpga_modular_add(&sum1,       &ecdsa_zero, &sum1);			// compulsory cycle to keep sum1 constant for next stage
	fpga_modular_sub(&difference, &s8,         &difference);	// difference = -(s6 + s7 + s8)

		/* Step 4. */
	fpga_modular_add(&sum0,       &sum1,       &sum0);			// sum0 = s1 + 2*s2 + 2*s3 + s4 + s5
//	fpga_modular_add(<dummy>,     <dummy>,     &sum1);			// dummy cycle, result ignored
	fpga_modular_sub(&difference, &s9,         &difference);	// difference = -(s6 + s7 + s8 + s9)

		/* Step 5. */
	fpga_modular_add(&sum0,       &difference, p);				// p = s1 + 2*s2 + 2*s3 + s4 + s5 - s6 - s7 - s8 - s9
//	fpga_modular_add(<dummy>,     <dummy>,     &sum1);			// dummy cycle, result ignored
//	fpga_modular_add(<dummy>,     <dummy>,     &difference);	// dummy cycle, result ignored
}
#endif


//------------------------------------------------------------------------------
//
// Fast modular reduction for NIST prime P-384.
//
// p = c mod p384
//
// This routine implements the algorithm 2.30 from "Guide to Elliptic Curve
// Cryptography".
//
// Output p is OPERAND_WIDTH wide (contains OPERAND_NUM_WORDS words), input c
// on the other hand is the output of the parallelized Comba multiplier, so it
// is 2*OPERAND_WIDTH wide and has twice as many words (2*OPERAND_NUM_WORDS).
//
// To save FPGA resources, the calculation is done using only two adders and
// one subtractor. The algorithm is split into five steps.
//
//------------------------------------------------------------------------------
#if USE_CURVE == 2
void fpga_modular_mul_helper_reduce_p384(FPGA_WORD *c, FPGA_BUFFER *p)
{
		// "funny" words
	FPGA_BUFFER s1, s2, s3, s4, s5, s6, s7, s8, s9, s10;

		// compose "funny" words
	 s1.words[11] = c[11],   s1.words[10] = c[10],   s1.words[ 9] = c[ 9],   s1.words[ 8] = c[ 8],   s1.words[ 7] = c[ 7],   s1.words[ 6] = c[ 6],   s1.words[ 5] = c[ 5],   s1.words[ 4] = c[ 4],   s1.words[ 3] = c[ 3],   s1.words[ 2] = c[ 2],   s1.words[ 1] = c[ 1],   s1.words[ 0] = c[ 0];
	 s2.words[11] = 0,       s2.words[10] = 0,       s2.words[ 9] = 0,       s2.words[ 8] = 0,       s2.words[ 7] = 0,       s2.words[ 6] = c[23],   s2.words[ 5] = c[22],   s2.words[ 4] = c[21],   s2.words[ 3] = 0,       s2.words[ 2] = 0,       s2.words[ 1] = 0,       s2.words[ 0] = 0;
	 s3.words[11] = c[23],   s3.words[10] = c[22],   s3.words[ 9] = c[21],   s3.words[ 8] = c[20],   s3.words[ 7] = c[19],   s3.words[ 6] = c[18],   s3.words[ 5] = c[17],   s3.words[ 4] = c[16],   s3.words[ 3] = c[15],   s3.words[ 2] = c[14],   s3.words[ 1] = c[13],   s3.words[ 0] = c[12];
	 s4.words[11] = c[20],   s4.words[10] = c[19],   s4.words[ 9] = c[18],   s4.words[ 8] = c[17],   s4.words[ 7] = c[16],   s4.words[ 6] = c[15],   s4.words[ 5] = c[14],   s4.words[ 4] = c[13],   s4.words[ 3] = c[12],   s4.words[ 2] = c[23],   s4.words[ 1] = c[22],   s4.words[ 0] = c[21];
	 s5.words[11] = c[19],   s5.words[10] = c[18],   s5.words[ 9] = c[17],   s5.words[ 8] = c[16],   s5.words[ 7] = c[15],   s5.words[ 6] = c[14],   s5.words[ 5] = c[13],   s5.words[ 4] = c[12],   s5.words[ 3] = c[20],   s5.words[ 2] = 0,       s5.words[ 1] = c[23],   s5.words[ 0] = 0;
	 s6.words[11] = 0,       s6.words[10] = 0,       s6.words[ 9] = 0,       s6.words[ 8] = 0,       s6.words[ 7] = c[23],   s6.words[ 6] = c[22],   s6.words[ 5] = c[21],   s6.words[ 4] = c[20],   s6.words[ 3] = 0,       s6.words[ 2] = 0,       s6.words[ 1] = 0,       s6.words[ 0] = 0;
	 s7.words[11] = 0,       s7.words[10] = 0,       s7.words[ 9] = 0,       s7.words[ 8] = 0,       s7.words[ 7] = 0,       s7.words[ 6] = 0,       s7.words[ 5] = c[23],   s7.words[ 4] = c[22],   s7.words[ 3] = c[21],   s7.words[ 2] = 0,       s7.words[ 1] = 0,       s7.words[ 0] = c[20];
	 s8.words[11] = c[22],   s8.words[10] = c[21],   s8.words[ 9] = c[20],   s8.words[ 8] = c[19],   s8.words[ 7] = c[18],   s8.words[ 6] = c[17],   s8.words[ 5] = c[16],   s8.words[ 4] = c[15],   s8.words[ 3] = c[14],   s8.words[ 2] = c[13],   s8.words[ 1] = c[12],   s8.words[ 0] = c[23];
	 s9.words[11] = 0,       s9.words[10] = 0,       s9.words[ 9] = 0,       s9.words[ 8] = 0,       s9.words[ 7] = 0,       s9.words[ 6] = 0,       s9.words[ 5] = 0,       s9.words[ 4] = c[23],   s9.words[ 3] = c[22],   s9.words[ 2] = c[21],   s9.words[ 1] = c[20],   s9.words[ 0] = 0;
	s10.words[11] = 0,      s10.words[10] = 0,      s10.words[ 9] = 0,      s10.words[ 8] = 0,      s10.words[ 7] = 0,      s10.words[ 6] = 0,      s10.words[ 5] = 0,      s10.words[ 4] = c[23],  s10.words[ 3] = c[23],  s10.words[ 2] = 0,      s10.words[ 1] = 0,      s10.words[ 0] = 0;

		// intermediate results
	FPGA_BUFFER sum0, sum1, difference;

		/* Step 1. */
	fpga_modular_add(&s1,         &s3,         &sum0);			// sum0 = s1 + s3
	fpga_modular_add(&s2,         &s2,         &sum1);			// sum1 = 2*s2
	fpga_modular_sub(&ecdsa_zero, &s8,         &difference);	// difference = -s8

		/* Step 2. */
	fpga_modular_add(&sum0,       &s4,         &sum0);			// sum0 = s1 + s3 + s4
	fpga_modular_add(&sum1,       &s5,         &sum1);			// sum1 = 2*s2 + s5
	fpga_modular_sub(&difference, &s9,         &difference);	// difference = -(s8 + s9)

		/* Step 3. */
	fpga_modular_add(&sum0,       &s6,         &sum0);			// sum0 = s1 + s3 + s4 + s6
	fpga_modular_add(&sum1,       &s7,         &sum1);			// sum1 = 2*s2 + s5 + s7
	fpga_modular_sub(&difference, &s10,        &difference);	// difference = -(s8 + s9 + s10)

		/* Step 4. */
	fpga_modular_add(&sum0,       &sum1,       &sum0);			// sum0 = s1 + 2*s2 + 2*s3 + s4 + s5
//	fpga_modular_add(<dummy>,     <dummy>,     &sum1);			// dummy cycle, result ignored
	fpga_modular_sub(&difference, &ecdsa_zero, &difference);	// compulsory cycle to keep difference constant for next stage

		/* Step 5. */
	fpga_modular_add(&sum0,       &difference, p);				// p = s1 + 2*s2 + s3 + s4 + s5 + s6 + s7 - s8 - s9 - s10
//	fpga_modular_add(<dummy>,     <dummy>,     &sum1);			// dummy cycle, result ignored
//	fpga_modular_add(<dummy>,     <dummy>,     &difference);	// dummy cycle, result ignored
}
#endif


//------------------------------------------------------------------------------
//
// Multi-word shift to the left by 1 bit.
//
// y = x << 1
//
//------------------------------------------------------------------------------
void fpga_modular_inv_helper_shl(FPGA_WORD *x, FPGA_WORD *y)
//------------------------------------------------------------------------------
{
	int w;							// word counter
	FPGA_WORD carry_in, carry_out;	// carries

	carry_in = 0;	// first word has no carry

		// shift word-by-word
	for (w=0; w<=OPERAND_NUM_WORDS; w++)
		carry_out = x[w] >> (FPGA_WORD_WIDTH - 1),	// store next carry
		y[w]  = x[w] << 1,							// shift
		y[w] |= carry_in,							// process carry
		carry_in = carry_out;						// propagate carry
}


//------------------------------------------------------------------------------
//
// Multi-word shift to the right by 1 bit.
//
// y = x >> 1
//
//------------------------------------------------------------------------------
void fpga_modular_inv_helper_shr(FPGA_WORD *x, FPGA_WORD *y)
//------------------------------------------------------------------------------
{
	int w;							// word counter
	FPGA_WORD carry_in, carry_out;	// carries

	carry_in = 0;	// first word has no carry

		// shift word-by-word
	for (w=OPERAND_NUM_WORDS; w>=0; w--)
		carry_out = x[w],							// store next carry
		y[w] = x[w] >> 1,							// shift
		y[w] |= carry_in << (FPGA_WORD_WIDTH - 1),	// process carry
		carry_in = carry_out;						// propagate carry
}


//------------------------------------------------------------------------------
//
// Multi-word addition.
//
// s = x + y
//
//------------------------------------------------------------------------------
void fpga_modular_inv_helper_add(FPGA_WORD *x, FPGA_WORD *y, FPGA_WORD *s)
//------------------------------------------------------------------------------
{
	int w;						// word counter
	bool carry_in, carry_out;	// carries

		// lowest word has no carry
	carry_in = false;

		// sum a and b word-by-word
	for (w=0; w<=OPERAND_NUM_WORDS; w++)
	{
			// low-level addition
		fpga_lowlevel_add32(x[w], y[w], carry_in, &s[w], &carry_out);

			// propagate carry bit
		carry_in = carry_out;
	}
}


//------------------------------------------------------------------------------
//
// Multi-word subtraction .
//
// d = x - y
//
//------------------------------------------------------------------------------
void fpga_modular_inv_helper_sub(FPGA_WORD *x, FPGA_WORD *y, FPGA_WORD *d)
//------------------------------------------------------------------------------
{
	int w;							// word counter
	bool borrow_in, borrow_out;		// borrows

		// lowest word has no borrow
	borrow_in = false;				

		// subtract b from a word-by-word
	for (w=0; w<=OPERAND_NUM_WORDS; w++)
	{
			// low-level subtraction
		fpga_lowlevel_sub32(x[w], y[w], borrow_in, &d[w], &borrow_out);

			// propagate borrow bit
		borrow_in = borrow_out;
	}

}


//------------------------------------------------------------------------------
//
// Multi-word copy.
//
// dst = src
//
//------------------------------------------------------------------------------
void fpga_modular_inv_helper_cpy(FPGA_WORD *dst, FPGA_WORD *src)
//------------------------------------------------------------------------------
{
	int w;	// word counter

		// copy all the words from src into dst
	for (w=0; w<=OPERAND_NUM_WORDS; w++)
		dst[w] = src[w];
}


//------------------------------------------------------------------------------
//
// Multi-word comparison.
//
// The return value is -1 when a<b, 0 when a=b and 1 when a>b.
//
//------------------------------------------------------------------------------
void fpga_modular_inv_helper_cmp(FPGA_WORD *a, FPGA_WORD *b, int *c)
//------------------------------------------------------------------------------
{
	int w;					// word counter
	int r, r0, ra, rb;		// result
	bool borrow;			// borrow
	FPGA_WORD d;			// partial difference

		// result is unknown so far
	r = 0;	

		// clear borrow for the very first word
	borrow = false;

		// compare a and b word-by-word
	for (w=OPERAND_NUM_WORDS; w>=0; w--)
	{
			// save result
		r0 = r;

			// subtract current words
		fpga_lowlevel_sub32(a[w], b[w], false, &d, &borrow);

			// analyze flags
		rb = borrow ? -1 : 0;					// a[w] < b[w]
		ra = (!borrow && (d != 0)) ? 1 : 0;		// a[w] > b[w]
		
			//
			// Note, that ra is either 0 or 1, rb is either 0 or -1 and they
			// can never be non-zero at the same time.
			//
			// Note, that r can only be updated if comparison result has not
			// been resolved yet. Even if we already know comparison result,
			// we continue doing dummy subtractions to keep run-time constant.
			//

			// update result
		r = (r == 0) ? ra + rb : r0;
	}

		// done
	*c = r;
}


//------------------------------------------------------------------------------
// End-of-File
//------------------------------------------------------------------------------
