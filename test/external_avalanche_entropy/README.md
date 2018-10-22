
External Avalanche Entropy
--------------------------
This is a test project of an entropy provider that collects entropy from
an avalanche noise based source.

The design expects a one bit digital input noise signal. The collector
observes positive flank events in the input noise signal and measures the
time between these events using a counter. The counter is free running
and increases once for each clock cycle (currently running av 50 MHz).

The LSB of the counter is added to a 32-bit entropy register at each
event.

As debug output the entropy register is sampled at a given rate
(currently a few times per second). The debug output is connected to LED
on the FPGA development board.

The project also contains project files, pin assignments and clock
definition neded to implement the design on a TerasIC DE0-Nano board.
