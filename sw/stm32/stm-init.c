/**
  ******************************************************************************
  * File Name          : main.c
  * Date               : 08/07/2015 17:46:00
  * Description        : Main program body
  ******************************************************************************
  *
  * COPYRIGHT(c) 2015 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm-init.h"
#ifdef HAL_GPIO_MODULE_ENABLED
#include "stm-led.h"
#endif
#ifdef HAL_UART_MODULE_ENABLED
#include "stm-uart.h"
#endif
#ifdef HAL_I2C_MODULE_ENABLED
#include "stm-rtc.h"
#endif
#ifdef HAL_SPI_MODULE_ENABLED
#include "stm-fpgacfg.h"
#include "stm-keystore.h"
#endif
#ifdef HAL_SRAM_MODULE_ENABLED
#include "stm-fmc.h"
#endif
#ifdef HAL_SDRAM_MODULE_ENABLED
#include "stm-sdram.h"
#endif

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
#ifdef HAL_GPIO_MODULE_ENABLED
static void MX_GPIO_Init(void);
#endif

void stm_init(void)
{

  /* MCU Configuration----------------------------------------------------------*/

  /* System interrupt init*/
  /* Sets the priority grouping field */
  HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_0);

  /* Initialize all configured peripherals */
#ifdef HAL_GPIO_MODULE_ENABLED
  MX_GPIO_Init();
#endif
#ifdef HAL_UART_MODULE_ENABLED
  uart_init();
#endif
#ifdef HAL_I2C_MODULE_ENABLED
  rtc_init();
#endif
#ifdef HAL_SPI_MODULE_ENABLED
  fpgacfg_init();
  keystore_init();
#endif
#ifdef TARGET_CRYPTECH_DEV_BRIDGE
  // Blink blue LED for six seconds to not upset the Novena at boot.
  led_on(LED_BLUE);
  for (int i = 0; i < 12; i++) {
    HAL_Delay(500);
    led_toggle(LED_BLUE);
  }
  led_off(LED_BLUE);
#endif
#ifdef HAL_SRAM_MODULE_ENABLED
  fmc_init();
#endif
#ifdef HAL_SDRAM_MODULE_ENABLED
  sdram_init();
#endif
}


#ifdef HAL_GPIO_MODULE_ENABLED

/* Configure General Purpose Input/Output pins */
static void MX_GPIO_Init(void)
{
  /* GPIO Ports Clock Enable */
  LED_CLK_ENABLE();

  /* Configure LED GPIO pins */
  gpio_output(LED_PORT, LED_RED | LED_YELLOW | LED_GREEN | LED_BLUE, GPIO_PIN_RESET);
}
#endif

/**
 * @brief  This function is executed in case of error occurrence.
 * @param  None
 * @retval None
 */
void Error_Handler(void)
{
#ifdef HAL_GPIO_MODULE_ENABLED
  HAL_GPIO_WritePin(LED_PORT, LED_RED, GPIO_PIN_SET);
#endif

  while (1) { ; }
}

/**
  * @}
  */

/**
  * @}
*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
