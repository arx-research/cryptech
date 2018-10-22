/**
  ******************************************************************************
  * File Name          : main.c
  * Description        : Main program body
  ******************************************************************************
  *
  * COPYRIGHT(c) 2016 STMicroelectronics
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


//-----------------------------------------------------------------------------
// Headers
//-----------------------------------------------------------------------------
#include "stm32f4xx_hal.h"
#include "gpio.h"
#include "fmc.h"
#include "stm32-sdram.h"


//-----------------------------------------------------------------------------
// Prototypes
//-----------------------------------------------------------------------------
void SystemClock_Config(void);

int	test_sdram_sequential(uint32_t *base_addr);
int test_sdram_random(uint32_t *base_addr);
int test_sdrams_interleaved(uint32_t *base_addr1, uint32_t *base_addr2);

uint32_t lfsr_next_32(uint32_t lfsr);
uint32_t lfsr_next_24(uint32_t lfsr);


//-----------------------------------------------------------------------------
// Locals
//-----------------------------------------------------------------------------
static uint32_t lfsr1;
static uint32_t lfsr2;


//-----------------------------------------------------------------------------
int main(void)
//-----------------------------------------------------------------------------
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_FMC_Init();

		// run external memory initialization sequence
	int ok = sdram_init(&hsdram1, &hsdram2);

		// turn on green LED in case of success,
		// and keep it on as long as we're happy
	if (ok) HAL_GPIO_WritePin(ARM_LED_GPIO_Port, ARM_LED_GREEN_Pin, GPIO_PIN_SET);
	
		// set LFSRs to some initial value, LFSRs will produce
		// pseudo-random 32-bit patterns to test our memories
	lfsr1 = 0xCCAA5533;
	lfsr2 = 0xCCAA5533;
	
		// continuosly test both chips
	while (ok)
	{
			// turn blue led off (testing chips separately)
		HAL_GPIO_WritePin(ARM_LED_GPIO_Port, ARM_LED_BLUE_Pin, GPIO_PIN_RESET);
		
			// run sequential write-then-read test for the first chip
		ok = test_sdram_sequential(SDRAM_BASEADDR_CHIP1);
		if (!ok) break;
	
			// run random write-then-read test for the first chip
		ok = test_sdram_random(SDRAM_BASEADDR_CHIP1);
		if (!ok) break;
		
			// run sequential write-then-read test for the second chip
		ok = test_sdram_sequential(SDRAM_BASEADDR_CHIP2);
		if (!ok) break;
	
			// run random write-then-read test for the second chip
		ok = test_sdram_random(SDRAM_BASEADDR_CHIP2);
		if (!ok) break;
		
			// turn blue led on (testing two chips at the same time)
		HAL_GPIO_WritePin(ARM_LED_GPIO_Port, ARM_LED_BLUE_Pin, GPIO_PIN_SET);
		
			// run interleaved write-then-read test for both chips at once
		ok = test_sdrams_interleaved(SDRAM_BASEADDR_CHIP1, SDRAM_BASEADDR_CHIP2);
		if (!ok) break;

	}
	
		/* should only get here in case of failure */

		//
		// turn all leds off
		//
	HAL_GPIO_WritePin(ARM_LED_GPIO_Port, ARM_LED_BLUE_Pin,   GPIO_PIN_RESET);
	HAL_GPIO_WritePin(ARM_LED_GPIO_Port, ARM_LED_GREEN_Pin,  GPIO_PIN_RESET);
	HAL_GPIO_WritePin(ARM_LED_GPIO_Port, ARM_LED_YELLOW_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(ARM_LED_GPIO_Port, ARM_LED_RED_Pin,    GPIO_PIN_RESET);
		
		//
		// repeatedly flash the red led to indicate failure
		//
	for (;;)
	{
		HAL_GPIO_WritePin(ARM_LED_GPIO_Port, ARM_LED_RED_Pin, GPIO_PIN_RESET);
		HAL_Delay(100);
		
		HAL_GPIO_WritePin(ARM_LED_GPIO_Port, ARM_LED_RED_Pin, GPIO_PIN_SET);		
		HAL_Delay(100);
	}

}


