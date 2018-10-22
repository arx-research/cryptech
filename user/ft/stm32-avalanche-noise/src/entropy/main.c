/*
 * Copyright (c) 2014, 2015 NORDUnet A/S
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *    3. Neither the name of the NORDUnet nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <string.h>

#include "main.h"
#include "stm_init.h"

#define MODE_DELTAS			1
#define MODE_ENTROPY			2

#define UART_RANDOM_BYTES_PER_CHUNK	8
#define UART_DELTA_WORDS_PER_CHUNK	32

extern DMA_HandleTypeDef hdma_tim;

UART_HandleTypeDef *huart;
__IO ITStatus UartReady = RESET;

static union {
  uint8_t rnd[257];		/* 256 bytes + 1 for use in the POST */
  uint32_t rnd32[64];
} buf;

/* First DMA value (DMA_counters[0]) is unreliable, leftover in DMA FIFO perhaps? */
#define FIRST_DMA_IDX_USED  3

/*
 * Number of counters used to produce 8 bits of entropy is:
 *   8 * 4 - four flanks are used to produce two (hopefully) uncorrelated bits (a and b)
 *     * 2 - von Neumann will on average discard 1/2 of the bits 'a' and 'b'
 */
#define DMA_COUNTERS_NUM ((UART_RANDOM_BYTES_PER_CHUNK * 8 * 4 * 2) + FIRST_DMA_IDX_USED + 1)
struct DMA_params {
  volatile uint32_t buf0[DMA_COUNTERS_NUM];
  volatile uint32_t buf1[DMA_COUNTERS_NUM];
  volatile uint32_t write_buf;
};

static struct DMA_params DMA = {
  {},
  {},
  0,
};


/* The main work horse functions */
void get_entropy32(uint32_t num_bytes, uint32_t buf_idx);
void perform_delta32(uint32_t count, const uint32_t start);
/* Various support functions */
inline uint32_t get_one_bit(void) __attribute__((__always_inline__));
volatile uint32_t *restart_DMA(void);
inline volatile uint32_t *get_DMA_read_buf(void);
inline uint32_t safe_get_counter(volatile uint32_t *dmabuf, const uint32_t dmabuf_idx);
void check_uart_rx(UART_HandleTypeDef *this);
/* static void Error_Handler(void); */



int
main()
{
  uint32_t count = 0, send_bytes, idx = 255, send_sync_bytes_in = 0, i, timeout;
  uint32_t mode = MODE_ENTROPY;

  /* Initialize buffers */
  memset(buf.rnd, 0, sizeof(buf.rnd));
  for (i = 0; i < DMA_COUNTERS_NUM; i++) {
    DMA.buf0[i] = 0xffff0000 + i;
    DMA.buf1[i] = 0xffff0100 + i;
  }

  stm_init((uint32_t *) &DMA.buf0, DMA_COUNTERS_NUM);
  /* Ensure there is actual Timer IC counters in both DMA buffers. */
  restart_DMA();
  restart_DMA();

  huart = &huart1;

  /* Toggle GREEN LED to show we've initialized */
  {
    for (i = 0; i < 10; i++) {
      HAL_GPIO_TogglePin(LED_PORT, LED_GREEN);
      HAL_Delay(125);
    }
  }

  if (mode == MODE_ENTROPY) {
    /* Initialize buffer with the first round of entropy */
    idx = 0;
    send_bytes = UART_RANDOM_BYTES_PER_CHUNK;
    get_entropy32(send_bytes / 4, idx);
  }

  /*
   * Main loop
   */
  while (1) {
    if (! (count % 1000)) {
      HAL_GPIO_TogglePin(LED_PORT, LED_YELLOW);
    }

    switch (mode)
      {
      case MODE_DELTAS:
	if (! send_sync_bytes_in) {
	  /* Send 128 bits of sync-bytes every 1024 bytes */
	  send_sync_bytes_in = 1024;
	  memset(buf.rnd, 0xf0, sizeof(buf.rnd));
	  send_bytes = 16;
	} else {
	  perform_delta32(UART_DELTA_WORDS_PER_CHUNK, 0);
	  send_bytes = UART_DELTA_WORDS_PER_CHUNK * 4;
	  send_sync_bytes_in -= send_bytes;
	}
	idx = 0;
	break;;
      case MODE_ENTROPY:
	break;;
      }

    /* Send buf on UART (non blocking interrupt driven send). */
    UartReady = RESET;
    if (HAL_UART_Transmit_IT(huart, (uint8_t *) buf.rnd + idx, (uint16_t) send_bytes) == HAL_OK) {

      if (mode == MODE_ENTROPY) {
	/* Flip-flop idx between the value 0 and the value BYTES_PER_CHUNK.
	 * This enables collecting of BYTES_PER_CHUNK bytes of entropy at the same
	 * time as the USART sends the previous BYTES_PER_CHUNK using interrupts.
	 */
	idx = idx ? 0 : UART_RANDOM_BYTES_PER_CHUNK;
	get_entropy32(send_bytes / 4, idx / 4);
      }

      timeout = 0xffff;
      while (UartReady != SET && timeout) { timeout--; }
    }

    if (UartReady != SET) {
      /* Failed to send, turn on RED LED for one second */
      HAL_GPIO_WritePin(LED_PORT, LED_RED, GPIO_PIN_SET);
      HAL_Delay(1000);
      HAL_GPIO_WritePin(LED_PORT, LED_RED, GPIO_PIN_RESET);
    }

    /* Check for UART change request */
    check_uart_rx(&huart1);
    check_uart_rx(&huart2);

    count++;
  }

}

