novena-fmc
==========

FMC interface arbiter for Novena's on-board Spartan-6 FPGA.

This demo project has one 32-bit test register instead of core selection logic.

Important things to note:

 * Width of address bus is now parametrized (bridge board has 22 address lines),
this way it will be easy to change it in the future (when time comes to
configure Alpha for the first time).

 * Clock manager no longer uses an IP core, instead of this clkmgr_dcm_new.v
now directly instantiates DCM_SP primitive. Clock frequency can be changed by
tweaking CLK_OUT_MUL and CLK_OUT_DIV parameters.
