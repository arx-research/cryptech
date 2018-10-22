/**
******************************************************************************
* @file    GPIO/GPIO_IOToggle/Src/stm32f4xx_it.c
* @author  MCD Application Team
* @version V1.0.1
* @date    26-February-2014
* @brief   Main Interrupt Service Routines.
*          This file provides template for all exceptions handler and
*          peripherals interrupt service routine.
******************************************************************************
* @attention
*
* <h2><center>&copy; COPYRIGHT(c) 2014 STMicroelectronics</center></h2>
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

#include "stm-init.h"
#include "stm-uart.h"

/******************************************************************************/
/*            Cortex-M4 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
 * @brief  This function handles Hard Fault exception.
 * @param  None
 * @retval None
 */
void HardFault_Handler(void)
{
/*
 * We define this to make debugging easier, because otherwise gdb reports
 * HardFault_Handler as WWDG_IRQHandler.
 */

#ifdef HAL_GPIO_MODULE_ENABLED
    //HAL_GPIO_WritePin(LED_PORT, LED_RED, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOK, GPIO_PIN_7, GPIO_PIN_SET);
#endif
    /* Go to infinite loop when Hard Fault exception occurs */
    while (1) { ; }
}

static void default_SysTick_hook(void) { };
static void (*SysTick_hook)(void) = default_SysTick_hook;
void set_SysTick_hook(void (*hook)(void))
{
    SysTick_hook = (hook == NULL) ? default_SysTick_hook : hook;
}

/**
 * @brief  This function handles SysTick Handler.
 * @param  None
 * @retval None
 */
void SysTick_Handler(void)
{
    HAL_IncTick();
    HAL_SYSTICK_IRQHandler();
}

/******************************************************************************/
/*                 STM32F4xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f4xx.s).                                               */
/******************************************************************************/

/**
* @brief This function handles DMA1 stream5 global interrupt.
*/
void DMA1_Stream5_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart_user_rx);
}

/**
* @brief This function handles DMA2 stream2 global interrupt.
*/
void DMA2_Stream2_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart_mgmt_rx);
}

/**
 * @brief  This function handles UART interrupt request.
 * @param  None
 * @retval None
 * @Note   HAL_UART_IRQHandler will call HAL_UART_RxCpltCallback in main.c.
 */
void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart_mgmt);
}

/**
 * @brief  This function handles UART interrupt request.
 * @param  None
 * @retval None
 * @Note   HAL_UART_IRQHandler will call HAL_UART_RxCpltCallback below.
 */
void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart_user);
}

/**
  * @brief  Rx Transfer completed callbacks.
  * @param  huart: pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    extern void HAL_UART1_RxCpltCallback(UART_HandleTypeDef *huart);
    extern void HAL_UART2_RxCpltCallback(UART_HandleTypeDef *huart);

    if (huart->Instance == USART1)
        HAL_UART1_RxCpltCallback(huart);

    else if (huart->Instance == USART2)
        HAL_UART2_RxCpltCallback(huart);
}

__weak void HAL_UART1_RxCpltCallback(UART_HandleTypeDef *huart)
{
  /* NOTE: This function Should not be modified, when the callback is needed,
           the HAL_UART1_RxCpltCallback could be implemented in the user file
   */
    huart = huart;
}

__weak void HAL_UART2_RxCpltCallback(UART_HandleTypeDef *huart)
{
  /* NOTE: This function Should not be modified, when the callback is needed,
           the HAL_UART2_RxCpltCallback could be implemented in the user file
   */
    huart = huart;
}

/**
  * @brief  Rx Half Transfer completed callbacks.
  * @param  huart: pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval None
  */
void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
    extern void HAL_UART1_RxHalfCpltCallback(UART_HandleTypeDef *huart);
    extern void HAL_UART2_RxHalfCpltCallback(UART_HandleTypeDef *huart);

    if (huart->Instance == USART1)
        HAL_UART1_RxHalfCpltCallback(huart);

    else if (huart->Instance == USART2)
        HAL_UART2_RxHalfCpltCallback(huart);
}

__weak void HAL_UART1_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
  /* NOTE: This function Should not be modified, when the callback is needed,
           the HAL_UART1_RxHalfCpltCallback could be implemented in the user file
   */
    huart = huart;
}

__weak void HAL_UART2_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
  /* NOTE: This function Should not be modified, when the callback is needed,
           the HAL_UART2_RxHalfCpltCallback could be implemented in the user file
   */
    huart = huart;
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    huart = huart;

    /* I dunno, just trap it for now */
    Error_Handler();
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
