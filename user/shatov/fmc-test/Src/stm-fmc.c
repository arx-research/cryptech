//------------------------------------------------------------------------------
// stm-fmc.c
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Headers
//------------------------------------------------------------------------------
#include "stm-fmc.h"


//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------
SRAM_HandleTypeDef _fmc_fpga_inst;


//------------------------------------------------------------------------------
void fmc_init(void)
//------------------------------------------------------------------------------
{
		// configure fmc pins
	_fmc_init_gpio();
	
		// configure fmc registers
	_fmc_init_params();
}


//------------------------------------------------------------------------------
int fmc_write_32(uint32_t addr, uint32_t *data)
//------------------------------------------------------------------------------
{
		// calculate target fpga address
	uint32_t ptr = FMC_FPGA_BASE_ADDR + (addr & FMC_FPGA_ADDR_MASK);

		// write data to fpga
	HAL_StatusTypeDef ok = HAL_SRAM_Write_32b(&_fmc_fpga_inst, (uint32_t *)ptr, data, 1);
	
		// check for error
	if (ok != HAL_OK) return -1;

		// wait for transaction to complete
	int wait = _fmc_nwait_idle();
	
		// check for timeout
	if (wait != 0) return -1;

		// everything went ok
	return 0;	
}


//------------------------------------------------------------------------------
int fmc_read_32(uint32_t addr, uint32_t *data)
	//------------------------------------------------------------------------------
{    
      // calculate target fpga address
    uint32_t ptr = FMC_FPGA_BASE_ADDR + (addr & FMC_FPGA_ADDR_MASK);
    
      // perform dummy read transaction
    HAL_StatusTypeDef ok = HAL_SRAM_Read_32b(&_fmc_fpga_inst, (uint32_t *)ptr, data, 1);
	
			// check for error
    if (ok != HAL_OK) return -1;
  
      // wait for dummy transaction to complete
    int wait = _fmc_nwait_idle();
	
			// check for timeout
    if (wait != 0) return -1;

      // read data from fpga
    ok = HAL_SRAM_Read_32b(&_fmc_fpga_inst, (uint32_t *)ptr, data, 1);
	
			// check for error
    if (ok != HAL_OK) return -1;
    
      // wait for read transaction to complete
    wait = _fmc_nwait_idle();
		
			// check for timeout
    if (wait != 0) return -1;
        
      // everything went ok
    return 0;
}


//------------------------------------------------------------------------------
int _fmc_nwait_idle()
//------------------------------------------------------------------------------
{
  int cnt; // counter
  
    // poll NWAIT (number of iterations is limited)
  for (cnt=0; cnt<FMC_FPGA_NWAIT_MAX_POLL_TICKS; cnt++)
  {
      // read pin state
    GPIO_PinState nwait = HAL_GPIO_ReadPin(FMC_GPIO_PORT_NWAIT, FMC_GPIO_PIN_NWAIT);
    
      // stop waiting if fpga is ready
    if (nwait == FMC_NWAIT_IDLE) break;
  }
  
    // check for timeout
  if (cnt >= FMC_FPGA_NWAIT_MAX_POLL_TICKS) return -1;
  
    // ok
  return 0;
}

//------------------------------------------------------------------------------
void _fmc_init_gpio(void)
//------------------------------------------------------------------------------
{
		// enable gpio clocks
	__GPIOA_CLK_ENABLE();
	__GPIOB_CLK_ENABLE();
	__GPIOD_CLK_ENABLE();
  __GPIOE_CLK_ENABLE();
	__GPIOF_CLK_ENABLE();
	__GPIOG_CLK_ENABLE();
	__GPIOH_CLK_ENABLE();
  __GPIOI_CLK_ENABLE();
	
		// enable fmc clock
	__FMC_CLK_ENABLE();
	
		// structure
	GPIO_InitTypeDef GPIO_InitStruct;
	
		// Port B
	GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
		// Port D
	GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11 
                          |GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15 
                          |GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_3|GPIO_PIN_4 
                          |GPIO_PIN_5|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
	
		/*
		 * When FMC is working with fixed latency, NWAIT pin must not be
		 * configured in AF mode, according to STM32F429 errata.
		 */
		
		// Port D (GPIO!)
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);	
	
		// Port E
	GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_7 
                          |GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11 
                          |GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
	
		// Port F
	GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3 
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_12|GPIO_PIN_13 
                          |GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);		
	
		// Port G
	GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3 
                          |GPIO_PIN_4|GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
	
		// Port H
	GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11 
                          |GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);		
	
		// Port I
  GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_0|GPIO_PIN_1 
                          |GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);
}


