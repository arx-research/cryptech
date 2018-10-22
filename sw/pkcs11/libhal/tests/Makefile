ifndef CRYPTECH_ROOT
  CRYPTECH_ROOT := $(abspath ../../../..)
endif

LIBHAL_SRC	?= ${CRYPTECH_ROOT}/sw/libhal
LIBHAL_BLD	?= $(abspath ..)

LIBTFM_SRC	?= ${CRYPTECH_ROOT}/sw/thirdparty/libtfm
LIBTFM_BLD	?= $(abspath ../../libtfm)

vpath %.c ${LIBHAL_SRC}/tests
vpath %.h ${LIBHAL_SRC}/tests:${LIBTFM_BLD}

include ${LIBHAL_SRC}/tests/Makefile
