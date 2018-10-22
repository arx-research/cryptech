//-----------------------------------------------------------------------------
// stm32-sdram.c
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Headers
//-----------------------------------------------------------------------------
#include "stm32f4xx_hal.h"
#include "stm32-sdram.h"


//-----------------------------------------------------------------------------
int sdram_init(SDRAM_HandleTypeDef *sdram1, SDRAM_HandleTypeDef *sdram2)
//-----------------------------------------------------------------------------
{
	HAL_StatusTypeDef ok;						// status
	FMC_SDRAM_CommandTypeDef cmd;		// command

	
		//
		// enable clocking
		//
	cmd.CommandMode = FMC_SDRAM_CMD_CLK_ENABLE;
	cmd.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1_2;
	cmd.AutoRefreshNumber = 1;
	cmd.ModeRegisterDefinition = 0;
	
	HAL_Delay(1);
	ok = HAL_SDRAM_SendCommand(sdram1, &cmd, 1);
	if (ok != HAL_OK) return 0;
	
		//
		// precharge all banks
		//
	cmd.CommandMode = FMC_SDRAM_CMD_PALL;
	cmd.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1_2;
	cmd.AutoRefreshNumber = 1;
	cmd.ModeRegisterDefinition = 0;
	
	HAL_Delay(1);
	ok = HAL_SDRAM_SendCommand(sdram1, &cmd, 1);
	if (ok != HAL_OK) return 0;
	
	
		//
		// send two auto-refresh commands in a row
		//
	cmd.CommandMode = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
	cmd.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1_2;
	cmd.AutoRefreshNumber = 1;
	cmd.ModeRegisterDefinition = 0;
		
	ok = HAL_SDRAM_SendCommand(sdram1, &cmd, 1);
	if (ok != HAL_OK) return 1;
		
	ok = HAL_SDRAM_SendCommand(sdram1, &cmd, 1);
	if (ok != HAL_OK) return 1;	
	
		
		//
		// load mode register
		//
	cmd.CommandMode = FMC_SDRAM_CMD_LOAD_MODE;
	cmd.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1_2;
	cmd.AutoRefreshNumber = 1;
	cmd.ModeRegisterDefinition = 	SDRAM_MODEREG_BURST_LENGTH_1					|
																SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL		|
																SDRAM_MODEREG_CAS_LATENCY_2						|
																SDRAM_MODEREG_OPERATING_MODE_STANDARD	|
																SDRAM_MODEREG_WRITEBURST_MODE_SINGLE	;

	ok = HAL_SDRAM_SendCommand(sdram1, &cmd, 1);
	if (ok != HAL_OK) return 1;


		//
		// set number of consequtive auto-refresh commands
		// and program refresh rate
		//

		//
		// RefreshRate = 64 ms / 8192 cyc = 7.8125 us/cyc
		//
		// RefreshCycles = 7.8125 us * 90 MHz = 703
		//
		// According to the formula on p.1665 of the reference manual,
		// we also need to subtract 20 from the value, so the target
		// refresh rate is 703 - 20 = 683.
		//
		
	ok = HAL_SDRAM_SetAutoRefreshNumber(sdram1, 8);
	if (ok != HAL_OK) return 1;

	HAL_SDRAM_ProgramRefreshRate(sdram1, 683);
	if (ok != HAL_OK) return 1;
	
	
		// done
	return 1;
}


//-----------------------------------------------------------------------------
// End-of-File
//-----------------------------------------------------------------------------
