sha1
====

## Introduction ##
Verilog implementation of the SHA-1 cryptgraphic hash function. The
functionality follows the specification in NIST FIPS 180-4.

The sha1 design is divided into the following sections.
 - src/rtl - RTL source files
 - src/tb  - Testbenches for the RTL files
 - src/model/python - Functional model written in python
 - doc/ - documentation (currently not done.)
 - toolruns/ - Where tools are supposed to be run. Includes a Makefile
   for building and simulating the design using [Icarus
   Verilog](http://iverilog.icarus.com/).

The actual core consists of the following RTL files:
 - sha1.v
 - sha1_core.v
 - sha1_w_mem.v

The main core functionality is in the sha1_core file. The file
sha1_w_mem contains the message block memory W (see FIPS 180-4).
The top level entity is called sha1_core. The sha1_core module has wide
interfaces (512 bit block input, 160 bit digest). In order to make it
usable you probably want to wrap the core with a bus interface.

The file sha1.v contains a top level wrapper that provides a simple
interface with 32-bit data access . This interface contains mesage block
and digest registers to allow a host to load the next block while the
current block is being processed.

## API ##
The following list contains the address map for all registers
implemented by the sha1 top level wrapper:

| address | name     | access | description                                     |
|---------|----------|--------|-------------                                    |
| 0x00    | name0    |   R    |  "SHA1"                                         |
| 0x01    | name1    |   R    |  "    "                                         |
| 0x02    | version  |   R    |  "0.50"                                         |
|         |          |        |                                                 |
| 0x08    | control  |  R/W   | Control of core. Bit 0: init, Bit 1: next       |
| 0x09    | status   |  R/W   | Status of core. Bit 0: Ready, Bit 1: valid data |
|         |          |        |                                                 |
| 0x10    | block0   |  R/W   | data block register                             |
| 0x11    | block1   |  R/W   | data block register                             |
| 0x12    | block2   |  R/W   | data block register                             |
| 0x13    | block3   |  R/W   | data block register                             |
| 0x14    | block4   |  R/W   | data block register                             |
| 0x15    | block5   |  R/W   | data block register                             |
| 0x16    | block6   |  R/W   | data block register                             |
| 0x17    | block7   |  R/W   | data block register                             |
| 0x18    | block8   |  R/W   | data block register                             |
| 0x19    | block9   |  R/W   | data block register                             |
| 0x1a    | block10  |  R/W   | data block register                             |
| 0x1b    | block11  |  R/W   | data block register                             |
| 0x1c    | block12  |  R/W   | data block register                             |
| 0x1d    | block13  |  R/W   | data block register                             |
| 0x1e    | block14  |  R/W   | data block register                             |
| 0x1f    | block15  |  R/W   | data block register                             |
|         |          |        |                                                 |
| 0x20    | digest0  |  R/W   | digest register                                 |
| 0x21    | digest1  |  R/W   | digest register                                 |
| 0x22    | digest2  |  R/W   | digest register                                 |
| 0x23    | digest3  |  R/W   | digest register                                 |
| 0x24    | digest4  |  R/W   | digest register                                 |


## Implementation details ##
The implementation is iterative with one cycle/round. The initialization
takes one cycle. The W memory is based around a sliding window of 16
32-bit registers that are updated in sync with the round processing. The
total latency/message block is 82 cycles.

All registers have asynchronous reset.

The design has been implemented and tested on TerasIC DE0-Nano and C5G
FPGA boards.


## Status ##
The design has been implemented and extensively been tested on TerasIC
DE0-Nano and C5G FPGA boards. The core has also been tested using SW
running on The Novena CPU talking to the core in the Xilinx Spartan-6
FPGA.


## FPGA-results ##

### Altera Cyclone FPGAs ###
Implementation results using Altera Quartus-II 13.1.

***Altera Cyclone IV E***
- EP4CE6F17C6
- 2913 LEs
- 1527 regs
- 107 MHz

***Altera Cyclone IV GX***
- EP4CGX22CF19C6
- 2814 LEs
- 1527 regs
- 105 MHz

***Altera Cyclone V***
- 5CGXFC7C7F23C8
- 1124 ALMs
- 1527 regs
- 104 MHz


### Xilinx FPGAs ###
Implementation results using ISE 14.7.

** Xilinx Spartan-6 **
- xc6slx45-3csg324
- 1589 LUTs
- 564 Slices
- 1592 regs
- 100 MHz


## TODO ##
* Documentation
