/*
 * spiflash_n25q128.c
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

#include "spiflash_n25q128.h"

#define N25Q128_NUM_BYTES	(N25Q128_PAGE_SIZE * N25Q128_NUM_PAGES)

#if N25Q128_SECTOR_SIZE    * N25Q128_NUM_SECTORS    != N25Q128_NUM_BYTES || \
    N25Q128_SUBSECTOR_SIZE * N25Q128_NUM_SUBSECTORS != N25Q128_NUM_BYTES
#error Inconsistent definitions for pages / sectors / subsectors
#endif


static inline void _n25q128_select(struct spiflash_ctx *ctx)
{
    HAL_GPIO_WritePin(ctx->cs_n_port, ctx->cs_n_pin, GPIO_PIN_RESET);
}

static inline void _n25q128_deselect(struct spiflash_ctx *ctx)
{
    HAL_GPIO_WritePin(ctx->cs_n_port, ctx->cs_n_pin, GPIO_PIN_SET);
}

/* Read a bit from the status register. */
static inline int _n25q128_get_status_bit(struct spiflash_ctx *ctx, unsigned bitnum)
{
    // tx, rx buffers
    uint8_t spi_tx[2];
    uint8_t spi_rx[2];

    //assert(bitnum < sizeof(uint8_t));

    // send READ STATUS command
    spi_tx[0] = N25Q128_COMMAND_READ_STATUS;

    // send command, read response, deselect
    _n25q128_select(ctx);
    int ok =
        HAL_SPI_TransmitReceive(ctx->hspi, spi_tx, spi_rx, 2, N25Q128_SPI_TIMEOUT) == HAL_OK;
    _n25q128_deselect(ctx);

    // check
    if (!ok) return -1;

    // done
    return ((spi_rx[1] >> bitnum) & 1);
}

/* Read the Write Enable Latch bit in the status register. */
static inline int _n25q128_get_wel_flag(struct spiflash_ctx *ctx)
{
    return _n25q128_get_status_bit(ctx, 1);
}

/* Read the Write In Progress bit in the status register. */
static inline int _n25q128_get_wip_flag(struct spiflash_ctx *ctx)
{
    return _n25q128_get_status_bit(ctx, 0);
}

/* Wait until the flash memory is done writing */
static HAL_StatusTypeDef _n25q128_wait_while_wip(struct spiflash_ctx *ctx, uint32_t timeout)
{
    uint32_t tick_end = HAL_GetTick() + timeout;

    do {
        switch (_n25q128_get_wip_flag(ctx)) {
        case 0:
            return HAL_OK;
        case -1:
            return HAL_ERROR;
        default:
            /* try again */
            continue;
        }
    } while (HAL_GetTick() < tick_end);

    return HAL_TIMEOUT;
}

/* Send the Write Enable command */
static HAL_StatusTypeDef _n25q128_write_enable(struct spiflash_ctx *ctx)
{
    // tx buffer
    uint8_t spi_tx[1];

    // enable writing
    spi_tx[0] = N25Q128_COMMAND_WRITE_ENABLE;

    // activate, send command, deselect
    _n25q128_select(ctx);
    int ok =
        HAL_SPI_Transmit(ctx->hspi, spi_tx, 1, N25Q128_SPI_TIMEOUT) == HAL_OK;
    _n25q128_deselect(ctx);

    // check
    if (!ok) return HAL_ERROR;

    // make sure, that write enable did the job
    return _n25q128_get_wel_flag(ctx) ? HAL_OK : HAL_ERROR;
}

