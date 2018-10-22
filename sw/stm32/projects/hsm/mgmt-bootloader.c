/*
 * mgmt-bootloader.c
 * -----------------
 * CLI code for updating the bootloader.
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

/* Rename both CMSIS HAL_OK and libhal HAL_OK to disambiguate */
#define HAL_OK CMSIS_HAL_OK
#include "stm-init.h"
#include "stm-uart.h"
#include "stm-flash.h"
#include "mgmt-cli.h"
#include "mgmt-misc.h"
#include "mgmt-bootloader.h"

#undef HAL_OK
#define HAL_OK LIBHAL_OK
#include "hal.h"
#undef HAL_OK

extern hal_user_t user;

static uint32_t dfu_offset;

static HAL_StatusTypeDef _flash_write_callback(uint8_t *buf, size_t len)
{
    HAL_StatusTypeDef status = stm_flash_write32(dfu_offset, (uint32_t *)buf, len/4);
    dfu_offset += DFU_UPLOAD_CHUNK_SIZE;
    return status;
}

static int cmd_bootloader_upload(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    command = command;
    argv = argv;
    argc = argc;

    if (user < HAL_USER_SO) {
        cli_print(cli, "Permission denied.");
        return CLI_ERROR;
    }

    uint8_t buf[DFU_UPLOAD_CHUNK_SIZE];
    dfu_offset = DFU_BOOTLOADER_ADDR;

    int ret = cli_receive_data(cli, buf, sizeof(buf), _flash_write_callback);
    if (ret == CLI_OK) {
        cli_print(cli, "\nRebooting\n");
        HAL_NVIC_SystemReset();
    }
    return ret;
}

void configure_cli_bootloader(struct cli_def *cli)
{
    struct cli_command *c;

    c = cli_register_command(cli, NULL, "bootloader", NULL, 0, 0, NULL);

    cli_register_command(cli, c, "upload", cmd_bootloader_upload, 0, 0, "Upload new bootloader image");
}
