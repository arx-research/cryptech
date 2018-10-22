/*
 * Test code that just sends the letters 'a' to 'z' over and
 * over again to both the USER and MGMT UARTs. If a CR is received,
 * it will toggle upper/lower case of the letters being sent.
 *
 * Toggles the BLUE LED slowly and the GREEN LED for every
 * character sent.
 */
#include "stm-init.h"
#include "stm-led.h"
#include "stm-uart.h"

#define DELAY() HAL_Delay(100)

int
main()
{
    char crlf[] = "\r\n";
    uint8_t tx = 'A';
    uint8_t rx = 0;
    uint8_t upper = 0;

    stm_init();

    while (1) {
	led_toggle(LED_GREEN);

	uart_send_char2(STM_UART_USER, tx + upper);
	uart_send_char2(STM_UART_MGMT, tx + upper);
	DELAY();

	if (uart_recv_char2(STM_UART_USER, &rx, 0) == HAL_OK ||
	    uart_recv_char2(STM_UART_MGMT, &rx, 0) == HAL_OK) {
	    led_toggle(LED_YELLOW);
	    if (rx == '\r') {
		upper = upper == 0 ? ('a' - 'A'):0;
	    }
	}

	if (tx++ == 'Z') {
	    /* linefeed after each alphabet */
	    uart_send_string2(STM_UART_USER, crlf);
	    uart_send_string2(STM_UART_MGMT, crlf);
	    tx = 'A';
	    led_toggle(LED_BLUE);
	}
    }
}
