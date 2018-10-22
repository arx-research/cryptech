CRYPTLIB_DIR := /Users/sra/cryptlib/cryptlib-3.4.2

CFLAGS	+= -g -I${CRYPTLIB_DIR} -DAES_KEY_WRAP_SELF_TEST
LDFLAGS += -g -L${CRYPTLIB_DIR} -lcl

EXE := aes_key_wrap
SRC := $(wildcard *.c)
OBJ := $(SRC:.c=.o)

all: ${EXE}

clean:
	rm -f *.o ${EXE}

${EXE}: ${OBJ}
	${CC} ${LDFLAGS} -o $@ $^

aes_key_wrap.o = aes_key_wrap.c aes_key_wrap.h

test: ${EXE}
	./${EXE}

