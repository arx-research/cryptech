novena_i2c_simple
=================

The coretest system for the Novena PVT1, over I2C, with simplified
user interface.

## Introduction ##

This variant of novena_i2c provides a more intuitive, more compact,
and more efficient user interface - just write() the block data, and
read() the digest. All signalling to/from the hash cores is implicit
and handled by the SHA wrapper cores.

Repeated writes to the same SHA core will be added to the same digest;
the act of reading the digest resets the internal state, so that the
next write will start a new digest.

Each hash algorithm is a separate virtual I2C device on bus 2, with
the following device addresses:
  SHA-1         0x1e
  SHA-256       0x1f
  SHA-512/224   0x20
  SHA-512/256   0x21
  SHA-384       0x22
  SHA-512       0x23

## Status ##
***(2014-09-18)***
Initial version. Built using Xilinx ISE 14.3.
