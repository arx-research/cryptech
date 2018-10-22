trng
====

True Random Number Generator core implemented in Verilog.

## Introduction ##
This repo contains the design of a True Random Number Generator (TRNG)
for the [Cryptech OpenHSM](http://cryptech.is/) project.


## Design inspiration, ideas and principles ##

The TRNG **MUST** be a really good one. Furthermore it must be trustable
by its users. That means it should not do wild and crazy stuff. And
users should be able to verify that the TRNG works as expected.

* Follow best practice
* Be conservative - No big untested ideas.
* Support transparency - The parts should be testable.


Some of our inspiration comes from:
* The Fortuna RNG by Ferguson and Schneier as described in Cryptography
Engineering.

* /dev/random in OpenBSD


## System description ##

The TRNG consists of a chain with three main subsystems

* Entropy generation
* Entropy mixing
* Random generation


### Entropy generation ###

The entropy generation subsystems consists of at least two separate entropy
generators. Each generator collects entropy from an independent physical
process. The entropy sources MUST be of different types. For example
avalance noise from a reversed bias P/N junction as one source and RSSI
LSB from a receiver.

The reason for having multiple entropy sources is both to provide
redundancy as well as making it harder for an attacker to affect the
entropy collection by forcing the attacker to try and affect different
physical processes simultaneously.

A given entropy generator is responsible for collecting the entropy
(possibly including A/D conversion.). The entropy generator MUST
implement some on-line testing of the physical entropy source based on
the entropy collected. The tests shall be described in detail here but
will at least include tests for:

* No long run lengths in generated values.
* Variance that exceeds a given threshhold.
* Mean value that don't deviate from expected mean.
* Frequency for all possible values are within expected variance.

If the tests fails over a period of generated values the entropy source
MUST raise an error flag. And MAY also block access to the entropy it
otherwise provides.

There shall also be possible to read out the raw entropy collected from
a given entropy generator. This MUST ONLY be possible in a specific
debug mode when no random generation is allowed. Also the entropy
provided in debug mode MUST NOT be used for later random number
generation. 

The entropy generator SHALL perform whitening on the collected entropy
before providing it as 32-bit values to the entropy accumulator.



### Entropy mixing ###

The entropy mixer subsystems reads 32-bit words from the entropy
generators to build a block of bits to be mixed.

When 1024 bits of mixed entropy has been collected the entropy is used
as a message block which is fed into a hash function.

The hash function used is SHA-512 (NIST FIPS 180-4).

The digest is then extracted and provided to the random generation as as
a seed.


### Random generation ###

The random generation consists of a cryptographically secure pseudo random
number generator (CSPRNG). The CSPRNG used in the trng is the stream
cipher ChaCha.

ChaCha is seeded with:
- 512 bits block
- 256 bits key
- 64 bits IV
- 64 bits counter

In total the seed used is: 896 bits. This requires getting two seed
blocks of 512 bits from the mixer.

The number of rounds used in ChaCha is conservatively
selected. We propose that the number of rounds shall be at least 24
rounds. Possibly 32 rounds. Given the performance in HW for ChaCha and
the size of the keystream block, the TRNG should be able to generate
plentiful of random values even with 32 rounds.

The random generator shall support the ability to test its functionality
by seeding it with a user supplied value and then generate a number of
values in a specific debug mode. The normal access to generated random
values MUST NOT be allowed during the debug mode. The random generator
MUST also set an error flag during debug mode. Finally, when exiting the
debug mode, reseeding MUST be done.

Finally the random generator provides random numbers as 32-bit
values. the 512 bit keystream blocks from ChaCha are divided into 16
32-bit words and provided in sequence.


## Implementation details ##

The core supports multpiple entropy sources as well as a CSPRNG. For
each entropy source there are some estimators that checks that the
sources are not broken.

There are also an ability to extract raw entropy as well as inject test
data into the CSPRNG to verify the functionality.

The core will include one FPGA based entropy source but expects the
other entropy source(s) to be connected on external ports. It is up to
the user/system implementer to provide physical entropy souces. We will
suggest and provide info on how to design at least one such source.

For simulation there are simplistic fake entropy sources that can be
found in the tb/fake_modules directory. This modules SHOULD NOT be used
as real sources.

For synthesis there are wrappers for the real entropy source cores to
adapt their interfaces to what we need in the trng. These wrappers
should not be included during simulation.



## API ##

Normal operation:
* Extract 32-bit random words.


Config parameters:
* Number of blocks in warm-up.
* Number of keystream blocks before reseeding.


Debug access
* Enable/disable entropy generator X
* Check health of entropy generator X
* Read raw entropy from entropy generator X as 32-bit word.
* Write 256 bit seed value as 8 32-bit words
* Read out one or more 512 bit keystream blocks as 32-bit words.



## Status ##

*** (2014-09-11) ***

The first version of the CSPRNG is debugged and completed. This version
supports automatic reseeding and an output fifo.


