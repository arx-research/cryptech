/*
 * Test code that just sends the letters 'a' to 'z' over and
 * over again using USART1.
 *
 * Toggles the BLUE LED slowly and the YELLOW LED for every
 * character sent.
 */
#include "stm_init.h"

#define DELAY() HAL_Delay(250)

UART_HandleTypeDef *huart;

/*
 * If a newline is received on UART1 or UART2, redirect output to that UART.
 */
void check_uart_rx(UART_HandleTypeDef *this) {
  uint8_t rx = 0;
  if (HAL_UART_Receive(this, &rx, 1, 0) == HAL_OK) {
    if (rx == '\n') {
      HAL_GPIO_TogglePin(LED_PORT, LED_GREEN);

      huart = this;
    }
  }
}

int
main()
{
  uint8_t c = 'a';

  stm_init();

  huart = &huart1;

  while (1)
  {
    HAL_GPIO_TogglePin(LED_PORT, LED_YELLOW);

    HAL_UART_Transmit(huart, (uint8_t *) &c, 1, 0xff);
    DELAY();

    if (c++ == 'z') {
      c = 'a';
      HAL_GPIO_TogglePin(LED_PORT, LED_BLUE);
    }

    /* Check for UART change request */
    check_uart_rx(&huart1);
    check_uart_rx(&huart2);
  }
}
