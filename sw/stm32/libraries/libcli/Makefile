vpath %.c ${LIBCLI_SRC}
vpath %.h ${LIBCLI_SRC}

CFLAGS += \
	-DDO_CRYPT=0 \
	-DDO_FILE=0 \
	-DDO_FILTER=0 \
	-DDO_IDLE_TIMEOUT=0 \
	-DDO_MALLOC=0 \
	-DDO_PRINT_BUFFERED=0 \
	-DDO_REGULAR=0 \
	-DDO_SOCKET=0 \
	-DDO_TAB_COMPLETION=1 \
	-DDO_TELNET=0 \
	-DCLI_MAX_LINE_WORDS=36
CFLAGS += -Wno-unused-parameter

all: libcli.a

libcli.a: libcli.o
	$(AR) rcs $@ $^

clean:
	rm -f libcli.[ao]
