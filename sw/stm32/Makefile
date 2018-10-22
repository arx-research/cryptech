# Copyright (c) 2015-2017, NORDUnet A/S
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
# - Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
#
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# - Neither the name of the NORDUnet nor the names of its contributors may
#   be used to endorse or promote products derived from this software
#   without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
# TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# A couple features that can be enabled at build time, but are not turned on
# by default:
# DO_PROFILING: Enable gmon profiling. See libraries/libprof/README.md for
# more details.
# DO_TASK_METRICS: Enable task metrics - average/max time between yields. This
# can be helpful when experimentally adding yields to improve responsiveness.
#
# To enable, run `make DO_PROFILING=1 DO_TASK_METRICS=1`
# (or DO_PROFILING=xyzzy - `make` just cares that the symbol is defined)

# export all variables to child processes by default
.EXPORT_ALL_VARIABLES:

# absolute path, because we're going to be passing things to sub-makes
TOPLEVEL = $(abspath .)
CRYPTECH_ROOT = $(abspath ../..)

# define board: dev-bridge or alpha
BOARD = TARGET_CRYPTECH_ALPHA

LIBS_DIR = $(TOPLEVEL)/libraries
MBED_DIR = $(LIBS_DIR)/mbed
CMSIS_DIR = $(MBED_DIR)/targets/cmsis/TARGET_STM/TARGET_STM32F4
BOARD_DIR = $(CMSIS_DIR)/$(BOARD)

LIBHAL_SRC = $(CRYPTECH_ROOT)/sw/libhal
LIBHAL_BLD = $(LIBS_DIR)/libhal

LIBCLI_SRC = $(CRYPTECH_ROOT)/user/paul/libcli
LIBCLI_BLD = $(LIBS_DIR)/libcli

LIBTFM_SRC = $(CRYPTECH_ROOT)/sw/thirdparty/libtfm
LIBTFM_BLD = $(LIBS_DIR)/libtfm

LIBPROF_SRC = $(LIBS_DIR)/libprof
LIBPROF_BLD = $(LIBS_DIR)/libprof

LIBS = $(MBED_DIR)/libstmf4.a

# linker script
LDSCRIPT = $(BOARD_DIR)/TOOLCHAIN_GCC_ARM/STM32F429BI.ld
BOOTLOADER_LDSCRIPT = $(BOARD_DIR)/TOOLCHAIN_GCC_ARM/STM32F429BI_bootloader.ld

# board-specific objects, to link into every project
BOARD_OBJS = \
	$(TOPLEVEL)/stm-init.o \
	$(TOPLEVEL)/stm-fmc.o \
	$(TOPLEVEL)/stm-uart.o \
	$(TOPLEVEL)/syscalls.o \
	$(BOARD_DIR)/TOOLCHAIN_GCC_ARM/startup_stm32f429xx.o \
	$(BOARD_DIR)/system_stm32f4xx.o \
	$(BOARD_DIR)/stm32f4xx_hal_msp.o \
	$(BOARD_DIR)/stm32f4xx_it.o
ifeq (${BOARD},TARGET_CRYPTECH_ALPHA)
BOARD_OBJS += \
	$(TOPLEVEL)/stm-rtc.o \
	$(TOPLEVEL)/spiflash_n25q128.o \
	$(TOPLEVEL)/stm-fpgacfg.o \
	$(TOPLEVEL)/stm-keystore.o \
	$(TOPLEVEL)/stm-sdram.o \
	$(TOPLEVEL)/stm-flash.o
endif

# cross-building tools
PREFIX=arm-none-eabi-
CC=$(PREFIX)gcc
AS=$(PREFIX)as
AR=$(PREFIX)ar
OBJCOPY=$(PREFIX)objcopy
OBJDUMP=$(PREFIX)objdump
SIZE=$(PREFIX)size

# The Alpha is a development platform, so set GCC optimization to a
# level suitable for debugging.  Recent versions of GCC have a special
# optimization setting -Og for exactly this purpose, so we use it,
# along with the flag to enable gdb symbols.  Note that some libraries
# (in particular, libtfm) may need different optimization settings,
# which is why this needs to remain a separate makefile variable.
#
# If you really want optimization without debugging support, try -O2
# or -O3.

STM32_CFLAGS_OPTIMIZATION ?= -ggdb -Og

