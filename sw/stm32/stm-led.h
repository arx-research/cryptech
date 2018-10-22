/*
 * stm-led.h
 * ---------
 * Defines to control the LEDs through GPIO pins.
 *
 * Copyright (c) 2015, NORDUnet A/S All rights reserved.
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

#ifndef __STM_LED_H
#define __STM_LED_H

#include "stm32f4xx_hal.h"

#ifdef TARGET_CRYPTECH_DEV_BRIDGE
#define LED_PORT        GPIOJ
#define LED_RED         GPIO_PIN_1
#define LED_YELLOW      GPIO_PIN_2
#define LED_GREEN       GPIO_PIN_3
#define LED_BLUE        GPIO_PIN_4
#define LED_CLK_ENABLE  __GPIOJ_CLK_ENABLE
#else
#define LED_PORT        GPIOK
#define LED_RED         GPIO_PIN_7
#define LED_YELLOW      GPIO_PIN_6
#define LED_GREEN       GPIO_PIN_5
#define LED_BLUE        GPIO_PIN_4
#define LED_CLK_ENABLE  __GPIOK_CLK_ENABLE
#endif

#define led_on(pin)     HAL_GPIO_WritePin(LED_PORT,pin,SET)
#define led_off(pin)    HAL_GPIO_WritePin(LED_PORT,pin,RESET)
#define led_toggle(pin) HAL_GPIO_TogglePin(LED_PORT,pin)

#endif /* __STM_LED_H */
