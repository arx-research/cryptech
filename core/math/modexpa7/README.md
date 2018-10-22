# ModExpA7

## Core Description

This core implements modular exponentiation using the Artix-7 FPGA found on CrypTech Alpha board. It can be used during RSA operations such as encryption/decryption and signing.

## Compile-Time Settings

The core has two synthesis-time parameters:

 * **OPERAND_ADDR_WIDTH** - Sets the _largest supported_ operand width. This affects the amount of block memory that is reserved for operand storage. Largest operand width in bits, that the core can handle is 32 * (2 ** OPERAND_ADDR_WIDTH). If the largest possible modulus is 1024 bits long, set OPERAND_ADDR_WIDTH = 5. For 2048-bit moduli support set OPERAND_ADDR_WIDTH = 6, for 4096-bit capable core set OPERAND_ADDR_WIDTH = 7 and so on.

 * **SYSTOLIC_ARRAY_POWER** - Determines the number of processing elements in the internal systolic array, total number of elements is 2 ** SYSTOLIC_ARRAY_POWER. This affects the number of DSP slices dedicated to parallelized multiplication. Allowed values are 1..OPERAND_ADDR_WIDTH-1, higher values produce higher performance core at the cost of higher device utilization. The number of used DSP slices is NUM_DSP = 10 + 2 * (2 + 7 * (2 ** SYSTOLIC_ARRAY_POWER)). Here's a quick reference table:
 
| SYSTOLIC_ARRAY_POWER | NUM_DSP |
|----------------------|---------|
|                    1 |      42 |
|                    2 |      70 |
|                    3 |     126 |
|                    4 |     238 |
|                    5 |     462 |
 
Given that Alpha board FPGA has 740 DSP slices, SYSTOLIC_ARRAY_POWER=5 is the largest possible setting. Note that if two cores are needed (eg. to do the two easier CRT exponentiations simultaneously), this parameter should be reduced to 4 to fit two cores into the device.
 
## API Specification

The interface of the core is similar to other CrypTech cores. FMC memory map is split into two parts, the first part contains registers and looks like the following:

| Offset | Register      |
|--------|---------------|
| 0x0000 | NAME0         |
| 0x0004 | NAME1         |
| 0x0008 | VERSION       |
| 0x0020 | CONTROL       |
| 0x0024 | STATUS        |
| 0x0040 | MODE          |
| 0x0044 | MODULUS_BITS  |
| 0x0048 | EXPONENT_BITS |
| 0x004C | BUFFER_BITS   |
| 0x0050 | ARRAY_BITS    |

The core has the following registers:

 * **NAME0**, **NAME1**  
Read-only core name ("mode", "xp7a").

 * **VERSION**  
Read-only core version, currently "0.20".

 * **CONTROL**  
Register bits:  
[31:2] Don't care, always read as 0  
[1] "next" control bit  
[0] "init" control bit  
The core uses Montgomery modular multiplier, that requires precomputation of modulus-dependent speed-up coefficient. Every time a new modulus is loaded into the core, this coefficient must be precalculated before exponentiation can be started. Changing the "init" bit from 0 to 1 starts precomputation. The core is edge-triggered, this way to start another precomputation the bit must be cleared first and then set to 1 again. The "next" control bit works the same way as the "init" bit, changing the bit from 0 to 1 triggers new exponentiation operation. The "init" bit has priority over the "next" bit, if both bits go high at the same time, precomputation will be started. When repeatedly encrypting/signing using the same modulus, precomputation needs to be done only once before the very first exponentiation.

 * **STATUS**
Read-only register bits:  
[31:2] Don't care, always read as 0  
[1] "valid" control bit  
[0] "ready" control bit  
The "valid" status bit is cleared as soon as the core starts exponentiation, and gets set after the operation is complete. The "ready" status bit is cleared when the core starts precomputation and is set after the speed-up coefficient is precalculated.

 * **MODE**  
Mode register bits:  
[31:2] Don't care, always read as 0  
[1] "CRT enable" control bit  
[0] Don't care, always read as 0  
The "CRT enable" control bit allows the core to take advantage of the Chinese remainder theorem to speed up RSA operations. When the CRT mode is disabled (MODE[1] = 0), the message (base) is assumed to be as large as the modulus. When the CRT mode is enabled (MODE[1] = 1), the message is assumed to be twice larger than the modulus and the core will reduce it before starting the exponentiation. Note that if the core was compiled for eg. 4096-bit operands (OPERAND_ADDR_WIDTH=7), it can only handle up to 2048-bit moduli in CRT mode. When singing using eg. 4096-bit public key without CRT, the modulus length must be set to 4096. When signing using the same 4096-bit public key with CRT, modulus length must be set to 2048.

