//-----------------------------------------------------------------------------
// alpha_n25q128.c
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Headers
//-----------------------------------------------------------------------------
#include "stm32f4xx_hal.h"
#include "alpha_n25q128.h"
#include "spi.h"


//-----------------------------------------------------------------------------
int n25q128_check_id()
//-----------------------------------------------------------------------------
{
		// tx, rx buffers
	uint8_t spi_tx[4];
	uint8_t spi_rx[4];
	
		// result
	HAL_StatusTypeDef ok;
	
		// send READ ID command
	spi_tx[0] = N25Q128_COMMAND_READ_ID;
	
		// select, send command & read response, deselect
	_n25q128_select();
	ok = HAL_SPI_TransmitReceive(N25Q128_SPI_HANDLE, spi_tx, spi_rx, 4, N25Q128_SPI_TIMEOUT);
	HAL_Delay(1);
	_n25q128_deselect();
	
		// check
	if (ok != HAL_OK) return 0;
		
		// parse response (note, that the very first byte was received during the
		// transfer of the command byte, so it contains garbage and should
		// be ignored here)
	if (spi_rx[1] != N25Q128_ID_MANUFACTURER) return 0;
	if (spi_rx[2] != N25Q128_ID_DEVICE_TYPE) return 0;
	if (spi_rx[3] != N25Q128_ID_DEVICE_CAPACITY) return 0;
	
		// done
	return 1;
}


//-----------------------------------------------------------------------------
int n25q128_read_page(uint32_t page_offset, uint8_t *page_buffer)
//-----------------------------------------------------------------------------
{
		// tx buffer
	uint8_t spi_tx[4];
	
		// result
	HAL_StatusTypeDef ok;
	
		// check offset
	if (page_offset >= N25Q128_NUM_PAGES) return 0;
	
		// calculate byte address
	page_offset *= N25Q128_PAGE_SIZE;
	
		// prepare READ command
	spi_tx[0] = N25Q128_COMMAND_READ_PAGE;
	spi_tx[1] = (uint8_t)(page_offset >> 16);
	spi_tx[2] = (uint8_t)(page_offset >>  8);
	spi_tx[3] = (uint8_t)(page_offset >>  0);
	
		// activate, send command
	_n25q128_select();
	ok = HAL_SPI_Transmit(N25Q128_SPI_HANDLE, spi_tx, 4, N25Q128_SPI_TIMEOUT);
	HAL_Delay(1);
	
		// check
	if (ok != HAL_OK)
	{	_n25q128_deselect();
		return 0;
	}
		
		// read response, deselect
	ok = HAL_SPI_Receive(N25Q128_SPI_HANDLE, page_buffer, N25Q128_PAGE_SIZE, N25Q128_SPI_TIMEOUT);
	HAL_Delay(1);
	_n25q128_deselect();
	
		// check
	if (ok != HAL_OK) return 0;
	
		// done
	return 1;
}


//-----------------------------------------------------------------------------
int n25q128_write_page(uint32_t page_offset, uint8_t *page_buffer)
//-----------------------------------------------------------------------------
{
		// tx buffer
	uint8_t spi_tx[4];
	
		// result
	HAL_StatusTypeDef ok;
	
		// check offset
	if (page_offset >= N25Q128_NUM_PAGES) return 0;
	
		// enable writing
	spi_tx[0] = N25Q128_COMMAND_WRITE_ENABLE;
	
		// activate, send command, deselect
	_n25q128_select();
	ok = HAL_SPI_Transmit(N25Q128_SPI_HANDLE, spi_tx, 1, N25Q128_SPI_TIMEOUT);
	HAL_Delay(1);
	_n25q128_deselect();
	
		// check
	if (ok != HAL_OK) return 0;
	
		// make sure, that write enable did the job
	int wel = _n25q128_get_wel_flag();
	if (wel != 1) return 0;
	
		// calculate byte address
	page_offset *= N25Q128_PAGE_SIZE;
	
		// prepare PROGRAM PAGE command
	spi_tx[0] = N25Q128_COMMAND_PAGE_PROGRAM;
	spi_tx[1] = (uint8_t)(page_offset >> 16);
	spi_tx[2] = (uint8_t)(page_offset >>  8);
	spi_tx[3] = (uint8_t)(page_offset >>  0);
	
		// activate, send command
	_n25q128_select();
	ok = HAL_SPI_Transmit(N25Q128_SPI_HANDLE, spi_tx, 4, N25Q128_SPI_TIMEOUT);
	HAL_Delay(1);
	
		// check
	if (ok != HAL_OK)
	{	_n25q128_deselect();
		return 0;
	}
	
		// send data, deselect
	ok = HAL_SPI_Transmit(N25Q128_SPI_HANDLE, page_buffer, N25Q128_PAGE_SIZE, N25Q128_SPI_TIMEOUT);
	HAL_Delay(1);
	_n25q128_deselect();
	
		// check
	if (ok != HAL_OK) return 0;
		
		// done
	return 1;
}