HAL_StatusTypeDef n25q128_check_id(struct spiflash_ctx *ctx)
{
    // tx, rx buffers
    uint8_t spi_tx[4];
    uint8_t spi_rx[4];

    // send READ ID command
    spi_tx[0] = N25Q128_COMMAND_READ_ID;

    // select, send command & read response, deselect
    _n25q128_select(ctx);
    int ok =
        HAL_SPI_TransmitReceive(ctx->hspi, spi_tx, spi_rx, 4, N25Q128_SPI_TIMEOUT) == HAL_OK;
    _n25q128_deselect(ctx);

    // check
    if (!ok) return HAL_ERROR;

    // parse response (note, that the very first byte was received during the
    // transfer of the command byte, so it contains garbage and should
    // be ignored here)
    return
        (spi_rx[1] == N25Q128_ID_MANUFACTURER &&
         spi_rx[2] == N25Q128_ID_DEVICE_TYPE &&
         spi_rx[3] == N25Q128_ID_DEVICE_CAPACITY) ? HAL_OK : HAL_ERROR;
}


HAL_StatusTypeDef n25q128_write_page(struct spiflash_ctx *ctx, uint32_t page_offset, const uint8_t *page_buffer)
{
    // tx buffer
    uint8_t spi_tx[4];

    // check offset
    if (page_offset >= N25Q128_NUM_PAGES) return HAL_ERROR;

    // enable writing
    if (_n25q128_write_enable(ctx) != 0) return HAL_ERROR;

    // calculate byte address
    uint32_t byte_offset = page_offset * N25Q128_PAGE_SIZE;

    // prepare PROGRAM PAGE command
    spi_tx[0] = N25Q128_COMMAND_PAGE_PROGRAM;
    spi_tx[1] = (uint8_t)(byte_offset >> 16);
    spi_tx[2] = (uint8_t)(byte_offset >>  8);
    spi_tx[3] = (uint8_t)(byte_offset >>  0);

    // activate, send command, send data, deselect
    _n25q128_select(ctx);
    int ok = 
        HAL_SPI_Transmit(ctx->hspi, spi_tx, 4, N25Q128_SPI_TIMEOUT) == HAL_OK &&
        HAL_SPI_Transmit(ctx->hspi, (uint8_t *) page_buffer, N25Q128_PAGE_SIZE, N25Q128_SPI_TIMEOUT) == HAL_OK;
    _n25q128_deselect(ctx);

    // check
    if (!ok) return HAL_ERROR;

    // wait until write finishes
    return _n25q128_wait_while_wip(ctx, 1000);
}


static HAL_StatusTypeDef n25q128_erase_something(struct spiflash_ctx *ctx, uint8_t command, uint32_t byte_offset)
{
    // check offset
    if (byte_offset >= N25Q128_NUM_BYTES) return HAL_ERROR;

    // tx buffer
    uint8_t spi_tx[4];

    // enable writing
    if (_n25q128_write_enable(ctx) != 0) return HAL_ERROR;

    // send command (ERASE SECTOR or ERASE SUBSECTOR)
    spi_tx[0] = command;
    spi_tx[1] = (uint8_t)(byte_offset >> 16);
    spi_tx[2] = (uint8_t)(byte_offset >>  8);
    spi_tx[3] = (uint8_t)(byte_offset >>  0);

    // activate, send command, deselect
    _n25q128_select(ctx);
    int ok =
        HAL_SPI_Transmit(ctx->hspi, spi_tx, 4, N25Q128_SPI_TIMEOUT) == HAL_OK;
    _n25q128_deselect(ctx);

    // check
    if (!ok) return HAL_ERROR;

    // wait for erase to finish
    return _n25q128_wait_while_wip(ctx, 1000);
}


HAL_StatusTypeDef n25q128_erase_sector(struct spiflash_ctx *ctx, uint32_t sector_offset)
{
    return n25q128_erase_something(ctx, N25Q128_COMMAND_ERASE_SECTOR,
				   sector_offset * N25Q128_SECTOR_SIZE);
}


HAL_StatusTypeDef n25q128_erase_subsector(struct spiflash_ctx *ctx, uint32_t subsector_offset)
{
    return n25q128_erase_something(ctx, N25Q128_COMMAND_ERASE_SUBSECTOR,
				   subsector_offset * N25Q128_SUBSECTOR_SIZE);
}


