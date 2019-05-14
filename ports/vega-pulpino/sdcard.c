/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <string.h>

#include "py/nlr.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "lib/oofatfs/ff.h"
#include "extmod/vfs_fat.h"
#include "fsl_debug_console.h"
#include "fsl_card.h"

#include "sdcard.h"
#include "bufhelper.h"


#if MICROPY_HW_HAS_SDCARD

// TODO: Since SDIO is fundamentally half-duplex, we really only need to
//       tie up one DMA channel. However, the HAL DMA API doesn't
// seem to provide a convenient way to change the direction. I believe that
// its as simple as changing the CR register and the Init.Direction field
// and make DMA_SetConfig public.

// TODO: I think that as an optimization, we can allocate these dynamically
//       if an sd card is detected. This will save approx 260 bytes of RAM
//       when no sdcard was being used.
// static SD_HandleTypeDef g_sd;
// static DMA_HandleTypeDef sd_rx_dma, sd_tx_dma;

/* State in Card driver. */

typedef sd_card_t SD_HandleTypeDef;
// g_sd must NOT be put in cacheable RAM
sd_card_t g_sd;

#define PIN6_IDX                         6u   /*!< Pin number for pin 6 in a port */
#define PIN7_IDX                         7u   /*!< Pin number for pin 7 in a port */
#define PIN8_IDX                         8u   /*!< Pin number for pin 8 in a port */
#define PIN9_IDX                         9u   /*!< Pin number for pin 9 in a port */
#define PIN10_IDX                       10u   /*!< Pin number for pin 10 in a port */
#define PIN11_IDX                       11u   /*!< Pin number for pin 11 in a port */
#define PIN27_IDX                       27u   /*!< Pin number for pin 27 in a port */

