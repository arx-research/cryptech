
# This duplicates more of sw/thirdparty/libtfm/Makefile than I
# would like, but it does the job.  Prettier makefiles can wait for another day.

ifndef CRYPTECH_ROOT
  CRYPTECH_ROOT := $(abspath ../../..)
endif

REPO := ${CRYPTECH_ROOT}/sw/thirdparty/libtfm

# vpath %.c ${REPO}
# vpath %.h ${REPO}

BITS := 8192

HDR  := ${REPO}/tomsfastmath/src/headers/tfm.h
LIB  := tomsfastmath/libtfm.a

#CFLAGS	+= -DTFM_X86
CFLAGS	+= -DTFM_NO_ASM

CFLAGS	+= -fPIC -Wall -W -Wshadow -I${REPO}/tomsfastmath/src/headers -g3 -DFP_MAX_SIZE="(${BITS}*2+(8*DIGIT_BIT))"

TARGETS	:= $(notdir ${HDR} ${LIB})

all: ${TARGETS}

clean:
	rm -rf ${TARGETS} $(notdir ${HDR}.tmp) ${LIB} tomsfastmath/src

distclean: clean
	rm -f TAGS

$(notdir ${HDR}): ${HDR}
	echo  >$@.tmp '/* Configure size of largest bignum we want to handle -- see notes in tfm.pdf */'
	echo >>$@.tmp '#define   FP_MAX_SIZE   (${BITS}*2+(8*DIGIT_BIT))'
	echo >>$@.tmp ''
	cat  >>$@.tmp $^
	mv -f $@.tmp $@

$(notdir ${LIB}): ${LIB}
	ln -f $^ $@

${LIB}: ${HDR}
	(cd ${REPO} && find tomsfastmath/src -type d) | xargs mkdir -p
	cd tomsfastmath; ${MAKE} CFLAGS='${CFLAGS}'
