/*
 * spiflash_n25q128.h
 * ------------------
 * Functions and defines for accessing SPI flash with part number n25q128.
 *
 * The Alpha board has two of these SPI flash memorys, the FPGA config memory
 * and the keystore memory.
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

#ifndef __STM32_SPIFLASH_N25Q128_H
#define __STM32_SPIFLASH_N25Q128_H

#include "stm32f4xx_hal.h"

#define N25Q128_COMMAND_READ		0x03
#define N25Q128_COMMAND_READ_STATUS	0x05
#define N25Q128_COMMAND_READ_ID		0x9E
#define N25Q128_COMMAND_WRITE_ENABLE	0x06
#define N25Q128_COMMAND_ERASE_SECTOR	0xD8
#define N25Q128_COMMAND_ERASE_SUBSECTOR	0x20
#define N25Q128_COMMAND_ERASE_BULK     	0xC7
#define N25Q128_COMMAND_PAGE_PROGRAM	0x02

#define N25Q128_PAGE_SIZE		0x100		// 256
#define N25Q128_NUM_PAGES		0x10000		// 65536

#define N25Q128_SECTOR_SIZE		0x10000		// 65536
#define N25Q128_NUM_SECTORS		0x100		// 256

#define N25Q128_SUBSECTOR_SIZE		0x1000		// 4096
#define N25Q128_NUM_SUBSECTORS		0x1000		// 4096

#define N25Q128_SPI_TIMEOUT		1000

#define N25Q128_ID_MANUFACTURER		0x20
#define N25Q128_ID_DEVICE_TYPE		0xBA
#define N25Q128_ID_DEVICE_CAPACITY	0x18

struct spiflash_ctx {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *cs_n_port;
    uint16_t cs_n_pin;
};

extern HAL_StatusTypeDef n25q128_check_id(struct spiflash_ctx *ctx);
extern HAL_StatusTypeDef n25q128_read_data(struct spiflash_ctx *ctx, uint32_t offset, uint8_t *buf, const uint32_t len);
extern HAL_StatusTypeDef n25q128_write_page(struct spiflash_ctx *ctx, uint32_t page_offset, const uint8_t *page_buffer);
extern HAL_StatusTypeDef n25q128_write_data(struct spiflash_ctx *ctx, uint32_t offset, const uint8_t *buf, const uint32_t len);
extern HAL_StatusTypeDef n25q128_erase_subsector(struct spiflash_ctx *ctx, uint32_t subsector_offset);
extern HAL_StatusTypeDef n25q128_erase_sector(struct spiflash_ctx *ctx, uint32_t sector_offset);
extern HAL_StatusTypeDef n25q128_erase_bulk(struct spiflash_ctx *ctx);

#define n25q128_read_page(ctx, page_offset, page_buffer) \
    n25q128_read_data(ctx, page_offset * N25Q128_PAGE_SIZE, page_buffer, N25Q128_PAGE_SIZE)
#define n25q128_read_subsector(ctx, subsector_offset, subsector_buffer) \
    n25q128_read_data(ctx, subsector_offset * N25Q128_SUBSECTOR_SIZE, subsector_buffer, N25Q128_SUBSECTOR_SIZE)

#endif /* __STM32_SPIFLASH_N25Q128_H */
