//
// modexp_fpga_systolic.cpp
// ------------------------
// Systolic Multiplier
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


#include <stdio.h>


//----------------------------------------------------------------
void multiply_systolic(const FPGA_WORD *A, const FPGA_WORD *B, FPGA_WORD *P, size_t len_ab, size_t len_p)
//----------------------------------------------------------------
{
		// counters
	size_t i, j, k;

		// index
	size_t j_index;
		
		// current words of A and B
	FPGA_WORD Aj, Bj;

		// number of full systolic cycles needed to multiply entire B by one word of A
	size_t num_systolic_cycles = len_ab / SYSTOLIC_NUM_WORDS;
	if ((num_systolic_cycles * SYSTOLIC_NUM_WORDS) < len_ab) num_systolic_cycles++;

		// buffers
	FPGA_WORD t[MAX_SYSTOLIC_CYCLES][SYSTOLIC_NUM_WORDS];		// accumulator
	FPGA_WORD s[MAX_SYSTOLIC_CYCLES][SYSTOLIC_NUM_WORDS];		// intermediate product
	FPGA_WORD c_in[MAX_SYSTOLIC_CYCLES][SYSTOLIC_NUM_WORDS];	// input carries
	FPGA_WORD c_out[MAX_SYSTOLIC_CYCLES][SYSTOLIC_NUM_WORDS];	// output carries

			// initialize arrays of accumulators and carries to zeroes
	for (i=0; i<num_systolic_cycles; i++)
		for (j=0; j<SYSTOLIC_NUM_WORDS; j++)
			c_in[i][j] = 0, t[i][j] = 0;

		// do the math
	for (i=0; i<len_p; i++)
	{
			// reset word index
		j_index = 0;

			// current word of A
		Aj = (i < len_ab) ? A[i] : 0;

			// scan chunks of B
		for (k=0; k<num_systolic_cycles; k++)
		{
				// simulate how systolic array would work
			for (j=0; j<SYSTOLIC_NUM_WORDS; j++)
			{
					// current word of B
				Bj = (j_index < len_ab) ? B[j_index] : 0;

					// Pj = Aj * Bj
				pe_mul(Aj, Bj, t[k][j], c_in[k][j], &s[k][j], &c_out[k][j]);

					// store current word of P
				if ((k == 0) && (j == 0)) P[i] = s[0][0];

					// update index
				j_index++;
			}

				// propagate carries
			for (j=0; j<SYSTOLIC_NUM_WORDS; j++)
				c_in[k][j] = c_out[k][j];

				// update accumulator
			for (j=1; j<SYSTOLIC_NUM_WORDS; j++)
				t[k][j-1] = s[k][j];
			
				// update accumulator
			if (k > 0)
				t[k-1][SYSTOLIC_NUM_WORDS-1] = s[k][0];

		}
	}
}


//----------------------------------------------------------------
// End of file
//----------------------------------------------------------------
