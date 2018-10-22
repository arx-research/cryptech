novena_entropy
==============
This is an experimental HW system for the Novena platform.

The purpose of the system is to debug, evaluate and qualify the
avalanche Cryptech entropy providers for the Novena platform with the
Xilinx Spartan-6 FPGA. The entropy providers being tested are the noise
based entropy provider and the ring oscillator based entropy provider.

The following cores are used in the system:
  - core/coretest, the test command parser

  - core/i2c, the serial interface that connects the system to the
    Novena CPU.

  - core/avalanche_entropy, the entropy provider that is driven by an
    external noise source.

  - core/rosc_entropy, the entropy provider driven by jitter between 32
  independently running ring oscillators in the FPGA.

Test SW is available in src/sw
