coretest_bpaysan_entropy
========================

Coretest system for testing the FPGA based entropy source developed by Bernd Paysan.

## Introduction ##
This project is a coretest system dedicated to test entropy sources
within a FPGA device. The specific entropy source has ben designed by
Bernd Paysan.

The entropy source used two sets of 16 free running digital oscillators
that are interconnected.

The system uses the coretest module to read and write 32-bit data to
core, In this case it allows a caller to read generated random 16-bit
values from the entropy source. The 16 bit data is in the LSB of the
word.

The completc system contains a UART core for external access. The
project contains pin assignments etc to implement the system on a
TerasIC C5G board.
