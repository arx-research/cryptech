vndecorrelator
==============

A Verilog implementation of a von Neumann decorrelator.

This tiny module consumes pairs of bits and generates decorrelated
bits. Basically given a sequence of two bits, the decorrelator will:

0, 1: Emit 1
1, 0: Emit 0
0, 0: Emit nothing
1, 1: Emit nothing

The rate of bits emitted is thus at most half of the bitrate on the
input.

The module is synchronous, but bits may arrive a number of cycles
between eachother. The module will set the syn_out flag during one cycle
to signal that the value in data_out is a valid bit.

