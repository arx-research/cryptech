fpga_entropy
============

Test implementation of FPGA-internal entropy source.

## Introduction ##

This is an attempt at building an entropy source within a FPGA
device. the entropy source consists of multiple delay loops that are
XORed together. The bits sampled are used to generate 32 bit words which
can be extracted to a host machine for analysis.

The main problem is probably being able to control the FPGA tool not to
kill the delay loops. We try to do this by building hierarchies. It
might work.
