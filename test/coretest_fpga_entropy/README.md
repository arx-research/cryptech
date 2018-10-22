coretest_fpga_entropy
=====================

Coretest system for testing FPGA based entropy source.
 
## Introduction ##
This project is a coretest system dedicated to test entropy sources
within a FPGA device. The specific entropy source is based on a digital
oscillator design by Bernd Paysan. In this entropy source, we use six
instances with different frequencies. The oscillator outputs are
combined to generate a bit value. 32 bit values are combined to create a
random word.

The system uses the coretest module to read and write 32-bit data to
core, In this case it allows a caller to read generated random 16-bit
values from the entropy source. The 16 bit data is in the LSB of the
word.

The completc system contains a UART core for external access. The
project contains pin assignments etc to implement the system on a
TerasIC C5G board.

## Implementation details. ##
This FPGA system consists of the following components:

- The FPGA entropy source core
- The UART core
- The coretest core

There are pin assignments and clock defines for the TerasIC C5G board.
