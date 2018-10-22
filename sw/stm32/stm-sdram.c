/*
 * stm-sdram.c
 * -----------
 * Functions concerning the 2x512 Mbit SDRAM working memory.
 *
 * Copyright (c) 2016, NORDUnet A/S All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the NORDUnet nor the names of its contributors may
 *   be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "stm-init.h"
#include "stm-sdram.h"
#include "stm-fmc.h"
#include "stm-led.h"

/* Mode Register Bits */
#define SDRAM_MODEREG_BURST_LENGTH_1             ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_LENGTH_2             ((uint16_t)0x0001)
#define SDRAM_MODEREG_BURST_LENGTH_4             ((uint16_t)0x0002)
#define SDRAM_MODEREG_BURST_LENGTH_8             ((uint16_t)0x0004)

#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL      ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED     ((uint16_t)0x0008)

#define SDRAM_MODEREG_CAS_LATENCY_2              ((uint16_t)0x0020)
#define SDRAM_MODEREG_CAS_LATENCY_3              ((uint16_t)0x0030)

#define SDRAM_MODEREG_OPERATING_MODE_STANDARD    ((uint16_t)0x0000)

#define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE     ((uint16_t)0x0200)

static SDRAM_HandleTypeDef hsdram1;
static SDRAM_HandleTypeDef hsdram2;

static void _sdram_init_gpio(void);
static void _sdram_init_fmc(void);
static void _sdram_init_params(void);

void sdram_init(void)
{
    static int initialized = 0;
    if (initialized) return;
    initialized = 1;

    /* We rely on several things being set up by fmc_init() instead of duplicating all
     * that code here for independent FPGA/SDRAM FMC setup. This means the FPGA<->STM32
     * FMC bus can be used without the SDRAMs initialized, but the SDRAMs can't be
     * initialized withouth the FPGA<->STM32 FMC bus being set up too.
     */
    fmc_init();

    /* configure FMC */
    _sdram_init_gpio();
    _sdram_init_fmc();

    /* configure SDRAM registers */
    _sdram_init_params();
}

static void _sdram_init_gpio(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    /* The bulk of the FMC GPIO pins are set up in fmc_init_gpio().
     * This function just needs to enable the additional ones used
     * with the SDRAMs.
     */
    fmc_af_gpio(GPIOB, GPIO_PIN_5 | GPIO_PIN_6);
    fmc_af_gpio(GPIOC, GPIO_PIN_0 | GPIO_PIN_2 | GPIO_PIN_3);
    fmc_af_gpio(GPIOE, GPIO_PIN_0 | GPIO_PIN_1);
    fmc_af_gpio(GPIOF, GPIO_PIN_11);
    fmc_af_gpio(GPIOG, GPIO_PIN_8 | GPIO_PIN_15);
    fmc_af_gpio(GPIOI, GPIO_PIN_4 | GPIO_PIN_5);
}

static void _sdram_config_bank(SDRAM_HandleTypeDef *hsdram, uint32_t SDBank, FMC_SDRAM_TimingTypeDef *SdramTiming)
{
    /* memory type */
    hsdram->Instance = FMC_SDRAM_DEVICE;

    /* bank */
    hsdram->Init.SDBank = SDBank;

    /* settings for IS42S32160F */
    hsdram->Init.ColumnBitsNumber	= FMC_SDRAM_COLUMN_BITS_NUM_9;
    hsdram->Init.RowBitsNumber		= FMC_SDRAM_ROW_BITS_NUM_13;
    hsdram->Init.MemoryDataWidth	= FMC_SDRAM_MEM_BUS_WIDTH_32;
    hsdram->Init.InternalBankNumber	= FMC_SDRAM_INTERN_BANKS_NUM_4;
    hsdram->Init.CASLatency		= FMC_SDRAM_CAS_LATENCY_2;

    /* write protection not needed */
    hsdram->Init.WriteProtection = FMC_SDRAM_WRITE_PROTECTION_DISABLE;

    /* memory clock is 90 MHz (HCLK / 2) */
    hsdram->Init.SDClockPeriod = FMC_SDRAM_CLOCK_PERIOD_2;

    /* read burst not needed */
    hsdram->Init.ReadBurst = FMC_SDRAM_RBURST_DISABLE;

    /* additional pipeline stages not neeed */
    hsdram->Init.ReadPipeDelay = FMC_SDRAM_RPIPE_DELAY_0;

    /* call HAL layer */
    HAL_SDRAM_Init(hsdram, SdramTiming);
}

