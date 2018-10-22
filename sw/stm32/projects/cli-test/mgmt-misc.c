/*
 * mgmt-misc.c
 * -----------
 * Miscellaneous CLI functions.
 *
 * Copyright (c) 2016, NORDUnet A/S All rights reserved.
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

#define HAL_OK CMSIS_HAL_OK
#include "stm-init.h"
#include "stm-uart.h"
#include "mgmt-cli.h"
#include "mgmt-misc.h"
#undef HAL_OK

#define HAL_OK LIBHAL_OK
#include "hal.h"
#include "hal_internal.h"
#undef HAL_OK

#include <string.h>


static volatile hal_crc32_t demo_crc;

static HAL_StatusTypeDef _count_bytes_callback(uint8_t *buf, size_t len) {
    demo_crc = hal_crc32_update(demo_crc, buf, len);
    return CMSIS_HAL_OK;
}

int cli_receive_data(struct cli_def *cli, uint8_t *buf, size_t len, cli_data_callback data_callback)
{
    hal_crc32_t crc = 0, my_crc = hal_crc32_init();
    uint32_t filesize = 0, counter = 0;
    size_t n = len;

    if (! control_mgmt_uart_dma_rx(DMA_RX_STOP)) {
	cli_print(cli, "Failed stopping DMA");
	goto fail;
    }

    cli_print(cli, "OK, write size (4 bytes), data in %li byte chunks, CRC-32 (4 bytes)", (uint32_t) n);

    if (uart_receive_bytes((void *) &filesize, sizeof(filesize), 1000) != CMSIS_HAL_OK) {
	cli_print(cli, "Receive timed out");
	goto fail;
    }

    cli_print(cli, "Send %li bytes of data", filesize);

    while (filesize) {
	/* By initializing buf to the same value that erased flash has (0xff), we don't
	 * have to try and be smart when writing the last page of data to a flash memory.
	 */
	memset(buf, 0xff, len);

	if (filesize < n) n = filesize;

	if (uart_receive_bytes((void *) buf, n, 1000) != CMSIS_HAL_OK) {
	    cli_print(cli, "Receive timed out");
	    goto fail;
	}
	filesize -= n;
	my_crc = hal_crc32_update(my_crc, buf, n);

	/* After reception of a chunk but before ACKing we have "all" the time in the world to
	 * calculate CRC and invoke the data_callback.
	 */
	if (data_callback != NULL && data_callback(buf, n) != CMSIS_HAL_OK) {
	    cli_print(cli, "Data processing failed");
	    goto fail;
	}

	counter++;
	uart_send_bytes((void *) &counter, 4);
    }

    my_crc = hal_crc32_finalize(my_crc);
    cli_print(cli, "Send CRC-32");
    uart_receive_bytes((void *) &crc, sizeof(crc), 1000);
    cli_print(cli, "CRC-32 0x%x, calculated CRC 0x%x", (unsigned int) crc, (unsigned int) my_crc);
    if (crc == my_crc) {
	cli_print(cli, "CRC checksum MATCHED");
    } else {
	cli_print(cli, "CRC checksum did NOT match");
    }

 fail:
    control_mgmt_uart_dma_rx(DMA_RX_START);
    return CLI_OK;
}

static int cmd_filetransfer(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    uint8_t buf[FILETRANSFER_UPLOAD_CHUNK_SIZE];

    command = command;
    argv = argv;
    argc = argc;

    demo_crc = hal_crc32_init();
    cli_receive_data(cli, &buf[0], sizeof(buf), _count_bytes_callback);
    demo_crc = hal_crc32_finalize(demo_crc);
    cli_print(cli, "Demo CRC is: %li/0x%x", demo_crc, (unsigned int) demo_crc);
    return CLI_OK;
}

static int cmd_reboot(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    command = command;
    argv = argv;
    argc = argc;

    cli_print(cli, "\n\n\nRebooting\n\n\n");
    HAL_NVIC_SystemReset();

    /*NOTREACHED*/
    return CLI_OK;
}

void configure_cli_misc(struct cli_def *cli)
{
    /* filetransfer */
    cli_register_command(cli, NULL, "filetransfer", cmd_filetransfer, 0, 0, "Test file transfering");

    /* reboot */
    cli_register_command(cli, NULL, "reboot", cmd_reboot, 0, 0, "Reboot the STM32");
}