/**
 * @brief  Fill a buffer (buf.rnd32) with Timer IC counter values.
 *         Each value is 16 bits, but this function packs two counters
 *         into a 32 bit word so (2 * count) timer values are collected.
 * @param  count: Number of 32 bit words to collect.
 * @param  start: Start index value into buf.rnd32.
 * @retval None
 */
void perform_delta32(uint32_t count, const uint32_t start)
{
  /* Start at end of buffer so restart_DMA() is called. */
  static uint32_t dmabuf_idx = DMA_COUNTERS_NUM - 1;
  volatile uint32_t *dmabuf;
  uint32_t i, buf_idx;

  dmabuf = get_DMA_read_buf();
  buf_idx = start;

  do {
    if (dmabuf_idx > DMA_COUNTERS_NUM - 1 - 2) {
      /* If there are less than two counters available in the dmabuf,
       * we need to get a fresh DMA buffer first.
       */
      dmabuf = restart_DMA();
      dmabuf_idx = FIRST_DMA_IDX_USED;
    }

    i  = safe_get_counter(dmabuf, dmabuf_idx++) << 16;
    i |= safe_get_counter(dmabuf, dmabuf_idx++);

    /* Store the 32 bits in output buffer */
    buf.rnd32[buf_idx++] = i;
  } while (--count);
}


/**
 * @brief  Collect `count' times 32 bits of entropy.
 * @param  count: Number of 32 bit words to collect.
 * @param  start: Start index value into buf.rnd32.
 * @retval None
 */
inline void get_entropy32(uint32_t count, const uint32_t start)
{
  uint32_t i, bits, buf_idx;

  buf_idx = start;

  do {
    bits = 0;
    /* Get 32 bits of entropy.
     */
    for (i = 32; i; i--) {
      bits <<= 1;
      bits += get_one_bit();
    }

    /* Store the 32 bits in output buffer */
    buf.rnd32[buf_idx++] = bits;
  } while (--count);
}

/**
 * @brief  Return one bit of entropy.
 * @param  None
 * @retval One bit, in the LSB of an uint32_t since this is a 32 bit MCU.
 */
