/*
 * mgmt-cli.c
 * ---------
 * Management CLI code.
 *
 * Copyright (c) 2016-2017, NORDUnet A/S All rights reserved.
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

#include <string.h>

/* Rename both CMSIS HAL_OK and libhal HAL_OK to disambiguate */
#define HAL_OK CMSIS_HAL_OK
#include "stm-init.h"
#include "stm-uart.h"
#include "stm-led.h"
#include "task.h"

#include "mgmt-cli.h"
#include "mgmt-firmware.h"
#include "mgmt-bootloader.h"
#include "mgmt-fpga.h"
#include "mgmt-misc.h"
#include "mgmt-keystore.h"
#include "mgmt-masterkey.h"
#include "mgmt-task.h"

#undef HAL_OK
#define HAL_OK LIBHAL_OK
#include "hal.h"
#warning Refactor so we do not need to include hal_internal.h here
#include "hal_internal.h"
#undef HAL_OK

static tcb_t *cli_task;

#ifndef CLI_UART_RECVBUF_SIZE
#define CLI_UART_RECVBUF_SIZE  256
#endif

typedef struct {
    unsigned ridx;
    unsigned widx;
    mgmt_cli_dma_state_t rx_state;
    uint8_t buf[CLI_UART_RECVBUF_SIZE];
} ringbuf_t;

inline void ringbuf_init(ringbuf_t *rb)
{
    memset(rb, 0, sizeof(*rb));
}

/* return number of characters read */
inline int ringbuf_read_char(ringbuf_t *rb, uint8_t *c)
{
    if (rb->ridx != rb->widx) {
        *c = rb->buf[rb->ridx];
        if (++rb->ridx >= sizeof(rb->buf))
            rb->ridx = 0;
        return 1;
    }
    return 0;
}

inline void ringbuf_write_char(ringbuf_t *rb, uint8_t c)
{
    rb->buf[rb->widx] = c;
    if (++rb->widx >= sizeof(rb->buf))
        rb->widx = 0;
}

static ringbuf_t uart_ringbuf;

/* current character received from UART */
static uint8_t uart_rx;

/* Callback for HAL_UART_Receive_DMA().
 */
void HAL_UART1_RxCpltCallback(UART_HandleTypeDef *huart)
{
    huart = huart;

    ringbuf_write_char(&uart_ringbuf, uart_rx);
    task_wake(cli_task);
}

static void uart_cli_print(struct cli_def *cli __attribute__ ((unused)), const char *buf)
{
    char crlf[] = "\r\n";
    uart_send_string(buf);
    uart_send_string(crlf);
}

static ssize_t uart_cli_read(struct cli_def *cli __attribute__ ((unused)), void *buf, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        while (ringbuf_read_char(&uart_ringbuf, (uint8_t *)(buf + i)) == 0)
            task_sleep();
    }
    return (ssize_t)count;
}

static ssize_t uart_cli_write(struct cli_def *cli __attribute__ ((unused)), const void *buf, size_t count)
{
    uart_send_bytes((uint8_t *) buf, count);
    return (ssize_t)count;
}

int control_mgmt_uart_dma_rx(mgmt_cli_dma_state_t state)
{
    if (state == DMA_RX_START) {
        if (uart_ringbuf.rx_state != DMA_RX_START) {
            ringbuf_init(&uart_ringbuf);
            HAL_UART_Receive_DMA(&huart_mgmt, &uart_rx, 1);
            uart_ringbuf.rx_state = DMA_RX_START;
        }
        return 1;
    } else if (state == DMA_RX_STOP) {
        if (HAL_UART_DMAStop(&huart_mgmt) != CMSIS_HAL_OK) return 0;
        uart_ringbuf.rx_state = DMA_RX_STOP;
        return 1;
    }
    return 0;
}

hal_user_t user;

static int check_auth(const char *username, const char *password)
{
    hal_client_handle_t client = { -1 };

    /* PIN-based login */
    if (strcmp(username, "wheel") == 0)
        user = HAL_USER_WHEEL;
    else if (strcmp(username, "so") == 0)
        user = HAL_USER_SO;
    else if (strcmp(username, "user") == 0)
        user = HAL_USER_NORMAL;
    else
        user = HAL_USER_NONE;

    if (hal_rpc_login(client, user, password, strlen(password)) == LIBHAL_OK)
        return CLI_OK;

    user = HAL_USER_NONE;
    return CLI_ERROR;
}

int cli_main(void)
{
    cli_task = task_get_tcb();

    struct cli_def *cli;
    cli = cli_init();
    if (cli == NULL)
        Error_Handler();

    cli_read_callback(cli, uart_cli_read);
    cli_write_callback(cli, uart_cli_write);
    cli_print_callback(cli, uart_cli_print);
    cli_set_banner(cli, "Cryptech Alpha");
    cli_set_hostname(cli, "cryptech");
    cli_set_auth_callback(cli, check_auth);

    /* we don't have any privileged commands at the moment */
    cli_unregister_command(cli, "enable");

    configure_cli_fpga(cli);
    configure_cli_keystore(cli);
    configure_cli_masterkey(cli);
    configure_cli_firmware(cli);
    configure_cli_bootloader(cli);
    configure_cli_misc(cli);
    configure_cli_task(cli);

    while (1) {
        control_mgmt_uart_dma_rx(DMA_RX_START);

        cli_loop(cli, 0);
        /* cli_loop returns when the user enters 'quit' or 'exit' */
        cli_print(cli, "\nLogging out...\n");
        user = HAL_USER_NONE;
    }

    /*NOTREACHED*/
    return -1;
}
