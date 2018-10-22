//
// modexp_fpga_model.cpp
// -----------------------------------------------
// Model of fast modular exponentiation on an FPGA
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "modexp_fpga_model.h"
#include "modexp_fpga_model_montgomery.h"
#include "test/modexp_fpga_model_vectors.h"


//----------------------------------------------------------------
// Defined values
//----------------------------------------------------------------
#define OPERAND_WIDTH_384	384
#define OPERAND_WIDTH_512	512

#define OPERAND_NUM_WORDS_384	(OPERAND_WIDTH_384 / (CHAR_BIT * sizeof(FPGA_WORD)))
#define OPERAND_NUM_WORDS_512	(OPERAND_WIDTH_512 / (CHAR_BIT * sizeof(FPGA_WORD)))


//----------------------------------------------------------------
// Test vectors
//----------------------------------------------------------------
static const FPGA_WORD N_384_ROM[]  = N_384;	// 384-bit
static const FPGA_WORD M_384_ROM[]  = M_384;	//
static const FPGA_WORD D_384_ROM[]  = D_384;	//
static const FPGA_WORD S_384_ROM[]  = S_384;	//

static const FPGA_WORD P_384_ROM[]  = P_384;	// 192-bit
static const FPGA_WORD Q_384_ROM[]  = Q_384;	//
static const FPGA_WORD DP_384_ROM[] = DP_384;	//
static const FPGA_WORD DQ_384_ROM[] = DQ_384;	//
static const FPGA_WORD MP_384_ROM[] = MP_384;	//
static const FPGA_WORD MQ_384_ROM[] = MQ_384;	//

static const FPGA_WORD N_512_ROM[] = N_512;		// 512-bit
static const FPGA_WORD M_512_ROM[] = M_512;		//
static const FPGA_WORD D_512_ROM[] = D_512;		//
static const FPGA_WORD S_512_ROM[] = S_512;		//

static const FPGA_WORD P_512_ROM[]  = P_512;	// 256-bit
static const FPGA_WORD Q_512_ROM[]  = Q_512;	//
static const FPGA_WORD DP_512_ROM[] = DP_512;	//
static const FPGA_WORD DQ_512_ROM[] = DQ_512;	//
static const FPGA_WORD MP_512_ROM[] = MP_512;	//
static const FPGA_WORD MQ_512_ROM[] = MQ_512;	//


//----------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------
void print_fpga_buffer		(const char *str, const FPGA_WORD *buf, size_t len);
bool compare_fpga_buffers	(const FPGA_WORD *src, const FPGA_WORD *dst, size_t len);
void load_value_from_rom	(const FPGA_WORD *src, FPGA_WORD *dst, size_t len);

void modexp				(const FPGA_WORD *M, const FPGA_WORD *D,
						 const FPGA_WORD *N,       FPGA_WORD *R, size_t len);

void modexp_crt			(const FPGA_WORD *M, const FPGA_WORD *D,
						 const FPGA_WORD *N,       FPGA_WORD *R, size_t len);

bool test_modexp		(const FPGA_WORD *n_rom, const FPGA_WORD *m_rom,
						 const FPGA_WORD *d_rom, const FPGA_WORD *s_rom, size_t len);

bool test_modexp_crt	(const FPGA_WORD *n_rom, const FPGA_WORD *m_rom,
						 const FPGA_WORD *d_rom, const FPGA_WORD *s_rom, size_t len);


