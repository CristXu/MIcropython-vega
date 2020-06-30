#include "flash.h"
#include "fsl_lpspi.h"
#include "fsl_gpio.h"
/*******************************************************************************
* Declaration
******************************************************************************/
void spi_read(uint8_t * pData, uint32_t size);
void spi_write(uint8_t * pData, uint32_t size);
/*******************************************************************************
* Definitions
******************************************************************************/
// SPI2_PCS0
#define EXAMPLE_LPSPI_MASTER_BASEADDR (LPSPI1)

#define EXAMPLE_LPSPI_MASTER_CLOCK_NAME (kCLOCK_Lpspi1)
#define EXAMPLE_LPSPI_MASTER_CLOCK_SOURCE (kCLOCK_IpSrcFircAsync)
#define EXAMPLE_LPSPI_MASTER_CLOCK_FREQ (CLOCK_GetIpFreq(EXAMPLE_LPSPI_MASTER_CLOCK_NAME))

#define EXAMPLE_LPSPI_MASTER_PCS_FOR_INIT (kLPSPI_Pcs2)
#define EXAMPLE_LPSPI_MASTER_PCS_FOR_TRANSFER (kLPSPI_MasterPcs2)


#define TRANSFER_SIZE (1U)        /*! Transfer dataSize.*/
#define TRANSFER_BAUDRATE (20000000) /*! Transfer baudrate - 500k */

// flash commands
//reset ops
#define FLASH_RESET_ENABLE          0x66
#define FLASH_RESET_MEMORY          0x99

// ID ops
#define FLASH_READ_ID               0x90
#define FLASH_READ_JEDEC_ID         0x9F

// read ops
#define FLASH_READ                  0x03
#define FLASH_FAST_READ				0x0B

// write ops
#define FLASH_WRITE_ENABLE    		0x06
#define FLASH_WRITE_DISABLE			0x04

// reg ops
#define FLASH_READ_STATUS_REG1 		0x05
#define FLASH_READ_STATUS_REG2 		0x35
#define FLASH_READ_STATUS_REG3 		0x15

#define FLASH_WRITE_STATUS_REG1 	0x01
#define FLASH_WRITE_STATUS_REG2 	0x31
#define FLASH_WRITE_STATUS_REG3 	0x11

// program ops
#define FLASH_PAGE_PROG				0x02

// erase ops
#define FLASH_SECTOR_ERASE			0x20
#define FLASH_BLOCK_ERASE			0xD8
#define FLASH_CHIP_ERASE			0xC7

#define FLASH_PROG_ERASE_RESUME		0x7A
#define FLASH_PROG_ERASE_SUSPEND	0x75

#define ADDR_SPLIT_3B(addr) \
	((addr >> 16) & 0xff), ((addr >> 8) & 0xff), (addr & 0xff)

#define ADDR_SPLIT_4B(addr) \
	((addr >> 24) & 0xff), ((addr >> 16) & 0xff), ((addr >> 8) & 0xff), (addr & 0xff)


#define BOARD_SPI_CSActivate() \
    		GPIO_WritePinOutput(GPIOB, 22, 0)

#define BOARD_SPI_CSDeActivate() \
    		GPIO_WritePinOutput(GPIOB, 22, 1);


#define SPI_WRITE_CMD(cmd , ...) \
	do{ \
		uint8_t cmd_len; \
		uint8_t data[] = {cmd , __VA_ARGS__}; \
		cmd_len = sizeof(data) / sizeof(data[0]); \
		spi_write(data, cmd_len); \
	}while(0);

/*******************************************************************************
* function
******************************************************************************/

void spi_flash_init(){
	/*Set clock source for LPSPI and get master clock source*/
    CLOCK_SetIpSrc(EXAMPLE_LPSPI_MASTER_CLOCK_NAME, EXAMPLE_LPSPI_MASTER_CLOCK_SOURCE);

    lpspi_master_config_t masterConfig;

    /*Master config */
    masterConfig.baudRate = TRANSFER_BAUDRATE;
    masterConfig.bitsPerFrame = 8 * TRANSFER_SIZE;
    masterConfig.cpol = kLPSPI_ClockPolarityActiveHigh; // cpol = 1
    masterConfig.cpha = kLPSPI_ClockPhaseFirstEdge; // cpha = 1
    masterConfig.direction = kLPSPI_MsbFirst;

    masterConfig.pcsToSckDelayInNanoSec = 1000000000 / masterConfig.baudRate;
    masterConfig.lastSckToPcsDelayInNanoSec = 1000000000 / masterConfig.baudRate;
    masterConfig.betweenTransferDelayInNanoSec = 1000000000 / masterConfig.baudRate;

    masterConfig.whichPcs = EXAMPLE_LPSPI_MASTER_PCS_FOR_INIT;
    masterConfig.pcsActiveHighOrLow = kLPSPI_PcsActiveLow;

    masterConfig.pinCfg = kLPSPI_SdiInSdoOut;
    masterConfig.dataOutConfig = kLpspiDataOutRetained;

    LPSPI_MasterInit(EXAMPLE_LPSPI_MASTER_BASEADDR, &masterConfig, EXAMPLE_LPSPI_MASTER_CLOCK_FREQ);

    /* Sets the FLASH PCS as low logic */
    gpio_pin_config_t csPinConfig = {
        kGPIO_DigitalOutput, 1,
    };
    GPIO_PinInit(GPIOB, 22, &csPinConfig);
    //SPI_WRITE_CMD(FLASH_RESET_ENABLE, FLASH_RESET_MEMORY);

}

