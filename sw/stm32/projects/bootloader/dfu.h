/*
 * dfu.h
 * ---------
 * Device Firmware Upgrade defines and prototypes.
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

#ifndef __STM32_BOOTLOADER_DFU_H
#define __STM32_BOOTLOADER_DFU_H

#include "stm-init.h"

/* symbols defined in the linker script (STM32F429BI_bootloader.ld) */
extern uint32_t CRYPTECH_FIRMWARE_START;
extern uint32_t CRYPTECH_FIRMWARE_END;
extern uint32_t CRYPTECH_DFU_CONTROL;

#define DFU_FIRMWARE_ADDR         ((uint32_t) &CRYPTECH_FIRMWARE_START)
#define DFU_FIRMWARE_END_ADDR     ((uint32_t) &CRYPTECH_FIRMWARE_END)
#define DFU_UPLOAD_CHUNK_SIZE     4096

/* Magic bytes to signal the bootloader it should jump to the firmware
 * instead of trying to receive a new firmware using the MGMT UART.
 */
#define HARDWARE_EARLY_DFU_JUMP   0xBADABADA

extern __IO uint32_t *dfu_control;
extern __IO uint32_t *dfu_firmware;
extern __IO uint32_t *dfu_msp_ptr;
extern __IO uint32_t *dfu_code_ptr;

extern int dfu_receive_firmware(void);


#endif /* __STM32_BOOTLOADER_DFU_H */
