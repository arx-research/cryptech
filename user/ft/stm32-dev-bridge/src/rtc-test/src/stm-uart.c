/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "stm-uart.h"

#include <string.h>

extern void Error_Handler();

UART_HandleTypeDef huart2;


/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

inline void uart_putc(uint8_t *ch) {
  HAL_UART_Transmit(&huart2, ch, 1, 0x1);
}

void uart_send_binary(uint32_t num, uint8_t bits)
{
  uint32_t i;
  unsigned char ch;

  bits--;  /* bits 4 should give i = 1000, not 10000 */

  i = 1 << bits;
  while (i) {
    ch = '0';
    if (num & i) {
      ch = '1';
    }

    uart_putc(&ch);
    i = i >> 1;
  }
}

void uart_send_string(char *s)
{
  HAL_UART_Transmit(&huart2, (uint8_t *) s, strlen(s), 0x1);
}

void uart_send_integer(uint32_t data, uint32_t mag) {
  uint32_t i, t;
  unsigned char ch;

  if (! mag) {
    /* Find magnitude */
    if (data < 10) {
      ch = '0' + data;
      HAL_UART_Transmit(&huart2, (uint8_t *) &ch, 1, 0x1);
      return;
    }

    for (mag = 10; mag < data; mag = i) {
      i = mag * 10;
      if (i > data || i < mag)
        break;
    }
  }
  /* mag is now 10 if data is 45, and 1000 if data is 1009 */
  for (i = mag; i; i /= 10) {
    t = (data / i);
    ch = '0' + t;
    HAL_UART_Transmit(&huart2, (uint8_t *) &ch, 1, 0x1);
    data -= (t * i);
  }
}

void uart_send_hexbyte(const uint32_t data)
{
  int i, j = 8;
  uint8_t ch;

  while (j) {
    j -= 4;
    i = (data >> j) & 0x0f;
    if (i > 9) {
      ch = 'a' + i - 10;
      uart_putc(&ch);
    } else {
      ch = '0' + i;
      uart_putc(&ch);
    }
  }
}

void uart_send_hexdump(const uint8_t *buf, const uint8_t start_offset, const uint8_t end_offset)
{
  uint32_t i;

  uart_send_string("00 -- ");

  for (i = 0; i <= end_offset; i++) {
    if (i && (! (i % 16))) {
      uart_send_string("\r\n");

      if (i != end_offset) {
	/* Output new offset unless the last byte is reached */
	uart_send_hexbyte(i);
	uart_send_string(" -- ");
      }
    }

    uart_send_hexbyte(*(buf + i));
    uart_send_string(" ");
  }
}