//-----------------------------------------------------------------------------
int	test_sdram_sequential(uint32_t *base_addr)
//-----------------------------------------------------------------------------
{
		// memory offset
	int offset;

		// readback value
	uint32_t sdram_readback;
	
	
		/* This test fills entire memory chip with some pseudo-random pattern
	     starting from the very first cell and going in linear fashion. It then
	     reads entire memory and compares read values with what was written. */
	
	
		// turn on yellow led to indicate, that we're writing
	HAL_GPIO_WritePin(ARM_LED_GPIO_Port, ARM_LED_YELLOW_Pin, GPIO_PIN_SET);
	
	
		//
		// Note, that SDRAM_SIZE is in BYTES, and since we write using
		// 32-bit words, total number of words is SDRAM_SIZE / 4.
		//
	
		// fill entire memory with "random" values
	for (offset=0; offset<(SDRAM_SIZE >> 2); offset++)
	{
			// generate next "random" value to write
		lfsr1 = lfsr_next_32(lfsr1);
		
			// write to memory
		base_addr[offset] = lfsr1;
	}
	
	
		// turn off yellow led to indicate, that we're going to read
	HAL_GPIO_WritePin(ARM_LED_GPIO_Port, ARM_LED_YELLOW_Pin, GPIO_PIN_RESET);
		
	
		// read entire memory and compare values
	for (offset=0; offset<(SDRAM_SIZE >> 2); offset++)
	{
			// generate next "random" value (we use the second LFSR to catch up)
		lfsr2 = lfsr_next_32(lfsr2);
		
			// read from memory
		sdram_readback = base_addr[offset];
		
			// compare and abort test in case of mismatch
		if (sdram_readback != lfsr2) return 0;
	}
		
		// done
	return 1;
}


//-----------------------------------------------------------------------------
int	test_sdram_random(uint32_t *base_addr)
//-----------------------------------------------------------------------------
{
		// cell counter, memory offset
	int counter, offset;

		// readback value
	uint32_t sdram_readback;
	
	
	/* This test fills entire memory chip with some pseudo-random pattern
		 starting from the very first cell, but then jumping around in pseudo-
	   random fashion to make sure, that SDRAM controller in STM32 handles
	   bank, row and column switching correctly. It then reads entire memory
	   and compares read values with what was written. */
	
	
		// turn on yellow led to indicate, that we're writing
	HAL_GPIO_WritePin(ARM_LED_GPIO_Port, ARM_LED_YELLOW_Pin, GPIO_PIN_SET);
	
	
		//
		// Note, that SDRAM_SIZE is in BYTES, and since we write using
		// 32-bit words, total number of words is SDRAM_SIZE / 4.
		//
	
		// start with the first cell
	for (counter=0, offset=0; counter<(SDRAM_SIZE >> 2); counter++)
	{
			// generate next "random" value to write
		lfsr1 = lfsr_next_32(lfsr1);

			// write to memory
		base_addr[offset] = lfsr1;
		
			// generate next "random" address
		
			//
			// Note, that for 64 MB memory with 32-bit data bus we need 24 bits
			// of address, so we use 24-bit LFSR here. Since LFSR has only 2^^24-1
			// states, i.e. all possible 24-bit values excluding 0, we have to
			// manually kick it into some arbitrary state during the first iteration.
			//
		
		offset = offset ? lfsr_next_24(offset) : 0x00DEC0DE;
	}
		
	
		// turn off yellow led to indicate, that we're going to read
	HAL_GPIO_WritePin(ARM_LED_GPIO_Port, ARM_LED_YELLOW_Pin, GPIO_PIN_RESET);
	
	
		// read entire memory and compare values
	for (counter=0, offset=0; counter<(SDRAM_SIZE >> 2); counter++)
	{
			// generate next "random" value (we use the second LFSR to catch up)
		lfsr2 = lfsr_next_32(lfsr2);
		
			// read from memory
		sdram_readback = base_addr[offset];
		
			// compare and abort test in case of mismatch
		if (sdram_readback != lfsr2) return 0;
		
			// generate next "random" address
		offset = offset ? lfsr_next_24(offset) : 0x00DEC0DE;
	}
	
		//
		// we should have walked exactly 2**24 iterations and returned
		// back to the arbitrary starting address...
		//
		
	if (offset != 0x00DEC0DE) return 0;
	
		
			// done
		return 1;
}