void _sdcard_pin_init(void)
{
  CLOCK_EnableClock(kCLOCK_PortD);                           /* Clock Gate Control: 0x01u */

  const port_pin_config_t portc27_pinP6_config = {
    kPORT_PullDisable,                                       /* Internal pull-up/down resistor is disabled */
    kPORT_FastSlewRate,                                      /* Fast slew rate is configured */
    kPORT_PassiveFilterDisable,                              /* Passive filter is disabled */
    kPORT_OpenDrainDisable,                                  /* Open drain is disabled */
    kPORT_LowDriveStrength,                                  /* Low drive strength is configured */
    kPORT_MuxAsGpio,                                         /* Pin is configured as PTC27 */
    kPORT_UnlockRegister                                     /* Pin Control Register fields [15:0] are not locked */
  };
  PORT_SetPinConfig(PORTC, PIN27_IDX, &portc27_pinP6_config); /* PORTC27 (pin P6) is configured as PTC27 */
  const port_pin_config_t portd10_pinP11_config = {
    kPORT_PullUp,                                            /* Internal pull-up resistor is enabled */
    kPORT_FastSlewRate,                                      /* Fast slew rate is configured */
    kPORT_PassiveFilterDisable,                              /* Passive filter is disabled */
    kPORT_OpenDrainDisable,                                  /* Open drain is disabled */
    kPORT_LowDriveStrength,                                  /* Low drive strength is configured */
    kPORT_MuxAlt2,                                           /* Pin is configured as SDHC0_D3 */
    kPORT_UnlockRegister                                     /* Pin Control Register fields [15:0] are not locked */
  };
  PORT_SetPinConfig(PORTD, PIN10_IDX, &portd10_pinP11_config); /* PORTD10 (pin P11) is configured as SDHC0_D3 */
  const port_pin_config_t portd11_pinR11_config = {
    kPORT_PullUp,                                            /* Internal pull-up resistor is enabled */
    kPORT_FastSlewRate,                                      /* Fast slew rate is configured */
    kPORT_PassiveFilterDisable,                              /* Passive filter is disabled */
    kPORT_OpenDrainDisable,                                  /* Open drain is disabled */
    kPORT_LowDriveStrength,                                  /* Low drive strength is configured */
    kPORT_MuxAlt2,                                           /* Pin is configured as SDHC0_D2 */
    kPORT_UnlockRegister                                     /* Pin Control Register fields [15:0] are not locked */
  };
  PORT_SetPinConfig(PORTD, PIN11_IDX, &portd11_pinR11_config); /* PORTD11 (pin R11) is configured as SDHC0_D2 */
  const port_pin_config_t portd6_pinU9_config = {
    kPORT_PullUp,                                            /* Internal pull-up resistor is enabled */
    kPORT_FastSlewRate,                                      /* Fast slew rate is configured */
    kPORT_PassiveFilterDisable,                              /* Passive filter is disabled */
    kPORT_OpenDrainDisable,                                  /* Open drain is disabled */
    kPORT_LowDriveStrength,                                  /* Low drive strength is configured */
    kPORT_MuxAlt2,                                           /* Pin is configured as SDHC0_D1 */
    kPORT_UnlockRegister                                     /* Pin Control Register fields [15:0] are not locked */
  };
  PORT_SetPinConfig(PORTD, PIN6_IDX, &portd6_pinU9_config);  /* PORTD6 (pin U9) is configured as SDHC0_D1 */
  const port_pin_config_t portd7_pinP10_config = {
    kPORT_PullUp,                                            /* Internal pull-up resistor is enabled */
    kPORT_FastSlewRate,                                      /* Fast slew rate is configured */
    kPORT_PassiveFilterDisable,                              /* Passive filter is disabled */
    kPORT_OpenDrainDisable,                                  /* Open drain is disabled */
    kPORT_LowDriveStrength,                                  /* Low drive strength is configured */
    kPORT_MuxAlt2,                                           /* Pin is configured as SDHC0_D0 */
    kPORT_UnlockRegister                                     /* Pin Control Register fields [15:0] are not locked */
  };
  PORT_SetPinConfig(PORTD, PIN7_IDX, &portd7_pinP10_config); /* PORTD7 (pin P10) is configured as SDHC0_D0 */
  const port_pin_config_t portd8_pinT9_config = {
    kPORT_PullUp,                                            /* Internal pull-up resistor is enabled */
    kPORT_FastSlewRate,                                      /* Fast slew rate is configured */
    kPORT_PassiveFilterDisable,                              /* Passive filter is disabled */
    kPORT_OpenDrainDisable,                                  /* Open drain is disabled */
    kPORT_LowDriveStrength,                                  /* Low drive strength is configured */
    kPORT_MuxAlt2,                                           /* Pin is configured as SDHC0_DCLK */
    kPORT_UnlockRegister                                     /* Pin Control Register fields [15:0] are not locked */
  };
  PORT_SetPinConfig(PORTD, PIN8_IDX, &portd8_pinT9_config);  /* PORTD8 (pin T9) is configured as SDHC0_DCLK */
  const port_pin_config_t portd9_pinU11_config = {
    kPORT_PullUp,                                            /* Internal pull-up resistor is enabled */
    kPORT_FastSlewRate,                                      /* Fast slew rate is configured */
    kPORT_PassiveFilterDisable,                              /* Passive filter is disabled */
    kPORT_OpenDrainDisable,                                  /* Open drain is disabled */
    kPORT_LowDriveStrength,                                  /* Low drive strength is configured */
    kPORT_MuxAlt2,                                           /* Pin is configured as SDHC0_CMD */
    kPORT_UnlockRegister                                     /* Pin Control Register fields [15:0] are not locked */
  };
  PORT_SetPinConfig(PORTD, PIN9_IDX, &portd9_pinU11_config); /* PORTD9 (pin U11) is configured as SDHC0_CMD */
}

