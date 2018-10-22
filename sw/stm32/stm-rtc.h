/*
 * stm-rtc.h
 * ---------
 * Functions and defines to use the externally connected RTC chip.
 *
 * Copyright (c) 2015-2016, NORDUnet A/S All rights reserved.
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

#ifndef __STM32_RTC_H
#define __STM32_RTC_H

#include "stm32f4xx_hal.h"

extern void rtc_init(void);
extern HAL_StatusTypeDef rtc_device_ready(uint16_t i2c_addr);
extern HAL_StatusTypeDef rtc_enable_oscillator();
extern HAL_StatusTypeDef rtc_send_byte(const uint16_t i2c_addr, const uint8_t value, const uint16_t timeout);
extern HAL_StatusTypeDef rtc_read_bytes (const uint16_t i2c_addr, uint8_t *buf, const uint8_t len, const uint16_t timeout);

#define RTC_RTC_ADDR			0xdf
#define RTC_EEPROM_ADDR			0xaf

#define RTC_RTC_ADDR_W			RTC_RTC_ADDR ^ 1	/* turn off LSB to write */
#define RTC_EEPROM_ADDR_W		RTC_EEPROM_ADDR ^ 1	/* turn off LSB to write */

#define RTC_SRAM_TOTAL_BYTES		0x5f
#define RTC_EEPROM_TOTAL_BYTES		0x7f

#define RTC_EEPROM_EUI48_OFFSET		0xf0
#define RTC_EEPROM_EUI48_BYTES		8

#define RTC_TIME_OFFSET			0x0  /* Time is at offest 0 in SRAM */
#define RTC_TIME_BYTES			8

#endif /* __STM32_RTC_H */
