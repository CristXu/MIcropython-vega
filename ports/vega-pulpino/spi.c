#include "fsl_common.h"
#include "spi.h"
#include "fsl_port.h"
#include "py/nlr.h"
#include "py/runtime.h"

#define PIN4_IDX                         4u   /*!< Pin number for pin 4 in a port */
#define PIN5_IDX                         5u   /*!< Pin number for pin 5 in a port */
#define PIN6_IDX                         6u   /*!< Pin number for pin 6 in a port */
#define PIN7_IDX                         7u   /*!< Pin number for pin 7 in a port */

#define LPSPI_BASEADDR (LPSPI0)
#define LPSPI_CLOCK_NAME (kCLOCK_Lpspi0)
#define LPSPI_CLOCK_SOURCE ( kCLOCK_IpSrcFircAsync)
#define LPSPI_CLOCK_FREQ (CLOCK_GetIpFreq(LPSPI_CLOCK_NAME))

#define LPSPI_PCS_FOR_INIT (kLPSPI_Pcs2)
#define LPSPI_PCS_FOR_TRANSFER (kLPSPI_MasterPcs2)

#define TRANSFER_SIZE (1U);	    /*! Transfer dataSize.*/
#define TRANSFER_BAUDRATE (500000U) /*! Transfer baudrate - 500k */
void ConfigSpiPin()
{
	PORT_SetPinMux(PORTB, PIN4_IDX, kPORT_MuxAlt2);            /* PORTB4 (pin C2) is configured as LPSPI0_SCK */
	PORT_SetPinMux(PORTB, PIN5_IDX, kPORT_MuxAlt2);            /* PORTB5 (pin D2) is configured as LPSPI0_SOUT */
	PORT_SetPinMux(PORTB, PIN6_IDX, kPORT_MuxAlt2);            /* PORTB6 (pin E1) is configured as LPSPI0_PCS2 */
	PORT_SetPinMux(PORTB, PIN7_IDX, kPORT_MuxAlt2);            /* PORTB7 (pin E2) is configured as LPSPI0_SIN */
}
void Init_SPI(uint32_t baudrate)
{
	ConfigSpiPin();
	/*Set clock source for LPSPI and get master clock source*/
    CLOCK_SetIpSrc(LPSPI_CLOCK_NAME, LPSPI_CLOCK_SOURCE);
	lpspi_master_config_t masterConfig;
	masterConfig.baudRate = baudrate;
	masterConfig.bitsPerFrame = 8 * TRANSFER_SIZE;
	masterConfig.cpol = kLPSPI_ClockPolarityActiveHigh;
	masterConfig.cpha = kLPSPI_ClockPhaseFirstEdge;
	masterConfig.direction = kLPSPI_MsbFirst;
	
	masterConfig.pcsToSckDelayInNanoSec = 1000000000 / masterConfig.baudRate;
    masterConfig.lastSckToPcsDelayInNanoSec = 1000000000 / masterConfig.baudRate;
    masterConfig.betweenTransferDelayInNanoSec = 1000000000 / masterConfig.baudRate;

	masterConfig.whichPcs = LPSPI_PCS_FOR_INIT;
    masterConfig.pcsActiveHighOrLow = kLPSPI_PcsActiveLow;

    masterConfig.pinCfg = kLPSPI_SdiInSdoOut;
    masterConfig.dataOutConfig = kLpspiDataOutRetained;

    LPSPI_MasterInit(LPSPI_BASEADDR, &masterConfig, LPSPI_CLOCK_FREQ);
	
}
void spi_transfer(size_t txLen, const uint8_t *src, size_t rxLen, uint8_t *dest, uint32_t timeout) 
{
    // Note: there seems to be a problem sending 1 byte using DMA the first
    // time directly after the SPI/DMA is initialised.  The cause of this is
    // unknown but we sidestep the issue by using polling for 1 byte transfer.
	lpspi_transfer_t xfer;
    status_t st = kStatus_Success;
	if (src) {
		xfer.txData = (uint8_t*) src;
		xfer.rxData = 0;
		xfer.dataSize = txLen;
		xfer.configFlags = LPSPI_PCS_FOR_TRANSFER | kLPSPI_MasterPcsContinuous | kLPSPI_SlaveByteSwap;
		st = LPSPI_MasterTransferBlocking(LPSPI_BASEADDR, &xfer);
		if (st != kStatus_Success)
			goto cleanup;
		while (LPSPI_GetStatusFlags(LPSPI_BASEADDR) & kLPSPI_ModuleBusyFlag) {}
	}
	if (dest) {
		xfer.txData = 0;
		xfer.rxData = dest;
		xfer.dataSize = rxLen;
		st = LPSPI_MasterTransferBlocking(LPSPI_BASEADDR, &xfer);
		while (LPSPI_GetStatusFlags(LPSPI_BASEADDR) & kLPSPI_ModuleBusyFlag) {}
	}	
cleanup:
	if (st != kStatus_Success){  //make a stupid error....ignore the brace, use the python so often... 
		LPSPI_Deinit(LPSPI_BASEADDR);
		printf("Please try to reinit a swim module!\n");
		nlr_raise(mp_obj_new_exception_msg(&mp_type_TypeError, "Transfer Failed!"));
	}
}
