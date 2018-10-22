/**
******************************************************************************
* File Name          : stm32f4xx_hal_msp.c
* Description        : This file provides code for the MSP Initialization
*                      and de-Initialization codes.
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
#include "stm32f4xx_hal.h"


/**
 * Initializes the Global MSP.
 */
void HAL_MspInit(void)
{
  /* USER CODE BEGIN MspInit 0 */

  /* USER CODE END MspInit 0 */

  /* USER CODE BEGIN MspInit 1 */

  /* USER CODE END MspInit 1 */
}

void HAL_RNG_MspInit(RNG_HandleTypeDef* hrng)
{

  if(hrng->Instance==RNG)
    {
      /* USER CODE BEGIN RNG_MspInit 0 */

      /* USER CODE END RNG_MspInit 0 */
      /* Peripheral clock enable */
      __RNG_CLK_ENABLE();
      /* USER CODE BEGIN RNG_MspInit 1 */

      /* USER CODE END RNG_MspInit 1 */
    }

}

void HAL_RNG_MspDeInit(RNG_HandleTypeDef* hrng)
{

  if(hrng->Instance==RNG)
    {
      /* USER CODE BEGIN RNG_MspDeInit 0 */

      /* USER CODE END RNG_MspDeInit 0 */
      /* Peripheral clock disable */
      __RNG_CLK_DISABLE();
    }
  /* USER CODE BEGIN RNG_MspDeInit 1 */

  /* USER CODE END RNG_MspDeInit 1 */

}



void HAL_SRAM_MspInit(SRAM_HandleTypeDef* hsram)
{
  hsram = hsram;
}


void HAL_SRAM_MspDeInit(SRAM_HandleTypeDef* hsram)
{
  hsram = hsram;
}

void HAL_SDRAM_MspInit(SDRAM_HandleTypeDef* hsdram)
{
  hsdram = hsdram;
}

void HAL_SDRAM_MspDeInit(SDRAM_HandleTypeDef* hsdram)
{
  hsdram = hsdram;
}

void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
  GPIO_InitTypeDef GPIO_InitStruct;
  IRQn_Type IRQn;
  DMA_Stream_TypeDef *hdma_instance;

  if (huart->Instance == USART1) {
    /* This is huart_mgmt (MGMT UART) */

    /* Peripheral clock enable */
    __USART1_CLK_ENABLE();
    __GPIOA_CLK_ENABLE();

    /**USART1 GPIO Configuration
       PA9     ------> USART1_TX
       PA10    ------> USART1_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;

    IRQn = USART1_IRQn;

    /* Peripheral DMA init*/
    hdma_instance = DMA2_Stream2;
  }

  else if (huart->Instance == USART2) {
    /* This is huart_user (USER UART) */

    /* Peripheral clock enable */
    __USART2_CLK_ENABLE();
    __GPIOA_CLK_ENABLE();

    /**USART2 GPIO Configuration
       PA2     ------> USART2_TX
       PA3     ------> USART2_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;

    IRQn = USART2_IRQn;

    /* Peripheral DMA init*/
    hdma_instance = DMA1_Stream5;
  }

  else {
    return;
  }

  /* common setup */
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  HAL_NVIC_SetPriority(IRQn, 0, 1);
  HAL_NVIC_EnableIRQ(IRQn);

  /* Peripheral DMA init*/
  DMA_HandleTypeDef *hdma = huart->hdmarx;
  if (hdma != NULL) {
    hdma->Instance = hdma_instance;
    hdma->Init.Channel = DMA_CHANNEL_4;
    hdma->Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma->Init.PeriphInc = DMA_PINC_DISABLE;
    hdma->Init.MemInc = DMA_MINC_ENABLE;
    hdma->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma->Init.Mode = DMA_CIRCULAR;
    hdma->Init.Priority = DMA_PRIORITY_HIGH;
    hdma->Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    /*
      hdma->Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL;
      hdma->Init.MemBurst = DMA_MBURST_SINGLE;
      hdma->Init.PeriphBurst = DMA_PBURST_SINGLE;
    */
    if (HAL_DMA_Init(hdma) != HAL_OK) {
      extern void mbed_die(void);
      mbed_die();
    }
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* huart)
{
  if (huart->Instance == USART1) {
    /* Peripheral clock disable */
    __USART1_CLK_DISABLE();

    /**USART2 GPIO Configuration
       PA2     ------> USART2_TX
       PA3     ------> USART2_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2 | GPIO_PIN_3);

    HAL_NVIC_DisableIRQ(USART1_IRQn);

    /* Peripheral DMA DeInit*/
    HAL_DMA_DeInit(huart->hdmarx);
  } else if (huart->Instance == USART2) {
    /* Peripheral clock disable */
    __USART2_CLK_DISABLE();

    /**USART2 GPIO Configuration
       PA2     ------> USART2_TX
       PA3     ------> USART2_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9 | GPIO_PIN_10);

    HAL_NVIC_DisableIRQ(USART2_IRQn);

    /* Peripheral DMA DeInit*/
    HAL_DMA_DeInit(huart->hdmarx);
  }
}

void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    if (hi2c->Instance == I2C2) {
	/*
	 * External RTC chip.
	 *
	 * I2C2 GPIO Configuration
	 * PH5     ------> I2C2_SDA
	 * PH4     ------> I2C2_SCL
	*/
	__GPIOH_CLK_ENABLE();

	GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_5;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF4_I2C2;
	HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

	/* Peripheral clock enable */
	__I2C2_CLK_ENABLE();
    }
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef* hi2c)
{
    if (hi2c->Instance == I2C2) {
	__I2C2_CLK_DISABLE();
	HAL_GPIO_DeInit(GPIOH, GPIO_PIN_4 | GPIO_PIN_5);
    }
}

void HAL_SPI_MspInit(SPI_HandleTypeDef* hspi)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    if (hspi->Instance == SPI1) {
	/* SPI1 is the keystore memory.
	 *
	 * SPI1 GPIO Configuration
	 * PA5     ------> SPI2_SCK
	 * PA6     ------> SPI2_MISO
	 * PA7     ------> SPI2_MOSI
	*/
	__GPIOA_CLK_ENABLE();
	GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/* Peripheral clock enable */
	__SPI1_CLK_ENABLE();
    } else if (hspi->Instance == SPI2) {
	/* SPI2 is the FPGA config memory.
	 *
	 * SPI2 GPIO Configuration
	 * PB13     ------> SPI2_SCK
	 * PB14     ------> SPI2_MISO
	 * PB15     ------> SPI2_MOSI
	*/
	__GPIOB_CLK_ENABLE();
	GPIO_InitStruct.Pin = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/* Peripheral clock enable */
	__SPI2_CLK_ENABLE();
    }
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef* hspi)
{
    if (hspi->Instance == SPI1) {
	/* Peripheral clock disable */
	__HAL_RCC_SPI1_CLK_DISABLE();
	HAL_GPIO_DeInit(GPIOB, GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
    } else if (hspi->Instance == SPI2) {
	/* Peripheral clock disable */
	__HAL_RCC_SPI2_CLK_DISABLE();
	HAL_GPIO_DeInit(GPIOB, GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);
    }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
