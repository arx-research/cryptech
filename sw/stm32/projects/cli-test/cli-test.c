/*
 * cli-test.c
 * ---------
 * Test code with a small CLI on the management interface
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

#include "mgmt-cli.h"
#include "mgmt-dfu.h"
#include "mgmt-fpga.h"
#include "mgmt-misc.h"
#include "mgmt-show.h"
#include "mgmt-test.h"
#include "mgmt-keystore.h"

#include <string.h>
#include <strings.h>


/* MGMT UART interrupt receive buffer (data will be put in a larger ring buffer) */
volatile uint8_t uart_rx;

/* Sleep for specified number of seconds -- used after bad PIN. */
void hal_sleep(const unsigned seconds) { HAL_Delay(seconds * 1000); }

int
main()
{
    stm_init();

    led_on(LED_GREEN);

    while (1) {
        cli_main();
        /* embedded_cli_loop returns when the user enters 'quit' or 'exit' */
    }

    /* NOT REACHED */
    Error_Handler();
}


/*
 * Dummy to solve link problem.  Not obvious to me that a program
 * called "cli-test" should be duplicating all of the HSM keystore
 * logic, let alone that it should be doing it badly, but, whatever.
 *
 * We could just copy the sdram_malloc() code from hsm.c, but since
 * one of the other commands linked into cli-test goes merrily stomping
 * all over the entire SDRAM chip, that might not work out so well.
 *
 * Issue deferred until somebody cares.
 */

#warning hal_allocate_static_memory() stubbed out in cli-test, see source code

void *hal_allocate_static_memory(const size_t size)
{
    return NULL;
}

int hal_free_static_memory(const void * const ptr)
{
    return 0;
}

