/*
 * Test read/write performance of the fmc bus
 */
#include "stm-init.h"
#include "stm-led.h"
#include "stm-fmc.h"
#include "stm-uart.h"

#define TEST_NUM_ROUNDS		2000000

RNG_HandleTypeDef rng_inst;

static void MX_RNG_Init(void)
{
    rng_inst.Instance = RNG;
    HAL_RNG_Init(&rng_inst);
}

static uint32_t random(void)
{
    uint32_t rnd;
    if (HAL_RNG_GenerateRandomNumber(&rng_inst, &rnd) != HAL_OK) {
	uart_send_string("HAL_RNG_GenerateRandomNumber failed\r\n");
	Error_Handler();
    }
    return rnd;
}

static void sanity(void)
{
    uint32_t rnd, data;
  
    rnd = random();  
    if (fmc_write_32(0, rnd) != 0) {
	uart_send_string("fmc_write_32 failed\r\n");
	Error_Handler();
    }
    if (fmc_read_32(0, &data) != 0) {
	uart_send_string("fmc_read_32 failed\r\n");
	Error_Handler();
    }
    if (data != rnd) {
	uart_send_string("Data bus fail: expected ");
	uart_send_hex(rnd, 8);
	uart_send_string(", got ");
	uart_send_hex(data, 8);
	uart_send_string(", diff ");
	uart_send_hex(data ^ rnd, 8);
	uart_send_string("\r\n");
	Error_Handler();
    }
}

static void _time_check(char *label, const uint32_t t0)
{
    uint32_t t = HAL_GetTick() - t0;

    uart_send_string(label);
    uart_send_integer(t / 1000, 1);
    uart_send_char('.');
    uart_send_integer(t % 1000, 3);
    uart_send_string(" seconds, ");
    uart_send_integer(((1000 * TEST_NUM_ROUNDS) / t), 1);
    uart_send_string("/sec\r\n");
}

#define time_check(_label_, _expr_)		\
    do {					\
	uint32_t _t = HAL_GetTick();		\
	(_expr_);				\
	_time_check(_label_, _t);		\
    } while (0)

static void test_read(void)
{
    uint32_t i, data;
    
    for (i = 0; i < TEST_NUM_ROUNDS; ++i) {
	if (fmc_read_32(0, &data) != 0) {
	    uart_send_string("fmc_read_32 failed\r\n");
	    Error_Handler();
	}
    }
}

static void test_write(void)
{
    uint32_t i;
    
    for (i = 0; i < TEST_NUM_ROUNDS; ++i) {
	if (fmc_write_32(0, i) != 0) {
	    uart_send_string("fmc_write_32 failed\r\n");
	    Error_Handler();
	}
    }
}

int main(void)
{
    stm_init();
    // initialize rng
    MX_RNG_Init();

    sanity();

    time_check("read  ", test_read());
    time_check("write ", test_write());

    uart_send_string("Done.\r\n\r\n");
    return 0;
}
