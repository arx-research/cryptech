AES key wrap
============

A preliminary implementation of AES Key Wrap, RFC 5649 flavor, using
Cryptlib to supply the AES ECB transformations.

aes_keywrap.py contains two different Python implementations:

1. An implementation using Python longs as 64-bit integers; and

2. An implementation using Python arrays.

The first of these is the easiest to understand, as it can just do
(long) integer arithmetic and follow the specification very closely.
The second is closer to what one would do to implement this in an
assembly language like C.

aes_keywrap.[ch] is a C implementation.  The API for this is not yet
set in stone.

All three implementations include test vectors.

The two implementations based on byte arrays use shift and mask
operations to handle the two numerical values ("m" and "t") which
require byte swapping on little endian hardware; this is not the most
efficient implementation possible, but it's portable, and will almost
certainly be lost in the noise under the AES operations.
