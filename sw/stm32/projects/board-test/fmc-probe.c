/* Read all registers from the FPGA. In some cases, this will be garbage;
 * in other cases, it will be the core name and version strings.
 */

#include "stm-init.h"
#include "stm-led.h"
#include "stm-fmc.h"
#include "stm-uart.h"

#define CORE_SIZE               (0x100)
#define SEGMENT_SIZE            (0x40 * CORE_SIZE)
#define SEGMENT_OFFSET_GLOBALS  (0 * SEGMENT_SIZE)
#define SEGMENT_OFFSET_HASHES   (1 * SEGMENT_SIZE)
#define BOARD_ADDR_BASE         (SEGMENT_OFFSET_GLOBALS + (0 * CORE_SIZE))
#define COMM_ADDR_BASE          (SEGMENT_OFFSET_GLOBALS + (1 * CORE_SIZE))
#define SHA1_ADDR_BASE          (SEGMENT_OFFSET_HASHES + (0 * CORE_SIZE))
#define SHA256_ADDR_BASE        (SEGMENT_OFFSET_HASHES + (1 * CORE_SIZE))
#define SHA512_ADDR_BASE        (SEGMENT_OFFSET_HASHES + (2 * CORE_SIZE))

static uint32_t read0(uint32_t addr)
{
    uint32_t data;

    if (fmc_read_32(addr, &data) != 0) {
	uart_send_string("fmc_read_32 failed\r\n");
	Error_Handler();
    }

    return data;
}

int main(void)
{
  stm_init();
  led_on(LED_GREEN);

  for (uint32_t addr = 0; addr < 0x00080000; addr += 4) {
      uint32_t data = read0(addr);
      if (data != 0) {
	  uart_send_hex(addr, 8);
	  uart_send_string(": ");
	  uart_send_hex(data, 8);
    uart_send_char(' ');
    for (int i = 24; i >= 0; i -= 8) {
	uint8_t c = (data >> i) & 0xff;
	if (c < 0x20 || c > 0x7e)
	    uart_send_char('.');
	else
	    uart_send_char(c);
    }
	  uart_send_string("\r\n");
      }
  }
  uart_send_string("Done.\r\n");

  return 0;
}
