#
# Thanks to Tom Van den Bon for cleaning up the STM templates into
# something easily built with GCC at
#
#   https://github.com/tomvdb/stm32f401-nucleo-basic-template
#

TOPLEVEL ?= .

# Location of the Libraries folder from the STM32F0xx Standard Peripheral Library
STD_PERIPH_LIB ?= $(TOPLEVEL)/Drivers

# Location of the linker scripts
LDSCRIPT_INC ?= $(TOPLEVEL)/Device/ldscripts

# location of OpenOCD Board .cfg files (only used with 'make flash-target')
#
# This path is from Ubuntu 14.04.
#
OPENOCD_BOARD_DIR ?= /usr/share/openocd/scripts/board

# Configuration (cfg) file containing programming directives for OpenOCD
#
# If you are using an STLINK v2.0 from an STM32 F4 DISCOVERY board,
# set this variable to "stm32f4discovery.cfg".
#
# If you are using a NUCLEO board, set it to "stm32f4-openocd.cfg" (tomvdb) or
# "board/st_nucleo_f401re.cfg" I guess.
#
# If you are using something else, look for a matching configuration file in
# the OPENOCD_BOARD_DIR directory.
#
OPENOCD_PROC_FILE ?= stm32f4discovery.cfg

# MCU selection parameters
#
STDPERIPH_SETTINGS ?= -DUSE_STDPERIPH_DRIVER -DSTM32F4XX -DSTM32F429xx
#
# For the dev-bridge rev01 board, use stm32f429bitx.ld.
MCU_LINKSCRIPT ?= stm32f429bitx.ld

# add startup file to build
#
# For the dev-bridge rev01 board, use startup_stm32f429xx.s.
SRCS += $(TOPLEVEL)/Device/startup_stm32f429xx.s
SRCS += $(TOPLEVEL)/Device/system_stm32f4xx.c

# that's it, no need to change anything below this line!

###################################################

CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy
OBJDUMP=arm-none-eabi-objdump
SIZE=arm-none-eabi-size
GDB=arm-none-eabi-gdb
OPENOCD=openocd

CFLAGS  = -ggdb -O2 -Wall -Wextra -Warray-bounds
CFLAGS += -mcpu=cortex-m4 -mthumb -mlittle-endian -mthumb-interwork
CFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
CFLAGS += $(STDPERIPH_SETTINGS)
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -Wl,--gc-sections -Wl,-Map=$(PROJ_NAME).map

###################################################

vpath %.c src
vpath %.a $(STD_PERIPH_LIB)

ROOT=$(shell pwd)

CFLAGS += -I include -I $(STD_PERIPH_LIB) -I $(STD_PERIPH_LIB)/CMSIS/Device/ST/STM32F4xx/Include
CFLAGS += -I $(STD_PERIPH_LIB)/CMSIS/Include -I $(STD_PERIPH_LIB)/STM32F4xx_HAL_Driver/Inc
#CFLAGS += -include $(STD_PERIPH_LIB)/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_conf.h
