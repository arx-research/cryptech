keywrap
=======
## Introduction ##
This core implememts AES KEY WRAP as defined in [RFC
3394](https://tools.ietf.org/html/rfc3394) and the keywrap with padding
according to [RFC 5694](https://tools.ietf.org/html/rfc5649)

The core supports wrap/unwrap of objects up to 64 kByte in size.
The core supports 128 and 256 bit wrapping keys.


## Status ##
First complete version developed. The core does work.

The core has been simulated with two different simulators and
linted.

The core has been implemented for Xilinx Artix7-t200 using ISE with the
following results:

Regs:    2906 (1%)
Slices:  1991 (5%)
RamB36E: 32 (8%)
Clock:   100+ MHz


## Implementation details
The core implements the wrap block processing part of the AES Key Wrap
as specified in chapter 2.1.1 of RFC 3394:

   For j = 0 to 5
           For i=1 to n
               B = AES(K, A | R[i])
               A = MSB(64, B) ^ t where t = (n*j)+i
               R[i] = LSB(64, B)

The core does not perform the calculation of the magic value, which is
the initial value of A. The core also does not perform padding om the
message to an even 8 byte block.

This means that SW needs to generate the 64-bit initial value of A and
perform padding as meeded.

(Similarly, the core implements the unwrap processng as specifie in
chapter 2.2.2 of RFC 3394.)


### API
Objects to be processed are written in word order (MSB words). The
caller writes the calculated magic value to the A regsisters in word
order. The caller also needs to write the number of blocks (excluding
magic block) into the RLEN register. Finally the caller needs to write
the wrapping key.

Due to address space limitations in the Cryptech cores (with 8-bit
address space) the object storage is divided into banks [0 .. 127]. Each
bank supports 128 32-bit words or 4096 bits. For objects lager than 4096
bits, it is the callers responsibilty to switch banks when reading and
writing to the storage.
