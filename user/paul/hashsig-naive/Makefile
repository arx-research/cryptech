LIBHAL = ../../../sw/libhal
CFLAGS = -Wall -Wextra -g -I$(LIBHAL)

all: hashsig

hashsig: hashsig_naive.o xdr.o errorstrings.o
	$(CC) -o $@ $^ -lcrypto

xdr.o: $(LIBHAL)/xdr.c
	$(CC) -c $(CFLAGS) -o $@ $^

errorstrings.o: $(LIBHAL)/errorstrings.c
	$(CC) -c $(CFLAGS) -o $@ $^

clean:
	rm -f hashsig hashsig_naive.o xdr.o errorstrings.o