//-----------------------------------------------------------------------------
int test_sdrams_interleaved(uint32_t *base_addr1, uint32_t *base_addr2)
//-----------------------------------------------------------------------------
{
	// cell counter, memory offsets
	int counter, offset1, offset2;

		// readback value
	uint32_t sdram_readback;
	
	
	/* Basically this is the same as test_sdram_random() except that it
		 tests both memory chips at the same time. */
	
	
		// turn on yellow led to indicate, that we're writing
	HAL_GPIO_WritePin(ARM_LED_GPIO_Port, ARM_LED_YELLOW_Pin, GPIO_PIN_SET);
	
	
		//
		// Note, that SDRAM_SIZE is in BYTES, and since we write using
		// 32-bit words, total number of words is SDRAM_SIZE / 4.
		//
	
		// start with the first cell
	for (counter=0, offset1=0, offset2=0; counter<(SDRAM_SIZE >> 2); counter++)
	{
			// generate next "random" value to write
		lfsr1 = lfsr_next_32(lfsr1);

			// write to memory
		base_addr1[offset1] = lfsr1;
		base_addr2[offset2] = lfsr1;
		
			// generate next "random" addresses (use different starting states!)
		
		offset1 = offset1 ? lfsr_next_24(offset1) : 0x00ABCDEF;
		offset2 = offset2 ? lfsr_next_24(offset2) : 0x00FEDCBA;
	}
		
	
		// turn off yellow led to indicate, that we're going to read
	HAL_GPIO_WritePin(ARM_LED_GPIO_Port, ARM_LED_YELLOW_Pin, GPIO_PIN_RESET);
	
	
		// read entire memory and compare values
	for (counter=0, offset1=0, offset2=0; counter<(SDRAM_SIZE >> 2); counter++)
	{
			// generate next "random" value (we use the second LFSR to catch up)
		lfsr2 = lfsr_next_32(lfsr2);
		
			// read from the first memory and compare
		sdram_readback = base_addr1[offset1];
		if (sdram_readback != lfsr2) return 0;
		
			// read from the second memory and compare
		sdram_readback = base_addr2[offset2];
		if (sdram_readback != lfsr2) return 0;
		
			// generate next "random" addresses
		offset1 = offset1 ? lfsr_next_24(offset1) : 0x00ABCDEF;
		offset2 = offset2 ? lfsr_next_24(offset2) : 0x00FEDCBA;
	}
	
		//
		// we should have walked exactly 2**24 iterations and returned
		// back to the arbitrary starting address...
		//
		
	if (offset1 != 0x00ABCDEF) return 0;
	if (offset2 != 0x00FEDCBA) return 0;
	
		
			// done
		return 1;
}


//-----------------------------------------------------------------------------
void SystemClock_Config(void)
//-----------------------------------------------------------------------------
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

  __HAL_RCC_PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 360;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  HAL_PWREx_EnableOverDrive();

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);

  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}



//-----------------------------------------------------------------------------
uint32_t lfsr_next_32(uint32_t lfsr)
//-----------------------------------------------------------------------------
{
	uint32_t tap = 0;

	tap ^= (lfsr >> 31);
	tap ^= (lfsr >> 30);
	tap ^= (lfsr >> 29);
	tap ^= (lfsr >> 9);

	return (lfsr << 1) | (tap & 1);
}


//-----------------------------------------------------------------------------
uint32_t lfsr_next_24(uint32_t lfsr)
//-----------------------------------------------------------------------------
{
	unsigned int tap = 0;

	tap ^= (lfsr >> 23);
	tap ^= (lfsr >> 22);
	tap ^= (lfsr >> 21);
	tap ^= (lfsr >> 16);

	return ((lfsr << 1) | (tap & 1)) & 0x00FFFFFF;
}


//-----------------------------------------------------------------------------
#ifdef USE_FULL_ASSERT
//-----------------------------------------------------------------------------
void assert_failed(uint8_t* file, uint32_t line)
//-----------------------------------------------------------------------------
{
}
//-----------------------------------------------------------------------------
#endif
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// End-of-File
//-----------------------------------------------------------------------------
