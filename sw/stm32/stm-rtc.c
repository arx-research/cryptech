/*
 * stm-rtc.c
 * ----------
 * Functions for using the externally connected RTC chip.
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

#include "stm-init.h"
#include "stm-rtc.h"

I2C_HandleTypeDef hi2c_rtc;

/* I2C2 init function (external RTC chip) */
void rtc_init(void)
{
    hi2c_rtc.Instance = I2C2;
    hi2c_rtc.Init.ClockSpeed = 10000;
    hi2c_rtc.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c_rtc.Init.OwnAddress1 = 0;  /* Will operate as Master */
    hi2c_rtc.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c_rtc.Init.DualAddressMode = I2C_DUALADDRESS_DISABLED;
    hi2c_rtc.Init.OwnAddress2 = 0;
    hi2c_rtc.Init.GeneralCallMode = I2C_GENERALCALL_DISABLED;
    hi2c_rtc.Init.NoStretchMode = I2C_NOSTRETCH_DISABLED;

    if (HAL_I2C_Init(&hi2c_rtc) != HAL_OK) {
        Error_Handler();
    }
}

HAL_StatusTypeDef rtc_device_ready(uint16_t i2c_addr)
{
    return HAL_I2C_IsDeviceReady (&hi2c_rtc, i2c_addr, 10, 1000);
}

HAL_StatusTypeDef rtc_enable_oscillator(void)
{
    uint8_t buf[2];
    HAL_StatusTypeDef res;

    buf[0] = 0;		/* Offset of RTCSEC */
    buf[1] = 1 << 7;	/* datasheet REGISTERS 5-1, bit 7 = ST (start oscillator) */

    while (HAL_I2C_Master_Transmit (&hi2c_rtc, RTC_RTC_ADDR_W, buf, 2, 1000) != HAL_OK) {
	res = HAL_I2C_GetError (&hi2c_rtc);
	if (res != HAL_I2C_ERROR_AF) {
	    return res;
	}
    }

    return HAL_OK;
}

HAL_StatusTypeDef rtc_send_byte(const uint16_t i2c_addr, uint8_t value, const uint16_t timeout)
{
    HAL_StatusTypeDef res;

    while (HAL_I2C_Master_Transmit (&hi2c_rtc, i2c_addr, &value, 1, timeout) != HAL_OK) {
	res = HAL_I2C_GetError (&hi2c_rtc);
	if (res != HAL_I2C_ERROR_AF) {
	    return res;
	}
    }

    return HAL_OK;
}

HAL_StatusTypeDef rtc_read_bytes (const uint16_t i2c_addr, uint8_t *buf, const uint8_t len, const uint16_t timeout)
{
    HAL_StatusTypeDef res;

    while (HAL_I2C_Master_Receive (&hi2c_rtc, i2c_addr, buf, len, timeout) != HAL_OK) {
	res = HAL_I2C_GetError (&hi2c_rtc);
	if (res != HAL_I2C_ERROR_AF) {
	    return res;
	}
    }

    return HAL_OK;
}
