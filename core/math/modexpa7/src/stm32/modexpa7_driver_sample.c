/*
 * modexpa7_driver_sample.c
 * ----------------------------------------------
 * Demo program to test ModExpA7 core in hardware
 *
 * Authors: Pavel Shatov
 * Copyright (c) 2017, NORDUnet A/S
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the NORDUnet nor the names of its contributors may
 *   be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
		/*
		 * Note, that the test program needs a custom bitstream without
		 * the core selector, where the DUT is at offset 0.
		 */

		// stm32 headers
#include "stm-init.h"
#include "stm-led.h"
#include "stm-fmc.h"

		// test vectors
#include "test/modexp_fpga_model_vectors.h"

		// locations of core registers
#define CORE_ADDR_NAME0						(0x00 << 2)
#define CORE_ADDR_NAME1						(0x01 << 2)
#define CORE_ADDR_VERSION					(0x02 << 2)
#define CORE_ADDR_CONTROL					(0x08 << 2)
#define CORE_ADDR_STATUS					(0x09 << 2)
#define CORE_ADDR_MODE						(0x10 << 2)
#define CORE_ADDR_MODULUS_BITS		(0x11 << 2)
#define CORE_ADDR_EXPONENT_BITS		(0x12 << 2)
#define CORE_ADDR_BUFFER_BITS			(0x13 << 2)
#define CORE_ADDR_ARRAY_BITS			(0x14 << 2)

		// operand bank size
#define BANK_LENGTH		0x200		// 0x200 = 512 bytes = 4096 bits

		// locations of operand buffers
#define CORE_ADDR_BANK_MODULUS		(BANK_LENGTH * (8 + 0))
#define CORE_ADDR_BANK_MESSAGE		(BANK_LENGTH * (8 + 1))
#define CORE_ADDR_BANK_EXPONENT		(BANK_LENGTH * (8 + 2))
#define CORE_ADDR_BANK_RESULT			(BANK_LENGTH * (8 + 3))

#define CORE_ADDR_BANK_MODULUS_COEFF_OUT			(BANK_LENGTH * (8 + 4))
#define CORE_ADDR_BANK_MODULUS_COEFF_IN				(BANK_LENGTH * (8 + 5))
#define CORE_ADDR_BANK_MONTGOMERY_FACTOR_OUT	(BANK_LENGTH * (8 + 6))
#define CORE_ADDR_BANK_MONTGOMERY_FACTOR_IN		(BANK_LENGTH * (8 + 7))

		// bit maps
#define CORE_CONTROL_BIT_INIT		0x00000001
#define CORE_CONTROL_BIT_NEXT		0x00000002

#define CORE_STATUS_BIT_READY		0x00000001
#define CORE_STATUS_BIT_VALID		0x00000002

#define CORE_MODE_BIT_CRT				0x00000002

		/*
		 * zero operands
		 */
#define Z_384 \
	{0x00000000, 0x00000000, 0x00000000, 0x00000000, \
	 0x00000000, 0x00000000, 0x00000000, 0x00000000, \
	 0x00000000, 0x00000000, 0x00000000, 0x00000000}

#define Z_192 \
	{0x00000000, 0x00000000, 0x00000000, 0x00000000, \
	 0x00000000, 0x00000000}

#define Z_512 \
	{0x00000000, 0x00000000, 0x00000000, 0x00000000, \
	 0x00000000, 0x00000000, 0x00000000, 0x00000000, \
	 0x00000000, 0x00000000, 0x00000000, 0x00000000, \
	 0x00000000, 0x00000000, 0x00000000, 0x00000000}

#define Z_256 \
	{0x00000000, 0x00000000, 0x00000000, 0x00000000, \
	 0x00000000, 0x00000000, 0x00000000, 0x00000000}

		/*
		 * test vectors
		 */
static const uint32_t m_384[]	= M_384;
static const uint32_t n_384[]	= N_384;
static const uint32_t d_384[]	= D_384;
static const uint32_t s_384[]	= S_384;
static uint32_t n_coeff_384[]	= Z_384;
static uint32_t factor_384[]	= Z_384;

static const uint32_t m_512[]	= M_512;
static const uint32_t n_512[]	= N_512;
static const uint32_t d_512[]	= D_512;
static const uint32_t s_512[]	= S_512;
static uint32_t n_coeff_512[]	= Z_512;
static uint32_t factor_512[]	= Z_512;

static const uint32_t p_192[]		= P_192;
static const uint32_t q_192[]		= Q_192;
static const uint32_t dp_192[]	= DP_192;
static const uint32_t dq_192[]	= DQ_192;
static const uint32_t mp_192[]	= MP_192;
static const uint32_t mq_192[]	= MQ_192;
static uint32_t p_coeff_192[]		= Z_192;
static uint32_t q_coeff_192[]		= Z_192;
static uint32_t factor_p_192[]	= Z_192;
static uint32_t factor_q_192[]	= Z_192;

