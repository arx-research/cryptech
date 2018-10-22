/*
 * Test code for the RTC.
 *
 * Dumps the SRAM and EEPROM on startup, then enables the oscillator.
 * After that, the clock registers are read once a second - the first byte
 * is seconds (and some control bits, so dont expect 0..59).
 *
 */
#include <string.h>

#include "stm_init.h"
#include "stm-uart.h"

#define DELAY() HAL_Delay(1000)

#define RTC_ADDR			0xdf
#define EEPROM_ADDR			0xaf

#define RTC_ADDR_W			RTC_ADDR ^ 1	/* turn off LSB to write */
#define EEPROM_ADDR_W			EEPROM_ADDR ^ 1	/* turn off LSB to write */

#define SRAM_TOTAL_BYTES		0x5f
#define EEPROM_TOTAL_BYTES		0x7f

#define EEPROM_EUI48_OFFSET		0xf0
#define EEPROM_EUI48_BYTES		8

#define TIME_OFFSET			0x0  /* Time is at offest 0 in SRAM */
#define TIME_BYTES			8


uint8_t buf[1024];
uint32_t i;


uint32_t device_ready(uint16_t i2c_addr)
{
  uart_send_string("Checking readiness of 0x");
  uart_send_hexbyte(i2c_addr);
  uart_send_string("...");

  i = HAL_I2C_IsDeviceReady (&hi2c2, i2c_addr, 10, 1000);
  if (i == HAL_OK) {
    uart_send_string("OK\r\n");
    return 1;
  }

  uart_send_string("Not ready (");
  uart_send_integer(i, 0);
  uart_send_string(")\r\n");

  return 0;
}


void send_byte(const uint16_t i2c_addr, const uint8_t value)
{
  uint8_t ch = value;

  uart_send_string ("Sending ");
  uart_send_hexbyte (ch);
  uart_send_string (" to 0x");
  uart_send_hexbyte(i2c_addr);
  uart_send_string("...");

  while (HAL_I2C_Master_Transmit (&hi2c2, i2c_addr, &ch, 1, 1000) != HAL_OK) {
    if (HAL_I2C_GetError (&hi2c2) != HAL_I2C_ERROR_AF) {
      uart_send_string("Timeout\r\n");
      Error_Handler();
    }
  }

  uart_send_string("OK\r\n");
}

void read_bytes (uint8_t *buf, const uint16_t i2c_addr, const uint8_t len)
{
  uart_send_string ("Reading ");
  uart_send_integer (len, 0);
  uart_send_string (" bytes from 0x");
  uart_send_hexbyte(i2c_addr);
  uart_send_string("...");

  while (HAL_I2C_Master_Receive (&hi2c2, i2c_addr, buf, len, 1000) != HAL_OK) {
    if (HAL_I2C_GetError (&hi2c2) != HAL_I2C_ERROR_AF) {
      uart_send_string("Timeout\r\n");
      Error_Handler();
    }
  }

  uart_send_string("OK\r\n");
}

void request_data(uint8_t *buf, const uint16_t i2c_addr, const uint8_t offset, const uint8_t bytes)
{
  send_byte(i2c_addr, offset);
  read_bytes(buf, i2c_addr, bytes);
}

void print_time()
{
  request_data(buf, RTC_ADDR, TIME_OFFSET, TIME_BYTES);

  for (i = 0; i < TIME_BYTES; i++) {
    uart_send_hexbyte(buf[i]);
    uart_send_string(" ");
  }
}

void dump_sram()
{
  request_data(buf, RTC_ADDR, 0x0, SRAM_TOTAL_BYTES);

  uart_send_string("SRAM contents:\r\n");
  uart_send_hexdump(buf, 0, SRAM_TOTAL_BYTES);

  uart_send_string("\r\n");
}

void dump_eeprom()
{
  request_data(buf, EEPROM_ADDR, 0x0, EEPROM_TOTAL_BYTES);

  uart_send_string("EEPROM contents:\r\n");
  uart_send_hexdump(buf, 0, EEPROM_TOTAL_BYTES);
  uart_send_string("\r\n");

  request_data(buf, EEPROM_ADDR, EEPROM_EUI48_OFFSET, EEPROM_EUI48_BYTES);
  uart_send_string("EEPROM EUI-48:\r\n");
  uart_send_hexdump(buf, EEPROM_EUI48_OFFSET, EEPROM_EUI48_BYTES);

  uart_send_string("\r\n");
}

void enable_oscillator()
{
  uart_send_string("Enabling oscillator...\r\n");

  buf[0] = 0;		/* Offset of RTCSEC */
  buf[1] = 1 << 7;	/* datasheet REGISTERS 5-1, bit 7 = ST (start oscillator) */

  while (HAL_I2C_Master_Transmit (&hi2c2, RTC_ADDR_W, buf, 2, 1000) != HAL_OK) {
    if (HAL_I2C_GetError (&hi2c2) != HAL_I2C_ERROR_AF) {
      uart_send_string("Timeout\r\n");
      Error_Handler();
    }
  }

  uart_send_string("OK\r\n");
}


int
main()
{
  stm_init();
  uart_send_string("\r\n\r\n*** Init done\r\n");

  dump_sram();
  dump_eeprom();

  enable_oscillator();

  while (1)
  {
    memset(buf, 0, sizeof(buf));

    if (! device_ready(RTC_ADDR)) {
      goto fail;
    }

    print_time(buf);

    uart_send_string("\r\n\r\n");

    HAL_GPIO_TogglePin(LED_PORT, LED_GREEN);
    DELAY();
    continue;

  fail:
    HAL_GPIO_TogglePin(LED_PORT, LED_RED);
    DELAY();
  }
}
