/*
 * stm-fpgacfg.c
 * ----------
 * Functions for accessing the FPGA config memory and controlling
 * the low-level status of the FPGA (reset registers/reboot etc.).
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

#include "stm-fpgacfg.h"
#include "stm-init.h"

static SPI_HandleTypeDef hspi_fpgacfg;

static struct spiflash_ctx fpgacfg_ctx = {&hspi_fpgacfg, PROM_CS_N_GPIO_Port, PROM_CS_N_Pin};

void fpgacfg_init(void)
{
    /* Give the FPGA access to it's bitstream ASAP (maybe this should actually
     * be done in the application, before calling stm_init()).
     */
    fpgacfg_access_control(ALLOW_FPGA);

    /* Set up GPIOs to manage access to the FPGA config memory.
     * FPGACFG_GPIO_INIT is defined in stm-fpgacfg.h.
     */
    FPGACFG_GPIO_INIT();

    /* SPI2 (FPGA config memory) init function */
    hspi_fpgacfg.Instance = SPI2;
    hspi_fpgacfg.Init.Mode = SPI_MODE_MASTER;
    hspi_fpgacfg.Init.Direction = SPI_DIRECTION_2LINES;
    hspi_fpgacfg.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi_fpgacfg.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi_fpgacfg.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi_fpgacfg.Init.NSS = SPI_NSS_SOFT;
    hspi_fpgacfg.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    hspi_fpgacfg.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi_fpgacfg.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi_fpgacfg.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi_fpgacfg.Init.CRCPolynomial = 10;
    HAL_SPI_Init(&hspi_fpgacfg);
}

HAL_StatusTypeDef fpgacfg_check_id(void)
{
    return n25q128_check_id(&fpgacfg_ctx);
}

HAL_StatusTypeDef fpgacfg_write_data(uint32_t offset, const uint8_t *buf, const uint32_t len)
{
    return n25q128_write_data(&fpgacfg_ctx, offset, buf, len);
}

void fpgacfg_access_control(enum fpgacfg_access_ctrl access)
{
    if (access == ALLOW_ARM) {
	// fpga disable = 1
	HAL_GPIO_WritePin(PROM_FPGA_DIS_GPIO_Port, PROM_FPGA_DIS_Pin, GPIO_PIN_SET);
	// arm enable = 0
	HAL_GPIO_WritePin(GPIOF, PROM_ARM_ENA_Pin, GPIO_PIN_RESET);
    } else if (access == ALLOW_FPGA) {
	// fpga disable = 0
	HAL_GPIO_WritePin(PROM_FPGA_DIS_GPIO_Port, PROM_FPGA_DIS_Pin, GPIO_PIN_RESET);
	// arm enable = 1
	HAL_GPIO_WritePin(GPIOF, PROM_ARM_ENA_Pin, GPIO_PIN_SET);
    } else {
	// fpga disable = 1
	HAL_GPIO_WritePin(PROM_FPGA_DIS_GPIO_Port, PROM_FPGA_DIS_Pin, GPIO_PIN_SET);
	// arm enable = 1
	HAL_GPIO_WritePin(GPIOF, PROM_ARM_ENA_Pin, GPIO_PIN_SET);
    }
}

void fpgacfg_reset_fpga(enum fpgacfg_reset reset)
{
    if (reset == RESET_FULL) {
	/* The delay should be at least 250 uS. With HAL_Delay(1) the pulse is very close
	 * to that, and With HAL_Delay(3) the pulse is close to 2 ms. */
	HAL_GPIO_WritePin(FPGA_PROGRAM_Port, FPGA_PROGRAM_Pin, GPIO_PIN_RESET);
	HAL_Delay(3);
	HAL_GPIO_WritePin(FPGA_PROGRAM_Port, FPGA_PROGRAM_Pin, GPIO_PIN_SET);
    } else if (reset == RESET_REGISTERS) {
	HAL_GPIO_WritePin(FPGA_INIT_Port, FPGA_INIT_Pin, GPIO_PIN_SET);
	HAL_Delay(3);
	HAL_GPIO_WritePin(FPGA_INIT_Port, FPGA_INIT_Pin, GPIO_PIN_RESET);
    }
}

HAL_StatusTypeDef fpgacfg_check_done(void)
{
    return (HAL_GPIO_ReadPin(FPGA_DONE_Port, FPGA_DONE_Pin) == GPIO_PIN_SET) ? HAL_OK : HAL_ERROR;
}

HAL_StatusTypeDef fpgacfg_erase_sector(uint32_t sector_offset)
{
    return n25q128_erase_sector(&fpgacfg_ctx, sector_offset);
}
