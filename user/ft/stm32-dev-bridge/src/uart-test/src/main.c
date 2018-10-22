/*
 * Test code that just sends the letters 'a' to 'z' over and
 * over again using USART2.
 *
 * Toggles the BLUE LED slowly and the RED LED for every
 * character sent.
 */
#include "stm_init.h"

#define DELAY() HAL_Delay(100)

int
main()
{
  uint8_t c = 'a';

  stm_init();

  while (1)
  {
    HAL_GPIO_TogglePin(LED_PORT, LED_RED);

    HAL_UART_Transmit(&huart2, (uint8_t *) &c, 1, 0x1);
    DELAY();

    if (c++ == 'z') {
      c = 'a';
      HAL_GPIO_TogglePin(LED_PORT, LED_BLUE);
    }
  }
}
