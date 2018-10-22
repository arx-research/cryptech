STM32 firmware for Cryptech Alpha board
=======================================

The Alpha board is our first full prototype for an open-source hardware
security module (HSM). It is a custom board with an STM32 Cortex-M4
microcontroller and an Artix-7 FPGA, flash-based keystore, separate memory
for the Key Encryption Key, etc. See the `hardware` repository for
schematics and production files. See the wiki for design documents.

The code in this repository builds the firmware that provides the HSM
functionality on the Alpha board.

There is some residual code here to support the "dev-bridge" board, a
daughterboard for the Novena, which talks to the Novena's FPGA through the
high-speed expansion connector. Only a few of these boards were ever made,
and all development/testing ceased as soon as the Alpha became available,
so the dev-bridge should be considered deprecated, and support may be
removed in the future.

Copyrights
==========

The license for all work done on this in the CrypTech project is a
3-clause BSD license.

Third-party components, as well as code generated using the
STMicroelectronics initialization code generator STM32CubeMX, or adapted
from STM example/support code, may have different licensing, detailed
below.

Components
==========

Libraries
---------

* `mbed` - A stripped down copy of the ARM CMSIS library, copied from the
  mbed github (see `libraries/mbed/README.txt` for details). The bulk of
  this library is covered under 3-clause BSD licenses from either ARM or
  STMicroelectronics, but one file is covered under an Apache license from
  ARM.

* `libhal` - Build directory for our own Hardware Adaption Library
  (hardware-independent Cryptech components). Source is expected to be in
  `sw/libhal`.

* `libtfm` - Build directory for "Tom's Fast Math", which is used heavily
  for bignum math in the RSA and ECDSA code. This code is covered under an
  unrestricted public domain license, and source is expected to be in
  `sw/thirdparty/libtfm`.

* `libcli` - Build directory for a third-party Command Line Interface
  library. The source is not currently under `sw/thirdparty` because the
  license is LGPLv2.1; we are negotiating to see if we can get a
  BSD-compatible license for it.

* `libprof` - A port of the `gmon` profiling package, to be used in
  development only, not in production code (obviously). The licensing is a
  mix of BSD and "Cygwin license", which now seems to be LGPLv3.

Projects
--------

These directories build different firmware images for the Alpha board.

* `hsm` - Firmware providing HSM functionality. Clients communicate via
  RPC requests on the USER USB port, or interactively on the MGMT USB
  port.

* `bootloader` - The first thing that runs on the device. It either starts
  the primary firmware, or installs new firmware.

* `board-test` - Tests of hardware components.

* `cli-test` - Test of the CLI itself, plus some interactive tests of
  hardware components. Duplicates way too much of the HSM CLI.

* `libhal-test` - A framework for running the libhal component
  tests. Hasn't been run in a while, probably still works.

Building
========

Our primary build environments are Debian and Ubuntu, but this should work
on any system with Gnu tools installed.

The following packages need to be installed:

    $ apt-get install gcc-arm-none-eabi gdb-arm-none-eabi openocd

The Makefile assumes that all Cryptech repositories have been fetched into
a canonical directory structure, e.g. `libhal` and `thirdparty` are
siblings to this directory, under `sw`.

To build the source code, issue `make` from the top level directory
(where this file is). The first time, this will build the complete STM
CMSIS library. A subsequent `make clean` will *not* clean away the CMSIS
library, but a `make distclean` will.

Installing
==========

Do `bin/flash-target` from the top level directory (where this file is)
to flash a built image into the microcontroller. See the section ST-LINK
below for information about the actual hardware programming device needed.

Example loading the HSM firmware:

    $ make hsm
    $ ./bin/flash-target projects/hsm/hsm

At this point, the STM32 will reset into the bootloader which flashes the
blue LED five times in one second, and then jumps to the primary firmware.

Once the bootloader is installed, regular firmware can be loaded without
an ST-LINK cable like this:

    $ cryptech_upload --firmware -i projects/hsm/hsm.bin

Then reboot the Alpha board.

ST-LINK
-------

To program the MCU, an ST-LINK adapter is used. The cheapest way to get
one is to buy an evaluation board with an ST-LINK integrated, and pinouts
to program external chips. This should work with any evaluation board from
STM; we have tested with STM32F4DISCOVERY (with ST-LINK v2.0) and
NUCLEO-F411RE (with ST-LINK v2.1).

The ST-LINK programming pins is called J1 and is near the CrypTech logo
printed on the circuit board. The pin-outs is shown on the circuit board
(follow the thin white line from J1 to the white box with STM32_SWD
written in it). From left to right, the pins are

    3V3, CLK, GND, I/O, NRST and N/C

This matches the pin-out on the DISCO and NUCLEO boards we have tried.

First remove the pair of ST-LINK jumpers (CN4 on the DISCO, CN2 on the
NUCLEO). Then find the 6-pin SWD header on the left of the STM board (CN2
on the DISCO, CN4 on the NUCLEO), and connect them to the Alpha board:

    NUCLEO / DISCO           CRYPTECH ALPHA
    --------------           --------------
    * 1 VDD_TARGET      <->  3V3
    * 2 SWCLK / T_JTCK  <->  CLK
    * 3 GND             <->  GND
    * 4 SWDIO / T_JTMS  <->  IO
    * 5 NRST / T_NRST   <->  NRST
    * 6 N/C

The Alpha board should be powered on before attempting to flash it.

Debugging the firmware
----------------------

[This site](http://fun-tech.se/stm32/OpenOCD/gdb.php) shows several ways
to use various debuggers to debug the firmware in an STM32.

There is a shell script called 'bin/debug' that starts an OpenOCD server
and GDB. Example:

    $ ./bin/debug projects/hsm/hsm

Once in GDB, issue `monitor reset halt` to reset the STM32 before debugging.

Remember that the first code to run will be the bootloader, but if you do
e.g. `break main` and `continue` you will end up in main() after the
bootloader has jumped there.
