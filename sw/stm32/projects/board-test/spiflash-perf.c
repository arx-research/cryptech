/*
 * Test read/write/erase performance of the N25Q128 SPI flash chip.
 */

#include "string.h"

#include "stm-init.h"
#include "stm-led.h"
#include "stm-uart.h"
#include "stm-keystore.h"
#include "spiflash_n25q128.h"

/*
 * Use the keystore memory for testing, because it's less involved than
 * using the FPGA configuration memory, and less work to restore it to a
 * useful configuration.
 *
 * However, rather than using the stm-keystore abstractions, this version
 * goes straight to the low-level API.
 */

extern struct spiflash_ctx keystore_ctx;
static struct spiflash_ctx *ctx = &keystore_ctx;

/*
 * 1a. Read the entire flash by pages, ignoring data.
 */
static void test_read_page(void)
{
    uint8_t read_buf[N25Q128_PAGE_SIZE];
    uint32_t i;
    int err;

    for (i = 0; i < N25Q128_NUM_PAGES; ++i) {
        err = n25q128_read_page(ctx, i, read_buf);
        if (err != HAL_OK) {
            uart_send_string("ERROR: n25q128_read_page returned ");
            uart_send_integer(err, 1);
            uart_send_string("\r\n");
            break;
        }
    }
}

/*
 * 1b. Read the entire flash by subsectors, ignoring data.
 */
static void test_read_subsector(void)
{
    uint8_t read_buf[N25Q128_SUBSECTOR_SIZE];
    uint32_t i;
    int err;

    for (i = 0; i < N25Q128_NUM_SUBSECTORS; ++i) {
        err = n25q128_read_subsector(ctx, i, read_buf);
        if (err != HAL_OK) {
            uart_send_string("ERROR: n25q128_read_subsector returned ");
            uart_send_integer(err, 1);
            uart_send_string("\r\n");
            break;
        }
    }
}

/*
 * Read the flash data and verify it against a known pattern.
 * It turns out that verification doesn't slow us down in any measurable
 * way, because memcmp on 256 bytes is pretty inconsequential.
 */
static void _read_verify(uint8_t *vrfy_buf)
{
    uint8_t read_buf[N25Q128_PAGE_SIZE];
    uint32_t i;
    int err;

    for (i = 0; i < N25Q128_NUM_PAGES; ++i) {
        err = n25q128_read_page(ctx, i, read_buf);
        if (err != HAL_OK) {
            uart_send_string("ERROR: n25q128_read_page returned ");
            uart_send_integer(err, 1);
            uart_send_string("\r\n");
            break;
        }
        if (memcmp(read_buf, vrfy_buf, N25Q128_PAGE_SIZE) != 0) {
            uart_send_string("ERROR: verify failed in page ");
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
    int err;

    for (i = 0; i < N25Q128_NUM_SECTORS; ++i) {
        err = n25q128_erase_sector(ctx, i);
        if (err != HAL_OK) {
            uart_send_string("ERROR: n25q128_erase_sector returned ");
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
    int err;

    for (i = 0; i < N25Q128_NUM_SUBSECTORS; ++i) {
        err = n25q128_erase_subsector(ctx, i);
        if (err != HAL_OK) {
            uart_send_string("ERROR: n25q128_erase_subsector returned ");
            uart_send_integer(err, 1);
            uart_send_string("\r\n");
            break;
        }
    }
}

/*
 * 2c. Erase the entire flash in bulk.
 */
static void test_erase_bulk(void)
{
    int err;

    err = n25q128_erase_bulk(ctx);
    if (err != HAL_OK) {
        uart_send_string("ERROR: n25q128_erase_bulk returned ");
        uart_send_integer(err, 1);
        uart_send_string("\r\n");
    }
}

/*
 * 2d. Read the entire flash, verify erasure.
 */
static void test_verify_erase(void)
{
    uint8_t vrfy_buf[N25Q128_PAGE_SIZE];
    uint32_t i;

    for (i = 0; i < sizeof(vrfy_buf); ++i)
        vrfy_buf[i] = 0xFF;

    _read_verify(vrfy_buf);
}

/*
 * 3a. Write the entire flash with a pattern.
 */
static void test_write_page(void)
{
    uint8_t write_buf[N25Q128_PAGE_SIZE];
    uint32_t i;
    int err;

    for (i = 0; i < sizeof(write_buf); ++i)
        write_buf[i] = i & 0xFF;

    for (i = 0; i < N25Q128_NUM_PAGES; ++i) {
        err = n25q128_write_page(ctx, i, write_buf);
        if (err != HAL_OK) {
            uart_send_string("ERROR: n25q128_write_page returned ");
            uart_send_integer(err, 1);
            uart_send_string(" for page ");
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
    uint8_t vrfy_buf[N25Q128_PAGE_SIZE];
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

    if (n25q128_check_id(ctx) != HAL_OK) {
        uart_send_string("ERROR: n25q128_check_id failed\r\n");
        return 0;
    }

    uart_send_string("Starting...\r\n");

    time_check("read page       ", test_read_page(),       N25Q128_NUM_PAGES);
    time_check("read subsector  ", test_read_subsector(),  N25Q128_NUM_SUBSECTORS);
    time_check("erase subsector ", test_erase_subsector(), N25Q128_NUM_SUBSECTORS);
    time_check("erase sector    ", test_erase_sector(),    N25Q128_NUM_SECTORS);
    time_check("erase bulk      ", test_erase_bulk(),      1);
    time_check("verify erase    ", test_verify_erase(),    N25Q128_NUM_PAGES);
    time_check("write page      ", test_write_page(),      N25Q128_NUM_PAGES);
    time_check("verify write    ", test_verify_write(),    N25Q128_NUM_PAGES);

    uart_send_string("Done.\r\n\r\n");
    return 0;
}
