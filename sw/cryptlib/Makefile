# Cryptech project makefile for Cryptlib.  This is a work in progress.

# The one thing that must be configured here is which Hardware
# Adaption Layer ("HAL") to build into Cryptlib.  Due to the need to
# support different Cryptech configurations on different boards, it's
# not really possible to provide a single HAL which supports all
# Cryptech configurations in a sane way, so you have to pick one.
# This makefile provides a default, but you can override it by
# providing a value for the CRYPTECH_HAL makefile variable on the
# command line, or by setting the CRYPTECH_HAL environment variable
# (explicit argument overrides environment variable, see GNU make
# documentation for details).

ifndef CRYPTECH_HAL
  CRYPTECH_HAL := src/cryptech_novena_eim_trng.c
endif

# RNG hack defaults to enabled if we're building the TRNG, disabled
# otherwise.  This is a kludge, do better later.

ifndef CRYPTECH_RANDOM
  CRYPTECH_RANDOM := $(and $(findstring trng,${CRYPTECH_HAL}),src/cryptech_random.c)
endif

ifndef CRYPTECH_DEBUG
  CRYPTECH_DEBUG := yes
endif

# Notes on the option settings we use when building cryptlib:
#
# - Python extension modules need to be position-independent code.  We
#   could handle this by building cryptlib as a shared library, but
#   for the moment it's simpler to force even the static library to
#   use position independent code.  Works with gcc and clang, anyway,
#   which are the only compilers we care about at the moment.
#
# - Point of the exercise is a hardware device, and we want the full
#   set of SHA-2 digests.
#
# - See config.h for other options we might want to add here.  In
#   particular, USE_ECDH, USE_ECDSA, and USE_GCM all touch on subjects
#   that came up during the early Cryptech design discussions.

################################################################

# From here down is not intended to be user-servicable.  Tinker to
# your heart's content, but don't complain if it breaks.

LIB = build/libcl.a

PYTHONPATH = $(firstword $(wildcard build/bindings/build/lib.*))

SED_COMMAND := -e '/^CFLAGS/s=$$= -fPIC -DUSE_SHA2_EXT -DUSE_HARDWARE -DUSE_DEVICES -DNO_THREADS='

ifeq "${CRYPTECH_DEBUG}" "yes"
  SED_COMMAND += -e 's=-DNDEBUG==g' -e 's=-O2==g' -e '/^CFLAGS	/s,^.*$$,CFLAGS = $$(CFLAGS_DEBUG),'
endif

ifneq "$(strip ${CRYPTECH_HAL})" ""
  SED_COMMAND += -e 's=device/hw_dummy=../$(basename ${CRYPTECH_HAL})=g' -e 's=hw_dummy=$(notdir $(basename ${CRYPTECH_HAL}))=g'
ifneq "$(findstring i2c,${CRYPTECH_HAL})" ""
  SED_CMD_SETUP := -e "s='cl'='cl', 'cryptech_i2c'="
else
  SED_CMD_SETUP := -e "s='cl'='cl', 'cryptech'="
endif
endif

ifneq "$(strip ${CRYPTECH_RANDOM})" ""
  SED_COMMAND += -e 's=random/unix=../$(basename ${CRYPTECH_RANDOM})=g' -e 's=unix\.o=$(notdir $(basename ${CRYPTECH_RANDOM})).o=g'
endif

all: build/makefile.ready
	cd build; ${MAKE} debug
	@${MAKE} python-bindings

clean:
	rm -rf build

build/makefile.ready: GNUmakefile dist/cl342.zip
	rm -rf build
	mkdir build
	cd build; unzip -a ../dist/cl342.zip
	sed <build/makefile >build/makefile.cryptech ${SED_COMMAND}
	mv build/makefile.cryptech build/makefile
	sed <build/bindings/setup.py >build/bindings/setup.py.cryptech ${SED_CMD_SETUP}
	mv build/bindings/setup.py.cryptech build/bindings/setup.py
	touch $@

ifeq "${PYTHONPATH}" ""

  python-bindings:
	cd build/bindings; python setup.py build

else

  python-bindings: ${PYTHONPATH}/cryptlib_py.so

  ${PYTHONPATH}/cryptlib_py.so: ${LIB}
	cd build/bindings; python setup.py build --force

endif

test: all
	@${MAKE} run-tests

run-tests:
	for script in tests/*.py; do echo Running $$script; PYTHONPATH=${PYTHONPATH} python $$script; done

tags: TAGS

TAGS: build/makefile.ready
	find src build -type f -name '*.[ch]' | etags -