//------------------------------------------------------------------------------
void _fmc_init_params(void)
//------------------------------------------------------------------------------
{
		/*
		 * fill internal fields
		 */
	_fmc_fpga_inst.Instance = FMC_NORSRAM_DEVICE;
  _fmc_fpga_inst.Extended = FMC_NORSRAM_EXTENDED_DEVICE;
	
	
		/*
		 * configure fmc interface settings
		 */
  
		// use the first bank and corresponding chip select
  _fmc_fpga_inst.Init.NSBank = FMC_NORSRAM_BANK1;
	
		// data and address buses are separate
  _fmc_fpga_inst.Init.DataAddressMux = FMC_DATA_ADDRESS_MUX_DISABLE;
	
		// fpga mimics psram-type memory
  _fmc_fpga_inst.Init.MemoryType = FMC_MEMORY_TYPE_PSRAM;
	
		// data bus is 32-bit
  _fmc_fpga_inst.Init.MemoryDataWidth = FMC_NORSRAM_MEM_BUS_WIDTH_32;
	
		// read transaction is sync
  _fmc_fpga_inst.Init.BurstAccessMode = FMC_BURST_ACCESS_MODE_ENABLE;
	
		// this _must_ be configured to high, according to errata, otherwise
		// the processor may hang after trying to access fpga via fmc
  _fmc_fpga_inst.Init.WaitSignalPolarity = FMC_WAIT_SIGNAL_POLARITY_HIGH;
	
		// wrap mode is not supported
  _fmc_fpga_inst.Init.WrapMode = FMC_WRAP_MODE_DISABLE;
	
		// don't care in fixed latency mode
  _fmc_fpga_inst.Init.WaitSignalActive = FMC_WAIT_TIMING_DURING_WS;
	
		// allow write access to fpga
  _fmc_fpga_inst.Init.WriteOperation = FMC_WRITE_OPERATION_ENABLE;
	
		// use fixed latency mode (ignore wait signal)
  _fmc_fpga_inst.Init.WaitSignal = FMC_WAIT_SIGNAL_DISABLE;
	
		// write and read have same timing
  _fmc_fpga_inst.Init.ExtendedMode = FMC_EXTENDED_MODE_DISABLE;
	
		// don't care in sync mode
  _fmc_fpga_inst.Init.AsynchronousWait = FMC_ASYNCHRONOUS_WAIT_DISABLE;
	
		// write transaction is sync
  _fmc_fpga_inst.Init.WriteBurst = FMC_WRITE_BURST_ENABLE;
	
		// keep clock always active
  _fmc_fpga_inst.Init.ContinuousClock = FMC_CONTINUOUS_CLOCK_SYNC_ASYNC;
	
		/*
		 * configure fmc timing parameters
		 */
  FMC_NORSRAM_TimingTypeDef fmc_timing;
  
		// don't care in sync mode
  fmc_timing.AddressSetupTime = 15;
	
		// don't care in sync mode
  fmc_timing.AddressHoldTime = 15;
	
		// don't care in sync mode
  fmc_timing.DataSetupTime = 255;
	
		// not needed, since nwait will be polled manually
  fmc_timing.BusTurnAroundDuration = 0;
	
		// use smallest allowed divisor for best performance
  fmc_timing.CLKDivision = 2;
	
		// stm is too slow to work with min allowed 2-cycle latency
  fmc_timing.DataLatency = 3;
	
		// don't care in sync mode
  fmc_timing.AccessMode = FMC_ACCESS_MODE_A;

		// initialize fmc
  HAL_SRAM_Init(&_fmc_fpga_inst, &fmc_timing, NULL);
}


//------------------------------------------------------------------------------
// EOF
//------------------------------------------------------------------------------
