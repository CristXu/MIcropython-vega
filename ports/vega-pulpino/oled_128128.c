/*
 * @brief OLED (implementation file)
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2015
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>

#include "py/nlr.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "pin.h"
#include "genhdr/pins.h"
#include "bufhelper.h"
#include "fsl_gpio.h"
#include "oled_128128.h"

#if 1
#include "fsl_common.h"
#include "fsl_lpspi.h"
#include "fsl_lpspi_edma.h"
#include "fsl_port.h"
#include "fsl_mux.h"
#define PIN4_IDX                         4u   /*!< Pin number for pin 4 in a port */
#define PIN5_IDX                         5u   /*!< Pin number for pin 5 in a port */
#define PIN6_IDX                         6u   /*!< Pin number for pin 6 in a port */
#define PIN7_IDX                         7u   /*!< Pin number for pin 7 in a port */

#define LPSPI_BASEADDR (LPSPI0)
#define LPSPI_CLOCK_NAME (kCLOCK_Lpspi0)
#define LPSPI_CLOCK_SOURCE ( kCLOCK_IpSrcFircAsync)
#define LPSPI_CLOCK_FREQ (CLOCK_GetIpFreq(LPISPI_CLOCK_NAME))

#define LPSPI_PCS_FOR_INIT (kLPSPI_Pcs2)
#define LPSPI_PCS_FOR_TRANSFER (kLPSPI_MasterPcs2)

#define TRANSFER_SIZE (512U);	    /*! Transfer dataSize.*/
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

	masterConfig.whichPcs = EXAMPLE_LPSPI_MASTER_PCS_FOR_INIT;
    masterConfig.pcsActiveHighOrLow = kLPSPI_PcsActiveLow;

    masterConfig.pinCfg = kLPSPI_SdiInSdoOut;
    masterConfig.dataOutConfig = kLpspiDataOutRetained;

    LPSPI_MasterInit(LPISPI_BASEADDR, &masterConfig, LPSPI_CLOCK_FREQ);
	
}
void spi_transfer(size_t txLen, const uint8_t *src, size_t rxLen, uint8_t *dest, uint32_t timeout, bool isCtrlSSEL) 
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
    LPSPI_Deinit(LPSPI_BASEADDR);
	if (st != kStatus_Success)
		printf("Please try to reinit a swim module!\n");
		nlr_raise(mp_obj_new_exception_msg(&mp_type_TypeError, "Transfer Failed!"));
}

pin_obj_t COMMON
#define DATA 
#define RST
#endif VPP
void OLED_IO_hw_init(void)
{
	gpio_pin_config_t t;
	t.pinDirection = kGPIO_DigitalOutput , t.outputLogic = 1;    
	// we use GPIO to control SSEL
	
	mp_hal_pin_config(&PIN_DATA, GPIO_FILTEROFF | GPIO_MODE_DIGITAL, MP_HAL_PIN_PULL_UP, 0);
	GPIO_PinInit(GPIO, PIN_DATA.port, PIN_DATA.pin, &t);

	mp_hal_pin_config(&PIN_RST, GPIO_FILTEROFF | GPIO_MODE_DIGITAL, MP_HAL_PIN_PULL_UP, 0);
	GPIO_PinInit(GPIO, PIN_RST.port, PIN_RST.pin, &t);

	mp_hal_pin_config(&PIN_VPP, GPIO_FILTEROFF | GPIO_MODE_DIGITAL, MP_HAL_PIN_PULL_UP, 0);
	t.outputLogic = 0; 
	GPIO_PinInit(GPIO, PIN_VPP.port, PIN_VPP.pin, &t);
}

void OLED_IO_vpp(bool state) {
	GPIO_WritePinOutput(GPIO, PIN_VPP.port, PIN_VPP.pin, state);
}

void OLED_IO_reset(bool state) {
	GPIO_WritePinOutput(GPIO, PIN_RST.port, PIN_RST.pin, !state);
}

void OLED_IO_cmd(void) {
	GPIO_WritePinOutput(GPIO, PIN_DATA.port, PIN_DATA.pin, 0);
}

void OLED_IO_data(void) {
	GPIO_WritePinOutput(GPIO, PIN_DATA.port, PIN_DATA.pin, 1);
}

void OLED_IO_write(uint8_t *pData, uint32_t cbTx) {
	spi_transfer(PYB_SPI_9, cbTx, pData, 0, 0, 100, 1);
}

