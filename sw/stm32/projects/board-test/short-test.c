/*
 * Test code that just sends the letters 'a' to 'z' over and
 * over again using USART2.
 *
 * Toggles the BLUE LED slowly and the RED LED for every
 * character sent.
 */
#include "stm-init.h"
#include "stm-led.h"
#include "stm-uart.h"

void test_for_shorts(char port, GPIO_TypeDef* GPIOx, uint16_t GPIO_Test_Pins);

//------------------------------------------------------------------------------
// Defines
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Macros
//------------------------------------------------------------------------------

/* These are all the pins used by the FMC interface */
#define GPIOB_PINS  (GPIO_PIN_7)

#define GPIOD_PINS  (GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11	\
		     |GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15	\
		     |GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_3|GPIO_PIN_4	\
		     |GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7)

#define GPIOE_PINS  (GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_7	\
		     |GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11	\
		     |GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15)

#define GPIOF_PINS  (GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3	\
		     |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_12|GPIO_PIN_13	\
		     |GPIO_PIN_14|GPIO_PIN_15)

#define GPIOG_PINS  (GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3	\
		     |GPIO_PIN_4|GPIO_PIN_5)

#define GPIOH_PINS  (GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11	\
		     |GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15)

#define GPIOI_PINS  (GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_0|GPIO_PIN_1	\
		     |GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_6|GPIO_PIN_7)

int
main()
{
  stm_init();

  // enable gpio clocks
  __GPIOA_CLK_ENABLE();
  __GPIOB_CLK_ENABLE();
  __GPIOD_CLK_ENABLE();
  __GPIOE_CLK_ENABLE();
  __GPIOF_CLK_ENABLE();
  __GPIOG_CLK_ENABLE();
  __GPIOH_CLK_ENABLE();
  __GPIOI_CLK_ENABLE();

  while (1) {
    HAL_GPIO_TogglePin(LED_PORT, LED_GREEN);
    uart_send_string("\r\n\r\n\r\n\r\n\r\n");

    test_for_shorts('B', GPIOB, GPIOB_PINS);
    test_for_shorts('D', GPIOD, GPIOD_PINS);
    test_for_shorts('E', GPIOE, GPIOE_PINS);
    test_for_shorts('F', GPIOF, GPIOF_PINS);
    test_for_shorts('G', GPIOG, GPIOG_PINS);
    test_for_shorts('H', GPIOH, GPIOH_PINS);
    test_for_shorts('I', GPIOI, GPIOI_PINS);
    led_toggle(LED_BLUE);
    HAL_Delay(2000);
  }
}

void configure_all_as_input(GPIO_TypeDef* GPIOx, uint16_t GPIO_Test_Pins)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  /* Configure all pins as input. XXX do all pins (0xffff) instead of just GPIO_Test_Pins? */
  GPIO_InitStruct.Pin = GPIO_Test_Pins;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
  HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

uint8_t check_no_input(char port, GPIO_TypeDef* GPIOx, uint16_t GPIO_Test_Pins, char wrote_port, uint16_t wrote_value)
{
  uint16_t read;

  /* Read all pins from port at once. XXX check all pins, not just GPIO_Test_Pins? */
  read = (GPIOx->IDR & GPIO_Test_Pins);

  if (! read) {
    /* No unexpected pins read as HIGH */
    return 0;
  }

  led_on(LED_RED);

  uart_send_string("Wrote ");
  uart_send_binary(wrote_value, 16);

  uart_send_string(" to port GPIO");
  uart_send_char(wrote_port);

  uart_send_string(", read ");
  uart_send_binary(read, 16);

  uart_send_string(" from GPIO");
  uart_send_char(port);

  uart_send_string("\r\n");

  return 1;
}

void test_for_shorts(char port, GPIO_TypeDef* GPIOx, uint16_t GPIO_Test_Pins)
{
  GPIO_InitTypeDef GPIO_InitStruct;
  uint16_t i, fail = 0, Test_Pin, read;

  configure_all_as_input(GPIOB, GPIOB_PINS);
  configure_all_as_input(GPIOD, GPIOD_PINS);
  configure_all_as_input(GPIOE, GPIOE_PINS);
  configure_all_as_input(GPIOF, GPIOF_PINS);
  configure_all_as_input(GPIOG, GPIOG_PINS);
  configure_all_as_input(GPIOH, GPIOH_PINS);
  configure_all_as_input(GPIOI, GPIOI_PINS);

  check_no_input('B', GPIOB, GPIOB_PINS, 'x', 0);
  check_no_input('D', GPIOD, GPIOD_PINS, 'x', 0);
  check_no_input('E', GPIOE, GPIOE_PINS, 'x', 0);
  check_no_input('F', GPIOF, GPIOF_PINS, 'x', 0);
  check_no_input('G', GPIOG, GPIOG_PINS, 'x', 0);
  check_no_input('H', GPIOH, GPIOH_PINS, 'x', 0);
  check_no_input('I', GPIOI, GPIOI_PINS, 'x', 0);

  for (i = 0; i < 31; i++) {
    Test_Pin = 1 << i;
    if (! (GPIO_Test_Pins & Test_Pin)) continue;

    configure_all_as_input(GPIOx, GPIO_Test_Pins);

    /* Change one pin to output */
    GPIO_InitStruct.Pin = Test_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
    HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOx, Test_Pin, GPIO_PIN_SET);

    /* Slight delay after setting the output pin. Without this, the Test_Pin
       bit might read as zero, as it is only sampled once every AHB1 clock cycle.
       Reference manual DM00031020 section 8.3.1.
    */
    HAL_Delay(1);

    /* Read all input GPIOs from port at once. XXX check all pins, not just GPIO_Test_Pins? */
    read = GPIOx->IDR & GPIO_Test_Pins;

    if (read == Test_Pin) {
      /* No unexpected pins read as HIGH */
      led_toggle(LED_GREEN);
    } else {
      led_on(LED_RED);
      uart_send_string("GPIO");
      uart_send_char(port);

      uart_send_string(" exp ");
      uart_send_binary(Test_Pin, 16);

      uart_send_string(" got ");
      uart_send_binary(read, 16);

      uart_send_string(" diff ");
      uart_send_binary(read ^ Test_Pin, 16);

      uart_send_string("\r\n");

      fail++;
    }

    /* Check there is no input on any of the other GPIO ports (adjacent pins might live on different ports) */
    if (port != 'B') fail += check_no_input('B', GPIOB, GPIOB_PINS, port, Test_Pin);
    if (port != 'D') fail += check_no_input('D', GPIOD, GPIOD_PINS, port, Test_Pin);
    if (port != 'E') fail += check_no_input('E', GPIOE, GPIOE_PINS, port, Test_Pin);
    if (port != 'F') fail += check_no_input('F', GPIOF, GPIOF_PINS, port, Test_Pin);
    if (port != 'G') fail += check_no_input('G', GPIOG, GPIOG_PINS, port, Test_Pin);
    if (port != 'H') fail += check_no_input('H', GPIOH, GPIOH_PINS, port, Test_Pin);
    if (port != 'I') fail += check_no_input('I', GPIOI, GPIOI_PINS, port, Test_Pin);

    HAL_GPIO_WritePin(GPIOx, Test_Pin, GPIO_PIN_RESET);
  }

  if (fail) {
    uart_send_string("\r\n");
  }
}
