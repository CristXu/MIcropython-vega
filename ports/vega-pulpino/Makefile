include ../../py/mkenv.mk

# Point this to Vega-SDK
VEGA_SDK ?= /opt/eclipse/workplace/rv32m1_sdk_riscv
NXP_SWIM_DIR ?=./nxp_swim
AY_IMU_DIR ?= ./ay_imu
LCD_DIR ?= ./lcd
ICON_DIR ?= ./icon
# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h

# include py core make definitions
include $(TOP)/py/py.mk

CROSS_COMPILE = /opt/riscv32-unknown-elf-gcc/bin/riscv32-unknown-elf-

INC += -I.
INC += -I$(TOP)
INC += -I$(BUILD)
INC += -I$(NXP_SWIM_DIR)
INC += -I$(NXP_SWIM_DIR)/inc
INC += -I$(AY_IMU_DIR)
INC += -I$(AY_IMU_DIR)/core
INC += -I$(AY_IMU_DIR)/filter
INC += -I$(AY_IMU_DIR)/math
INC += -I$(LCD_DIR)
INC += -I$(ICON_DIR)
INC += -I$(VEGA_SDK)/RISCV
INC += -I$(VEGA_SDK)/devices
INC += -I$(VEGA_SDK)/devices/RV32M1
INC += -I$(VEGA_SDK)/devices/RV32M1/drivers
INC += -I$(VEGA_SDK)/devices/RV32M1/utilities
INC += -I$(TOP)/lib/oofatfs
INC += -I$(VEGA_SDK)/middleware/sdmmc_2.1.2/inc

TFLITE_INC += -I/opt/tensorflow-lite-riscv
TFLITE_INC += -I/opt/tensorflow-lite-riscv/third_party/flatbuffers/include

ASM_SRCS += $(VEGA_SDK)/devices/RV32M1/gcc/startup_RV32M1_ri5cy.S 

OP_LEVEL = -O0
DBFLAGS = -DNDEBUG -DCPU_RV32M1_ri5cy -DPRINTF_FLOAT_ENABLE=0 -DSCANF_FLOAT_ENABLE=0 -DPRINTF_ADVANCED_ENABLE=0 -DSCANF_ADVANCED_ENABLE=0 -DHIGH_LEVEL_OPTIMIZE=0
# CFLAGS for MP-file
CFLAGS = $(INC) $(DBFLAGS) $(OP_LEVEL) -msmall-data-limit=8 -mno-save-restore -flto -g -fmessage-length=0 -fsigned-char -fno-common -ffreestanding -fno-builtin -std=gnu99 -march=rv32imcxpulpv2 -Wa,-march=rv32imcxpulpv2 -fno-strict-aliasing
# LDFLAGS is for the input SRC_C
LDFLAGS = $(TFLITE_INC) -DTFLITE_MCU -T RV32M1_ri5cy_flash.ld -g -nostdlib -std=gnu++11 -flto $(OP_LEVEL) -fno-strict-aliasing -Wl,-Map,$(BUILD)/micropython.map -march=rv32imcxpulpv2 -Xlinker -static -Xlinker -z -Xlinker muldefs -Xlinker -static -Xlinker -z -Xlinker muldefs
LIBS = -lnosys -lgcc -lc -lm -lg -lstdc++ -lnosys -Xlinker --end-group 

SRC_C = \
	main.c \
	board.c \
	pin_mux.c \
	clock_config.c \
	test.c \
	irq.c \
	i2c.c \
	led.c \
	rng.c \
	spi.c \
	event.c \
	fatfs_port.c \
	fsl_host.c \
    storage.c \
	flash.c \
	sdcard.c \
	py_time.c \
	modpyb.c \
	systick.c \
	modmcu.c \
	moduos.c \
	usrsw.c \
	oled_128128.c \
	modmachine.c \
	mphalport.c \
	bufhelper.c \
	$(wildcard $(LCD_DIR)/*.c) \
	$(wildcard $(NXP_SWIM_DIR)/*.c) \
    $(NXP_SWIM_DIR)/drivers/lpc_oled_driver.c \
	$(wildcard $(AY_IMU_DIR)/core/*.c $(AY_IMU_DIR)/filter/*.c $(AY_IMU_DIR)/math/*.c) \
	$(AY_IMU_DIR)/py_imu.c \
	$(ICON_DIR)/icon.c \
	$(VEGA_SDK)/devices/RV32M1/drivers/fsl_common.c \
	$(VEGA_SDK)/devices/RV32M1/system_RV32M1_ri5cy.c \
	$(VEGA_SDK)/devices/RV32M1/drivers/fsl_msmc.c \
	$(VEGA_SDK)/devices/RV32M1/utilities/fsl_str.c \
	$(VEGA_SDK)/devices/RV32M1/utilities/fsl_io.c \
	$(VEGA_SDK)/devices/RV32M1/drivers/fsl_clock.c \
	$(VEGA_SDK)/devices/RV32M1/utilities/fsl_log.c \
	$(VEGA_SDK)/devices/RV32M1/utilities/fsl_debug_console.c \
	$(VEGA_SDK)/devices/RV32M1/drivers/fsl_gpio.c \
	$(VEGA_SDK)/devices/RV32M1/drivers/fsl_lpuart.c \
	$(VEGA_SDK)/devices/RV32M1/drivers/fsl_intmux.c \
	$(VEGA_SDK)/devices/RV32M1/drivers/fsl_lpi2c.c \
	$(VEGA_SDK)/devices/RV32M1/drivers/fsl_lpuart.c \
	$(VEGA_SDK)/devices/RV32M1/drivers/fsl_lpspi.c \
	$(VEGA_SDK)/devices/RV32M1/drivers/fsl_trng.c \
	$(VEGA_SDK)/devices/RV32M1/drivers/fsl_flash.c \
	$(VEGA_SDK)/devices/RV32M1/drivers/fsl_usdhc.c \
	$(VEGA_SDK)/middleware/sdmmc_2.1.2/src/fsl_sd.c \
	$(VEGA_SDK)/middleware/sdmmc_2.1.2/src/fsl_sdmmc.c \
	lib/utils/stdout_helpers.c \
	lib/utils/pyexec.c \
	lib/timeutils/timeutils.c \
	lib/utils/interrupt_char.c \
	lib/mp-readline/readline.c \
	lib/oofatfs/ff.c \
	lib/oofatfs/option/ccsbcs.c \
	extmod/vfs.c \
	$(wildcard extmod/vfs_*.c)

#SRC_CXX = \
#	tflite/resize_test.cc \
#	tflite/libtensorflow_lite.a 

OBJ = $(PY_O) $(addprefix $(BUILD)/, $(SRC_C:.c=.o)) $(addprefix $(BUILD)/, $(ASM_SRCS:.S=.o))

all: micropython

micropython: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(CXX) $(LDFLAGS) -o $(BUILD)/$@.elf -Xlinker --start-group $(OBJ) $(LIBS)
	$(Q)$(SIZE) $(BUILD)/$@.elf

# List of sources for qstr extraction
SRC_QSTR += $(SRC_C) mpconfigport.h
# Append any auto-generated sources that are needed by sources listed in
# SRC_QSTR
SRC_QSTR_AUTO_DEPS +=

include $(TOP)/py/mkrules.mk