//
//	Initialization sequence from:
//		Visionox Product Specification
//		Product Name: VGM128128A7W01
//		Product Code: M01500
//
static uint8_t init_seq[] = {
	0xae,		//	Display OFF
	0xd5,		//	Set D-clock
	0x50,		//	100Hz
	0x20,		//	Set row address
	0x81,		//	Set contrast control
	0xc0,
	0xa0,		//	Segment remap
	0xa4,		//	Set Entire Display ON
	0xa6,		//	Normal display
	0xad,		//	Set external VPP
	0x80,
	0xc0,		//	Set Common scan direction
	0xd9,		//	Set phase length
	0x25,
	0xdb,		//	Set Vcomh voltage
	0x28,		//	0.687*VPP
//	0xaf		//	Display ON
};
static const uint32_t init_seq_ct = sizeof(init_seq) / sizeof(uint8_t);

static void WriteCmd(uint8_t cmd)
{
	OLED_IO_cmd();
	OLED_IO_write(&cmd, 1);
}

static void WriteDat(uint8_t *pDat, uint32_t cbTx)
{
	OLED_IO_data();
	OLED_IO_write(pDat, cbTx);
}

static void SetPos(uint8_t x, uint8_t y)
{ 
	WriteCmd(0xb0+y);
	WriteCmd(((x&0xf0)>>4)|0x10);
	WriteCmd((x&0x0f)|0x10);
}

void OLED_Fill(uint8_t c)
{
	uint8_t m,n;
	uint8_t buf[8] = {c,c,c,c, c,c,c,c};
	for(m=0; m<16; m++) {
		WriteCmd(0xb0+m);				//	page0-page1
		WriteCmd(0x00);					//	low column start address
		WriteCmd(0x10);					//	high column start address
		for(n=0; n<128/8; n++)
			WriteDat(buf, 8);
	}
}

void OLED_cls(void)
{
	OLED_Fill(0x00);
}

void OLED_on(void)
{
	WriteCmd(0x8d);						//	set charge pump
	WriteCmd(0x14);						//	enable charge pump
	WriteCmd(0xaf);						//	enable OLED
}

void OLED_off(void)
{
	WriteCmd(0x8d);						//	set charge pump
	WriteCmd(0x10);						//	disable charge pump
	WriteCmd(0xae);						//	disable OLED
}

void OLED_init(uint32_t dispClockRate)
{
	uint32_t i;

    mp_arg_val_t args[] = {
        {.u_int = 0},	// 0 = master, !0 = slave
        {.u_int = 2000000},
        {.u_int = 0},
        {.u_int = 0},
        {.u_int = 8},
        {.u_int = kSPI_MsbFirst},
		// >>> unsupported/unnessary args, keep for compatibility
        {.u_int = 0xffffffff},
        {.u_int = 0},
        {.u_int = 0},
        {.u_bool = false},
        {.u_obj = mp_const_none},
		// <<<
    };
	
	
	OLED_IO_hw_init();	
	//args[1].u_int = dispClockRate;
	//pyb_spi_init_c(OLED_SPI, args);
	Init_SPI(dispClockRate);

	OLED_IO_vpp(1);				//	power panel
	OLED_IO_reset(1);				//	start reset
	mp_hal_delay_us(100);						//	20uS delay
	OLED_IO_reset(0);			//	release reset
	mp_hal_delay_ms(100);						//	wait for SH1107 to come out of reset

	for (i=0; i<init_seq_ct; i++) {		//	loop through init sequence
		WriteCmd(init_seq[i]);			//	write the command
	}
	OLED_Fill(0x08);							//	clear oled
	WriteCmd(0xaf);						//	enable OLED
}

static void oled_write_64(uint8_t* pblk)
{
	uint8_t data_out=0;
	uint8_t *data_in;
	uint8_t buf[8];
	int32_t x,y;

	for (x=7; x>=0; x--) {
		data_out = 0;
		for (y=0; y<128; y+=16) {
			data_in = pblk+y;
			data_out >>= 1;
			if (*data_in & 1<<x) data_out |= 0x80;
		}
		buf[7 - x] = data_out;
	}
	WriteDat(buf, 8);
}

void OLED_update_swim_fb(uint8_t* pfb)
{
	uint32_t	y_idx, x_idx;
	uint8_t		*pblk;

	for (y_idx=0; y_idx<16; y_idx+=1) {
		SetPos(0,y_idx);
		for (x_idx=0; x_idx<16; x_idx+=1) {
			pblk = pfb + x_idx + (y_idx * 128);
			oled_write_64(pblk);
		}
	}
}