//----------------------------------------------------------------
int main()
//----------------------------------------------------------------
{
	bool ok;

	printf("Trying to sign 384-bit message...\n\n");
	ok = test_modexp(N_384_ROM, M_384_ROM, D_384_ROM, S_384_ROM, OPERAND_NUM_WORDS_384);
	if (!ok) return EXIT_FAILURE;

	printf("Trying to exponentiate 384-bit message with 192-bit prime P and exponent dP...\n\n");
	ok = test_modexp_crt(P_384_ROM, M_384_ROM, DP_384_ROM, MP_384_ROM, OPERAND_NUM_WORDS_384 >> 1);
	if (!ok) return EXIT_FAILURE;

	printf("Trying to exponentiate 384-bit message with 192-bit prime Q and exponent dQ...\n\n");
	ok = test_modexp_crt(Q_384_ROM, M_384_ROM, DQ_384_ROM, MQ_384_ROM, OPERAND_NUM_WORDS_384 >> 1);
	if (!ok) return EXIT_FAILURE;

	printf("Trying to sign 512-bit message...\n\n");
	ok = test_modexp(N_512_ROM, M_512_ROM, D_512_ROM, S_512_ROM, OPERAND_NUM_WORDS_512);
	if (!ok) return EXIT_FAILURE;

	printf("Trying to exponentiate 512-bit message with 256-bit prime P and exponent dP...\n\n");
	ok = test_modexp_crt(P_512_ROM, M_512_ROM, DP_512_ROM, MP_512_ROM, OPERAND_NUM_WORDS_512 >> 1);
	if (!ok) return EXIT_FAILURE;

	printf("Trying to exponentiate 512-bit message with 256-bit prime Q and exponent dQ...\n\n");
	ok = test_modexp_crt(Q_512_ROM, M_512_ROM, DQ_512_ROM, MQ_512_ROM, OPERAND_NUM_WORDS_512 >> 1);
	if (!ok) return EXIT_FAILURE;

	return EXIT_SUCCESS;
}


//----------------------------------------------------------------
// Modular exponentiation routine
//----------------------------------------------------------------
void modexp(	const FPGA_WORD *M,
				const FPGA_WORD *D,
				const FPGA_WORD *N,
				      FPGA_WORD *R,
					  size_t     len)
//----------------------------------------------------------------
//
// R = A ** B mod N
//
//----------------------------------------------------------------
{
		// temporary buffers
	FPGA_WORD FACTOR [MAX_OPERAND_WORDS];
	FPGA_WORD N_COEFF[MAX_OPERAND_WORDS];
	FPGA_WORD M_FACTOR[MAX_OPERAND_WORDS];

		// pre-calculate modulus-dependant coefficients
	montgomery_calc_factor(N, FACTOR, len);
	montgomery_calc_n_coeff(N, N_COEFF, len);
		
		// bring M into Montgomery domain
	montgomery_multiply(M, FACTOR, N, N_COEFF, M_FACTOR, len, false);

		/*
		 * Montgomery multiplication adds an extra factor of 2 ^ -w to every product.
		 * We pre-calculate a special factor of 2 ^ 2w and multiply the message
		 * by this factor using our Montgomery multiplier. This way we get the message
		 * with the an extra factor of just 2 ^ w:
		 * (m) * (2 ^ 2w) * (2 ^ -w) = m * 2 ^ w 
		 *
		 * Now we feed this message with that extra factor to the binary exponentiation
		 * routine. The current power of m will always keep that additional factor:
		 * (p * 2 ^ w) * (p * 2 ^ w) * (2 ^ -w) = p ^ 2 * 2 ^ w
		 *
		 * The result starts at 1, i.e. without any extra factors. If at any particular
		 * iteration it gets multiplied with the current power of m, the product will
		 * not carry any extra factors, because the power's factor gets eliminated
		 * by the extra factor of Montgomery multiplication:
		 * (r) * (p * 2 ^ w) * (2 ^ -w) = r * p
		 *
		 * This way we don't need any extra post-processing to convert the final result
		 * from Montgomery domain. 
		 *
		 */

		// exponentiate
	montgomery_exponentiate(M_FACTOR, D, N, N_COEFF, R, len);
}


//----------------------------------------------------------------
// Modular exponentiation routine with CRT support
//----------------------------------------------------------------
void modexp_crt(	const FPGA_WORD *M,
					const FPGA_WORD *D,
					const FPGA_WORD *N,
					      FPGA_WORD *R,
						  size_t     len)