const scg_lpfll_config_t g_appScgLpFllConfig_BOARD_BootClockRUN = {
    .enableMode = kSCG_LpFllEnable, /* LPFLL clock disabled */
    .div1 = kSCG_AsyncClkDivBy1,    /* Low Power FLL Clock Divider 1: Clock output is disabled */
    .div2 = kSCG_AsyncClkDisable,   /* Low Power FLL Clock Divider 2: Clock output is disabled */
    .div3 = kSCG_AsyncClkDisable,   /* Low Power FLL Clock Divider 3: Clock output is disabled */
    .range = kSCG_LpFllRange72M,    /* LPFLL is trimmed to 72MHz */
    .trimConfig = NULL,
};

/*! @brief Data block count accessed in card */
#define DATA_BLOCK_COUNT (5U)
/*! @brief Start data block number accessed in card */
#define DATA_BLOCK_START (2U)
/*! @brief Data buffer size */
#define DATA_BUFFER_SIZE (FSL_SDMMC_DEFAULT_BLOCK_SIZE * DATA_BLOCK_COUNT)

static void USDHCClockConfiguration(void)
{
	CLOCK_InitLpFll(&g_appScgLpFllConfig_BOARD_BootClockRUN);
    /* set SDHC0 clock source */
    CLOCK_SetIpSrc(kCLOCK_Sdhc0, kCLOCK_IpSrcLpFllAsync);
    CLOCK_SetIpSrc(kCLOCK_Lpit0, kCLOCK_IpSrcFircAsync);
}

bool CardDetect()
{
	return GPIO_ReadPinInput(GPIOC, 27);
}

extern uint32_t HAL_GetTick();
status_t sdcard_init(void) {
    // invalidate the g_sd
	// sdtest_main();
    g_sd.isHostReady = 0;
	
	_sdcard_pin_init();
	USDHCClockConfiguration();

    status_t error = kStatus_Success;

    g_sd.host.base = SD_HOST_BASEADDR;
    g_sd.host.sourceClock_Hz = SD_HOST_CLK_FREQ;


	/* wait card insert */
	uint32_t t1 = HAL_GetTick();
	PRINTF("Please insert the sdcard in 10s if needed!\r\n");
	while (!CardDetect())
	{
		// wait 5s to insert the sdcard if need
		if (( HAL_GetTick() -  t1 ) > 10000)
		{
			PRINTF("Boot without sdcard!\r\n");
			return kWithout_Sdcard;
		}
	}

	PRINTF("Card inserted.\r\n");
    /* Init card. */
    if (SD_Init(&g_sd))
    {
        mp_print_str(MP_PYTHON_PRINTER, "\n SD card init failed \n");
        error = kStatus_Fail;
    } else {
		
    }
    return error;
}

status_t sdcard_deinit(void)
{
	SD_Deinit(&g_sd);
	return kStatus_Success;
}

void HAL_SD_MspInit(SD_HandleTypeDef *hsd) {
	// todo: low level init
}

void HAL_SD_MspDeInit(SD_HandleTypeDef *hsd) {
	// todo: low level deinit
}

bool sdcard_is_present(void) {
	if (CardDetect() && g_sd.isHostReady)
		return 1;
	return 0;}

bool sdcard_power_on(void) {
    if (!sdcard_is_present()) {
        return false;
    }
    if (g_sd.isHostReady) {
        return true;
    }
	if (sdcard_init() == kStatus_Success)
		return 1;
	sdcard_deinit();
    return 0;
}

void sdcard_power_off(void) {
    if (!g_sd.isHostReady) {
        return;
    }
    SD_Deinit(&g_sd);
}

uint32_t sdcard_get_lba_count(void)
{
	return g_sd.blockCount;
}

uint64_t sdcard_get_capacity_in_bytes(void) {
    if (g_sd.isHostReady == 0) {
        return 0;
    }
	return (uint64_t)g_sd.blockCount * g_sd.blockSize;
}

mp_uint_t sdcard_read_blocks(uint8_t *dest, uint32_t block_num, uint32_t num_blocks) {
    // check that SD card is initialised
    status_t ret;
    if (g_sd.isHostReady == 0) {
        return kStatus_Fail;
    }
	ret = SD_ReadBlocks(&g_sd, dest, block_num, num_blocks);
    return ret;
}