static const uint32_t p_256[]		= P_256;
static const uint32_t q_256[]		= Q_256;
static const uint32_t dp_256[]	= DP_256;
static const uint32_t dq_256[]	= DQ_256;
static const uint32_t mp_256[]	= MP_256;
static const uint32_t mq_256[]	= MQ_256;
static uint32_t p_coeff_256[]		= Z_256;
static uint32_t q_coeff_256[]		= Z_256;
static uint32_t factor_p_256[]	= Z_256;
static uint32_t factor_q_256[]	= Z_256;


		/*
		 * prototypes
		 */
void toggle_yellow_led(void);

void setup_modexpa7(	const uint32_t *n,
														uint32_t *coeff,
														uint32_t *factor,
														size_t		l);

int test_modexpa7(		const uint32_t *n,
											const uint32_t *m,
											const uint32_t *d,
											const uint32_t *s,
											const uint32_t *coeff,
											const uint32_t *factor,
											      size_t    l);

int test_modexpa7_crt(		const uint32_t *n,
													const uint32_t *m,
													const uint32_t *d,
													const uint32_t *s,
													const uint32_t *coeff,
													const uint32_t *factor,
																size_t    l);


		/*
		 * test routine
		 */
int main()
{
		int ok;
	
    stm_init();
    fmc_init();
	
				// turn on the green led
    led_on(LED_GREEN);
    led_off(LED_RED);
    led_off(LED_YELLOW);
    led_off(LED_BLUE);

				// check, that core is present
		uint32_t core_name0;
		uint32_t core_name1;
		uint32_t core_version;
	
		fmc_read_32(CORE_ADDR_NAME0,   &core_name0);
		fmc_read_32(CORE_ADDR_NAME1,   &core_name1);
		fmc_read_32(CORE_ADDR_VERSION, &core_version);
			
				// must be "mode", "xpa7", "0.25"
		if (	(core_name0   != 0x6D6F6465) ||
					(core_name1   != 0x78706137) ||
					(core_version != 0x302E3235))
		{
				led_off(LED_GREEN);
				led_on(LED_RED);
				while (1);
		}

				// read compile-time settings
		uint32_t core_buffer_bits;
		uint32_t core_array_bits;
	
			// largest supported operand width, systolic array "power"
		fmc_read_32(CORE_ADDR_BUFFER_BITS, &core_buffer_bits);
		fmc_read_32(CORE_ADDR_ARRAY_BITS,  &core_array_bits);

			//
			// do pre-computation for all the moduli and store speed-up quantities,
			// note that each key requires three precomputations: one for the entire
			// public key and two for each of the corresponding private key components
			//
			// we set the 'init' control bit, wait for `ready' status bit to go high,
			// then retrieve the calculated values from the corresponding "output" banks
			//
			// we turn off the green led and turn the yellow led during the process to
			// get an idea of how long it takes
			//
	
		led_off(LED_GREEN);
		led_on(LED_YELLOW);

			// 384-bit key and 192-bit primes
		setup_modexpa7(n_384, n_coeff_384, factor_384,   384);
		setup_modexpa7(p_192, p_coeff_192, factor_p_192, 192);
		setup_modexpa7(q_192, q_coeff_192, factor_q_192, 192);
		
			// 512-bit key and 256-bit primes
		setup_modexpa7(n_512, n_coeff_512, factor_512,   512);
		setup_modexpa7(p_256, p_coeff_256, factor_p_256, 256);
		setup_modexpa7(q_256, q_coeff_256, factor_q_256, 256);
		
		led_off(LED_YELLOW);
		led_on(LED_GREEN);

		
			// repeat forever
		while (1)
		{			
						// fresh start
				ok = 1;

				{
								// try signing the message with the 384-bit test vector
						ok = ok && test_modexpa7(n_384, m_384, d_384, s_384, n_coeff_384, factor_384, 384);

								// try signing 384-bit base using 192-bit exponent
						ok = ok && test_modexpa7_crt(p_192, m_384, dp_192, mp_192, p_coeff_192, factor_p_192, 192);
					
								// try signing 384-bit base using 192-bit exponent
						ok = ok && test_modexpa7_crt(q_192, m_384, dq_192, mq_192, q_coeff_192, factor_q_192, 192);
				}

				{
								// try signing the message with the 512-bit test vector
						ok = ok && test_modexpa7(n_512, m_512, d_512, s_512, n_coeff_512, factor_512, 512);			
				
								// try signing 512-bit base using 256-bit exponent
						ok = ok && test_modexpa7_crt(p_256, m_512, dp_256, mp_256, p_coeff_256, factor_p_256, 256);
					
								// try signing 512-bit base using 256-bit exponent
						ok = ok && test_modexpa7_crt(q_256, m_512, dq_256, mq_256, q_coeff_256, factor_q_256, 256);
				}
				
						// turn on the red led to indicate something went wrong
				if (!ok)
				{		led_off(LED_GREEN);
						led_on(LED_RED);
				}
				
						// indicate, that we're alive doing something...
				toggle_yellow_led();
		}
}


		/*
		 * Load new modulus and do all the necessary precomputations.
		 */
