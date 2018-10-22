#ifndef __STM32_DEV_BRIDGE_UART_H
#define __STM32_DEV_BRIDGE_UART_H

#include "stm32f4xx_hal.h"

#define USART2_BAUD_RATE	115200

extern void MX_USART2_UART_Init(void);

extern void uart_send_binary(uint32_t num, uint8_t bits);
extern void uart_send_string(char *s);
extern void uart_send_integer(uint32_t data, uint32_t mag);

extern UART_HandleTypeDef huart2;

#endif /* __STM32_DEV_BRIDGE_UART_H */
