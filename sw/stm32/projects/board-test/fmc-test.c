//------------------------------------------------------------------------------
// main.c
//------------------------------------------------------------------------------

/*
 This requires a special bitstream with a special test register.
 See core/platform/alpha/rtl/alpha_fmc_test.v:
   //----------------------------------------------------------------
   // Dummy Register
   //
   // General-purpose register to test FMC interface using STM32
   // demo program instead of core selector logic.
   //
   // This register is a bit tricky, but it allows testing of both
   // data and address buses. Reading from FPGA will always return
   // value, which is currently stored in the test register, 
   // regardless of read transaction address. Writing to FPGA has
   // two variants: a) writing to address 0 will store output data
   // data value in the test register, b) writing to any non-zero
   // address will store _address_ of write transaction in the test
   // register.
   //
   // To test data bus, write some different patterns to address 0,
   // then readback from any address and compare.
   //
   // To test address bus, write anything to some different non-zero
   // addresses, then readback from any address and compare returned
   // value with previously written address.
   //
   //----------------------------------------------------------------
 */

//------------------------------------------------------------------------------
// Headers
//------------------------------------------------------------------------------
#include "stm-init.h"
#include "stm-led.h"
#include "stm-fmc.h"
#include "stm-uart.h"
#include "stm-fpgacfg.h"

//------------------------------------------------------------------------------
// Defines
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Macros
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------
RNG_HandleTypeDef rng_inst;

// FT: "I changed some interesting-to-look-at-in-the-debugger values to be
// volatile, so that my compiler wouldn't optimize/obscure them."

volatile uint32_t data_diff = 0;
volatile uint32_t addr_diff = 0;


//------------------------------------------------------------------------------
// Prototypes
//------------------------------------------------------------------------------
/* XXX move this to stm-rng.[ch] */
static void MX_RNG_Init(void);

int test_fpga_data_bus(void);
int test_fpga_address_bus(void);


//------------------------------------------------------------------------------
// Defines
//------------------------------------------------------------------------------
#define TEST_NUM_ROUNDS		100000
#define VERBOSE			0


//------------------------------------------------------------------------------
int main(void)
//------------------------------------------------------------------------------
{
  int i;
  
  stm_init();

  uart_send_string("Keep calm for FPGA bitstream loading...\r\n");

  // Blink blue LED until the FPGA reports it has loaded its bitstream
  led_on(LED_BLUE);
  while (! fpgacfg_check_done()) {
      for (i = 0; i < 4; i++) {
	  HAL_Delay(500);
	  led_toggle(LED_BLUE);
      }
  }

  // initialize rng
  MX_RNG_Init();

  // turn on green led, turn off other leds
  led_on(LED_GREEN);
  led_off(LED_YELLOW);
  led_off(LED_RED);
  led_off(LED_BLUE);

  // vars
  volatile int data_test_ok = 0, addr_test_ok = 0, successful_runs = 0, failed_runs = 0, sleep = 0;

  // main loop (test, until an error is detected)
  while (1)
    {
      // test data bus
      data_test_ok = test_fpga_data_bus();
      // test address bus
      addr_test_ok = test_fpga_address_bus();

      if (VERBOSE ||
          (data_test_ok != TEST_NUM_ROUNDS ||
           addr_test_ok != TEST_NUM_ROUNDS)) {
        uart_send_string("Data: ");
        uart_send_integer(data_test_ok, 6);
        uart_send_string(", addr: ");
        uart_send_integer(addr_test_ok, 6);
        uart_send_string("\r\n");
      }

      if (data_test_ok == TEST_NUM_ROUNDS &&
	  addr_test_ok == TEST_NUM_ROUNDS) {
	// toggle yellow led to indicate, that we are alive
	led_toggle(LED_YELLOW);

	successful_runs++;
	sleep = 0;
      } else {
	led_on(LED_RED);
	failed_runs++;
	sleep = 2000;
      }

      uart_send_string("Success ");
      uart_send_integer(successful_runs, 1);
      uart_send_string(", fail ");
      uart_send_integer(failed_runs, 1);
      uart_send_string("\r\n");
      if (VERBOSE)
        uart_send_string("\r\n");
      HAL_Delay(sleep);
    }

  // should never reach this line
}


