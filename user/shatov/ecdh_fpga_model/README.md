# ecdh_model_fpga

This reference model was written to help debug Verilog code, it mimics how an FPGA would do elliptic curve point scalar multiplication for ECDH using curves P-256 and P-384. Note, that the model may do weird (from CPU point of view, of course) things at times. Another important thing is that while FPGA modules are actually written to operate in constant-time manner, this model itself doesn't take any active measures to keep run-time constant. Do **NOT** use it in production as-is!

The model is split into 4 layers:

 * Low-level primitives (32- and 48-bit adders, 32-bit subtractor, 16x16-bit multiplier, 48-bit accumulator)
 * Utility routines (copier, comparator)
 * Modular arithmetic (adder, subtractor, multiplier, invertor)
 * EC arithmetic (adder, doubler, multiplier)

Modular multiplier and invertor use complex algorithms and are thus further split into "helper" sub-routines.

This model uses tips and tricks from the following sources:
1. [Guide to Elliptic Curve Cryptography](http://diamond.boisestate.edu/~liljanab/MATH308/GuideToECC.pdf)
2. [Ultra High Performance ECC over NIST Primes
on Commercial FPGAs](https://www.iacr.org/archive/ches2008/51540064/51540064.pdf)
3. [Constant Time Modular Inversion](http://joppebos.com/files/CTInversion.pdf)
