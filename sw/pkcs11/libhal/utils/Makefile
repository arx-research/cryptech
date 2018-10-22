ifndef CRYPTECH_ROOT
  CRYPTECH_ROOT := $(abspath ../../../..)
endif

LIBHAL_SRC	?= ${CRYPTECH_ROOT}/sw/libhal
LIBHAL_BLD	?= $(abspath ..)

LIBTFM_SRC	?= ${CRYPTECH_ROOT}/sw/thirdparty/libtfm
LIBTFM_BLD	?= $(abspath ../../libtfm)

vpath %.c ${LIBHAL_SRC}/utils
vpath %.h ${LIBHAL_SRC}/utils:${LIBTFM_BLD}

include ${LIBHAL_SRC}/utils/Makefile
