# Use a git submodule to download and build libtfm with the options we want.
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

# Maximum size of a bignum.  See tfm.pdf section 1.3.6 ("Precision
# configuration") for details on how FP_MAX_SIZE works.

BITS	:= 8192

REPO	:= tomsfastmath
HDR	:= ${REPO}/src/headers/tfm.h
LIB	:= ${REPO}/libtfm.a

CFLAGS	+= -fPIC -Wall -W -Wshadow -Isrc/headers -g3 -DFP_MAX_SIZE="(${BITS}*2+(8*DIGIT_BIT))"

TARGETS	:= $(notdir ${HDR} ${LIB})

SHA256SUM := $(firstword $(wildcard /usr/local/bin/sha256sum /usr/local/bin/gsha256sum /usr/bin/sha256sum))

CHECKSUMS ?= Checksums

all: ${TARGETS}

clean:
	rm -f ${TARGETS} $(notdir ${HDR}.tmp)
	cd ${REPO}; ${MAKE} clean

distclean: clean
	rm -f TAGS

${HDR}:
	git submodule update --init

${LIB}: ${HDR}
	cd ${REPO}; ${MAKE} clean
ifeq "" "${SHA256SUM}"
	@echo "Couldn't find sha256sum, not verifying distribution checksums"
else
	${SHA256SUM} --check ${CHECKSUMS}
endif
	cd ${REPO}; ${MAKE} CFLAGS='${CFLAGS}'

$(notdir ${HDR}): ${HDR}
	echo  >$@.tmp '/* Configure size of largest bignum we want to handle -- see notes in tfm.pdf */'
	echo >>$@.tmp '#define   FP_MAX_SIZE   (${BITS}*2+(8*DIGIT_BIT))'
	echo >>$@.tmp ''
	cat  >>$@.tmp $^
	mv -f $@.tmp $@

$(notdir ${LIB}): ${LIB}
	ln -f $^ $@

tags: TAGS

TAGS: ${HDR}
	find ${REPO} -type f -name '*.[ch]' -print | etags -

ifneq "" "${SHA256SUM}"
regenerate-checksums: ${HDR}
	cd ${REPO}; git clean -dxf
	find ${REPO} -name .git -prune -o -type f -print | sort | xargs ${SHA256SUM} >${CHECKSUMS}
endif
