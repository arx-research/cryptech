# (GNU) Makefile for Cryptech PKCS #11 implementation.
#
# Author: Rob Austein
# Copyright (c) 2015-2016, NORDUnet A/S
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
# - Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
#
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# - Neither the name of the NORDUnet nor the names of its contributors may
#   be used to endorse or promote products derived from this software
#   without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
# TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Locations of libraries on which this code depends.

ifndef CRYPTECH_ROOT
  CRYPTECH_ROOT := $(abspath ../..)
endif

PKCS11_DIR	?= ${CRYPTECH_ROOT}/sw/pkcs11

LIBHAL_SRC	?= ${CRYPTECH_ROOT}/sw/libhal
LIBHAL_BLD	?= ${PKCS11_DIR}/libhal

LIBTFM_SRC	?= ${CRYPTECH_ROOT}/sw/thirdparty/libtfm
LIBTFM_BLD	?= ${PKCS11_DIR}/libtfm

export LIBHAL_SRC LIBHAL_BLD LIBTFM_SRC LIBTFM_BLD

# Whether to enable threading.  Main reason for being able to turn it
# off is that gdb on the Novena (sometimes) goes bananas when
# threading is enabled.

ENABLE_THREADS ?= yes

# Whether to enable debugging code that prints diagnostic information
# to stderr on various conditions (mostly failures).

ENABLE_DEBUGGING ?= no

# Whether to disable #warning statements; generally these are present for
# a reason, but they can get distracting when one is attempting to debug
# something else.

ENABLE_FOOTNOTE_WARNINGS ?= yes

# Target platform for shared library.  Every platform has its own
# kinks, as does GNU libtool, so we just suck it up and do the
# necessary kinks for the platforms we support.  Yuck.

UNAME := $(shell uname)

# Compilation flags, etc.

CFLAGS	+= -g3 -fPIC -Wall -std=c99 -I${LIBHAL_SRC}
LIBS	:= ${LIBHAL_BLD}/libhal.a ${LIBTFM_BLD}/libtfm.a

# libhal supports two different methods of connecting to the HSM:
#
# * Directly, via the USB serial port (LIBHAL_TARGET = serial), or
#
# * Via a multiplexing daemon which listens on a PF_UNIX socket and
#   can interleave connections from multiple clients onto the single
#   USB serial port (LIBHAL_TARGET = daemon).
#
# Without the daemon, one can only have one PKCS #11 "application" at
# a time.  This is a problem for packages like OpenDNSSEC, which have
# multiple programs which want to be able to talk to the HSM at once,
# so the default is (now) daemon mode.
#
# The original RPC daemon was a C program using a protocol based on
# SOCK_SEQPACKET, which worked on Linux but not on OSX (Apple doesn't
# support SOCK_SEQPACKET).  The current RPC daemon is a Python program
# using SLIP framing over a SOCK_STREAM connection; since we were
# already using SLIP framing on the USB serial port, this is easy.
#
# Conceptually, the daemon is not really part of the conversation
# between libhal and the HSM, it's just a multiplexer.  In the long
# run, the traffic between libhal and the HSM will use some kind of
# secure channel protocol, which we'll probably want to run over a
# SOCK_STREAM connection in any case.

# Temporary kludge: attempt to detect whether the version of libhal
# we're using includes cryptech_muxd, and set LIBHAL_TARGET
# accordingly.  This should go away when all the current branches
# finally get merged back into master, but for the moment it's simpler
# to do this than to have to maintain a separate pymux branch of
# sw/pkcs11 whose sole difference from the ksng branch is the
# LIBHAL_TARGET setting.

#LIBHAL_TARGET := daemon
ifeq "$(wildcard ../libhal/cryptech_muxd)" ""
  LIBHAL_TARGET := serial
else
  LIBHAL_TARGET := daemon
endif

ifeq "${UNAME}" "Darwin"
  SONAME  := libcryptech-pkcs11.dylib
  SOFLAGS := -dynamiclib
else
  SONAME  := libcryptech-pkcs11.so
  SOFLAGS := -Wl,-Bsymbolic-functions -Wl,-Bsymbolic -Wl,-z,noexecstack -Wl,-soname,${SONAME}.0
endif

ifeq "${ENABLE_FOOTNOTE_WARNINGS}" "no"
  CFLAGS += -Wno-\#warnings -Wno-cpp
endif

ifneq "${ENABLE_THREADS}" "yes"
  CFLAGS += -DUSE_PTHREADS=0
else ifneq "${UNAME}" "Darwin"
  CFLAGS += -pthread
endif

ifeq "${ENABLE_DEBUGGING}" "yes"
  CFLAGS += -DDEBUG_HAL=1 -DDEBUG_PKCS11=1
endif

ifndef OBJCOPY
  OBJCOPY := objcopy
endif

all: ${SONAME} cryptech/py11/attribute_map.py

clean:
	rm -rf *.o ${SONAME}* attributes.h cryptech/*.pyc cryptech/py11/*.pyc
	${MAKE} -C libtfm  $@
	${MAKE} -C libhal  $@

distclean: clean
	rm -f TAGS

.FORCE:

${LIBTFM_BLD}/libtfm.a: .FORCE
	${MAKE} -C libtfm

${LIBHAL_BLD}/libhal.a: .FORCE ${LIBTFM_BLD}/libtfm.a
	${MAKE} -C libhal ${LIBHAL_TARGET}

attributes.h: attributes.yaml scripts/build-attributes Makefile
	python scripts/build-attributes attributes.yaml attributes.h

cryptech/py11/attribute_map.py: attributes.yaml scripts/build-py11-attributes Makefile
	python scripts/build-py11-attributes attributes.yaml $@

pkcs11.o: pkcs11.c attributes.h ${LIBS}
	${CC} ${CFLAGS} -c $<

ifeq "${UNAME}" "Darwin"

  ${SONAME}: pkcs11.o ${LIBS}
	nm $< | awk 'NF == 3 && $$2 == "T" && $$3 ~ /^_C_/ {print $$3}' >$@.tmp
	${CC} -Wl,-exported_symbols_list,$@.tmp -o $@ $^ ${SOFLAGS} ${LDFLAGS}
	rm -f $@.tmp

else

  ${SONAME}: pkcs11.o ${LIBS}
	${CC} ${CFLAGS} -Wl,--version-script=libcryptech-pkcs11.map -shared -o $@ $^ ${SOFLAGS} ${LDFLAGS}

endif

tags: TAGS

TAGS: *.[ch]
	etags $^

# Basic testing, via the Python unittest library and our cryptech.py11 interface code

test: all
	python unit_tests.py --libpkcs11 ./${SONAME}

# Further testing using hsmbully, if we can find a copy of it.

HSMBULLY := $(firstword $(wildcard $(addsuffix /hsmbully,$(subst :, ,.:${PATH}))))

ifneq "${HSMBULLY}" ""

  HSMBULLY_OPTIONS := \
	--pin fnord --so-pin fnord --pkcs11lib $(abspath ${SONAME}) \
	--verbose=9 --fast-and-frivolous --skip-fragmentation --skip-keysizing

  bully: all
	${HSMBULLY} ${HSMBULLY_OPTIONS}

endif
