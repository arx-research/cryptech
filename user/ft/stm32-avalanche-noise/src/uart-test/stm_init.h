#ifndef __STM_INIT_H
#define __STM_INIT_H

#include "stm32f4xx_hal.h"

#define LED_PORT	GPIOB
#define LED_RED		GPIO_PIN_12
#define LED_YELLOW	GPIO_PIN_13
#define LED_GREEN	GPIO_PIN_14
#define LED_BLUE	GPIO_PIN_15

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern void stm_init(void);


#endif /* __STM_INIT_H */
