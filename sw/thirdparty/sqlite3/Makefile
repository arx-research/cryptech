# Build SQLite3 from source with the options we want for PKCS #11.
#
# Author: Rob Austein
# Copyright (c) 2015, SUNET
#
# Redistribution and use in source and binary forms, with or
# without modification, are permitted provided that the following
# conditions are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# URL from which we downloaded the source tarball.  SQLite3 uses an
# oddball version control system I've never head of, and the SQLite3
# project advises using this pre-processed tarball instead of messing
# with their original source.  We could download the tarball every
# time we build rather than stuffing it in our repository, but that
# generates a lot of fetches during release engineering and continuous
# integration, which would be a bit anti-social.

URL := http://sqlite.org/2016/sqlite-autoconf-3120100.tar.gz

# Whether to enable threading.  Main reason for being able to turn it
# off is that gdb on the Novena goes bananas when threading is enabled.

ifndef ENABLE_THREADS
  ENABLE_THREADS := yes
endif

OPTIONS	:= --enable-static --disable-shared --disable-dynamic-extensions CFLAGS=-fPIC LDFLAGS=-fPIC

ifneq "${ENABLE_THREADS}" "yes"
  OPTIONS += --disable-threadsafe
endif

ifdef CROSS_COMPILE
  OPTIONS += --host=${CROSS_COMPILE}
endif

TOP	:= $(shell pwd)
TARBALL	?= ${TOP}/$(notdir ${URL})
BUILD	:= ${TOP}/build
OUTPUT	:= ${BUILD}/sqlite3.h ${BUILD}/.libs/libsqlite3.a ${BUILD}/sqlite3
TARGETS	:= $(notdir ${OUTPUT})

all: ${TARGETS}

clean:
	rm -rf ${BUILD} ${TARGETS}

distclean: clean

${BUILD}/.build_done: ${TARBALL} Makefile
	rm -rf ${BUILD}
	mkdir ${BUILD}
	cd ${BUILD}; tar -xf ${TARBALL} --strip-components=1
	cd ${BUILD}; ./configure ${OPTIONS}
	cd ${BUILD}; make
	ln -f ${OUTPUT} .
	touch $@

${TARGETS}: ${BUILD}/.build_done

fetch:
	wget ${URL}

.PHONY: all clean distclean fetch
