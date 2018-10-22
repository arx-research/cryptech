/*
 * Test Joachim's MKMIF core.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <sys/time.h>

#include <hal.h>

#define SCLK_DIV 0x00000020

typedef union {
    uint8_t byte[4];
    uint32_t word;
} byteword_t;

static hal_error_t sclk_test(hal_core_t *core, const uint32_t divisor)
{
    uint32_t readback;
    hal_error_t err;

    printf("Trying to adjust the clockspeed.\n");

    if ((err = hal_mkmif_set_clockspeed(core, divisor)) != HAL_OK) {
        printf("hal_mkmif_set_clockspeed: %s\n", hal_error_string(err));
        return err;
    }
    if ((err = hal_mkmif_get_clockspeed(core, &readback)) != HAL_OK) {
        printf("hal_mkmif_get_clockspeed: %s\n", hal_error_string(err));
        return err;
    }
    if (readback != divisor) {
        printf("expected %x, got %x\n", (unsigned int)divisor, (unsigned int)readback);
        return HAL_ERROR_IO_UNEXPECTED;
    }
    return HAL_OK;
}

static hal_error_t init_test(hal_core_t *core)
{
    hal_error_t err;

    printf("Trying to init to the memory in continuous mode.\n");

    if ((err = hal_mkmif_init(core)) != HAL_OK) {
        printf("hal_mkmif_init: %s\n", hal_error_string(err));
        return err;
    }

    return HAL_OK;
}

static hal_error_t write_test(hal_core_t *core)
{
    uint32_t write_data;
    uint32_t write_address;
    int i;
    hal_error_t err;

    for (write_data = 0x01020304, write_address = 0, i = 0;
         i < 0x10;
         write_data += 0x01010101, write_address += 4, ++i) {

        printf("Trying to write 0x%08x to memory address 0x%08x.\n",
               (unsigned int)write_data, (unsigned int)write_address);

        if ((err = hal_mkmif_write_word(core, write_address, write_data)) != HAL_OK) {
            printf("hal_mkmif_write: %s\n", hal_error_string(err));
            return err;
        }
    }

    return HAL_OK;
}

static hal_error_t read_test(hal_core_t *core)
{
    uint32_t read_data;
    uint32_t read_address;
    int i;
    hal_error_t err;

    for (read_address = 0, i = 0;
         i < 0x10;
         read_address += 4, ++i) {

        printf("Trying to read from memory address 0x%08x.\n", (unsigned int)read_address);

        if ((err = hal_mkmif_read_word(core, read_address, &read_data)) != HAL_OK) {
            printf("hal_mkmif_read: %s\n", hal_error_string(err));
            return err;
        }
        printf("Data read: 0x%08x\n", (unsigned int)read_data);
    }

    return HAL_OK;
}

static hal_error_t write_read_test(hal_core_t *core)
{
    uint32_t data;
    uint32_t readback;
    hal_error_t err;

    printf("Trying to write 0xdeadbeef to the memory and then read back.\n");

    data = 0xdeadbeef;

    if ((err = hal_mkmif_write_word(core, 0x00000000, data)) != HAL_OK) {
        printf("write error: %s\n", hal_error_string(err));
        return err;
    }

    if ((err = hal_mkmif_read_word(core, 0x00000000, &readback)) != HAL_OK) {
        printf("read error: %s\n", hal_error_string(err));
        return err;
    }

    if (readback != data) {
        printf("read %08x, expected %08x\n", (unsigned int)readback, (unsigned int)data);
        return HAL_ERROR_IO_UNEXPECTED;
    }

    return HAL_OK;
}

int main(void)
{
    hal_core_t *core = hal_core_find(MKMIF_NAME, NULL);

    if (core == NULL) {
        printf("MKMIF core not present, not testing.\n");
        return HAL_ERROR_CORE_NOT_FOUND;
    }

    hal_io_set_debug(1);

    return
        sclk_test(core, SCLK_DIV) ||
        init_test(core) ||
        write_read_test(core) ||
        write_test(core) ||
        read_test(core);
}
