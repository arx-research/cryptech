/*
 * stm-keystore.c
 * ----------
 * Functions for accessing the keystore memory.
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

#include "stm-init.h"
#include "stm-keystore.h"

static SPI_HandleTypeDef hspi_keystore;

struct spiflash_ctx keystore_ctx = {&hspi_keystore, KSM_PROM_CS_N_GPIO_Port, KSM_PROM_CS_N_Pin};

/* SPI1 (keystore memory) init function */
void keystore_init(void)
{
    /* Set up GPIOs for the keystore memory.
     * KEYSTORE_GPIO_INIT is defined in stm-keystore.h.
     */
    KEYSTORE_GPIO_INIT();

    hspi_keystore.Instance = SPI1;
    hspi_keystore.Init.Mode = SPI_MODE_MASTER;
    hspi_keystore.Init.Direction = SPI_DIRECTION_2LINES;
    hspi_keystore.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi_keystore.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi_keystore.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi_keystore.Init.NSS = SPI_NSS_SOFT;
    hspi_keystore.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    hspi_keystore.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi_keystore.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi_keystore.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi_keystore.Init.CRCPolynomial = 10;
    HAL_SPI_Init(&hspi_keystore);
}

HAL_StatusTypeDef keystore_check_id(void)
{
    return n25q128_check_id(&keystore_ctx);
}

HAL_StatusTypeDef keystore_read_data(uint32_t offset, uint8_t *buf, const uint32_t len)
{
    return n25q128_read_data(&keystore_ctx, offset, buf, len);
}

HAL_StatusTypeDef keystore_write_data(uint32_t offset, const uint8_t *buf, const uint32_t len)
{
    return n25q128_write_data(&keystore_ctx, offset, buf, len);
}

HAL_StatusTypeDef keystore_erase_subsector(uint32_t subsector_offset)
{
    return n25q128_erase_subsector(&keystore_ctx, subsector_offset);
}

HAL_StatusTypeDef keystore_erase_sector(uint32_t sector_offset)
{
    return n25q128_erase_sector(&keystore_ctx, sector_offset);
}

HAL_StatusTypeDef keystore_erase_bulk(void)
{
    return n25q128_erase_bulk(&keystore_ctx);
}
