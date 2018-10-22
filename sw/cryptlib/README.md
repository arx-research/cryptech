cryptlib
========

## Introduction ##

This is a port of Peter Gutmann's
[cryptlib package](https://www.cs.auckland.ac.nz/~pgut001/cryptlib/)
to the Cryptech project's environment.  This is a work in progress,
and still at a very early stage as of this writing.

The main addition to the stock cryptlib environment is a set of
Hardware Adaption Layer (HAL) implementations that use the Cryptech
FPGA cores.

While we expect to be making more significant use of cryptlib in the
future, the main purposes of this code at the moment are
proof-of-concept and connecting the Cryptech cores to a more complete
cryptographic programming environment for testing and development
purposes.

## Current status ##

At present, the Cryptech HAL code runs only on the Novena PVT1.  There
are three variants of the HAL, all using the I2C bus, but speaking
different protocols:

* An implementation using the `coretest` byte-stream protocol
  implemented by the `core/novena` FPGA build.

* An implementation using the simpler interface implemented by the
  `core/novena_i2c_simple` environment.

* An implementation using the `coretest` byt-stream protocol as
  implemented by the `test/novena_trng` FPGA build.  This differs from
  the others in that it supports the Cryptech TRNG.  Note that neither
  this HAL nor this FPGA build supports any cryptographic algorithms.

All of these HAL implementations are in the `src/` directory.  See the
`GNUmakefile` for details on how to select the variant you want.

At present, the only relevant Cryptech cores are the TRNG and several
digest algorithms.   The current HAL uses the SHA-1, SHA-256, and
SHA-512 cores to implement the SHA-1, SHA-256, SHA-384, and SHA-512
digests.  SHA-512/224 and SHA-512/256 are not supported.

In principal there is no reason why one could not write a HAL which
spoke to a Terasic board, perhaps via the `coretest` protocol over a
UART, but to date this has not been done.

## Code import status ##

Cryptlib itself is present in the repository in the form of a verbatim
copy of the Cryptlib 3.4.2 distribution zipfile, which the top-level
makefile unpacks while building.  This has proven simpler to work with
than importing the entire Cryptlib distribution into a vendor branch.

Packaging Cryptlib this way has two implications:

* You may need to `apt-get install unzip` on your Novena.

* Any changes you might make to Cryptlib itself will be lost when you
  run `make clean`.

## Test code ##

The `tests/` directory contains a few test scripts, written in Python,
using the standard Cryptlib Python bindings.  The Cryptlib Python
environment is a fairly literaly translation of the Cryptlib C
environment, so portions of it will be a bit, um, surprising to Python
programmers, but the basic functionality works.  Note that it's normal
for test scripts to fail when the functionality they're testing isn't
loaded on the FPGA.

## Copyright status ##

Cryptlib itself is copyright by Peter Gutmann.  See the Cryptlib web
site for licensing details.

Code written for the Cryptech project is under the usual Cryptech
BSD-style license.