# whew, that's a lot of cflags
CFLAGS  = $(STM32_CFLAGS_OPTIMIZATION) -Wall -Warray-bounds -Wextra
CFLAGS += -mcpu=cortex-m4 -mthumb -mlittle-endian -mthumb-interwork
CFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
CFLAGS += -DUSE_STDPERIPH_DRIVER -DSTM32F4XX -DSTM32F429xx
CFLAGS += -D__CORTEX_M4 -DTARGET_STM -DTARGET_STM32F4 -DTARGET_STM32F429ZI -DTOOLCHAIN_GCC -D__FPU_PRESENT=1 -D$(BOARD)
CFLAGS += -DENABLE_WEAK_FUNCTIONS
CFLAGS += -ffunction-sections -fdata-sections -Wl,--gc-sections
CFLAGS += -std=c99
CFLAGS += -I$(TOPLEVEL)
CFLAGS += -I$(MBED_DIR)/api
CFLAGS += -I$(MBED_DIR)/targets/cmsis
CFLAGS += -I$(MBED_DIR)/targets/cmsis/TARGET_STM/TARGET_STM32F4
CFLAGS += -I$(MBED_DIR)/targets/cmsis/TARGET_STM/TARGET_STM32F4/$(BOARD)
CFLAGS += -I$(MBED_DIR)/targets/hal/TARGET_STM/TARGET_STM32F4
CFLAGS += -I$(MBED_DIR)/targets/hal/TARGET_STM/TARGET_STM32F4/$(BOARD)
ifdef DO_TASK_METRICS
CFLAGS += -DDO_TASK_METRICS
endif

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o : %.S
	$(CC) $(CFLAGS) -c -o $@ $<

ifdef DO_PROFILING
CFLAGS += -pg -DDO_PROFILING
LIBS += $(LIBPROF_BLD)/libprof.a
all: hsm
else
all: board-test cli-test libhal-test hsm bootloader
endif

$(MBED_DIR)/libstmf4.a: .FORCE
	$(MAKE) -C $(MBED_DIR)

board-test: $(BOARD_OBJS) $(LIBS) .FORCE
	$(MAKE) -C projects/board-test

cli-test: $(BOARD_OBJS) $(LIBS) $(LIBCLI_BLD)/libcli.a $(LIBHAL_BLD)/libhal.a .FORCE
	$(MAKE) -C projects/cli-test

$(LIBTFM_BLD)/libtfm.a: .FORCE
	$(MAKE) -C $(LIBTFM_BLD) PREFIX=$(PREFIX)

$(LIBHAL_BLD)/libhal.a: $(LIBTFM_BLD)/libtfm.a .FORCE
	$(MAKE) -C $(LIBHAL_BLD) IO_BUS=fmc RPC_MODE=server RPC_TRANSPORT=serial KS=flash libhal.a

$(LIBCLI_BLD)/libcli.a: .FORCE
	$(MAKE) -C $(LIBCLI_BLD)

$(LIBPROF_BLD)/libprof.a: .FORCE
	$(MAKE) -C $(LIBPROF_BLD)

libhal-test: $(BOARD_OBJS) $(LIBS) $(LIBHAL_BLD)/libhal.a .FORCE
	$(MAKE) -C projects/libhal-test

hsm: $(BOARD_OBJS) $(LIBS) $(LIBHAL_BLD)/libhal.a $(LIBCLI_BLD)/libcli.a .FORCE
	$(MAKE) -C projects/hsm

bootloader: $(BOARD_OBJS) $(LIBS) $(LIBHAL_BLD)/libhal.a .FORCE
	$(MAKE) -C projects/bootloader

# don't automatically delete objects, to avoid a lot of unnecessary rebuilding
.SECONDARY: $(BOARD_OBJS)

.PHONY: board-test libhal-test cli-test hsm bootloader

# We don't (and shouldn't) know enough about libraries and projects to
# know whether they need rebuilding or not, so we let their Makefiles
# decide that.  Which means we always need to run all the sub-makes.
# We could do this with .PHONY (which is supposedly more "efficient")
# but using a .FORCE target is simpler once one takes inter-library
# dependency specifications into account.

.FORCE:				# (sic)

clean:
	rm -f $(BOARD_OBJS)
	$(MAKE) -C $(LIBHAL_BLD) clean
	$(MAKE) -C projects/board-test clean
	$(MAKE) -C projects/cli-test clean
	$(MAKE) -C projects/libhal-test clean
	$(MAKE) -C projects/hsm clean
	$(MAKE) -C projects/bootloader clean

distclean: clean
	$(MAKE) -C $(MBED_DIR) clean
	$(MAKE) -C $(LIBTFM_BLD) clean
	$(MAKE) -C $(LIBCLI_BLD) clean
	$(MAKE) -C $(LIBPROF_BLD) clean
