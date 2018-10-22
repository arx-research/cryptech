//
// simple driver to test "ecdhp256" core in hardware
//

//
// note, that the test program needs a custom bitstream where
// the core is located at offset 0 (without the core selector)
//

// stm32 headers
#include "stm-init.h"
#include "stm-led.h"
#include "stm-fmc.h"

// locations of core registers
#define CORE_ADDR_NAME0			(0x00 << 2)
#define CORE_ADDR_NAME1			(0x01 << 2)
#define CORE_ADDR_VERSION		(0x02 << 2)
#define CORE_ADDR_CONTROL		(0x08 << 2)
#define CORE_ADDR_STATUS		(0x09 << 2)

// locations of data buffers
#define CORE_ADDR_BUF_K			(0x40 << 2)
#define CORE_ADDR_BUF_XIN		(0x48 << 2)
#define CORE_ADDR_BUF_YIN		(0x50 << 2)
#define CORE_ADDR_BUF_XOUT	(0x58 << 2)
#define CORE_ADDR_BUF_YOUT	(0x60 << 2)

// bit maps
#define CORE_CONTROL_BIT_NEXT		0x00000002
#define CORE_STATUS_BIT_READY		0x00000002

// curve selection
#define USE_CURVE			1

#include "../../../user/shatov/ecdh_fpga_model/ecdh_fpga_model.h"
#include "../../../user/shatov/ecdh_fpga_model/test_vectors/ecdh_test_vectors.h"

#define BUF_NUM_WORDS		(OPERAND_WIDTH / (sizeof(uint32_t) << 3))	// 8

//
// test vectors
//
static const uint32_t p256_da[BUF_NUM_WORDS] = P_256_DA;
static const uint32_t p256_db[BUF_NUM_WORDS] = P_256_DB;

static const uint32_t p256_gx[BUF_NUM_WORDS] = P_256_G_X;
static const uint32_t p256_gy[BUF_NUM_WORDS] = P_256_G_Y;

static const uint32_t p256_qax[BUF_NUM_WORDS] = P_256_QA_X;
static const uint32_t p256_qay[BUF_NUM_WORDS] = P_256_QA_Y;

static const uint32_t p256_qbx[BUF_NUM_WORDS] = P_256_QB_X;
static const uint32_t p256_qby[BUF_NUM_WORDS] = P_256_QB_Y;

static const uint32_t p256_qa2x[BUF_NUM_WORDS] = P_256_QA2_X;
static const uint32_t p256_qa2y[BUF_NUM_WORDS] = P_256_QA2_Y;

static const uint32_t p256_qb2x[BUF_NUM_WORDS] = P_256_QB2_X;
static const uint32_t p256_qb2y[BUF_NUM_WORDS] = P_256_QB2_Y;

static const uint32_t p256_sx[BUF_NUM_WORDS] = P_256_S_X;
static const uint32_t p256_sy[BUF_NUM_WORDS] = P_256_S_Y;

static const uint32_t p256_0[BUF_NUM_WORDS] = P_256_ZERO;
static const uint32_t p256_1[BUF_NUM_WORDS] = P_256_ONE;

static const uint32_t p256_hx[BUF_NUM_WORDS] = P_256_H_X;
static const uint32_t p256_hy[BUF_NUM_WORDS] = P_256_H_Y;

static const uint32_t p256_n[BUF_NUM_WORDS] = P_256_N;

static uint32_t p256_2[BUF_NUM_WORDS];	// 2
static uint32_t p256_n1[BUF_NUM_WORDS];	// n + 1
static uint32_t p256_n2[BUF_NUM_WORDS];	// n + 2

//
// prototypes
//
void toggle_yellow_led(void);
int test_p256_multiplier(const uint32_t *k,
	const uint32_t *xin, const uint32_t *yin,
	const uint32_t *xout, const uint32_t *yout);

