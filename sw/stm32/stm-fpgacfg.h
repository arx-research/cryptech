/*
 * stm-fpgacfg.h
 * ---------
 * Functions and defines for accessing the FPGA config memory and controlling
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

#ifndef __STM32_FPGACFG_H
#define __STM32_FPGACFG_H

#include "stm32f4xx_hal.h"
#include "spiflash_n25q128.h"

#define FPGACFG_SECTOR_SIZE		N25Q128_SECTOR_SIZE

/* Pins connected to the FPGA config memory (SPI flash) */
#define PROM_FPGA_DIS_Pin              GPIO_PIN_14
#define PROM_FPGA_DIS_GPIO_Port        GPIOI
#define PROM_ARM_ENA_Pin               GPIO_PIN_6
#define PROM_ARM_ENA_GPIO_Port         GPIOF
#define PROM_CS_N_Pin                  GPIO_PIN_12
#define PROM_CS_N_GPIO_Port            GPIOB
/* Pins for controlling the FPGA */
#define FPGA_INIT_Port                 GPIOJ
#define FPGA_INIT_Pin                  GPIO_PIN_7
#define FPGA_PROGRAM_Port              GPIOJ
#define FPGA_PROGRAM_Pin               GPIO_PIN_8
/* FPGA status */
#define FPGA_DONE_Port                 GPIOJ
#define FPGA_DONE_Pin                  GPIO_PIN_15

#define FPGACFG_GPIO_INIT()		\
    __GPIOI_CLK_ENABLE();		\
    __GPIOF_CLK_ENABLE();		\
    __GPIOB_CLK_ENABLE();		\
    __GPIOJ_CLK_ENABLE();		\
    /* Configure GPIO pins for FPGA access control: PROM_FPGA_DIS, PROM_ARM_ENA */ \
    gpio_output(PROM_FPGA_DIS_GPIO_Port, PROM_FPGA_DIS_Pin, GPIO_PIN_RESET);       \
    gpio_output(PROM_ARM_ENA_GPIO_Port, PROM_ARM_ENA_Pin, GPIO_PIN_RESET);         \
    /* Configure GPIO pin for FPGA config memory chip select : PROM_CS_N */        \
    gpio_output(PROM_CS_N_GPIO_Port, PROM_CS_N_Pin, GPIO_PIN_SET);	           \
    /* Configure GPIO pins FPGA_INIT and FPGA_PROGRAM to reset the FPGA */         \
    gpio_output(FPGA_INIT_Port, FPGA_INIT_Pin, GPIO_PIN_RESET);			   \
    gpio_output(FPGA_PROGRAM_Port, FPGA_PROGRAM_Pin, GPIO_PIN_SET);		   \
    /* Configure FPGA_DONE input pin */   					   \
    gpio_input(FPGA_DONE_Port, FPGA_DONE_Pin, GPIO_PULLUP)


enum fpgacfg_access_ctrl {
    ALLOW_NONE,
    ALLOW_FPGA,
    ALLOW_ARM,
};

enum fpgacfg_reset {
    RESET_FULL,
    RESET_REGISTERS,
};

extern void fpgacfg_init(void);
extern HAL_StatusTypeDef fpgacfg_check_id(void);
extern HAL_StatusTypeDef fpgacfg_write_data(uint32_t offset, const uint8_t *buf, const uint32_t len);
extern HAL_StatusTypeDef fpgacfg_erase_sector(uint32_t sector_offset);
extern void fpgacfg_access_control(enum fpgacfg_access_ctrl access);
/* Reset the FPGA */
extern void fpgacfg_reset_fpga(enum fpgacfg_reset reset);
/* Check status of FPGA bitstream loading */
extern HAL_StatusTypeDef fpgacfg_check_done(void);

#endif /* __STM32_FPGACFG_H */