//-----------------------------------------------------------------------------
int n25q128_get_wip_flag(void)
//-----------------------------------------------------------------------------
{
		// tx, rx buffers
	uint8_t spi_tx[2];
	uint8_t spi_rx[2];
	
		// result
	HAL_StatusTypeDef ok;
	
		// send READ STATUS command
	spi_tx[0] = N25Q128_COMMAND_READ_STATUS;
	
		// send command, read response, deselect
	_n25q128_select();
	ok = HAL_SPI_TransmitReceive(N25Q128_SPI_HANDLE, spi_tx, spi_rx, 2, N25Q128_SPI_TIMEOUT);
	HAL_Delay(1);
	_n25q128_deselect();
	
		// check
	if (ok != HAL_OK) return -1;
		
		// done
	return (spi_rx[1] & 1);
}


//-----------------------------------------------------------------------------
int n25q128_erase_sector(uint32_t sector_offset)
//-----------------------------------------------------------------------------
{
		// tx buffer
	uint8_t spi_tx[4];
	
		// result
	HAL_StatusTypeDef ok;
	
		// check offset
	if (sector_offset >= N25Q128_NUM_SECTORS) return 0;
	
		// enable writing
	spi_tx[0] = N25Q128_COMMAND_WRITE_ENABLE;
	
		// select, send command, deselect
	_n25q128_select();
	ok = HAL_SPI_Transmit(N25Q128_SPI_HANDLE, spi_tx, 1, N25Q128_SPI_TIMEOUT);
	HAL_Delay(1);
	_n25q128_deselect();
	
		// check
	if (ok != HAL_OK) return 0;
	
		// make sure, that write enable did the job
	int wel = _n25q128_get_wel_flag();
	if (wel != 1) return 0;
	
		// calculate byte address
	sector_offset *= N25Q128_SECTOR_SIZE;
	
		// send ERASE SUBSECTOR command
	spi_tx[0] = N25Q128_COMMAND_ERASE_SECTOR;
	spi_tx[1] = (uint8_t)(sector_offset >> 16);
	spi_tx[2] = (uint8_t)(sector_offset >>  8);
	spi_tx[3] = (uint8_t)(sector_offset >>  0);
	
		// activate, send command, deselect
	_n25q128_select();
	ok = HAL_SPI_Transmit(N25Q128_SPI_HANDLE, spi_tx, 4, N25Q128_SPI_TIMEOUT);
	HAL_Delay(1);
	_n25q128_deselect();
	
		// check
	if (ok != HAL_OK) return 0;
		
		// done
	return 1;
}


//-----------------------------------------------------------------------------
int _n25q128_get_wel_flag(void)
//-----------------------------------------------------------------------------
{
		// tx, rx buffers
	uint8_t spi_tx[2];
	uint8_t spi_rx[2];
	
		// result
	HAL_StatusTypeDef ok;
	
		// send READ STATUS command
	spi_tx[0] = N25Q128_COMMAND_READ_STATUS;
	
		// send command, read response, deselect
	_n25q128_select();
	ok = HAL_SPI_TransmitReceive(N25Q128_SPI_HANDLE, spi_tx, spi_rx, 2, N25Q128_SPI_TIMEOUT);
	HAL_Delay(1);
	_n25q128_deselect();
	
		// check
	if (ok != HAL_OK) return -1;
		
		// done
	return ((spi_rx[1] >> 1) & 1);
}

//-----------------------------------------------------------------------------
// End-of-File
//-----------------------------------------------------------------------------
