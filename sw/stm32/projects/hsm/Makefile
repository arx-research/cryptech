PROJ = hsm

# objs in addition to $(PROJ).o
OBJS =	mgmt-cli.o \
	mgmt-firmware.o \
	mgmt-bootloader.o \
	mgmt-fpga.o \
	mgmt-keystore.o \
	mgmt-masterkey.o \
	mgmt-misc.o \
	mgmt-task.o \
	log.o \
	$(TOPLEVEL)/task.o

CFLAGS += -DNUM_RPC_TASK=4

CFLAGS += -I$(LIBHAL_SRC)
CFLAGS += -I$(LIBCLI_SRC)

LIBS += $(LIBHAL_BLD)/libhal.a $(LIBTFM_BLD)/libtfm.a
LIBS += $(LIBCLI_BLD)/libcli.a

LDFLAGS += -mcpu=cortex-m4 -mthumb -mlittle-endian -mthumb-interwork
LDFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
LDFLAGS += -Wl,--gc-sections

ifdef DO_PROFILING
OBJS += $(TOPLEVEL)/memfunc.o
LDFLAGS += --specs=rdimon.specs -lc -lrdimon
endif

ifdef DO_TASK_METRICS
CFLAGS += -DDO_TASK_METRICS
endif

all: $(PROJ:=.elf)

%.elf: %.o $(BOARD_OBJS) $(OBJS) $(LIBS)
	$(CC) $^ -o $@ -T$(LDSCRIPT) -g -Wl,-Map=$*.map $(LDFLAGS)
	$(OBJCOPY) -O binary $*.elf $*.bin
	$(SIZE) $*.elf

clean:
	rm -f *.o
	rm -f *.elf
	rm -f *.bin
	rm -f *.map
