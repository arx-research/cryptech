# ecdhp256

## Core Description

This core implements the scalar point multiplier for ECDSA curve P-256. It can be used during generation of public keys, the core can also be used as part of the signing operation, it can also do ECDH key exchange.

## API Specification

The core interface is similar to other Cryptech cores. FMC memory map looks like the following:

`0x0000 | NAME0`  
`0x0004 | NAME1`  
`0x0008 | VERSION`  

`0x0020 | CONTROL`  
`0x0024 | STATUS`  

`0x0100 | K0`  
`0x0104 | K1`  
`...`  
`0x011C | K7`  

`0x0120 | XIN0`  
`0x0124 | XIN1`  
`...`  
`0x013C | XIN7`
  
`0x0140 | YIN0`  
`0x0144 | YIN1`  
`...`  
`0x015C | YIN7`  

`0x0160 | XOUT0`  
`0x0164 | XOUT1`  
`...`  
`0x017C | XOUT7`  

`0x0180 | YOUT0`  
`0x0184 | YOUT1`  
`...`  
`0x019C | YOUT7`  

The core has the following registers:

 * **NAME0**, **NAME1**
Read-only core name ("ecdhp256").

 * **VERSION**
Read-only core version, currently "0.10".

 * **CONTROL**
Control register bits:
[31:2] Don't care, always read as 0
[1] "next" control bit
[0] Don't care, always read as 0
The core starts multiplication when the "next" control bit changes from 0 to 1. This way when the bit is set, the core will only perform one multiplication and then stop. To start another operation, the bit must be cleared at first and then set to 1 again.

 * **STATUS**
Read-only status register bits:
[31:2] Don't care, always read as 0
[1] "valid" control bit
[0] "ready" control bit (always read as 1)
The "valid" control bit is cleared as soon as the core starts operation, and gets set after the multiplication operations is complete. Note, that unlike some other Cryptech cores, this core doesn't need any special initialization, so the "ready" control bit is simply hardwired to always read as 1. This is to keep general core interface consistency.

 * **K0**-**K7**
Buffer for the 256-bit multiplication factor (multiplier) K. The core will compute R(XOUT, YOUT) = K * P(XIN, YIN). K0 is the least significant 32-bit word of K, i.e. bits [31:0], while K7 is the most significant 32-bit word of K, i.e. bits [255:224].

 * **XIN0**-**XIN7**, **YIN0**-**YIN7**
Writeable buffers for the 256-bit coordinates X and Y of the input multiplicand P(XIN, YIN). Values should be in affine coordinates. XIN0 and YIN0 contain the least significant 32-bit words, i.e. bits [31:0], while XIN7 and YIN7 contain the most significant 32-bit words, i.e. bits [255:224]. Fill the buffers with coordinates of the base point during public key generation and during multiplication by the per-message (random) number. Fill the buffers with coordinates of Bob's public key to derive Alice's copy of the shared secret key.

 * **XOUT0**-**XOUT7**, **YOUT0**-**YOUT7**
Read-only buffers for the 256-bit coordinates X and Y of the product R(XOUT, YOUT). Values are returned in affine coordinates. XOUT0 and YOUT0 contain the least significant 32-bit words, i.e. bits [31:0], while XOUT7 and YOUT7 contain the most significant 32-bit words, i.e. bits [255:224].

## Implementation Details

The top-level core module contains block memory buffers for input and output operands and the base point multiplier, that reads from the input buffer and writes to the output buffers.

The base point multiplier itself consists of the following:
 * Buffers for storage of temporary values
 * Configurable "worker" unit
 * Microprograms for the worker unit
 * Multi-word mover unit
 * Modular inversion unit

The "worker" unit can execute five basic operations:
 * comparison
 * copying
 * modular addition
 * modular subtraction
 * modular multiplications

There are two primary microprograms, that the worker runs: curve point doubling and addition of curve point to the base point. Those microprograms use projective Jacobian coordinates, so one more microprogram is used to convert the product into affine coordinates with the help of modular inversion unit.

Note, that the core is supplemented by a reference model written in C, that has extensive comments describing tricky corners of the underlying math.

## Vendor-specific Primitives

Cryptech Alpha platform is based on Xilinx Artix-7 200T FPGA, so this core takes advantage of Xilinx-specific DSP slices to carry out math-intensive operations. All vendor-specific math primitives are placed under /rtl/lowlevel/artix7, the core also offers generic replacements under /rtl/lowlevel/generic, they can be used for simulation with 3rd party tools, that are not aware of Xilinx-specific stuff. Selection of vendor/generic primitives is done in ecdsa_lowlevel_settings.v, when porting to other architectures, only those four low-level modules need to be ported.
