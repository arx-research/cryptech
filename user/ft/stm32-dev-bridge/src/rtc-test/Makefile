# put your *.o targets here, make should handle the rest!
SRCS = main.c stm_init.c stm-uart.c stm32f4xx_it.c stm32f4xx_hal_msp.c

# all the files will be generated with this name
PROJ_NAME=rtc-test

TOPLEVEL=../..
include $(TOPLEVEL)/common.mk

OBJS = $(SRCS:.c=.o)

###################################################

.PHONY: lib proj

all: lib proj

lib:
	$(MAKE) -C $(STD_PERIPH_LIB) STDPERIPH_SETTINGS="$(STDPERIPH_SETTINGS) -I $(PWD)/include"

proj: 	$(PROJ_NAME).elf

$(PROJ_NAME).elf: $(SRCS)
	$(CC) $(CFLAGS) $^ -o $@ -L$(STD_PERIPH_LIB) -lstmf4 -L$(LDSCRIPT_INC) -T$(MCU_LINKSCRIPT) -g
	$(OBJCOPY) -O ihex $(PROJ_NAME).elf $(PROJ_NAME).hex
	$(OBJCOPY) -O binary $(PROJ_NAME).elf $(PROJ_NAME).bin
	$(OBJDUMP) -St $(PROJ_NAME).elf >$(PROJ_NAME).lst
	$(SIZE) $(PROJ_NAME).elf

clean:
	find ./ -name '*~' | xargs rm -f
	rm -f *.o
	rm -f $(PROJ_NAME).elf
	rm -f $(PROJ_NAME).hex
	rm -f $(PROJ_NAME).bin
	rm -f $(PROJ_NAME).map
	rm -f $(PROJ_NAME).lst

really-clean: clean
	$(MAKE) -C $(STD_PERIPH_LIB) clean

debug:
    $(GDB) -ex "target remote localhost:3333" \
        -ex "set remote hardware-breakpoint-limit 6" \
        -ex "set remote hardware-watchpoint-limit 4" $(PROJ_NAME).elf

flash-target:
	$(OPENOCD) -f $(OPENOCD_BOARD_DIR)/$(OPENOCD_PROC_FILE) \
		-c "program $(PROJ_NAME).elf verify reset"
