//-----------------------------------------------------------------------------
// alpha_n25q128.h
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Interface Map
//-----------------------------------------------------------------------------
#define N25Q128_SPI_HANDLE	(&hspi2)


//-----------------------------------------------------------------------------
// Defined Values
//-----------------------------------------------------------------------------
#define N25Q128_COMMAND_READ_ID						0x9E
#define N25Q128_COMMAND_READ_PAGE					0x03
#define N25Q128_COMMAND_READ_STATUS				0x05
#define N25Q128_COMMAND_WRITE_ENABLE			0x06
#define N25Q128_COMMAND_ERASE_SECTOR			0xD8
#define N25Q128_COMMAND_PAGE_PROGRAM			0x02

#define N25Q128_PAGE_SIZE									0x100			// 256
#define N25Q128_NUM_PAGES									0x10000		// 65536

#define N25Q128_SECTOR_SIZE								0x10000		// 65536
#define N25Q128_NUM_SECTORS								0x100			// 256

#define N25Q128_SPI_TIMEOUT								1000

#define N25Q128_ID_MANUFACTURER						0x20
#define N25Q128_ID_DEVICE_TYPE						0xBA
#define N25Q128_ID_DEVICE_CAPACITY				0x18


//-----------------------------------------------------------------------------
// Macros
//-----------------------------------------------------------------------------
#define _n25q128_select()			HAL_GPIO_WritePin(PROM_CS_N_GPIO_Port,PROM_CS_N_Pin,GPIO_PIN_RESET);
#define _n25q128_deselect()		HAL_GPIO_WritePin(PROM_CS_N_GPIO_Port,PROM_CS_N_Pin,GPIO_PIN_SET);


//-----------------------------------------------------------------------------
// Prototypes
//-----------------------------------------------------------------------------
int		n25q128_check_id					(void);
int		n25q128_get_wip_flag			(void);
int		n25q128_read_page					(uint32_t page_offset, uint8_t *page_buffer);
int		n25q128_write_page				(uint32_t page_offset, uint8_t *page_buffer);
int		n25q128_erase_sector			(uint32_t sector_offset);

int		_n25q128_get_wel_flag			(void);


//-----------------------------------------------------------------------------
// End-of-File
//-----------------------------------------------------------------------------
