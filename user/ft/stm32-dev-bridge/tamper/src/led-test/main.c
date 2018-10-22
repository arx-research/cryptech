#include <inttypes.h>
#include <avr/io.h>
//#include <util/delay.h>

#include "iotn828.h"

#define AVR_LED4 PORTA4  /* RED */
#define AVR_LED3 5  /* YELLOW */
#define AVR_LED2 6  /* GREEN */
#define AVR_LED1 7  /* BLUE */


void main() {
  int i;
  uint8_t j = 0;

  /* Enable PA4 output */
  //  DDRA |= (1 << AVR_LED4) | (1 << AVR_LED3) | (1 << AVR_LED2) | (1 << AVR_LED1);
  DDRA |= 0xf0;

  //PORTA = 0;
  while (1) {
    PORTA = (PORTA & 0x0f) | ((j++<<4) & 0xf0);
    //PORTA ^= (1 << AVR_LED1);
    //PORTA = j++;
    for (i = 0; i < 10000; i++) { ; }
    //_delay_ms (100);
  }
}
