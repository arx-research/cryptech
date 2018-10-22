/*
 * stm-keystore.h
 * ---------
 * Functions and defines for accessing the keystore memory.
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

#ifndef __STM32_KEYSTORE_H
#define __STM32_KEYSTORE_H

#include "stm32f4xx_hal.h"
#include "spiflash_n25q128.h"

#define KEYSTORE_PAGE_SIZE		   N25Q128_PAGE_SIZE
#define KEYSTORE_NUM_PAGES		   N25Q128_NUM_PAGES
#define KEYSTORE_SECTOR_SIZE		   N25Q128_SECTOR_SIZE
#define KEYSTORE_NUM_SECTORS		   N25Q128_NUM_SECTORS
#define KEYSTORE_SUBSECTOR_SIZE		   N25Q128_SUBSECTOR_SIZE
#define KEYSTORE_NUM_SUBSECTORS		   N25Q128_NUM_SUBSECTORS

/* Pins connected to the FPGA config memory (SPI flash) */
#define KSM_PROM_CS_N_Pin                  GPIO_PIN_0
#define KSM_PROM_CS_N_GPIO_Port            GPIOB

#define KEYSTORE_GPIO_INIT()		\
    __GPIOB_CLK_ENABLE();		\
    /* Configure GPIO pin for FPGA config memory chip select : KSM_PROM_CS_N */        \
    gpio_output(KSM_PROM_CS_N_GPIO_Port, KSM_PROM_CS_N_Pin, GPIO_PIN_SET)


extern void keystore_init(void);
extern HAL_StatusTypeDef keystore_check_id(void);
extern HAL_StatusTypeDef keystore_read_data(uint32_t offset, uint8_t *buf, const uint32_t len);
extern HAL_StatusTypeDef keystore_write_data(uint32_t offset, const uint8_t *buf, const uint32_t len);
extern HAL_StatusTypeDef keystore_erase_subsector(uint32_t subsector_offset);
extern HAL_StatusTypeDef keystore_erase_sector(uint32_t sector_offset);
extern HAL_StatusTypeDef keystore_erase_bulk(void);

#endif /* __STM32_KEYSTORE_H */
