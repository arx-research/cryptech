PROG = bootloader

OBJS = dfu.o log.o

BOARD_OBJS = \
	./stm-init.o \
	$(TOPLEVEL)/stm-fmc.o \
	$(TOPLEVEL)/stm-uart.o \
	$(TOPLEVEL)/spiflash_n25q128.o \
	$(TOPLEVEL)/stm-keystore.o \
	$(TOPLEVEL)/stm-flash.o \
	$(TOPLEVEL)/syscalls.o \
	$(BOARD_DIR)/system_stm32f4xx.o \
	$(BOARD_DIR)/stm32f4xx_hal_msp.o \
	./startup_stm32f429xx.o \
	$(BOARD_DIR)/stm32f4xx_it.o

CFLAGS += -I$(LIBHAL_SRC)
LIBS += $(LIBHAL_BLD)/libhal.a $(LIBTFM_BLD)/libtfm.a

all: $(PROG:=.elf)

%.elf: %.o $(BOARD_OBJS) $(OBJS) $(LIBS)
	$(CC) $(CFLAGS) $^ -o $@ -T$(BOOTLOADER_LDSCRIPT) -g -Wl,-Map=$*.map
	$(OBJCOPY) -O ihex $*.elf $*.hex
	$(OBJCOPY) -O binary $*.elf $*.bin
	$(OBJDUMP) -St $*.elf >$*.lst
	$(SIZE) $*.elf

clean:
	rm -f *.o
	rm -f *.elf
	rm -f *.hex
	rm -f *.bin
	rm -f *.map
	rm -f *.lst
