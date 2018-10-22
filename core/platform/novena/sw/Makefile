CC = gcc
AR = ar
CFLAGS = -Wall -fPIC

LIB = libcryptech.a
BIN = hash hash_tester trng_extractor trng_tester aes_tester modexp_tester modexps6_tester devmem3
INC = cryptech.h

PREFIX = /usr/local
LIB_DIR = $(PREFIX)/lib
BIN_DIR = $(PREFIX)/bin
INC_DIR = $(PREFIX)/include

all: $(LIB) $(BIN)

%.o: %.c $(INC)
	$(CC) $(CFLAGS) -c -o $@ $<

libcryptech.a: tc_eim.o novena-eim.o capability.o
	$(AR) rcs $@ $^

hash_tester: hash_tester.o $(LIB)
	$(CC) -o $@ $^

trng_tester: trng_tester.o $(LIB)
	$(CC) -o $@ $^

aes_tester: aes_tester.o $(LIB)
	$(CC) -o $@ $^

modexp_tester: modexp_tester.o $(LIB)
	$(CC) -o $@ $^

modexps6_tester: modexps6_tester.o $(LIB)
	$(CC) -o $@ $^

hash: hash.o $(LIB)
	$(CC) -o $@ $^

trng_extractor: trng_extractor.o $(LIB)
	$(CC) -o $@ $^

devmem3: devmem3.o $(LIB)
	$(CC) -o $@ $^

install: $(LIB) $(BIN) $(INC)
	install $(LIB) $(LIB_DIR)
	install $(BIN) configure-fpga.sh $(BIN_DIR)
	install $(INC) $(INC_DIR)

uninstall:
	rm -f $(LIB_DIR)/$(LIB)
	rm -f $(foreach bin,$(BIN) configure-fpga.sh,$(BIN_DIR)/$(bin))
	rm -f $(INC_DIR)/$(INC)

clean:
	rm -f *.o $(LIB) $(BIN)
