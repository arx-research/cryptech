//
// simple driver to test "ecdsa384" core in hardware
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
#define CORE_ADDR_BUF_X			(0x50 << 2)
#define CORE_ADDR_BUF_Y			(0x60 << 2)

// bit maps
#define CORE_CONTROL_BIT_NEXT		0x00000002
#define CORE_STATUS_BIT_READY		0x00000002

// curve selection
#define USE_CURVE				2

#include "../../../user/shatov/ecdsa_fpga_model/ecdsa_model.h"

#define BUF_NUM_WORDS		(OPERAND_WIDTH / (sizeof(uint32_t) << 3))	// 8

//
// test vectors
//
static const uint32_t p384_d[BUF_NUM_WORDS]  = ECDSA_D;
static const uint32_t p384_qx[BUF_NUM_WORDS] = ECDSA_Q_X;
static const uint32_t p384_qy[BUF_NUM_WORDS] = ECDSA_Q_Y;

static const uint32_t p384_k[BUF_NUM_WORDS]  = ECDSA_K;
static const uint32_t p384_rx[BUF_NUM_WORDS] = ECDSA_R_X;
static const uint32_t p384_ry[BUF_NUM_WORDS] = ECDSA_R_Y;

static const uint32_t p384_i[BUF_NUM_WORDS]  = ECDSA_ONE;
static const uint32_t p384_gx[BUF_NUM_WORDS] = ECDSA_G_X;
static const uint32_t p384_gy[BUF_NUM_WORDS] = ECDSA_G_Y;

static const uint32_t p384_hx[BUF_NUM_WORDS] = ECDSA_H_X;
static const uint32_t p384_hy[BUF_NUM_WORDS] = ECDSA_H_Y;

static const uint32_t p384_z[BUF_NUM_WORDS]  = ECDSA_ZERO;
static const uint32_t p384_n[BUF_NUM_WORDS]  = ECDSA_N;

static uint32_t p384_2[BUF_NUM_WORDS];	// 2
static uint32_t p384_n1[BUF_NUM_WORDS];	// n + 1
static uint32_t p384_n2[BUF_NUM_WORDS];	// n + 2

//
// prototypes
//
void toggle_yellow_led(void);
int test_p384_multiplier(const uint32_t *k, const uint32_t *px, const uint32_t *py);


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

  // "ecds", "a384"
  if ((core_name0 != 0x65636473) || (core_name1 != 0x61333834)) {
    led_off(LED_GREEN);
    led_on(LED_RED);
    while (1);
  }

	// prepare more numbers
	size_t w;
	for (w=0; w<BUF_NUM_WORDS; w++)
	{	p384_2[w]  = p384_z[w];	// p384_2 = p384_z = 0
		p384_n1[w] = p384_n[w];	// p384_n1 = p384_n = N
		p384_n2[w] = p384_n[w];	// p384_n2 = p384_n = N
	}
	
	p384_2[BUF_NUM_WORDS-1]  += 2;	// p384_2 = 2
	p384_n1[BUF_NUM_WORDS-1] += 1;	// p384_n1 = N + 1
	p384_n2[BUF_NUM_WORDS-1] += 2;	// p384_n2 = N + 2
  // repeat forever
  while (1)
	{
    ok = 1;

    ok = ok && test_p384_multiplier(p384_d, p384_qx, p384_qy);	/* Q = d * G */
    ok = ok && test_p384_multiplier(p384_k, p384_rx, p384_ry);	/* R = k * G */
 
    ok = ok && test_p384_multiplier(p384_z, p384_z,  p384_z);	/* O = 0 * G */
    ok = ok && test_p384_multiplier(p384_i, p384_gx, p384_gy);	/* G = 1 * G */

    ok = ok && test_p384_multiplier(p384_n, p384_z,  p384_z);	/* O = n * G */

    ok = ok && test_p384_multiplier(p384_n1, p384_gx, p384_gy);	/* G = (n + 1) * G */

	//
	// The following two vectors test the virtually never taken path in the curve point
	// addition routine when both input points are the same. During the first test (2 * G)
	// the double of the base point is computed at the second doubling step of the multiplication
	// algorithm, which does not require any special handling. During the second test the
	// precomputed double of the base point (stored in internal read-only memory) is returned,
	// because after doubling of G * ((n + 1) / 2) we get G * (n + 1) = G. The adder then has to
	// compute G + G for which the formulae don't work, and special handling is required. The two
	// test vectors verify that the hardcoded double of the base point matches the one computed
	// on the fly. Note that in practice one should never be multiplying by anything larger than (n-1),
	// because both the secret key and the per-message (random) number must be from [1, n-1].
	//
	ok = ok && test_p384_multiplier(p384_2, p384_hx, p384_hy);	/* H = 2 * G */
	ok = ok && test_p384_multiplier(p384_n2, p384_hx, p384_hy);	/* H = (n + 2) * G */			

    if (!ok) {
		led_off(LED_GREEN);
	    led_on(LED_RED);
    }

    toggle_yellow_led();
  }
}


//
// this routine uses the hardware multiplier to obtain Q(qx,qy), which is the
// scalar multiple of the base point, qx and qy are then compared to the values
// px and py (correct result known in advance)
//
int test_p384_multiplier(const uint32_t *k, const uint32_t *px, const uint32_t *py)
{
  int i, num_cyc;
  uint32_t reg_control, reg_status;
  uint32_t k_word, qx_word, qy_word;

  // fill k
  for (i=0; i<BUF_NUM_WORDS; i++) {
    k_word = k[i];
    fmc_write_32(CORE_ADDR_BUF_K + ((BUF_NUM_WORDS - (i + 1)) * sizeof(uint32_t)), &k_word);
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
    fmc_read_32(CORE_ADDR_BUF_X + (i * sizeof(uint32_t)), &qx_word);
    fmc_read_32(CORE_ADDR_BUF_Y + (i * sizeof(uint32_t)), &qy_word);

    if ((qx_word != px[BUF_NUM_WORDS - (i + 1)])) return 0;
    if ((qy_word != py[BUF_NUM_WORDS - (i + 1)])) return 0;
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
