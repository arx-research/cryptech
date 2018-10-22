ifndef CRYPTECH_ROOT
  CRYPTECH_ROOT := $(abspath ../../..)
endif

LIBHAL_SRC   ?= ${CRYPTECH_ROOT}/sw/libhal
LIBTFM_BLD   ?= $(abspath ../libtfm)

CFLAGS += -I${LIBHAL_SRC}

vpath %.c ${LIBHAL_SRC}
vpath %.h ${LIBHAL_SRC}:${LIBTFM_BLD}

include ${LIBHAL_SRC}/Makefile