void setup_modexpa7(	const uint32_t *n,
														uint32_t *coeff,
														uint32_t *factor,
										        size_t    l)
{
		size_t i, num_words;
		uint32_t num_bits;
		uint32_t reg_control, reg_status;
		uint32_t n_word;
		uint32_t coeff_word, factor_word;
		uint32_t dummy_num_cyc;		
	
			// determine numbers of 32-bit words
		num_words = l >> 5;
	
			// set modulus width
		num_bits = l;
		fmc_write_32(CORE_ADDR_MODULUS_BITS, &num_bits);
	
			// fill modulus bank (the least significant word is at the lowest offset)
		for (i=0; i<num_words; i++)
		{		n_word = n[i];
				fmc_write_32(CORE_ADDR_BANK_MODULUS  + ((num_words - (i + 1)) * sizeof(uint32_t)), &n_word);
		}

				// clear 'init' control bit, then set 'init' control bit again
				// to trigger precomputation (core is edge-triggered)
		reg_control = 0;
		fmc_write_32(CORE_ADDR_CONTROL, &reg_control);
		reg_control = CORE_CONTROL_BIT_INIT;
		fmc_write_32(CORE_ADDR_CONTROL, &reg_control);
	
				// wait for 'ready' status bit to be set
		dummy_num_cyc = 0;
		do
		{		dummy_num_cyc++;
				fmc_read_32(CORE_ADDR_STATUS, &reg_status);
		}
		while (!(reg_status & CORE_STATUS_BIT_READY));
		
				// retrieve the modulus-dependent coefficient and Montgomery factor
				// from the corresponding core "output" banks and store them for later use
		for (i=0; i<num_words; i++)
		{
				fmc_read_32(CORE_ADDR_BANK_MODULUS_COEFF_OUT + i * sizeof(uint32_t), &coeff_word);	
				coeff[i] = coeff_word;

				fmc_read_32(CORE_ADDR_BANK_MONTGOMERY_FACTOR_OUT + i * sizeof(uint32_t), &factor_word);
				factor[i] = factor_word;
		}
}


		//
		// Sign the message and compare it against the correct reference value.
		//
int test_modexpa7(	const uint32_t *n,	
										const uint32_t *m,
										const uint32_t *d,
										const uint32_t *s,
										const uint32_t *coeff,
										const uint32_t *factor,
										      size_t    l)
{
		size_t i, num_words;
		uint32_t num_bits;
		uint32_t reg_control, reg_status;
		uint32_t n_word, m_word, d_word, s_word;
		uint32_t coeff_word, factor_word;
		uint32_t dummy_num_cyc;		
		uint32_t mode;
		
				// determine numbers of 32-bit words
		num_words = l >> 5;
	
				// set modulus width, exponent width
		num_bits = l;
		fmc_write_32(CORE_ADDR_MODULUS_BITS, &num_bits);
		fmc_write_32(CORE_ADDR_EXPONENT_BITS, &num_bits);
	
				// disable CRT mode
		mode = 0;
		fmc_write_32(CORE_ADDR_MODE, &mode);
	
				// fill modulus, message and exponent banks (the least significant
				// word is at the lowest offset), we also need to fill "input" core
				// banks with previously pre-calculated and saved modulus-dependent
				// speed-up coefficient and Montgomery factor
		for (i=0; i<num_words; i++)
		{		
				n_word = n[i];
				m_word = m[i];
				d_word = d[i];

				fmc_write_32(CORE_ADDR_BANK_MODULUS  + ((num_words - (i + 1)) * sizeof(uint32_t)), &n_word);
				fmc_write_32(CORE_ADDR_BANK_MESSAGE  + ((num_words - (i + 1)) * sizeof(uint32_t)), &m_word);
				fmc_write_32(CORE_ADDR_BANK_EXPONENT + ((num_words - (i + 1)) * sizeof(uint32_t)), &d_word);

				coeff_word = coeff[i];
				factor_word = factor[i];
			
				fmc_write_32(CORE_ADDR_BANK_MODULUS_COEFF_IN     + i * sizeof(uint32_t), &coeff_word);
				fmc_write_32(CORE_ADDR_BANK_MONTGOMERY_FACTOR_IN + i * sizeof(uint32_t), &factor_word);
		}

				// clear 'next' control bit, then set 'next' control bit again
				// to trigger exponentiation (core is edge-triggered)
		reg_control = 0;
		fmc_write_32(CORE_ADDR_CONTROL, &reg_control);
		reg_control = CORE_CONTROL_BIT_NEXT;
		fmc_write_32(CORE_ADDR_CONTROL, &reg_control);
	
				// wait for 'valid' status bit to be set
		dummy_num_cyc = 0;
		do
		{		dummy_num_cyc++;
				fmc_read_32(CORE_ADDR_STATUS, &reg_status);
		}
		while (!(reg_status & CORE_STATUS_BIT_VALID));
		
				// read back the result word-by-word, then compare to the reference values
		for (i=0; i<num_words; i++)
		{		
				fmc_read_32(CORE_ADDR_BANK_RESULT + (i * sizeof(uint32_t)), &s_word);
			
				if (s_word != s[num_words - (i + 1)]) return 0;
		}
	
				// everything went just fine
		return 1;
}


