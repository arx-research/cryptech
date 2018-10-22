#ifndef __STM32_DEV_BRIDGE_UART_H
#define __STM32_DEV_BRIDGE_UART_H

#include "stm32f4xx_hal.h"

#define USART2_BAUD_RATE	921600

extern void uart_send_binary(uint32_t num, uint8_t bits);
extern void uart_send_string(char *s);
extern void uart_send_integer(uint32_t data, uint32_t mag);
extern void uart_send_hexbyte(const uint32_t data);
extern void uart_send_hexdump(const uint8_t *buf, const uint8_t start_offset, const uint8_t end_offset);

extern UART_HandleTypeDef huart2;

#endif /* __STM32_DEV_BRIDGE_UART_H */
