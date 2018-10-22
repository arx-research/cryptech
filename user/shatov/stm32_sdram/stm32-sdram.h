//-----------------------------------------------------------------------------
// stm32-sdram.h
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Defined Values
//-----------------------------------------------------------------------------

	/* Base Addresses */
#define SDRAM_BASEADDR_CHIP1		((uint32_t *)0xC0000000)
#define SDRAM_BASEADDR_CHIP2		((uint32_t *)0xD0000000)

	/* Memory Size */
#define SDRAM_SIZE		0x4000000		// 64 MBytes (512 Mbits)

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


//-----------------------------------------------------------------------------
// Prototypes
//-----------------------------------------------------------------------------
int sdram_init(SDRAM_HandleTypeDef *sdram1, SDRAM_HandleTypeDef *sdram2);


//-----------------------------------------------------------------------------
// End-of-File
//-----------------------------------------------------------------------------
