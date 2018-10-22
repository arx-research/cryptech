/*
 * stm-uart.c
 * ----------
 * Functions for sending strings and numbers over the uart.
 *
 * Copyright (c) 2015, NORDUnet A/S All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the NORDUnet nor the names of its contributors may
 *   be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "stm-init.h"
#include "stm-uart.h"

#include <string.h>

UART_HandleTypeDef huart_mgmt;  /* USART1 */
UART_HandleTypeDef huart_user;  /* USART2 */

DMA_HandleTypeDef hdma_usart_mgmt_rx;
DMA_HandleTypeDef hdma_usart_user_rx;

UART_HandleTypeDef* default_uart = STM_UART_MGMT;

#ifdef HAL_DMA_MODULE_ENABLED
/**
 * Enable DMA controller clock
 */
static void MX_DMA_Init(void)
{
    /* DMA controller clock enable */
    __HAL_RCC_DMA2_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* DMA interrupt init */

    /* USER UART RX */
    HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
    /* MGMT UART RX */
    HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);
}
#endif /* HAL_DMA_MODULE_ENABLED */

/* USART1 init function */
static void MX_USART1_UART_Init(void)
{
  huart_mgmt.Instance = USART1;
  huart_mgmt.Init.BaudRate = USART_MGMT_BAUD_RATE;
  huart_mgmt.Init.WordLength = UART_WORDLENGTH_8B;
  huart_mgmt.Init.StopBits = UART_STOPBITS_1;
  huart_mgmt.Init.Parity = UART_PARITY_NONE;
  huart_mgmt.Init.Mode = UART_MODE_TX_RX;
  huart_mgmt.Init.HwFlowCtl = UART_HWCONTROL_RTS_CTS;
  huart_mgmt.Init.OverSampling = UART_OVERSAMPLING_16;

#ifdef HAL_DMA_MODULE_ENABLED
  __HAL_LINKDMA(&huart_mgmt, hdmarx, hdma_usart_mgmt_rx);
#endif

  if (HAL_UART_Init(&huart_mgmt) != HAL_OK) {
    /* Initialization Error */
    Error_Handler();
  }
}
/* USART2 init function */
static void MX_USART2_UART_Init(void)
{
  huart_user.Instance = USART2;
  huart_user.Init.BaudRate = USART_USER_BAUD_RATE;
  huart_user.Init.WordLength = UART_WORDLENGTH_8B;
  huart_user.Init.StopBits = UART_STOPBITS_1;
  huart_user.Init.Parity = UART_PARITY_NONE;
  huart_user.Init.Mode = UART_MODE_TX_RX;
  huart_user.Init.HwFlowCtl = UART_HWCONTROL_RTS_CTS;
  huart_user.Init.OverSampling = UART_OVERSAMPLING_16;

#ifdef HAL_DMA_MODULE_ENABLED
  __HAL_LINKDMA(&huart_user, hdmarx, hdma_usart_user_rx);
#endif

  if (HAL_UART_Init(&huart_user) != HAL_OK) {
    /* Initialization Error */
    Error_Handler();
  }
}

void uart_init(void)
{
#ifdef HAL_DMA_MODULE_ENABLED
  MX_DMA_Init();
#endif
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
}

void uart_set_default(UART_HandleTypeDef *uart)
{
    default_uart = uart;
}

/* send a single character */
HAL_StatusTypeDef uart_send_char2(UART_HandleTypeDef *uart, uint8_t ch)
{
    return uart_send_bytes2(uart, &ch, 1);
}

/* receive a single character */
HAL_StatusTypeDef uart_recv_char2(UART_HandleTypeDef *uart, uint8_t *cp, uint32_t timeout)
{
    return HAL_UART_Receive(uart, cp, 1, timeout);
}

/* send a string */
HAL_StatusTypeDef uart_send_string2(UART_HandleTypeDef *uart, const char *s)
{
    return uart_send_bytes2(uart, (uint8_t *) s, strlen(s));
}

/* send raw bytes */
HAL_StatusTypeDef uart_send_bytes2(UART_HandleTypeDef *uart, uint8_t *buf, size_t len)
{
    for (int timeout = 0; timeout < 100; ++timeout) {
        HAL_UART_StateTypeDef status = HAL_UART_GetState(uart);
        if (status == HAL_UART_STATE_READY ||
            status == HAL_UART_STATE_BUSY_RX)
            return HAL_UART_Transmit(uart, (uint8_t *) buf, (uint32_t) len, 0x1);
    }

    return HAL_TIMEOUT;
}

/* receive raw bytes */
HAL_StatusTypeDef uart_receive_bytes2(UART_HandleTypeDef *uart, uint8_t *buf, size_t len, uint32_t timeout)
{
    return HAL_UART_Receive(uart, (uint8_t *) buf, (uint32_t) len, timeout);
}

/* Generalized routine to send binary, decimal, and hex integers.
 * This code is adapted from Chris Giese's printf.c
 */
HAL_StatusTypeDef uart_send_number2(UART_HandleTypeDef *uart, uint32_t num, uint8_t digits, uint8_t radix)
{
    #define BUFSIZE 32
    char buf[BUFSIZE];
    char *where = buf + BUFSIZE;

    /* initialize buf so we can add leading 0 by adjusting the pointer */
    memset(buf, '0', BUFSIZE);

    /* build the string backwards, starting with the least significant digit */
    do {
	uint32_t temp;
	temp = num % radix;
	where--;
	if (temp < 10)
	    *where = temp + '0';
	else
	    *where = temp - 10 + 'A';
	num = num / radix;
    } while (num != 0);

    if (where > buf + BUFSIZE - digits)
	/* pad with leading 0 */
	where = buf + BUFSIZE - digits;
    else
	/* number is larger than the specified number of digits */
	digits = buf + BUFSIZE - where;

    return uart_send_bytes2(uart, (uint8_t *) where, digits);
}

HAL_StatusTypeDef uart_send_hexdump2(UART_HandleTypeDef *uart, const uint8_t *buf,
                                     const uint8_t start_offset, const uint8_t end_offset)
{
    uint32_t i;

    uart_send_number2(uart, start_offset, 2, 16);
    uart_send_string2(uart, " -- ");

    for (i = start_offset; i <= end_offset; i++) {
	if (i && (! (i % 16))) {
	    uart_send_string2(uart, "\r\n");

	    if (i != end_offset) {
		/* Output new offset unless the last byte is reached */
		uart_send_number2(uart, i, 2, 16);
		uart_send_string2(uart, " -- ");
	    }
	}

	uart_send_number2(uart, *(buf + i), 2, 16);
	uart_send_string2(uart, " ");
    }

    return HAL_OK;
}
