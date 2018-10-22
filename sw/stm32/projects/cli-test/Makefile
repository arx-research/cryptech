TEST = cli-test

OBJS = \
	mgmt-cli.o \
	mgmt-dfu.o \
	mgmt-fpga.o \
	mgmt-keystore.o \
	mgmt-masterkey.o \
	mgmt-misc.o \
	mgmt-show.o \
	mgmt-test.o \
	test-fmc.o \
	test-mkmif.o \
	test_sdram.o

CFLAGS += -I$(LIBCLI_SRC) -I$(LIBHAL_SRC)

LIBS += $(LIBCLI_BLD)/libcli.a $(LIBHAL_BLD)/libhal.a $(LIBTFM_BLD)/libtfm.a

all: $(TEST:=.elf)

%.elf: %.o $(BOARD_OBJS) $(OBJS) $(LIBS)
	$(CC) $(CFLAGS) $^ -o $@ -T$(LDSCRIPT) -g -Wl,-Map=$*.map
	$(OBJCOPY) -O binary $*.elf $*.bin
	$(SIZE) $*.elf

clean:
	rm -f *.o
	rm -f *.elf
	rm -f *.bin
	rm -f *.map