mp_uint_t sdcard_write_blocks(const uint8_t *src, uint32_t block_num, uint32_t num_blocks) {
    // check that SD card is initialised
    // check that SD card is initialised
    status_t ret;
    if (g_sd.isHostReady == 0) {
        return kStatus_Fail;
    }	
	ret = SD_WriteBlocks(&g_sd, src, block_num, num_blocks);
    return ret;
}

/******************************************************************************/
// Micro Python bindings
//
// Expose the SD card as an object with the block protocol.

// there is a singleton SDCard object
const mp_obj_base_t pyb_sdcard_obj = {&pyb_sdcard_type};

STATIC mp_obj_t pyb_sdcard_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    // return singleton object
    return (mp_obj_t)&pyb_sdcard_obj;
}

STATIC mp_obj_t sd_present(mp_obj_t self) {
    return mp_obj_new_bool(sdcard_is_present());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(sd_present_obj, sd_present);

STATIC mp_obj_t sd_power(mp_obj_t self, mp_obj_t state) {
    bool result;
    if (mp_obj_is_true(state)) {
        result = sdcard_power_on();
    } else {
        sdcard_power_off();
        result = true;
    }
    return mp_obj_new_bool(result);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(sd_power_obj, sd_power);

STATIC mp_obj_t sd_info(mp_obj_t self) {
    if (g_sd.isHostReady == 0 || CardDetect() == false) {
        return mp_const_none;
    }
    // cardinfo.SD_csd and cardinfo.SD_cid have lots of info but we don't use them
    mp_obj_t tuple[3] = {
        mp_obj_new_int_from_ull(g_sd.blockCount * g_sd.blockSize),
        mp_obj_new_int_from_uint(g_sd.blockSize),
        mp_obj_new_int(g_sd.busClock_Hz / 1000),
    };
    return mp_obj_new_tuple(3, tuple);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(sd_info_obj, sd_info);

// now obsolete, kept for backwards compatibility
STATIC mp_obj_t sd_read(mp_obj_t self, mp_obj_t block_num) {
    uint8_t *dest = m_new(uint8_t, SDCARD_BLOCK_SIZE);
    mp_uint_t ret = sdcard_read_blocks(dest, mp_obj_get_int(block_num), 1);

    if (ret != 0) {
        m_del(uint8_t, dest, SDCARD_BLOCK_SIZE);
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_Exception, "sdcard_read_blocks failed [%u]", ret));
    }

    return mp_obj_new_bytearray_by_ref(SDCARD_BLOCK_SIZE, dest);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(sd_read_obj, sd_read);

// now obsolete, kept for backwards compatibility
STATIC mp_obj_t sd_write(mp_obj_t self, mp_obj_t block_num, mp_obj_t data) {
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(data, &bufinfo, MP_BUFFER_READ);
    if (bufinfo.len % SDCARD_BLOCK_SIZE != 0) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "writes must be a multiple of %d bytes", SDCARD_BLOCK_SIZE));
    }

    mp_uint_t ret = sdcard_write_blocks(bufinfo.buf, mp_obj_get_int(block_num), bufinfo.len / SDCARD_BLOCK_SIZE);

    if (ret != 0) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_Exception, "sdcard_write_blocks failed [%u]", ret));
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(sd_write_obj, sd_write);

