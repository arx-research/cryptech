/*
 * mgmt-test.c
 * -----------
 * CLI hardware test functions.
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
#include "stm-sdram.h"
#include "stm-fmc.h"
#include "stm-fpgacfg.h"

#include "mgmt-cli.h"
#include "mgmt-test.h"

#include "test_sdram.h"
#include "test_mkmif.h"
#include "test-fmc.h"

#include <stdlib.h>

static int cmd_test_sdram(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    // run external memory initialization sequence
    int ok, num_cycles = 1, i, test_completed;

    command = command;

    if (argc == 1) {
	num_cycles = strtol(argv[0], NULL, 0);
	if (num_cycles > 100) num_cycles = 100;
	if (num_cycles < 1) num_cycles = 1;
    }

    for (i = 1; i <= num_cycles; i++) {
	cli_print(cli, "Starting SDRAM test (%i/%i)", i, num_cycles);
	test_completed = 0;
	// set LFSRs to some initial value, LFSRs will produce
	// pseudo-random 32-bit patterns to test our memories
	lfsr1 = 0xCCAA5533;
	lfsr2 = 0xCCAA5533;

	cli_print(cli, "Run sequential write-then-read test for the first chip");
	ok = test_sdram_sequential(SDRAM_BASEADDR_CHIP1);
	if (!ok) break;

	cli_print(cli, "Run random write-then-read test for the first chip");
	ok = test_sdram_random(SDRAM_BASEADDR_CHIP1);
	if (!ok) break;

	cli_print(cli, "Run sequential write-then-read test for the second chip");
	ok = test_sdram_sequential(SDRAM_BASEADDR_CHIP2);
	if (!ok) break;

	cli_print(cli, "Run random write-then-read test for the second chip");
	ok = test_sdram_random(SDRAM_BASEADDR_CHIP2);
	if (!ok) break;

	// turn blue led on (testing two chips at the same time)
	led_on(LED_BLUE);

	cli_print(cli, "Run interleaved write-then-read test for both chips at once");
	ok = test_sdrams_interleaved(SDRAM_BASEADDR_CHIP1, SDRAM_BASEADDR_CHIP2);

	led_off(LED_BLUE);
	test_completed = 1;
	cli_print(cli, "SDRAM test (%i/%i) completed\r\n", i, num_cycles);
    }

    if (! test_completed) {
	cli_print(cli, "SDRAM test failed (%i/%i)", i, num_cycles);
    } else {
	cli_print(cli, "SDRAM test completed successfully");
    }

    return CLI_OK;
}

static int cmd_test_fmc(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    int i, num_cycles = 1, num_rounds = 100000;

    command = command;

    if (argc >= 1) {
	num_cycles = strtol(argv[0], NULL, 0);
	if (num_cycles > 100000) num_cycles = 100000;
	if (num_cycles < 1) num_cycles = 1;
    }

    if (argc == 2) {
	num_rounds = strtol(argv[1], NULL, 0);
	if (num_rounds > 1000000) num_rounds = 1000000;
	if (num_rounds < 1) num_rounds = 1;
    }

    cli_print(cli, "Checking if FPGA has loaded it's bitstream");
    // Blink blue LED until the FPGA reports it has loaded it's bitstream
    led_on(LED_BLUE);
    while (! fpgacfg_check_done()) {
	for (i = 0; i < 4; i++) {
	    HAL_Delay(500);
	    led_toggle(LED_BLUE);
	}
    }

    // turn on green led, turn off other leds
    led_on(LED_GREEN);
    led_off(LED_YELLOW);
    led_off(LED_RED);
    led_off(LED_BLUE);

    // vars
    volatile int data_test_ok = 0, addr_test_ok = 0, successful_runs = 0, failed_runs = 0, sleep = 0;

    for (i = 1; i <= num_cycles; i++) {
	cli_print(cli, "Starting FMC test (%i/%i)", i, num_cycles);

	// test data bus
	data_test_ok = test_fpga_data_bus(cli, num_rounds);
	// test address bus
	addr_test_ok = test_fpga_address_bus(cli, num_rounds);

	cli_print(cli, "Data: %i, addr %i", data_test_ok, addr_test_ok);

	if (data_test_ok == num_rounds &&
	    addr_test_ok == num_rounds) {
	    // toggle yellow led to indicate, that we are alive
	    led_toggle(LED_YELLOW);

	    successful_runs++;
	    sleep = 0;
	} else {
	    led_on(LED_RED);
	    failed_runs++;
	    sleep = 2000;
	}

	cli_print(cli, "Success %i, failed %i runs\r\n", successful_runs, failed_runs);
	HAL_Delay(sleep);
    }

    return CLI_OK;
}

void configure_cli_test(struct cli_def *cli)
{
    struct cli_command *c = cli_register_command(cli, NULL, "test", NULL, 0, 0, NULL);

    /* test sdram */
    cli_register_command(cli, c, "sdram", cmd_test_sdram, 0, 0, "Run SDRAM tests");

    /* test mkmif */
    cli_register_command(cli, c, "mkmif", cmd_test_mkmif, 0, 0, "Run Master Key Memory Interface tests");

    /* test fmc */
    cli_register_command(cli, c, "fmc", cmd_test_fmc, 0, 0, "Run FMC bus tests");
}