//----------------------------------------------------------------
//
// R = (A mod N) ** B mod N
//
//----------------------------------------------------------------
{
		// temporary buffers
	FPGA_WORD M0     [MAX_OPERAND_WORDS];
	FPGA_WORD M1     [MAX_OPERAND_WORDS];
	FPGA_WORD FACTOR [MAX_OPERAND_WORDS];
	FPGA_WORD N_COEFF[MAX_OPERAND_WORDS];
	FPGA_WORD M_FACTOR[MAX_OPERAND_WORDS];

		// pre-calculate modulus-dependant coefficients
	montgomery_calc_factor(N, FACTOR, len);
	montgomery_calc_n_coeff(N, N_COEFF, len);
	
		// reduce M to make it smaller than N
	montgomery_multiply(M, NULL, N, N_COEFF, M0, len, true);

		// bring M into Montgomery domain
	montgomery_multiply(M0, FACTOR, N, N_COEFF, M1,       len, false);
	montgomery_multiply(M1, FACTOR, N, N_COEFF, M_FACTOR, len, false);

		/*
		 * Montgomery multiplication adds an extra factor of 2 ^ -w to every product,
		 * Montgomery reduction adds that factor too. The message must be reduced before
		 * exponentiation, because in CRT mode it is twice larger, than the modulus
		 * and the exponent. After reduction the message carries an extra factor of
		 * 2 ^ -w. We pre-calculate a special factor of 2 ^ 2w and multiply the message
		 * by this factor *twice* using our Montgomery multiplier. This way we get the
		 * message with an extra factor of just 2 ^ w:
		 * 1. (m * 2 ^ -w) * (2 ^ 2w) * (2 ^ -w) = m
		 * 2. (m) * (2 ^ 2w) * (2 ^ -w) = m * 2 ^ w
		 *
		 * Now we feed this message with that extra factor to the binary exponentiation
		 * routine. The current power of m will always keep that additional factor:
		 * (p * 2 ^ w) * (p * 2 ^ w) * (2 ^ -w) = p ^ 2 * 2 ^ w
		 *
		 * The result starts at 1, i.e. without any extra factors. If at any particular
		 * iteration it gets multiplied with the current power of m, the product will
		 * not carry any extra factors, because the power's factor gets eliminated
		 * by the extra factor of Montgomery multiplication:
		 * (r) * (p * 2 ^ w) * (2 ^ -w) = r * p
		 *
		 * This way we don't need any extra post-processing to convert the final result
		 * from Montgomery domain. 
		 *
		 */

		// exponentiate
	montgomery_exponentiate(M_FACTOR, D, N, N_COEFF, R, len);
}


//----------------------------------------------------------------
// Copies words from src into dst reversing their order
//----------------------------------------------------------------
void load_value_from_rom(const FPGA_WORD *src, FPGA_WORD *dst, size_t len)
//----------------------------------------------------------------
//
// This routine copies src into dst word-by-word reversing their order
// in the process. This reversal is necessary because of the way C
// arrays are initialized. This model requires the least significant
// word of operand to be stored at array offset 0, while C places
// the most significant word there instead.
//
// Suppose that the operand is 0xFEDCBA9876543210, now the following line
// uint32_t X[2] = {0xFEDCBA98, 0x76543210}
// will place the most significant word 0xFEDCBA98 at index [0].
//
//----------------------------------------------------------------
{
	size_t w;

	for (w=0; w<len; w++)
		dst[w] = src[len - (w + 1)];
}


//----------------------------------------------------------------
// Compare two operands
//----------------------------------------------------------------
bool compare_fpga_buffers(const FPGA_WORD *src, const FPGA_WORD *dst, size_t len)
//----------------------------------------------------------------
//
// This routine compares two multi-word intergers, it is used to compare
// the calculated value against the reference one.
//
//----------------------------------------------------------------
{
	size_t w;	// word counter

		// print all the values
	print_fpga_buffer("  Expected:   M = ", src, len);
	print_fpga_buffer("  Calculated: R = ", dst, len);

		// compare values
	for (w=0; w<len; w++)
	{
			// compare
		if (src[w] != dst[w]) return false;
	}

		// values are the same
	return true;
}


