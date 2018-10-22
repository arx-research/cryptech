STM32 avalanche noise entropy source
====================================

This is an open source and open hardware entropy source, using an
STM32 microcontroller to gather entropy from a common avalanche
noise circuit.

A special thanks goes to Benedikt Stockebrand who designed the circuit
and the currently used core extraction algorithm in his ARRGH project.

  http://www.stepladder-it.com/Downloads/arrgh-0.2.1alpha.txz


Copyrights
==========

The license for all work done on this in the CrypTech project is a
3-clause BSD license (see LICENSE.txt for details). Some files have
been generated using the STMicroelectronics initialization code
generator STM32CubeMX and thus have additional copyright header(s).

The "Noise generator" and "Amplifier" parts of the circuit diagram are
copied from the ARRGH project. ARRGH copyright statement is included
in LICENSE.txt.

A stripped down copy of the ARM CMSIS library version 3.20 is included
in the Drivers/CMSIS/ directory. Unused parts (and documentation etc.)
have been removed, but every attempt have been made to keep any
licensing information intact. See in particular the file
Drivers/CMSIS/CMSIS END USER LICENCE AGREEMENT.pdf.

A full copy of the STM32F4xx HAL Drivers is included in the
Drivers/STM32F4xx_HAL_Driver/ directory.


Building
========

The following packages need to be installed (on Ubuntu 14.04):

  apt-get install gcc-arm-none-eabi gdb-arm-none-eabi openocd

XXX not sure this is the complete set, if you find that you need
additional packages please let me know. See e-mail address at the bottom.

To build the source code, issue "make" from the top level directory
(where this file is). The first time, this will build the complete STM
CMSIS library. A subsequent "make clean" will *not* clean away the CMSIS
library, but a "make really-clean" will.


Installing
==========

Do "make flash-target" from the top level directory (where this file is)
to build the firmware for the application selected in the top level
Makefile and flash it into the microcontroller. See the section STLINK
below for information about the actual hardware programming device needed.


Using
=====

The microcontroller code can currently run in one of two modes, set
statically at the beginning of main(): MODE_DELTAS and MODE_ENTROPY.

MODE_ENTROPY is the default, and means the microcontroller will send
entropy as binary data as fast as it can get it, which is about 24 kB/s
in the current version of hardware and software. To get some entropy
and perform rudimentary analysis of it, and assuming USB is used and
the device was enumerated as ttyUSB0, do

  ldattach -8 -n -1 -s 460800 tty /dev/ttyUSB0
  echo > /dev/ttyUSB0
  cat /dev/ttyUSB0 | rngtest -c 10
  cat /dev/ttyUSB0 | head -c 100000 | ent

For Raspberry-Pi, follow any of the guides on the internet for how to
enable the serial port on the GPIO pin header and then try

  ldattach -s 115200 -8 -n -1 tty /dev/ttyAMA0
  echo > /dev/ttyAMA0
  cat /dev/ttyAMA0 | rngtest -c 10
  cat /dev/ttyAMA0 | head -c 100000 | ent

(the baud rate used with the R-Pi could probably be increased with a
little hardware debugging effort).

Which UART on the board that will receive the entropy is controlled
by the sending of a newline to the UART ('echo > /dev/ttyUSB0' and
'echo > /dev/ttyAMA0' in the examples above). The power on default is
the USB UART.


MODE_DELTAS is a quality assurance mode, and outputs the raw Timer IC
values captured for analysis. The stand alone program in src/delta16/
parses the data format used by MODE_DELTAS and can convert it to
something you can analyse. More about how to do that later.


Contents
========

This documentation needs to be improved, but here are some quick notes:

Hardware design (Eagle and PDF files) are in hardware/rev09/

The firmware to extract entropy from this hardware is in src/entropy/

There are additional firmwares to aid in debugging any hardware issues
in src/led-test/ and src/uart-test/


Hardware
========

The avalanche noise circuit was first implemented using a NUCLEO-F401RE
evaluation board that has an STM32F401RET6 MCU. Because of human error,
the STM32F401RBT6 was used when assembling rev08 and rev09 boards. This
chip has less flash and RAM, so some region mappings had to change.

MCU dependant parameters are found in the top level common.mk near the
top, read the comments regarding STDPERIPH_SETTINGS, MCU_LINKSCRIPT and
SRCS.


STLINK
======
To program the MCU, an STLINK adapter is used. The cheapest way to get
one is to buy an evaluation board with an STLINK integrated, and pinouts
to program external chips. All the evaluation boards I've encountered
from STM has this ability. I'm using an STLINK from an STM32F4DISCOVERY
board, but the even cheaper NUCLEO-F401RE should work too. The NUCLEO
one has a STLINK v2.1 which is probably, but not necessarily, supported
by the OpenOCD version in your Linux distribution (as of end of 2014).

The STLINK programming pins are the 1+4 throughole pads above the ARM
on the circuit board. See the schematics for details, but the pinout
from left to right (1, space, 4) of rev09 is

  NRST, space, CLK, IO, GND, VCC


Debugging the firmware
======================

This site shows several ways to use various debuggers to debug the
firmware in an STM32:

  http://fun-tech.se/stm32/OpenOCD/gdb.php

I've only managed to get the most basic text line gdb to work,
something along these lines:

1) Start OpenOCD server (with a configuration file for your type of STLINK
   adapter)

   $ openocd -f /usr/share/openocd/scripts/board/stm32f4discovery.cfg

2) Connect to the OpenOCD server and re-flash already compiled firmware:

   $ telnet localhost 4444
   reset halt
   flash probe 0
   stm32f2x mass_erase 0
   flash write_bank 0 /path/to/main.bin 0
   reset halt

3) Start GDB and have it connect to the OpenOCD server:

   $ arm-none-eabi-gdb --eval-command="target remote localhost:3333" main.elf



---
Fredrik Thulin <fredrik@thulin.net>, for the
CrypTech project <https://cryptech.is/>
2015-01-14