* **MODULUS_BITS**  
Length of modulus in bits, must be a multiple of 32. Smallest allowed value is 64, largest allowed value is 32 * (2 ** OPERAND_ADDR_WIDTH). If the modulus is eg. 1000 bits wide, it must be prepended with 24 zeroes to make it contain an integer number of 32-bit words.

* **EXPONENT_BITS**  
Length of exponent in bits. Smallest allowed value is 2, largest allowed value is 32 * (2 ** OPERAND_ADDR_WIDTH).

* **BUFFER_BITS**  
Length of operand buffer in bits. This read-only parameter returns the length of internal operand buffer and allows the largest supported operand width to be determined at run-time.

* **ARRAY_BITS**  
Length of systolic array in bits. This read-only parameter returns the length of internal systolic multiplier array, it allows SYSTOLIC_ARRAY_POWER compile-time setting to be determined at run-time.


The second part of the address space contains eight operand banks.

Length of each bank (BANK_LENGTH) depends on the largest supported operand width: 0x80 bytes for 1024-bit core (OPERAND_ADDR_WIDTH = 5), 0x100 bytes for 2048-bit core (OPERAND_ADDR_WIDTH = 6), 0x200 bytes for 4096-bit core (OPERAND_ADDR_WIDTH = 7) and so on.

The offset of the second part is 8 * BANK_LENGTH: 0x400 for 1024-bit core, 0x800 for 2048-bit core, 0x1000 for 4096-bit core and so on. The core has the following eight banks:

| Offset           | Bank                  |
|------------------|-----------------------|
|  8 * BANK_LENGTH | MODULUS               |
|  9 * BANK_LENGTH | MESSAGE (BASE)        |
| 10 * BANK_LENGTH | EXPONENT              |
| 11 * BANK_LENGTH | RESULT                |
| 12 * BANK_LENGTH | MODULUS_COEFF_OUT     |
| 13 * BANK_LENGTH | MODULUS_COEFF_IN      |
| 14 * BANK_LENGTH | MONTGOMERY_FACTOR_OUT |
| 15 * BANK_LENGTH | MONTGOMERY_FACTOR_IN  |

MODULUS, MESSAGE and EXPONENT banks are read-write, the RESULT bank stores the result of the exponentiation and is read-only.

After precomputation the modulus-dependent speed-up coefficient and the Montgomery factor are placed in "output" MODULUS_COEFF_OUT and MONTGOMERY_FACTOR_OUT banks, the two banks are read-only. Before exponentiation corresponding modulus-dependent coefficient and Montgomery factor must be placed in "input" MODULUS_COEFF_IN and MONTGOMERY_FACTOR_IN banks, they are read-write. This split input/output banks design allows precomputed quantities to be retrieved from the core and stored along with the key for later reuse. Note that each key requires three pairs of precomputed numbers: one for the public key and two for each of the secret key components.

## Implementation Details

The top-level core module contains:
 * Block memory buffers for input and output operands
 * Block memory buffers for internal quantities
 * Precomputation module (Montgomery modulus-dependent speed-up coefficient)
 * Precomputation module (Montgomery parasitic power compensation factor)
 * Exponentiation module

The exponentiation module contains:
 * Buffers for storage of temporary values
 * Two modular multipliers that do right-to-left binary exponentiation (one multiplier does squaring, the other one does multiplication simultaneously)

The modular multiplier module contains:
 * Buffers for storage of temporary values
 * Wide operand loader
 * Systolic array of processing elements
 * Adder
 * Subtractor
 
The systolic array of processing elements contains:
 * Array of processing elements
 * Two FIFOs that accomodate carries and products

Note, that the core is supplemented by a reference model written in C, that has extensive comments describing tricky corners of the underlying math.

## Vendor-specific Primitives

CrypTech Alpha platform is based on the Xilinx Artix-7 200T FPGA, this core takes advantage of Xilinx-specific DSP slices to carry out math-intensive operations. All vendor-specific math primitives are placed under /rtl/pe/artix7/. The core also offers generic replacements under /rtl/pe/generic, they can be used for simulation with 3rd party tools, that are not aware of Xilinx-specific stuff. When porting to other architectures, only those three low-level modules need to be ported. Selection of vendor/generic primitives is done in modexpa7_primitive_switch.v. Note that if you change the latency of the processing element, the SYSTOLIC_PE_LATENCY setting in modexpa7_settings.v must be changed accordingly.
