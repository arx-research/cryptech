/*
 * Blink the four LEDs on the rev01 board in a pattern.
 */
#include "stm-init.h"
#include "stm-led.h"

#define DELAY() HAL_Delay(125)

/*
 * PK4 = ARM_LED1 - GREEN
 * PK5 = ARM_LED2 - RED
 * PK6 = ARM_LED3 - BLUE
 ' PK7 = ARM_LED4 - YELLOW
 */


void toggle_led(uint32_t times, uint32_t led_pin)
{
  uint32_t i;

  for (i = 0; i < times; i++) {
    HAL_GPIO_TogglePin(LED_PORT, led_pin);
    DELAY();
  }
}

int
main()
{
  stm_init();

  while (1)
  {
    toggle_led(2, LED_BLUE);
    toggle_led(2, LED_GREEN);
    toggle_led(2, LED_YELLOW);
    toggle_led(2, LED_RED);
  }

}
