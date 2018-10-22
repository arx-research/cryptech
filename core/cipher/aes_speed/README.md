aes_speed
=========
Speed optimized Verilog implementation of the symmetric block cipher AES
(Advanced Encryption Standard) as specified in the NIST document [FIPS
197](http://csrc.nist.gov/publications/fips/fips197/fips-197.pdf).

This core is modified version of the Cryptech AES core. Note that the
name of the core modules are identical to that core. The purpose of this
is to allow a drop-in replacement in Cryptech designs.


## Status ##
Second round of optimizations done. Core similates correctly. Core has
been implemented in FPGA, but not functionally tested in real HW.


## Introduction ##
This implementation supports 128 and 256 bit keys. The
implementation is iterative and process one 128 block at a time.

The encipher and decipher block processing datapaths are separated and
basically self contained given access to a set of round keys and a
block. This makes it possible to hard wire either encipher or decipher
and allow the build tools to optimize away the other functionality which
will reduce the size to about 50%. For cipher modes such as CTR, GCM
decryption in the AES core will never be used and thus the decipher
block processing can be removed.

The core has been equipped with 16 S-boxes for encipher and 16 Inverse
S-boxes for decipher. This allows the core to perform the SubBytes and
InverseSubBytes operations in the AES round functions in one cycle.

The key expansion does not share S-boxes with the encipher datapath, so
the total number of S-boxes is 40.


## Performance comparison
Number of cycles for the old Cryptech AES core:
- AES-128 Encipher one block with key expansion: 57
- AES-256 Decipher one block with key expansion: 77

Number of cycles for the Cryptech AES speed core:
- AES-128 Encipher one block with key expansion: 16
- AES-255 Decipher one block with key expansion: 20


## Implementation comparison
Implementation results for Xilinx Artix7-t200.

Old Cryptech AES core:
- 2094 slices
- 2854 regs
- 114 MHz (8.76ns)


Cryptec AES speed core:
- 2112 slices
- 2984 regs
- 116 MHz. (8.62ns)
