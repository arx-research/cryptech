#ifndef __STM_INIT_H
#define __STM_INIT_H

#include "stm32f4xx_hal.h"

#define LED_PORT	GPIOJ
#define LED_RED		GPIO_PIN_1
#define LED_YELLOW	GPIO_PIN_2
#define LED_GREEN	GPIO_PIN_3
#define LED_BLUE	GPIO_PIN_4

extern UART_HandleTypeDef huart2;
extern I2C_HandleTypeDef hi2c2;
extern void stm_init(void);
extern void Error_Handler(void);

#endif /* __STM_INIT_H */
