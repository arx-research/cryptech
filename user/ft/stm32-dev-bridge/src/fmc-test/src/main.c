//------------------------------------------------------------------------------
// main.c
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Headers
//------------------------------------------------------------------------------
#include "stm32f4xx_hal.h"
#include "stm-fmc.h"
#include "stm-uart.h"

//------------------------------------------------------------------------------
// Defines
//------------------------------------------------------------------------------
#define GPIO_PORT_LEDS        GPIOJ

#define GPIO_PIN_LED_RED      GPIO_PIN_1
#define GPIO_PIN_LED_YELLOW		GPIO_PIN_2
#define GPIO_PIN_LED_GREEN    GPIO_PIN_3
#define GPIO_PIN_LED_BLUE     GPIO_PIN_4


//------------------------------------------------------------------------------
// Macros
//------------------------------------------------------------------------------
#define led_on(pin)			HAL_GPIO_WritePin(GPIO_PORT_LEDS,pin,GPIO_PIN_SET)
#define led_off(pin)		HAL_GPIO_WritePin(GPIO_PORT_LEDS,pin,GPIO_PIN_RESET)
#define led_toggle(pin)	HAL_GPIO_TogglePin(GPIO_PORT_LEDS,pin)


//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------
RNG_HandleTypeDef rng_inst;

volatile uint32_t data_diff = 0;
volatile uint32_t addr_diff = 0;


//------------------------------------------------------------------------------
// Prototypes
//------------------------------------------------------------------------------
void SystemClock_Config(void);

static void MX_RNG_Init(void);
static void MX_GPIO_Init(void);

int test_fpga_data_bus(void);
int test_fpga_address_bus(void);


//------------------------------------------------------------------------------
// Defines
//------------------------------------------------------------------------------
#define TEST_NUM_ROUNDS		100000


//------------------------------------------------------------------------------
int main(void)
//------------------------------------------------------------------------------
{
  int i;
  // initialize hal
  HAL_Init();

  // configure system clock
  SystemClock_Config();

  // initialize gpio
  MX_GPIO_Init();

  // initialize UART for debug output
  MX_USART2_UART_Init();

  volatile uint32_t hclk;

  hclk = HAL_RCC_GetHCLKFreq();
  uart_send_string("HCLK: ");
  uart_send_integer(hclk, 0);
  uart_send_string("\r\n");

  uart_send_string("Keep calm for Novena boot...\r\n");

  // Blink blue LED for six seconds to not upset the Novena at boot.
  led_on(GPIO_PIN_LED_BLUE);
  for (i = 0; i < 12; i++) {
    HAL_Delay(500);
    led_toggle(GPIO_PIN_LED_BLUE);
  }

  // initialize rng
  MX_RNG_Init();

  // prepare fmc interface
  fmc_init();

  // turn on green led, turn off other leds
  led_on(GPIO_PIN_LED_GREEN);
  led_off(GPIO_PIN_LED_YELLOW);
  led_off(GPIO_PIN_LED_RED);
  led_off(GPIO_PIN_LED_BLUE);

  // vars
  volatile int data_test_ok = 0, addr_test_ok = 0, successful_runs = 0, failed_runs = 0, sleep = 0;

  // main loop (test, until an error is detected)
  while (1)
    {
      // test data bus
      data_test_ok = test_fpga_data_bus();
      // test address bus
      addr_test_ok = test_fpga_address_bus();

      uart_send_string("Data: ");
      uart_send_integer(data_test_ok, 100000);
      uart_send_string(", addr: ");
      uart_send_integer(addr_test_ok, 100000);
      uart_send_string("\r\n");

      if (data_test_ok == TEST_NUM_ROUNDS &&
	  addr_test_ok == TEST_NUM_ROUNDS) {
	// toggle yellow led to indicate, that we are alive
	led_toggle(GPIO_PIN_LED_YELLOW);

	successful_runs++;
	sleep = 100;
      } else {
	led_on(GPIO_PIN_LED_RED);
	failed_runs++;
	sleep = 2000;
      }

      uart_send_string("Success ");
      uart_send_integer(successful_runs, 0);
      uart_send_string(", fail ");
      uart_send_integer(failed_runs, 0);
      uart_send_string("\r\n\r\n");

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
      ok = fmc_write_32(0, &rnd);
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

      // we only have 2^22 32-bit words
      rnd &= 0x00FFFFFC;

      // don't test zero addresses (fpga will store data, not address)
      if (rnd == 0) continue;

      // write dummy value to fpga at some non-zero address
      ok = fmc_write_32(rnd, &buf);
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

  return c;
}


//------------------------------------------------------------------------------
void SystemClock_Config(void)
//------------------------------------------------------------------------------
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

  __PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 12;
  RCC_OscInitStruct.PLL.PLLN = 270;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 8;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  HAL_PWREx_ActivateOverDrive();

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1
    |RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);

  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

}


//------------------------------------------------------------------------------
void MX_RNG_Init(void)
//------------------------------------------------------------------------------
{
  rng_inst.Instance = RNG;
  HAL_RNG_Init(&rng_inst);
}


//------------------------------------------------------------------------------
void MX_GPIO_Init(void)
//------------------------------------------------------------------------------
{
  GPIO_InitTypeDef GPIO_InitStruct;

  __GPIOJ_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
  HAL_GPIO_Init(GPIOJ, &GPIO_InitStruct);
}



/**
 * @brief  This function is executed in case of error occurrence.
 * @param  None
 * @retval None
 */
void Error_Handler(void)
{
  volatile int i = 0;
  led_on(GPIO_PIN_LED_RED);
  while (1) {
    i++; /* To not get optimized away/obscured */
  }
}

//------------------------------------------------------------------------------
#ifdef USE_FULL_ASSERT
//------------------------------------------------------------------------------
void assert_failed(uint8_t* file, uint32_t line)
//------------------------------------------------------------------------------
{
}
//------------------------------------------------------------------------------
#endif
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
