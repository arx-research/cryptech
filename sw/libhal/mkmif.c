/*
 * mkmif.c
 * -------
 * HAL interface to Cryptech Master Key Memory Interface.
 *
 * Copyright (c) 2016, NORDUnet A/S
 * All rights reserved.
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

#include <stdlib.h>

#include "hal.h"
#include "hal_internal.h"

typedef union {
    uint8_t byte[4];
    uint32_t word;
} byteword_t;

hal_error_t hal_mkmif_init(hal_core_t *core)
{
    const int free_core = core == NULL;
    byteword_t cmd;
    hal_error_t err;

    cmd.word = htonl(MKMIF_CTRL_CMD_INIT);

    if (free_core && (err = hal_core_alloc(MKMIF_NAME, &core, NULL)) != HAL_OK)
        return err;

    if ((err = hal_io_write(core, MKMIF_ADDR_CTRL, cmd.byte, 4)) == HAL_OK)
        err = hal_io_wait_ready(core);

    if (free_core)
        hal_core_free(core);
    return err;
}

hal_error_t hal_mkmif_set_clockspeed(hal_core_t *core, const uint32_t divisor)
{
    const int free_core = core == NULL;
    byteword_t data;
    hal_error_t err;

    data.word = htonl(divisor);

    if (free_core && (err = hal_core_alloc(MKMIF_NAME, &core, NULL)) != HAL_OK)
        return err;

    err = hal_io_write(core, MKMIF_ADDR_SCLK_DIV, data.byte, 4);

    if (free_core)
        hal_core_free(core);

    return err;
}

hal_error_t hal_mkmif_get_clockspeed(hal_core_t *core, uint32_t *divisor)
{
    const int free_core = core == NULL;
    byteword_t data;
    hal_error_t err;

    if (free_core && (err = hal_core_alloc(MKMIF_NAME, &core, NULL)) != HAL_OK)
        return err;

    if ((err = hal_io_read(core, MKMIF_ADDR_SCLK_DIV, data.byte, 4)) == HAL_OK)
        *divisor = htonl(data.word);

    if (free_core)
        hal_core_free(core);

    return err;
}

hal_error_t hal_mkmif_write(hal_core_t *core, uint32_t addr, const uint8_t *buf, size_t len)
{
    const int free_core = core == NULL;
    hal_error_t err = HAL_OK;
    byteword_t cmd;

    if (len % 4 != 0)
        return HAL_ERROR_IO_BAD_COUNT;

    cmd.word = htonl(MKMIF_CTRL_CMD_WRITE);

    if (!free_core || (err = hal_core_alloc(MKMIF_NAME, &core, NULL)) == HAL_OK) {
        for (; len > 0; addr += 4, buf += 4, len -= 4) {
            byteword_t write_addr;
            write_addr.word = htonl((uint32_t)addr);
            if ((err = hal_io_write(core, MKMIF_ADDR_EMEM_ADDR, write_addr.byte, 4)) != HAL_OK ||
                (err = hal_io_write(core, MKMIF_ADDR_EMEM_DATA, buf, 4)) != HAL_OK ||
                (err = hal_io_write(core, MKMIF_ADDR_CTRL, cmd.byte, 4)) != HAL_OK ||
                (err = hal_io_wait_ready(core)) != HAL_OK)
	        break;
        }
    }

    if (free_core)
        hal_core_free(core);

    return err;
}

hal_error_t hal_mkmif_write_word(hal_core_t *core, uint32_t addr, const uint32_t data)
{
    byteword_t d;

    d.word = htonl(data);

    return hal_mkmif_write(core, addr, d.byte, 4);
}

hal_error_t hal_mkmif_read(hal_core_t *core, uint32_t addr, uint8_t *buf, size_t len)
{
    const int free_core = core == NULL;
    hal_error_t err = HAL_OK;
    byteword_t cmd;

    if (len % 4 != 0)
        return HAL_ERROR_IO_BAD_COUNT;

    cmd.word = htonl(MKMIF_CTRL_CMD_READ);

    if (free_core && (err = hal_core_alloc(MKMIF_NAME, &core, NULL)) != HAL_OK)
        return err;

    for (; len > 0; addr += 4, buf += 4, len -= 4) {
        byteword_t read_addr;
	read_addr.word = htonl((uint32_t)addr);
	if ((err = hal_io_write(core, MKMIF_ADDR_EMEM_ADDR, read_addr.byte, 4)) != HAL_OK ||
	    (err = hal_io_write(core, MKMIF_ADDR_CTRL, cmd.byte, 4)) != HAL_OK ||
	    (err = hal_io_wait_valid(core)) != HAL_OK ||
	    (err = hal_io_read(core, MKMIF_ADDR_EMEM_DATA, buf, 4)) != HAL_OK)
	    break;
    }

    if (free_core)
        hal_core_free(core);

    return err;
}

hal_error_t hal_mkmif_read_word(hal_core_t *core, uint32_t addr, uint32_t *data)
{
    byteword_t d;
    hal_error_t err;

    if ((err = hal_mkmif_read(core, addr, d.byte, 4)) != HAL_OK)
        return err;

    *data = htonl(d.word);

    return HAL_OK;
}
