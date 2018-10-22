
# This duplicates more of sw/thirdparty/libtfm/Makefile than I
# would like, but it does the job.  Prettier makefiles can wait for another day.

# vpath %.c ${LIBTFM_SRC}
# vpath %.h ${LIBTFM_SRC}

BITS := 8192

HDR  := ${LIBTFM_SRC}/tomsfastmath/src/headers/tfm.h
LIB  := tomsfastmath/libtfm.a

# See sw/thirdparty/libtfm/Makefile for compilation options.  Note
# that libtfm platform-specific assembly code has opinions on the
# optimization level (and appears to be best tested with -O3).

# Using $(subst...) here is a kludge.  A cleaner approach might be for
# sw/stm32/Makefile to build up the non-variant parts of CFLAGS in a
# different variable before merging the variant and non-variant parts
# into CFLAGS, which would give us a clean copy of the non-variant
# parts to use when constructing our own CFLAGS.  Later.

# The ARM assembly code in libtfm still generates a lot of warnings of the form:
#
#   warning: matching constraint does not allow a register [enabled by default]
#
# This is just a warning, the resulting library appears to work
# correctly, and the fix appears to require a nasty intervention in
# the guts of the libtfm assembly code, so we live with the warning
# for now, at least until we confirm that it hasn't already been fixed
# in a newer version of libtfm.

ifdef DO_PROFILING
# arm-none-eabi-gcc: error: -pg and -fomit-frame-pointer are incompatible
STM32_LIBTFM_CFLAGS_OPTIMIZATION := -O3 -funroll-loops
else
STM32_LIBTFM_CFLAGS_OPTIMIZATION := -O3 -funroll-loops -fomit-frame-pointer
endif

CFLAGS := $(subst ${STM32_CFLAGS_OPTIMIZATION},${STM32_LIBTFM_CFLAGS_OPTIMIZATION},${CFLAGS})
CFLAGS += -DTFM_ARM -Dasm=__asm__ -Wa,-mimplicit-it=thumb
CFLAGS += -I${LIBTFM_SRC}/tomsfastmath/src/headers
CFLAGS += -DFP_MAX_SIZE="(${BITS}*2+(8*DIGIT_BIT))"
CFLAGS += -Wall -W -Wshadow -Wno-uninitialized

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
	(cd ${LIBTFM_SRC} && find tomsfastmath/src -type d) | xargs mkdir -p
	cd tomsfastmath; ${MAKE} CFLAGS='${CFLAGS}'
