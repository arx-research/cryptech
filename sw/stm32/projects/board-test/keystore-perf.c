/*
 * Test read/write/erase performance of the flash keystore.
 */

#include "string.h"

#include "stm-init.h"
#include "stm-led.h"
#include "stm-uart.h"
#include "stm-keystore.h"

/*
 * 1. Read the entire flash by subsectors, ignoring data.
 */
static void test_read_data(void)
{
    uint8_t read_buf[KEYSTORE_SUBSECTOR_SIZE];
    uint32_t i;
    HAL_StatusTypeDef err;

    for (i = 0; i < KEYSTORE_NUM_SUBSECTORS; ++i) {
        err = keystore_read_data(i * KEYSTORE_SUBSECTOR_SIZE, read_buf, KEYSTORE_SUBSECTOR_SIZE);
        if (err != HAL_OK) {
            uart_send_string("ERROR: keystore_read_data returned ");
            uart_send_integer(err, 1);
            uart_send_string("\r\n");
            break;
        }
    }
}

/*
 * Read the flash data and verify it against a known pattern.
 */
static void _read_verify(uint8_t *vrfy_buf)
{
    uint8_t read_buf[KEYSTORE_SUBSECTOR_SIZE];
    uint32_t i;
    HAL_StatusTypeDef err;

    for (i = 0; i < KEYSTORE_NUM_SUBSECTORS; ++i) {
        err = keystore_read_data(i * KEYSTORE_SUBSECTOR_SIZE, read_buf, KEYSTORE_SUBSECTOR_SIZE);
        if (err != HAL_OK) {
            uart_send_string("ERROR: keystore_read_data returned ");
            uart_send_integer(err, 1);
            uart_send_string("\r\n");
            break;
        }
        if (memcmp(read_buf, vrfy_buf, KEYSTORE_SUBSECTOR_SIZE) != 0) {
            uart_send_string("ERROR: verify failed in subsector ");
            uart_send_integer(i, 1);
            uart_send_string("\r\n");
            break;
        }
    }
}

/*
 * 2a. Erase the entire flash by sectors.
 */
static void test_erase_sector(void)
{
    uint32_t i;
    HAL_StatusTypeDef err;

    for (i = 0; i < KEYSTORE_NUM_SECTORS; ++i) {
        err = keystore_erase_sector(i);
        if (err != HAL_OK) {
            uart_send_string("ERROR: keystore_erase_sector returned ");
            uart_send_integer(err, 1);
            uart_send_string("\r\n");
            break;
        }
    }
}

/*
 * 2b. Erase the entire flash by subsectors.
 */
static void test_erase_subsector(void)
{
    uint32_t i;
    HAL_StatusTypeDef err;

    for (i = 0; i < KEYSTORE_NUM_SUBSECTORS; ++i) {
        err = keystore_erase_subsector(i);
        if (err != HAL_OK) {
            uart_send_string("ERROR: keystore_erase_subsector returned ");
            uart_send_integer(err, 1);
            uart_send_string("\r\n");
            break;
        }
    }
}

/*
 * 2c. Read the entire flash, verify erasure.
 */
static void test_verify_erase(void)
{
    uint8_t vrfy_buf[KEYSTORE_SUBSECTOR_SIZE];
    uint32_t i;

    for (i = 0; i < sizeof(vrfy_buf); ++i)
        vrfy_buf[i] = 0xFF;

    _read_verify(vrfy_buf);
}

/*
 * 3a. Write the entire flash with a pattern.
 */
static void test_write_data(void)
{
    uint8_t write_buf[KEYSTORE_SUBSECTOR_SIZE];
    uint32_t i;
    HAL_StatusTypeDef err;

    for (i = 0; i < sizeof(write_buf); ++i)
        write_buf[i] = i & 0xFF;

    for (i = 0; i < KEYSTORE_NUM_SUBSECTORS; ++i) {
        err = keystore_write_data(i * KEYSTORE_SUBSECTOR_SIZE, write_buf, KEYSTORE_SUBSECTOR_SIZE);
        if (err != HAL_OK) {
            uart_send_string("ERROR: keystore_write_data returned ");
            uart_send_integer(err, 1);
            uart_send_string(" for subsector ");
            uart_send_integer(i, 1);
            uart_send_string("\r\n");
            break;
        }
    }
}

/*
 * 3b. Read the entire flash, verify data.
 */
static void test_verify_write(void)
{
    uint8_t vrfy_buf[KEYSTORE_SUBSECTOR_SIZE];
    uint32_t i;

    for (i = 0; i < sizeof(vrfy_buf); ++i)
        vrfy_buf[i] = i & 0xFF;

    _read_verify(vrfy_buf);
}

static void _time_check(char *label, const uint32_t t0, uint32_t n_rounds)
{
    uint32_t t = HAL_GetTick() - t0;

    uart_send_string(label);
    uart_send_integer(t / 1000, 1);
    uart_send_char('.');
    uart_send_integer(t % 1000, 3);
    uart_send_string(" sec");
    if (n_rounds > 1) {
        uart_send_string(" for ");
        uart_send_integer(n_rounds, 1);
        uart_send_string(" rounds, ");
        uart_send_integer(t / n_rounds, 1);
        uart_send_char('.');
        uart_send_integer(((t % n_rounds) * 100) / n_rounds, 2);
        uart_send_string(" ms each");
    }
    uart_send_string("\r\n");
}

#define time_check(_label_, _expr_, _n_)	\
    do {					\
	uint32_t _t = HAL_GetTick();		\
	(_expr_);				\
	_time_check(_label_, _t, _n_);		\
    } while (0)

int main(void)
{
    stm_init();

    if (keystore_check_id() != HAL_OK) {
        uart_send_string("ERROR: keystore_check_id failed\r\n");
        return 0;
    }

    uart_send_string("Starting...\r\n");

    time_check("read data       ", test_read_data(),       KEYSTORE_NUM_SUBSECTORS);
    time_check("erase subsector ", test_erase_subsector(), KEYSTORE_NUM_SUBSECTORS);
    time_check("erase sector    ", test_erase_sector(),    KEYSTORE_NUM_SECTORS);
    time_check("verify erase    ", test_verify_erase(),    KEYSTORE_NUM_SUBSECTORS);
    time_check("write data      ", test_write_data(),      KEYSTORE_NUM_SUBSECTORS);
    time_check("verify write    ", test_verify_write(),    KEYSTORE_NUM_SUBSECTORS);

    uart_send_string("Done.\r\n\r\n");
    return 0;
}