static void _sdram_init_fmc(void)
{
    FMC_SDRAM_TimingTypeDef SdramTiming;

    /*
     * following settings are for -75E speed grade memory chip
     * clocked at only 90 MHz instead of the rated 133 MHz
     *
     * ExitSelfRefreshDelay: 67 ns @ 90 MHz is 6.03 cycles, so in theory
     *                       6 can be used here, but let's be on the safe side
     *
     * WriteRecoveryTime:    must be >= tRAS - tRCD (5 - 2 = 3 cycles),
     *                       and >= tRC - tRCD - tRP (8 - 2 - 2 = 4 cycles)
     */
    SdramTiming.LoadToActiveDelay	= 2;	// tMRD
    SdramTiming.ExitSelfRefreshDelay	= 7;    // (see above)
    SdramTiming.SelfRefreshTime		= 5;	// should be >= tRAS (5 cycles)
    SdramTiming.RowCycleDelay		= 8;	// tRC
    SdramTiming.WriteRecoveryTime	= 4;	// (see above)
    SdramTiming.RPDelay			= 2;	// tRP
    SdramTiming.RCDDelay		= 2;	// tRCD

    /* configure the first bank */
    _sdram_config_bank(&hsdram1, FMC_SDRAM_BANK1, &SdramTiming);

    /* configure the second bank */
    _sdram_config_bank(&hsdram2, FMC_SDRAM_BANK2, &SdramTiming);
}

static void _sdram_init_params(void)
{
    FMC_SDRAM_CommandTypeDef cmd;

    /* enable clocking */
    cmd.CommandMode = FMC_SDRAM_CMD_CLK_ENABLE;
    cmd.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1_2;
    cmd.AutoRefreshNumber = 1;
    cmd.ModeRegisterDefinition = 0;
    HAL_Delay(1);
    HAL_SDRAM_SendCommand(&hsdram1, &cmd, 1);

    /* precharge all banks */
    cmd.CommandMode = FMC_SDRAM_CMD_PALL;
    cmd.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1_2;
    cmd.AutoRefreshNumber = 1;
    cmd.ModeRegisterDefinition = 0;
    HAL_Delay(1);
    HAL_SDRAM_SendCommand(&hsdram1, &cmd, 1);

    /* send two auto-refresh commands in a row */
    cmd.CommandMode = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
    cmd.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1_2;
    cmd.AutoRefreshNumber = 1;
    cmd.ModeRegisterDefinition = 0;
    HAL_SDRAM_SendCommand(&hsdram1, &cmd, 1);
    HAL_SDRAM_SendCommand(&hsdram1, &cmd, 1);

    /* load mode register */
    cmd.CommandMode = FMC_SDRAM_CMD_LOAD_MODE;
    cmd.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1_2;
    cmd.AutoRefreshNumber = 1;
    cmd.ModeRegisterDefinition =
	SDRAM_MODEREG_BURST_LENGTH_1		|
	SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL	|
	SDRAM_MODEREG_CAS_LATENCY_2		|
	SDRAM_MODEREG_OPERATING_MODE_STANDARD	|
	SDRAM_MODEREG_WRITEBURST_MODE_SINGLE	;
    HAL_SDRAM_SendCommand(&hsdram1, &cmd, 1);

    /*
     * set number of consequtive auto-refresh commands
     * and program refresh rate
     *
     * RefreshRate = 64 ms / 8192 cyc = 7.8125 us/cyc
     *
     * RefreshCycles = 7.8125 us * 90 MHz = 703
     *
     * According to the formula on p.1665 of the reference manual,
     * we also need to subtract 20 from the value, so the target
     * refresh rate is 703 - 20 = 683.
     */

    HAL_SDRAM_SetAutoRefreshNumber(&hsdram1, 8);

    HAL_SDRAM_ProgramRefreshRate(&hsdram1, 683);
}