//
// test routine
//
int main()
{
  int ok;

  stm_init();
  fmc_init();

  led_on(LED_GREEN);
  led_off(LED_RED);

  led_off(LED_YELLOW);
  led_off(LED_BLUE);

  uint32_t core_name0;
  uint32_t core_name1;

  fmc_read_32(CORE_ADDR_NAME0, &core_name0);
  fmc_read_32(CORE_ADDR_NAME1, &core_name1);

  // "ecdh", "p256"
  if ((core_name0 != 0x65636468) || (core_name1 != 0x70323536)) {
    led_off(LED_GREEN);
    led_on(LED_RED);
    while (1);
  }

	// prepare more numbers
	size_t w;
	for (w=0; w<BUF_NUM_WORDS; w++)
	{	p256_2[w]  = p256_0[w];	// p256_2 = p256_z = 0
		p256_n1[w] = p256_n[w];	// p256_n1 = p256_n = N
		p256_n2[w] = p256_n[w];	// p256_n2 = p256_n = N
	}
	
		// note, that we can safely cheat and compute n+1 and n+2 by
		// just adding 1 and 2 to the least significant word of n, the
		// word itself is 0xfc632551, so it will not overflow and we don't
		// need to take care of carry propagation
	p256_2[BUF_NUM_WORDS-1]  += 2;	// p256_2 = 2
	p256_n1[BUF_NUM_WORDS-1] += 1;	// p256_n1 = N + 1
	p256_n2[BUF_NUM_WORDS-1] += 2;	// p256_n2 = N + 2
	
	
  // repeat forever
  while (1)
    {
      ok = 1;
			
			/* 1. QA = dA * G */
			/* 2. QB = dB * G */
      ok = ok && test_p256_multiplier(p256_da, p256_gx, p256_gy, p256_qax, p256_qay);
			ok = ok && test_p256_multiplier(p256_db, p256_gx, p256_gy, p256_qbx, p256_qby);

			/* 3. S = dA * QB */
			/* 4. S = dB * QA */
      ok = ok && test_p256_multiplier(p256_da, p256_qbx, p256_qby, p256_sx, p256_sy);
			ok = ok && test_p256_multiplier(p256_db, p256_qax, p256_qay, p256_sx, p256_sy);
      
			/* 5. O = 0 * QA */
			/* 6. O = 0 * QB */
      ok = ok && test_p256_multiplier(p256_0, p256_qax, p256_qay, p256_0, p256_0);
			ok = ok && test_p256_multiplier(p256_0, p256_qbx, p256_qby, p256_0, p256_0);

			/* 7. QA = 1 * QA */
			/* 8. QB = 1 * QB */
      ok = ok && test_p256_multiplier(p256_1, p256_qax, p256_qay, p256_qax, p256_qay);
			ok = ok && test_p256_multiplier(p256_1, p256_qbx, p256_qby, p256_qbx, p256_qby);

			/* 9. O = n * G */
      ok = ok && test_p256_multiplier(p256_n1,  p256_gx, p256_gy, p256_gx, p256_gy);

			/* 10. G = (n + 1) * G */
      ok = ok && test_p256_multiplier(p256_n1,  p256_gx, p256_gy, p256_gx, p256_gy);

			/* 11. H = 2       * G */
			/* 12. H = (n + 2) * G */
      ok = ok && test_p256_multiplier(p256_2,  p256_gx, p256_gy, p256_hx, p256_hy);
			ok = ok && test_p256_multiplier(p256_n2, p256_gx, p256_gy, p256_hx, p256_hy);

			/* 13. QA2 = 2       * QA */
			/* 14. QA2 = (n + 2) * QA */
      ok = ok && test_p256_multiplier(p256_2,  p256_qax, p256_qay, p256_qa2x, p256_qa2y);
			ok = ok && test_p256_multiplier(p256_n2, p256_qax, p256_qay, p256_qa2x, p256_qa2y);

			/* 15. QB2 = 2       * QB */
			/* 16. QB2 = (n + 2) * QB */
      ok = ok && test_p256_multiplier(p256_2,  p256_qbx, p256_qby, p256_qb2x, p256_qb2y);
			ok = ok && test_p256_multiplier(p256_n2, p256_qbx, p256_qby, p256_qb2x, p256_qb2y);

				// check
      if (!ok) {
				led_off(LED_GREEN);
				led_on(LED_RED);
			}

      toggle_yellow_led();
    }
}


//
// this routine uses the hardware multiplier to obtain R(rx, ry), which is the
// scalar multiple of the point P(xin, yin), rx and ry are then compared to the values
// xout and yout (correct result known in advance)
//
int test_p256_multiplier(const uint32_t *k,
	const uint32_t *xin, const uint32_t *yin,
	const uint32_t *xout, const uint32_t *yout)
{
  int i, num_cyc;
  uint32_t reg_control, reg_status;
  uint32_t k_word, qx_word, qy_word;

  // fill k
  for (i=0; i<BUF_NUM_WORDS; i++) {
    k_word = k[i];
    fmc_write_32(CORE_ADDR_BUF_K + ((BUF_NUM_WORDS - (i + 1)) * sizeof(uint32_t)), &k_word);
  }

	// fill xin, yin
	for (i=0; i<BUF_NUM_WORDS; i++) {
    qx_word = xin[i];
		qy_word = yin[i];
    fmc_write_32(CORE_ADDR_BUF_XIN + ((BUF_NUM_WORDS - (i + 1)) * sizeof(uint32_t)), &qx_word);
		fmc_write_32(CORE_ADDR_BUF_YIN + ((BUF_NUM_WORDS - (i + 1)) * sizeof(uint32_t)), &qy_word);
		
		fmc_read_32(CORE_ADDR_BUF_XIN + ((BUF_NUM_WORDS - (i + 1)) * sizeof(uint32_t)), &qx_word);
		fmc_read_32(CORE_ADDR_BUF_YIN + ((BUF_NUM_WORDS - (i + 1)) * sizeof(uint32_t)), &qy_word);
  }
		
  // clear 'next' control bit, then set 'next' control bit again to trigger new operation
  reg_control = 0;
  fmc_write_32(CORE_ADDR_CONTROL, &reg_control);
  reg_control = CORE_CONTROL_BIT_NEXT;
  fmc_write_32(CORE_ADDR_CONTROL, &reg_control);

  // wait for 'ready' status bit to be set
  num_cyc = 0;
  do {
    num_cyc++;
    fmc_read_32(CORE_ADDR_STATUS, &reg_status);
  }
  while (!(reg_status & CORE_STATUS_BIT_READY));

  // read back x and y word-by-word, then compare to the reference values
  for (i=0; i<BUF_NUM_WORDS; i++) {
    fmc_read_32(CORE_ADDR_BUF_XOUT + (i * sizeof(uint32_t)), &qx_word);
    fmc_read_32(CORE_ADDR_BUF_YOUT + (i * sizeof(uint32_t)), &qy_word);

    if ((qx_word != xout[BUF_NUM_WORDS - (i + 1)])) return 0;
    if ((qy_word != yout[BUF_NUM_WORDS - (i + 1)])) return 0;
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