int test_modexpa7_crt(	const uint32_t *n,
												const uint32_t *m,
												const uint32_t *d,
												const uint32_t *s,
												const uint32_t *coeff,
												const uint32_t *factor,
															size_t    l)
{
		size_t i, num_words;
		uint32_t num_bits;
		uint32_t reg_control, reg_status;
		uint32_t n_word, m_word, d_word, s_word;
		uint32_t coeff_word, factor_word;
		uint32_t dummy_num_cyc;		
		uint32_t mode;
		
				// determine numbers of 32-bit words
		num_words = l >> 5;
	
				// set modulus width, exponent width
		num_bits = l;
		fmc_write_32(CORE_ADDR_MODULUS_BITS, &num_bits);
		fmc_write_32(CORE_ADDR_EXPONENT_BITS, &num_bits);
	
				// enable CRT mode
		mode = CORE_MODE_BIT_CRT;
		fmc_write_32(CORE_ADDR_MODE, &mode);
	
				// fill modulus and exponent banks (the least significant word is at
				// the lowest offset), we also need to fill "input" core banks with
				// previously pre-calculated and saved modulus-dependent speed-up
				// coefficient and Montgomery factor
		for (i=0; i<num_words; i++)
		{		n_word = n[i];
				d_word = d[i];
				fmc_write_32(CORE_ADDR_BANK_MODULUS  + ((num_words - (i + 1)) * sizeof(uint32_t)), &n_word);
				fmc_write_32(CORE_ADDR_BANK_EXPONENT + ((num_words - (i + 1)) * sizeof(uint32_t)), &d_word);
			
				coeff_word = coeff[i];
				factor_word = factor[i];
			
				fmc_write_32(CORE_ADDR_BANK_MODULUS_COEFF_IN     + i * sizeof(uint32_t), &coeff_word);
				fmc_write_32(CORE_ADDR_BANK_MONTGOMERY_FACTOR_IN + i * sizeof(uint32_t), &factor_word);
		}

				// fill message bank (the least significant word
				// is at the lowest offset, message is twice larger
				// than the modulus in CRT mode!)
		for (i=0; i<(2 * num_words); i++)
		{		m_word = m[i];
				fmc_write_32(CORE_ADDR_BANK_MESSAGE  + ((2 * num_words - (i + 1)) * sizeof(uint32_t)), &m_word);
		}

				// clear 'next' control bit, then set 'next' control bit again
				// to trigger exponentiation (core is edge-triggered)
		reg_control = 0;
		fmc_write_32(CORE_ADDR_CONTROL, &reg_control);
		reg_control = CORE_CONTROL_BIT_NEXT;
		fmc_write_32(CORE_ADDR_CONTROL, &reg_control);
	
				// wait for 'valid' status bit to be set
		dummy_num_cyc = 0;
		do
		{		dummy_num_cyc++;
				fmc_read_32(CORE_ADDR_STATUS, &reg_status);
		}
		while (!(reg_status & CORE_STATUS_BIT_VALID));
		
				// read back the result word-by-word, then compare to the reference values
		for (i=0; i<num_words; i++)
		{		
				fmc_read_32(CORE_ADDR_BANK_RESULT + (i * sizeof(uint32_t)), &s_word);
			
				if (s_word != s[num_words - (i + 1)])
					return 0;
		}
	
				// everything went just fine
		return 1;
}


		//
		// toggle the yellow led to indicate that we're not stuck somewhere
		//
void toggle_yellow_led(void)
{
		static int led_state = 0;
	
		led_state = !led_state;
	
		if (led_state) led_on(LED_YELLOW);
		else           led_off(LED_YELLOW);
}


		//
		// end of file
		//
