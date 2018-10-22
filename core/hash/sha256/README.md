# sha256 #
Hardware implementation of the SHA-256 cryptographic hash function. The
implementation is written in Verilog 2001 compliant code. The
implementation includes a core and a wrapper that provides a 32-bit
interface for simple integration. There is also an alternative wrapper
that implements a Wishbone compliant interface.

This is a low area implementation that iterates over the rounds but
there is no sharing of operations such as adders.

The hardware implementation is complemented by a functional model
written in Python.

## Implementation details ##
The sha256 is divided into the following sections.
- src/rtl - RTL source files
- src/tb  - Testbenches for the RTL files
- src/model/python - Functional model written in python
- doc - documentation (currently not done.)
- toolruns - Where tools are supposed to be run. Includes a Makefile for
building and simulating the design using [Icarus Verilog](http://iverilog.icarus.com/)

The actual core consists of the following files:
- sha256_core.v - The core itself with wide interfaces.
- sha256_w_mem.v - W message block memort and expansion logic.
- sha256_k_constants.v - K constants ROM memory.

The top level entity is called sha256_core. This entity has wide
interfaces (512 bit block input, 256 bit digest). In order to make it
usable you probably want to wrap the core with a bus interface.

Unless you want to provide your own interface you therefore also need to
select one top level wrapper. There are two wrappers provided:
- sha256.v - A wrapper with a 32-bit memory like interface.
- wb_sha256.v - A wrapper that implements a [Wishbone](http://opencores.org/opencores,wishbone) interface.

***Do not include both wrappers in the same project.***

The core (sha256_core) will sample all data inputs when given the init
or next signal. the wrappers provided contains additional data
registers. This allows you to load a new block while the core is
processing the previous block.

The W-memory scheduler is based on 16 32-bit registers. Thee registers
are loaded with the current block. After 16 rounds the contents of the
registers slide through the registers r5..r0 while the new W word is
inserted at r15 as well as being returned to the core.


## FPGA-results ##

### Altera Cyclone FPGAs ###
Implementation results using Altera Quartus-II 13.1.

***Cyclone IV E***
- EP4CE6F17C6
- 3882 LEs
- 1813 registers
- 74 MHz
- 66 cycles latency

***Cyclone IV GX***
- EP4CGX22CF19C6
- 3773 LEs
- 1813 registers
- 76 MHz
- 66 cycles latency

***Cyclone V***
- 5CGXFC7C7F23C8
- 1469 ALMs
- 1813 registers
- 79 MHz
- 66 cycles latency


### Xilinx Artix-7 FPGAs ###
Implementation results using Xilinx ISE 14.7
This implementation includes pipeline regsisters.

- xc7a200t-1fbg484
- 2229 Slice LUTs
- 775 Slices
- 1935 registers
- 101 MHz
- 130 cycles latency



## TODO ##
- Extensive verification in physical device.
- Complete documentation.


## Status ##
***(2016-05-31)***

The core now supports both sha224 and sha256 modes. The default mode is
sha256.

NOTE: The mode bit is located in the ADDR_CTRL API register and this
means that when writing to this register to start processing a block,
care must be taken to set the mode bit to the intended mode. This means
that old code that for example simply wrote 0x01 to initiate SHA256
processing will now initiate SHA224 processing. Writing 0x05 will
now initiate SHA256 processing.

The API version has been bumped a major number to reflect this change.

Regarding SHA224, it is up to the user to only read seven, not eight
words from the digest registers. The core will update the LSW too.


***(2013-02-23)***

Cleanup, more results etc. Move all wmem update logic to a separate
process for a cleaner code.


**(2014-02-22)**

Redesigned the W-memory into a sliding window solution. This not only
removed 48 32-registers but also several muxes and address decoders.

The old implementation resources and performance:
- 9587 LEs
- 3349 registers
- 73 MHz
- 66 cycles latency

The new implementation resources and performance:
- 3765 LEs
- 1813 registers
- 76 MHz
- 66 cycles latency



**(2014-02-19)**
- The core has been added to the Cryptech repo. The core comes from
  https://github.com/secworks/sha256