//----------------------------------------------------------------
// Prints large multi-word integer
//----------------------------------------------------------------
void print_fpga_buffer(const char *str, const FPGA_WORD *buf, size_t len)
//----------------------------------------------------------------
{
	size_t w, s;	// word counter

		// print header
	printf("%s", str);

		// print all bytes
	for (w=len; w>0; w--)
	{	
		printf("%08x", buf[w-1]);

			// insert space after every group of 4 bytes or print new line
		if (w > 1)
		{
			if (((len - w) % 4) == 3)
			{	printf("\n");
				s = strlen(str);
				while (s)
				{	printf(" ");
					s--;
				}
			}
			else printf(" ");
		}
	}

		// print footer
	printf("\n\n");
}


//----------------------------------------------------------------
// Test the modular exponentiation model
//----------------------------------------------------------------
bool test_modexp(const FPGA_WORD *n_rom, const FPGA_WORD *m_rom, const FPGA_WORD *d_rom, const FPGA_WORD *s_rom, size_t len)
//----------------------------------------------------------------
//
// This routine uses the Montgomery exponentiation model to
// calculate r = m ** d mod n, and then compares it to the
// reference value s.
//
//----------------------------------------------------------------
{
	bool ok;	// flag

		// buffers
	FPGA_WORD N[MAX_OPERAND_WORDS];
	FPGA_WORD M[MAX_OPERAND_WORDS];
	FPGA_WORD D[MAX_OPERAND_WORDS];
	FPGA_WORD S[MAX_OPERAND_WORDS];
	FPGA_WORD R[MAX_OPERAND_WORDS];

		// fill buffers with test vector
	load_value_from_rom(n_rom, N, len);
	load_value_from_rom(m_rom, M, len);
	load_value_from_rom(d_rom, D, len);
	load_value_from_rom(s_rom, S, len);

		// calculate power
	modexp(M, D, N, R, len);

		// check result
	ok = compare_fpga_buffers(S, R, len);
	if (!ok)
	{	printf("    ERROR\n\n\n");
		return false;
	}

		// everything went just fine
	printf("    OK\n\n\n");
	return true;
}


//----------------------------------------------------------------
// Test the modular exponentiation model with CRT enabled
//----------------------------------------------------------------
bool test_modexp_crt(const FPGA_WORD *n_rom, const FPGA_WORD *m_rom, const FPGA_WORD *d_rom, const FPGA_WORD *s_rom, size_t len)
//----------------------------------------------------------------
//
// This routine uses the Montgomery exponentiation model to
// calculate r = (m mod n) ** d mod n, and then compares it to the
// reference value s. The difference from test_modexp() is that
// m_rom is twice larger than n_rom and d_rom.
//
//----------------------------------------------------------------
{
	bool ok;	// flag

		// buffers
	FPGA_WORD N[MAX_OPERAND_WORDS];
	FPGA_WORD M[MAX_OPERAND_WORDS];
	FPGA_WORD D[MAX_OPERAND_WORDS];
	FPGA_WORD S[MAX_OPERAND_WORDS];
	FPGA_WORD R[MAX_OPERAND_WORDS];

		// fill buffers with test vector (message is twice as large!)
	load_value_from_rom(n_rom, N, len);
	load_value_from_rom(m_rom, M, len << 1);
	load_value_from_rom(d_rom, D, len);
	load_value_from_rom(s_rom, S, len);

		// calculate power
	modexp_crt(M, D, N, R, len);

		// check result
	ok = compare_fpga_buffers(S, R, len);
	if (!ok)
	{	printf("    ERROR\n\n\n");
		return false;
	}

		// everything went just fine
	printf("    OK\n\n\n");
	return true;
}


//----------------------------------------------------------------
// End of file
//----------------------------------------------------------------
