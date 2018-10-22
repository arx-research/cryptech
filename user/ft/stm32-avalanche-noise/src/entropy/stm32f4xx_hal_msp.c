#include "stm_init.h"
#include "stm32f4xx_hal.h"

static void Error_Handler(void);
extern void HAL_DMA_XferCpltCallback(DMA_HandleTypeDef *hdma);

void HAL_TIM_IC_MspInit(TIM_HandleTypeDef* htim)
{
  if (htim->Instance == TIM2) {
    /* Configure DMA input capturing.
     *
     * Amplified avalanche noise is present at pin PA1,
     * which has Alternate Function TIM2_CH2.
     *
     * TIM2_CH2 is DMA1 stream 6, channel 3 according to Table 28
     * (DMA1 request mapping) in the reference manual (RM0368).
     */
    __DMA1_CLK_ENABLE();
    __TIM2_CLK_ENABLE();
    __GPIOA_CLK_ENABLE();

    hdma_tim.Instance = DMA1_Stream6;

    hdma_tim.Init.Channel             = DMA_CHANNEL_3;
    hdma_tim.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_tim.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_tim.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_tim.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_tim.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
    hdma_tim.Init.Mode                = DMA_NORMAL;
    hdma_tim.Init.Priority            = DMA_PRIORITY_HIGH;
    hdma_tim.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    hdma_tim.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    hdma_tim.Init.MemBurst            = DMA_MBURST_SINGLE;
    hdma_tim.Init.PeriphBurst         = DMA_PBURST_SINGLE;

    /* Link hdma_tim to hdma[2] (channel3) */
    __HAL_LINKDMA(htim, hdma[TIM_DMA_ID_CC2], hdma_tim);

    /* Initialize TIMx DMA handle */
    if (HAL_DMA_Init(&hdma_tim) != HAL_OK) {
      Error_Handler();
    }
  }
}

void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  if (huart->Instance == USART1) {
    /* Peripheral clock enable */
    __USART1_CLK_ENABLE();

    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10    ------> USART1_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* NVIC for interrupt mode */
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 1);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  } else if (huart->Instance == USART2) {
    /* Peripheral clock enable */
    __USART2_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* NVIC for interrupt mode */
    HAL_NVIC_SetPriority(USART2_IRQn, 0, 1);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* huart)
{
  if (huart->Instance == USART1) {
    /* Peripheral clock disable */
    __USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10    ------> USART1_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9 | GPIO_PIN_10);
  } else if (huart->Instance == USART2) {
    __USART2_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2 | GPIO_PIN_3);
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
  /* Turn on RED LED and then loop indefinitely */
  HAL_GPIO_WritePin(LED_PORT, LED_RED, GPIO_PIN_SET);

  while (1) { ; }
}
