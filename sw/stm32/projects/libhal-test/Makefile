TEST = cores test-bus test-trng test-hash test-aes-key-wrap test-pbkdf2 test-ecdsa test-rsa test-mkmif

CFLAGS += -I $(LIBHAL_SRC)
LIBC_OBJS = printf.o gettimeofday.o
LIBS += $(LIBHAL_BLD)/libhal.a $(LIBTFM_BLD)/libtfm.a

all: $(TEST:=.elf)

vpath %.c $(LIBHAL_SRC)/tests $(LIBHAL_SRC)/utils

# .mo extension for files with main() that need to be wrapped as __main()
%.mo: %.c
	$(CC) -c $(CFLAGS) -Dmain=__main -o $@ $<

%.elf: %.mo main.o $(BOARD_OBJS) $(LIBC_OBJS) $(LIBS)
	$(CC) $(CFLAGS) $^ -o $*.elf -T$(LDSCRIPT) -g -Wl,-Map=$*.map
	$(OBJCOPY) -O binary $*.elf $*.bin
	$(SIZE) $*.elf

clean:
	rm -f *.o *.mo
	rm -f *.elf
	rm -f *.bin
	rm -f *.map
