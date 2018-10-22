/*
 * stm-flash.c
 * -----------
 * Functions for writing/erasing the STM32 internal flash memory.
 * The flash is memory mapped, so no code is needed here to read it.
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

#include "stm-init.h"
#include "stm-flash.h"


/* Flash sector offsets from RM0090, Table 6. Flash module - 2 Mbyte dual bank organization */
#define FLASH_NUM_SECTORS 24
uint32_t flash_sector_offsets[FLASH_NUM_SECTORS + 1] = {
    /* Bank 1 */
    0x08000000, /* #0,  16 KBytes */
    0x08004000, /* #1,  16 Kbytes */
    0x08008000, /* #2,  16 Kbytes */
    0x0800C000, /* #3,  16 Kbytes */
    0x08010000, /* #4,  64 Kbytes */
    0x08020000, /* #5,  128 Kbytes */
    0x08040000, /* #6,  128 Kbytes */
    0x08060000, /* #7,  128 Kbytes */
    0x08080000, /* #8,  128 Kbytes */
    0x080A0000, /* #9,  128 Kbytes */
    0x080C0000, /* #10, 128 Kbytes */
    0x080E0000, /* #11, 128 Kbytes */
    /* Bank 2 */
    0x08100000, /* #12,  16 Kbytes */
    0x08104000, /* #13,  16 Kbytes */
    0x08108000, /* #14,  16 Kbytes */
    0x0810C000, /* #15,  16 Kbytes */
    0x08110000, /* #16,  64 Kbytes */
    0x08120000, /* #17, 128 Kbytes */
    0x08140000, /* #18, 128 Kbytes */
    0x08160000, /* #19, 128 Kbytes */
    0x08180000, /* #20, 128 Kbytes */
    0x081A0000, /* #21, 128 Kbytes */
    0x081C0000, /* #22, 128 Kbytes */
    0x081E0000, /* #23, 128 Kbytes */
    0x08200000  /* first address *after* flash */
};

static uint32_t stm_flash_sector_num(const uint32_t offset)
{
    uint32_t i;

    if (offset < flash_sector_offsets[0])
        return 0xFFFFFFFF;

    for (i = 0; i < FLASH_NUM_SECTORS; ++i)
	if (offset < flash_sector_offsets[i + 1])
	    return i;

    return 0xFFFFFFFF;
}

HAL_StatusTypeDef stm_flash_erase_sectors(const uint32_t start_offset, const uint32_t end_offset)
{
    uint32_t start_sector = stm_flash_sector_num(start_offset);
    uint32_t end_sector = stm_flash_sector_num(end_offset);
    FLASH_EraseInitTypeDef FLASH_EraseInitStruct;
    uint32_t SectorError = 0;
    HAL_StatusTypeDef err;

    if (start_sector > end_sector || end_sector > FLASH_NUM_SECTORS)
        return HAL_ERROR;

    FLASH_EraseInitStruct.Sector = start_sector;
    FLASH_EraseInitStruct.NbSectors = (end_sector - start_sector) + 1;
    FLASH_EraseInitStruct.TypeErase = TYPEERASE_SECTORS;
    FLASH_EraseInitStruct.VoltageRange = VOLTAGE_RANGE_3;

    HAL_FLASH_Unlock();
    err = HAL_FLASHEx_Erase(&FLASH_EraseInitStruct, &SectorError);
    HAL_FLASH_Lock();

    if (err != HAL_OK || SectorError != 0xFFFFFFFF)
        return HAL_ERROR;

    return HAL_OK;
}

HAL_StatusTypeDef stm_flash_write32(uint32_t offset, const uint32_t *buf, const size_t elements)
{
    uint32_t sector = stm_flash_sector_num(offset);
    size_t i;
    HAL_StatusTypeDef err = HAL_OK;

    if (offset == flash_sector_offsets[sector]) {
	/* Request to write to beginning of a flash sector, erase it first. */
	if (stm_flash_erase_sectors(offset, offset) != 0) {
	    return HAL_ERROR;
	}
    }

    HAL_FLASH_Unlock();

    for (i = 0; i < elements; i++) {
	if ((err = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, offset, buf[i])) != HAL_OK) {
	    break;
	}
	offset += 4;
    }

    HAL_FLASH_Lock();

    return err;
}