inline uint32_t get_one_bit()
{
  register uint32_t a, b, temp;
  /* Start at end of buffer so restart_DMA() is called. */
  static uint32_t dmabuf_idx = DMA_COUNTERS_NUM - 1;
  volatile uint32_t *dmabuf;

  dmabuf = get_DMA_read_buf();

  do {
    if (dmabuf_idx > DMA_COUNTERS_NUM - 1 - 4) {
      /* If there are less than four counters available in the dmabuf,
       * we need to get a fresh DMA buffer first.
       */
      dmabuf = restart_DMA();
      dmabuf_idx = FIRST_DMA_IDX_USED;
    }

    /* Get one bit from two subsequent counter values */
    a = safe_get_counter(dmabuf, dmabuf_idx++) & 1;
    temp = safe_get_counter(dmabuf, dmabuf_idx++) & 1;
    a ^= temp;

    /* Get another bit from two other counter values. Getting
     * two bits from two unrelated [1] pairs of counters is
     * supposed to help against phase correlations between the
     * frequency of the noise and the MCU sampling rate.
     *
     * [1] This is how it is done in the ARRGH board, although
     *     since this is a faster MCU and DMA is used the two
     *     pairs are more likely to still be related since they
     *     are more likely to be directly subsequent diode
     *     breakdowns. Have to evaluate if this is enough.
     */
    b = safe_get_counter(dmabuf, dmabuf_idx++) & 1;
    temp = safe_get_counter(dmabuf, dmabuf_idx++) & 1;
    b ^= temp;

    /* Do von Neumann extraction of a and b to eliminate bias
     * (only eliminates bias if a and b are uncorrelated)
     */
  } while (a == b);

  return a;
}

/**
 * @brief  Return a pointer to the DMA.buf NOT currently being written to
 * @param  None
 * @retval Pointer to buffer currently being read from.
 */
inline volatile uint32_t *get_DMA_read_buf(void)
{
  return DMA.write_buf ? DMA.buf0 : DMA.buf1;
}

/**
 * @brief  Return a pointer to the DMA.buf currently being written to
 * @param  None
 * @retval Pointer to buffer currently being written to.
 */
inline volatile uint32_t *get_DMA_write_buf(void)
{
  return DMA.write_buf ? DMA.buf1 : DMA.buf0;
}

/**
 * @brief  Initiate DMA collection of another buffer of Timer IC values.
 * @param  None
 * @retval Pointer to buffer full of Timer IC values ready to be consumed.
 */
volatile uint32_t *restart_DMA(void)
{
  /* Wait for transfer complete flag to become SET. Trying to change the
   * M0AR register while the DMA is running is a no-no.
   */
  while(__HAL_DMA_GET_FLAG(&hdma_tim, __HAL_DMA_GET_TC_FLAG_INDEX(&hdma_tim)) == RESET) { ; }

  /* Switch buffer being written to */
  DMA.write_buf ^= 1;
  hdma_tim.Instance->M0AR = (uint32_t) get_DMA_write_buf();

  /* Start at 0 to help manual inspection */
  TIM2->CNT = 0;

  /* Clear the transfer complete flag before re-enabling DMA */
  __HAL_DMA_CLEAR_FLAG(&hdma_tim, __HAL_DMA_GET_TC_FLAG_INDEX(&hdma_tim));
  __HAL_DMA_ENABLE(&hdma_tim);

  return get_DMA_read_buf();
}

/**
 * @brief  Get one counter value, guaranteed to not have been used before.
 * @param  dmabuf:     Pointer to the current DMA read buffer.
 * @param  dmabuf_idx: Word index into `dmabuf'.
 * @retval One Timer IC counter value.
 */
inline uint32_t safe_get_counter(volatile uint32_t *dmabuf, const uint32_t dmabuf_idx) {
  register uint32_t a;
  /* Prevent re-use of values. DMA stored values are <= 0xffff. */
  do {
    a = dmabuf[dmabuf_idx];
  } while (a > 0xffff);
  dmabuf[dmabuf_idx] = 0xffff0000;
  return a;
}

/* UART transmit complete callback */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *UH)
{
  if ((UH->Instance == USART1 && huart->Instance == USART1) ||
      (UH->Instance == USART2 && huart->Instance == USART2)) {
    /* Signal UART transmit complete to the code in the main loop. */
    UartReady = SET;
  }
}

/*
 * If a newline is received on UART1 or UART2, redirect output to that UART.
 */
void check_uart_rx(UART_HandleTypeDef *this) {
  uint8_t rx = 0;
  if (HAL_UART_Receive(this, &rx, 1, 0) == HAL_OK) {
    if (rx == '\n') {
      huart = this;
      /* Signal UART transmit complete to the code in the main loop. */
      UartReady = SET;
    }
  }
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @param  None
 * @retval None
 */
/* Currently unused function */
#if 0
static void Error_Handler(void)
{
  /* Turn on RED LED and then loop indefinitely */
  HAL_GPIO_WritePin(LED_PORT, LED_RED, GPIO_PIN_SET);
  while(1) { ; }
}
#endif
