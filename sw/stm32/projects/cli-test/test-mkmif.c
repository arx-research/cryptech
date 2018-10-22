/*
 * Test Joachim's MKMIF core.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <sys/time.h>

/* Rename both CMSIS HAL_OK and libhal HAL_OK to disambiguate */
#define HAL_OK CMSIS_HAL_OK
#include "mgmt-cli.h"

#undef HAL_OK
#define HAL_OK LIBHAL_OK
#include "hal.h"
#undef HAL_OK


#define SCLK_DIV 0x20

typedef union {
    uint8_t byte[4];
    uint32_t word;
} byteword_t;

static hal_error_t sclk_test(struct cli_def *cli, hal_core_t *core, const uint32_t divisor)
{
    uint32_t readback;
    hal_error_t err;

    cli_print(cli, "Trying to adjust the clockspeed (divisor %x).", (unsigned int) divisor);

    if ((err = hal_mkmif_set_clockspeed(core, divisor)) != LIBHAL_OK) {
        cli_print(cli, "hal_mkmif_set_clockspeed: %s", hal_error_string(err));
        return err;
    }
    if ((err = hal_mkmif_get_clockspeed(core, &readback)) != LIBHAL_OK) {
        cli_print(cli, "hal_mkmif_get_clockspeed: %s", hal_error_string(err));
        return err;
    }
    if (readback != divisor) {
        cli_print(cli, "expected %x, got %x", (unsigned int)divisor, (unsigned int)readback);
        return HAL_ERROR_IO_UNEXPECTED;
    }
    return LIBHAL_OK;
}

static hal_error_t init_test(struct cli_def *cli, hal_core_t *core)
{
    hal_error_t err;

    cli_print(cli, "Trying to init to the memory in continuous mode.");

    if ((err = hal_mkmif_init(core)) != LIBHAL_OK) {
        cli_print(cli, "hal_mkmif_init: %s", hal_error_string(err));
        return err;
    }

    return LIBHAL_OK;
}

static hal_error_t write_test(struct cli_def *cli, hal_core_t *core)
{
    uint32_t write_data;
    uint32_t write_address;
    int i;
    hal_error_t err;

    for (write_data = 0x01020304, write_address = 0, i = 0;
         i < 0x10;
         write_data += 0x01010101, write_address += 4, ++i) {

        cli_print(cli, "Trying to write 0x%08x to memory address 0x%08x.",
               (unsigned int)write_data, (unsigned int)write_address);

        if ((err = hal_mkmif_write_word(core, write_address, write_data)) != LIBHAL_OK) {
            cli_print(cli, "hal_mkmif_write: %s", hal_error_string(err));
            return err;
        }
    }

    return LIBHAL_OK;
}

static hal_error_t read_test(struct cli_def *cli, hal_core_t *core)
{
    uint32_t read_data;
    uint32_t read_address;
    int i;
    hal_error_t err;

    for (read_address = 0, i = 0;
         i < 0x10;
         read_address += 4, ++i) {

        cli_print(cli, "Trying to read from memory address 0x%08x.", (unsigned int)read_address);

        if ((err = hal_mkmif_read_word(core, read_address, &read_data)) != LIBHAL_OK) {
            cli_print(cli, "hal_mkmif_read: %s", hal_error_string(err));
            return err;
        }
        cli_print(cli, "Data read: 0x%08x", (unsigned int)read_data);
    }

    return LIBHAL_OK;
}

static hal_error_t write_read_test(struct cli_def *cli, hal_core_t *core)
{
    uint32_t data;
    uint32_t readback;
    hal_error_t err;

    cli_print(cli, "Trying to write 0xdeadbeef to the memory and then read back.");

    data = 0xdeadbeef;

    if ((err = hal_mkmif_write_word(core, 0x00000000, data)) != LIBHAL_OK) {
        cli_print(cli, "write error: %s", hal_error_string(err));
        return err;
    }

    if ((err = hal_mkmif_read_word(core, 0x00000000, &readback)) != LIBHAL_OK) {
        cli_print(cli, "read error: %s", hal_error_string(err));
        return err;
    }

    if (readback != data) {
        cli_print(cli, "read %08x, expected %08x", (unsigned int)readback, (unsigned int)data);
        return HAL_ERROR_IO_UNEXPECTED;
    }

    return LIBHAL_OK;
}

int cmd_test_mkmif(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    hal_core_t *core = hal_core_find(MKMIF_NAME, NULL);
    hal_error_t res;

    command = command;
    argv = argv;
    argc = argc;

    if (core == NULL) {
        cli_print(cli, "MKMIF core not present, not testing.");
        return HAL_ERROR_CORE_NOT_FOUND;
    }

    res =
        sclk_test(cli, core, SCLK_DIV) ||
        init_test(cli, core) ||
        write_read_test(cli, core) ||
        write_test(cli, core) ||
        read_test(cli, core);

    if (res != LIBHAL_OK) {
	cli_print(cli, "\nTest FAILED");
    }

    return CLI_OK;
}
