/*
 * bootloader.c
 * ------------
 * Bootloader to either install new firmware received from the MGMT UART,
 * or jump to previously installed firmware.
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
#include "stm-led.h"
#include "stm-uart.h"
#include "stm-fmc.h"
#include "dfu.h"

/* stub these out to avoid linker error */
void fpgacfg_init(void) { }
void sdram_init(void) { }
void *hal_allocate_static_memory(const size_t size) { return 0; }

/* Linker symbols are strange in C. Make regular pointers for sanity. */
__IO uint32_t *dfu_control = &CRYPTECH_DFU_CONTROL;
__IO uint32_t *dfu_firmware = &CRYPTECH_FIRMWARE_START;
/* The first word in the firmware is an address to the stack (msp) */
__IO uint32_t *dfu_msp_ptr = &CRYPTECH_FIRMWARE_START;
/* The second word in the firmware is a pointer to the code
 * (points at the Reset_Handler from the linker script).
 */
__IO uint32_t *dfu_code_ptr = &CRYPTECH_FIRMWARE_START + 1;

typedef  void (*pFunction)(void);

/* called from Reset_Handler */
void check_early_dfu_jump(void)
{
    /* Check if we've just rebooted in order to jump to the firmware. */
    if (*dfu_control == HARDWARE_EARLY_DFU_JUMP) {
	*dfu_control = 0;
        pFunction loaded_app = (pFunction) *dfu_code_ptr;
        /* Set the stack pointer to the correct one for the firmware */
        __set_MSP(*dfu_msp_ptr);
        /* Set the Vector Table Offset Register */
        SCB->VTOR = (uint32_t) dfu_firmware;
        loaded_app();
        while (1);
    }
}

int should_dfu()
{
    int i;
    uint8_t rx = 0;

    /* While blinking the blue LED for 5 seconds, see if we receive a CR on the MGMT UART.
     * We've discussed also requiring one or both of the FPGA config jumpers installed
     * before allowing DFU of the STM32 - that check could be done here.
     */
    led_on(LED_BLUE);
    for (i = 0; i < 50; i++) {
	HAL_Delay(100);
	led_toggle(LED_BLUE);
	if (uart_recv_char(&rx, 0) == HAL_OK) {
	    if (rx == 13) return 1;
	}
    }
    return 0;
}

/* Sleep for specified number of seconds -- used after bad PIN. */
void hal_sleep(const unsigned seconds) { HAL_Delay(seconds * 1000); }

int main(void)
{
    int status;

    stm_init();

    uart_send_string("\r\n\r\nThis is the bootloader speaking...");

    if (should_dfu()) {
	led_off(LED_BLUE);
	if ((status = dfu_receive_firmware()) != 0) {
	    /* Upload of new firmware failed, reboot after lighting the red LED
	     * for three seconds.
	     */
	    led_off(LED_BLUE);
	    led_on(LED_RED);
	    uart_send_string("dfu_receive_firmware failed: ");
	    uart_send_hex(status, 2);
	    uart_send_string("\r\n\r\nRebooting in three seconds\r\n");
	    HAL_Delay(3000);
	    HAL_NVIC_SystemReset();
	    while (1) {};
	}
    }

    /* Set dfu_control to the magic value that will cause the us to call do_early_dfu_jump
     * after rebooting back into this main() function.
     */
    *dfu_control = HARDWARE_EARLY_DFU_JUMP;

    uart_send_string("loading firmware\r\n\r\n");

    /* De-initialize hardware by rebooting */
    HAL_NVIC_SystemReset();
    while (1) {};
}