HAL_StatusTypeDef n25q128_erase_bulk(struct spiflash_ctx *ctx)
{
    // tx buffer
    uint8_t spi_tx[1];

    // enable writing
    if (_n25q128_write_enable(ctx) != 0) return HAL_ERROR;

    // send command
    spi_tx[0] = N25Q128_COMMAND_ERASE_BULK;

    // activate, send command, deselect
    _n25q128_select(ctx);
    int ok =
        HAL_SPI_Transmit(ctx->hspi, spi_tx, 1, N25Q128_SPI_TIMEOUT) == HAL_OK;
    _n25q128_deselect(ctx);

    // check
    if (!ok) return HAL_ERROR;

    // wait for erase to finish
    return _n25q128_wait_while_wip(ctx, 60000);
}


/* This function writes of a number of pages to the flash memory.
 * The caller is responsible for ensuring that the pages have been erased.
 */
HAL_StatusTypeDef n25q128_write_data(struct spiflash_ctx *ctx, uint32_t offset, const uint8_t *buf, const uint32_t len)
{
    uint32_t page;

    /*
     * The data sheet says:
     * If the bits of the least significant address, which is the starting
     * address, are not all zero, all data transmitted beyond the end of the
     * current page is programmed from the starting address of the same page.
     * If the number of bytes sent to the device exceed the maximum page size,
     * previously latched data is discarded and only the last maximum
     * page-size number of data bytes are guaranteed to be programmed
     * correctly within the same page. If the number of bytes sent to the
     * device is less than the maximum page size, they are correctly
     * programmed at the specified addresses without any effect on the other
     * bytes of the same page.
     *
     * This is sufficiently confusing that it makes sense to constrain the API
     * to page alignment in address and length, because that's how we're using
     * it anyway.
     */

    if (offset % N25Q128_PAGE_SIZE != 0 || len % N25Q128_PAGE_SIZE != 0) return HAL_ERROR;

    for (page = 0; page < len / N25Q128_PAGE_SIZE; page++) {
	if (n25q128_write_page(ctx, offset / N25Q128_PAGE_SIZE, buf) != 0) {
	    return HAL_ERROR;
	}
	buf += N25Q128_PAGE_SIZE;
	offset += N25Q128_PAGE_SIZE;
    }

    return HAL_OK;
}

/* This function reads zero or more pages from the SPI flash. */
HAL_StatusTypeDef n25q128_read_data(struct spiflash_ctx *ctx, uint32_t offset, uint8_t *buf, const uint32_t len)
{
    // tx buffer
    uint8_t spi_tx[4];

    /*
     * The data sheet says:
     * The addressed byte can be at any location, and the address
     * automatically increments to the next address after each byte of data is
     * shifted out; therefore, the entire memory can be read with a single
     * command. The operation is terminated by driving S# [chip select] HIGH
     * at any time during data output.
     *
     * We're only going to call this with page-aligned address and length, but
     * we're not going to enforce it here.
     */

    // avoid overflow
    if (offset + len > N25Q128_NUM_BYTES) return HAL_ERROR;

    // prepare READ command
    spi_tx[0] = N25Q128_COMMAND_READ;
    spi_tx[1] = (uint8_t)(offset >> 16);
    spi_tx[2] = (uint8_t)(offset >>  8);
    spi_tx[3] = (uint8_t)(offset >>  0);

    // activate, send command, read response, deselect
    _n25q128_select(ctx);
    int ok =
        HAL_SPI_Transmit(ctx->hspi, spi_tx, 4, N25Q128_SPI_TIMEOUT) == HAL_OK &&
        HAL_SPI_Receive(ctx->hspi, buf, len, N25Q128_SPI_TIMEOUT) == HAL_OK;
    _n25q128_deselect(ctx);

    // check
    return ok ? HAL_OK : HAL_ERROR;
}