STATIC mp_obj_t pyb_sdcard_readblocks(mp_obj_t self, mp_obj_t block_num, mp_obj_t buf) {
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf, &bufinfo, MP_BUFFER_WRITE);
    mp_uint_t ret = sdcard_read_blocks(bufinfo.buf, mp_obj_get_int(block_num), bufinfo.len / SDCARD_BLOCK_SIZE);
    return mp_obj_new_bool(ret == 0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(pyb_sdcard_readblocks_obj, pyb_sdcard_readblocks);

STATIC mp_obj_t pyb_sdcard_writeblocks(mp_obj_t self, mp_obj_t block_num, mp_obj_t buf) {
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf, &bufinfo, MP_BUFFER_READ);
    mp_uint_t ret = sdcard_write_blocks(bufinfo.buf, mp_obj_get_int(block_num), bufinfo.len / SDCARD_BLOCK_SIZE);
    return mp_obj_new_bool(ret == 0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(pyb_sdcard_writeblocks_obj, pyb_sdcard_writeblocks);

STATIC mp_obj_t pyb_sdcard_ioctl(mp_obj_t self, mp_obj_t cmd_in, mp_obj_t arg_in) {
    mp_int_t cmd = mp_obj_get_int(cmd_in);
    switch (cmd) {
        case BP_IOCTL_INIT:
            if (!sdcard_power_on()) {
                return MP_OBJ_NEW_SMALL_INT(-1); // error
            }
            return MP_OBJ_NEW_SMALL_INT(0); // success

        case BP_IOCTL_DEINIT:
            sdcard_power_off();
            return MP_OBJ_NEW_SMALL_INT(0); // success

        case BP_IOCTL_SYNC:
            // nothing to do
            return MP_OBJ_NEW_SMALL_INT(0); // success

        case BP_IOCTL_SEC_COUNT:
            return MP_OBJ_NEW_SMALL_INT(0); // TODO

        case BP_IOCTL_SEC_SIZE:
            return MP_OBJ_NEW_SMALL_INT(SDCARD_BLOCK_SIZE);

        default: // unknown command
            return MP_OBJ_NEW_SMALL_INT(-1); // error
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(pyb_sdcard_ioctl_obj, pyb_sdcard_ioctl);

STATIC const mp_rom_map_elem_t pyb_sdcard_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_present), MP_ROM_PTR(&sd_present_obj) },
    { MP_ROM_QSTR(MP_QSTR_power), MP_ROM_PTR(&sd_power_obj) },
    { MP_ROM_QSTR(MP_QSTR_info), MP_ROM_PTR(&sd_info_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&sd_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&sd_write_obj) },
    // block device protocol
    { MP_ROM_QSTR(MP_QSTR_readblocks), MP_ROM_PTR(&pyb_sdcard_readblocks_obj) },
    { MP_ROM_QSTR(MP_QSTR_writeblocks), MP_ROM_PTR(&pyb_sdcard_writeblocks_obj) },
    { MP_ROM_QSTR(MP_QSTR_ioctl), MP_ROM_PTR(&pyb_sdcard_ioctl_obj) },
};

STATIC MP_DEFINE_CONST_DICT(pyb_sdcard_locals_dict, pyb_sdcard_locals_dict_table);

const mp_obj_type_t pyb_sdcard_type = {
    { &mp_type_type },
    .name = MP_QSTR_SDCard,
    .make_new = pyb_sdcard_make_new,
    .locals_dict = (mp_obj_dict_t*)&pyb_sdcard_locals_dict,
};

void sdcard_init_vfs(fs_user_mount_t *vfs, int part) {
    vfs->base.type = &mp_fat_vfs_type;
    vfs->flags |= FSUSER_NATIVE | FSUSER_HAVE_IOCTL;
    vfs->fatfs.drv = vfs;
    vfs->fatfs.part = part;
    vfs->readblocks[0] = (mp_obj_t)&pyb_sdcard_readblocks_obj;
    vfs->readblocks[1] = (mp_obj_t)&pyb_sdcard_obj;
    vfs->readblocks[2] = (mp_obj_t)sdcard_read_blocks; // native version
    vfs->writeblocks[0] = (mp_obj_t)&pyb_sdcard_writeblocks_obj;
    vfs->writeblocks[1] = (mp_obj_t)&pyb_sdcard_obj;
    vfs->writeblocks[2] = (mp_obj_t)sdcard_write_blocks; // native version
    vfs->u.ioctl[0] = (mp_obj_t)&pyb_sdcard_ioctl_obj;
    vfs->u.ioctl[1] = (mp_obj_t)&pyb_sdcard_obj;
}

#endif // MICROPY_HW_HAS_SDCARD