//------------------------------------------------------------------------------
int test_fpga_data_bus(void)
//------------------------------------------------------------------------------
{
  int c, ok;
  uint32_t rnd, buf;
  HAL_StatusTypeDef hal_result;

  // run some rounds of data bus test
  for (c=0; c<TEST_NUM_ROUNDS; c++)
    {
      data_diff = 0;
      // try to generate "random" number
      hal_result = HAL_RNG_GenerateRandomNumber(&rng_inst, &rnd);
      if (hal_result != HAL_OK) break;

      // write value to fpga at address 0
      ok = fmc_write_32(0, rnd);
      if (ok != 0) break;

      // read value from fpga
      ok = fmc_read_32(0, &buf);
      if (ok != 0) break;

      // compare (abort testing in case of error)
      if (buf != rnd)
	{
	  data_diff = buf;
	  data_diff ^= rnd;

	  uart_send_string("Data bus fail: expected ");
	  uart_send_binary(rnd, 32);
	  uart_send_string(", got ");
	  uart_send_binary(buf, 32);
	  uart_send_string(", diff ");
	  uart_send_binary(data_diff, 32);
	  uart_send_string("\r\n");

	  break;
	}
    }

  data_diff = buf;
  data_diff ^= rnd;

  if (VERBOSE || data_diff) {
      uart_send_string("Sample of data bus test data: expected ");
      uart_send_binary(rnd, 32);
      uart_send_string(", got ");
      uart_send_binary(buf, 32);
      uart_send_string(", diff ");
      uart_send_binary(data_diff, 32);
      uart_send_string("\r\n");
  }

  // return number of successful tests
  return c;
}


//------------------------------------------------------------------------------
int test_fpga_address_bus(void)
//------------------------------------------------------------------------------
{
  int c, ok;
  uint32_t rnd, buf;
  HAL_StatusTypeDef hal_result;

  // run some rounds of address bus test
  for (c=0; c<TEST_NUM_ROUNDS; c++)
    {
      addr_diff = 0;
      // try to generate "random" number
      hal_result = HAL_RNG_GenerateRandomNumber(&rng_inst, &rnd);
      if (hal_result != HAL_OK) break;

      // there are 26 physicaly connected address lines on the alpha,
      // but "only" 24 usable for now (the top two ones are used by FMC
      // to choose bank, and we only have one bank set up currently)
      rnd &= 0x3fffffc;

      // don't test zero addresses (fpga will store data, not address)
      if (rnd == 0) continue;

      // write dummy value to fpga at some non-zero address
      ok = fmc_write_32(rnd, buf);
      if (ok != 0) break;

      // read value from fpga
      ok = fmc_read_32(0, &buf);
      if (ok != 0) break;

      // fpga receives address of 32-bit word, while we need
      // byte address here to compare
      buf <<= 2;

      // compare (abort testing in case of error)
      if (buf != rnd)
	{
	  addr_diff = buf;
	  addr_diff ^= rnd;

	  uart_send_string("Addr bus fail: expected ");
	  uart_send_binary(rnd, 32);
	  uart_send_string(", got ");
	  uart_send_binary(buf, 32);
	  uart_send_string(", diff ");
	  uart_send_binary(addr_diff, 32);
	  uart_send_string("\r\n");

	  break;
	}
    }

  addr_diff = buf;
  addr_diff ^= rnd;

  if (VERBOSE || addr_diff) {
      uart_send_string("Sample of addr bus test data: expected ");
      uart_send_binary(rnd, 32);
      uart_send_string(", got ");
      uart_send_binary(buf, 32);
      uart_send_string(", diff ");
      uart_send_binary(addr_diff, 32);
      uart_send_string("\r\n");
  }

  return c;
}


//------------------------------------------------------------------------------
static void MX_RNG_Init(void)
//------------------------------------------------------------------------------
{
  rng_inst.Instance = RNG;
  HAL_RNG_Init(&rng_inst);
}


//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
