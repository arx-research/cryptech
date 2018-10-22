avalanche_entropy
=================

Entropy provider core for an external avalanche noise based entropy source.

## Functional Description ##

This core samples noise provided on an input pin. The noise is expected
to be 'digital' that is fairly rapidly move from voltage levels
matching ones and zeros as handled by the digital process used to
implement the core.

The noise is sampled with double registers. Then phase detection is
applied to find positive flanks. The core contains a free running clock
(clocked at the provided core clock frequency). When a positive flank in
the noise is detected, the LSB of the clock is sampled and added to a
shift registers. When at least 32 bits has been collected, the result is
presented as entropy available to any entropy consumer connected to the
core.

The core also includes a delta time counter. This counter is used for
testing of the core and is available via the API.

The fact that the core uses the flank of the to drive the entropy bit
generation, but that the timing between the flanks means that if
the noise source have a bias for zero or one state does not affect which
entropy bits are generated.


## Implementation Status ##

The core has been tested with several revisions of the Cryptech
avalanche noise board. The core has been implemented in Altera
Cyclone-IV and Cyclone-V devices as well as in Xilinx Spartan-6
devices. The core clock frequency used has been 25 MHz, 33 MHz and 50
MHz.

The generated entropy has been extensively tested (using the ent tool as
well as other custom tools) and found to be generating entropy with good
quality.