void spi_read(uint8_t * pData, uint32_t size){
    lpspi_transfer_t masterXfer;
    /*Start master transfer*/
    masterXfer.txData = 0;
    masterXfer.rxData = pData;
    masterXfer.dataSize = size;
    masterXfer.configFlags = EXAMPLE_LPSPI_MASTER_PCS_FOR_TRANSFER | kLPSPI_MasterPcsContinuous | kLPSPI_SlaveByteSwap;
    LPSPI_MasterTransferBlocking(EXAMPLE_LPSPI_MASTER_BASEADDR, &masterXfer);

    while(LPSPI_GetStatusFlags(EXAMPLE_LPSPI_MASTER_BASEADDR) & kLPSPI_ModuleBusyFlag );
}

void spi_write(uint8_t* pData, uint32_t size){
    lpspi_transfer_t masterXfer;
    /*Start master transfer*/
    masterXfer.txData = pData;
    masterXfer.rxData = 0;  // do not need to set a rdata, the function has done it by setting the rxmsk=1 when only transfer, in case receive data into fifo
    masterXfer.dataSize = size;
    masterXfer.configFlags = EXAMPLE_LPSPI_MASTER_PCS_FOR_TRANSFER | kLPSPI_MasterPcsContinuous | kLPSPI_SlaveByteSwap;
    LPSPI_MasterTransferBlocking(EXAMPLE_LPSPI_MASTER_BASEADDR, &masterXfer);
    while(LPSPI_GetStatusFlags(EXAMPLE_LPSPI_MASTER_BASEADDR) & kLPSPI_ModuleBusyFlag );
}
typedef enum {
	FLASH_OK = 0,
	FLASH_BUSY,
}flash_state;

flash_state spi_flash_idle(){
	BOARD_SPI_CSActivate();
	SPI_WRITE_CMD(FLASH_READ_STATUS_REG1);
	uint8_t status;
	spi_read(&status, 1);
	flash_state state;
	state = (status & 0x01)?FLASH_BUSY:FLASH_OK;
	BOARD_SPI_CSDeActivate();
	return state;

}
void spi_flash_read_id(uint8_t *id, ID_TYPE type){
	BOARD_SPI_CSActivate();
	switch(type){
		case ID:
			SPI_WRITE_CMD(FLASH_READ_ID, 0x00, 0x00, 0x00);
			spi_read(id, 2);
			break;
		case JDEC_ID:
			SPI_WRITE_CMD(FLASH_READ_JEDEC_ID);
			spi_read(id, 3);
			break;
		default:
			break;
	}
	BOARD_SPI_CSDeActivate();
}

void spi_flash_read(uint8_t* pData, uint32_t addr, uint32_t size){
	BOARD_SPI_CSActivate();
	SPI_WRITE_CMD(FLASH_READ , ADDR_SPLIT_3B(addr));
	spi_read(pData, size);
	BOARD_SPI_CSDeActivate();
}
int spi_flash_write_page(uint8_t* pData, uint32_t addr){
	while(spi_flash_idle() != FLASH_OK);
	// enable write
	BOARD_SPI_CSActivate();
	SPI_WRITE_CMD(FLASH_WRITE_ENABLE);
	BOARD_SPI_CSDeActivate();
	// write data
	BOARD_SPI_CSActivate();
	SPI_WRITE_CMD(FLASH_PAGE_PROG, ADDR_SPLIT_3B(addr));
	spi_write(pData, FLASH_PAGE_SIZE);
	BOARD_SPI_CSDeActivate();
	while(spi_flash_idle() != FLASH_OK);
	return 0;
}
void spi_flash_erase_sector(uint32_t addr){
	while(spi_flash_idle() != FLASH_OK);
	// enable write
	BOARD_SPI_CSActivate();
	SPI_WRITE_CMD(FLASH_WRITE_ENABLE);
	BOARD_SPI_CSDeActivate();
	//erase
	BOARD_SPI_CSActivate();
	SPI_WRITE_CMD(FLASH_SECTOR_ERASE, ADDR_SPLIT_3B(addr));
	BOARD_SPI_CSDeActivate();
	while(spi_flash_idle() != FLASH_OK);
}
void spi_flash_erase_chip(void){
	while(spi_flash_idle() != FLASH_OK);
	// enable write
	BOARD_SPI_CSActivate();
	SPI_WRITE_CMD(FLASH_WRITE_ENABLE);
	BOARD_SPI_CSDeActivate();
	// erase whole chip
	BOARD_SPI_CSActivate();
	SPI_WRITE_CMD(FLASH_CHIP_ERASE);
	BOARD_SPI_CSDeActivate();
	while(spi_flash_idle() != FLASH_OK);
}
int spi_flash_validate(uint32_t addr, uint32_t size, uint8_t* data){
	uint8_t *cmp_data = (uint8_t*)malloc(size * sizeof(uint8_t));
	spi_flash_read(cmp_data, addr, size);
	int result =  memcmp(cmp_data, data, size);
	free(cmp_data);
	return result;
}


