fmc-test
========

Demo program for stm32-dev-bridge board to test FMC arbiter in Novena's
on-board FPGA.

Current issues:
 * SystemClock_Config() currently uses 16 MHz HSI oscillator as PLL input,
this should be changed to 25 MHz crystal later.
