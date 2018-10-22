# Cryptech Novena FPGA baseline #

## Introduction ##

This repo contains the Novena FPGA baseline developed as part of the
Cryptech project. The design contains a new FPGA top level, now clock
implementation and reworked EIM interface.

The main purpose of the baseline is to allow us to run the Cryptech
cores and FPGA system with the general system clock and then interface
to the EIM with the EIM burst clock.


## Technical details ##

The design tries to be a clean top that is easy for others to work on
and adapt to their needs. The top is stripped from ports not needed for
the baseline. All clock and reset implementation is placed in a separate
module. The EIM interface is in a separate module and then the rest of
the system is in a third module.

Internally the baseline contains an arbiter to connect cores with a
32-bit memory like interface to the EIM. Finally there is SW to
configure the EIM interface as well as talking to a test core in the
FPGA.

For information about the EIM clocking and the baseline HW and SW
design, see the documentation.


## Author ##

The baseline has been written by Pavel Shatov.
