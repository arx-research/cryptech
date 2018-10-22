libhal
======

## Overview ##

This library combines a set of low-level API functions which talk to
the Cryptech FPGA cores with a set of higher-level functions providing
various cryptographic services.

There's some overlap between the low-level code here and the low-level
code in core/platform/novena, which will need sorting out some day,
but at the time this library forked that code, the
core/platform/novena code was all written to support a test harness
rather than a higher-level API.

Current contents of the library:

* Low-level I/O code (FMC, EIM, and I2C).

* An implementation of AES Key Wrap using the Cryptech AES core.

* An interface to the Cryptech CSPRNG.

* An interface to the Cryptech hash cores, including HMAC.

* An implementation of PBKDF2.

* An implementation of RSA, optionally using the Cryptech ModExp core.

* An implementation of ECDSA, optionally using the Cryptech ECDSA base
  point multiplier cores.

* An implementation of HSS/LMS hash-based signatures.

* An interface to the Master Key Memory interface core on the Cryptech
  Alpha platform.

* A simple keystore implementation with drivers for RAM and flash
  storage on the Cryptech Alpha platform.

* A remote procedure call (RPC) interface.

* (Just enough) ASN.1 code to support a uniform interface to public
  (SubjectPublicKeyInformation (SPKI)) and private (PKCS #8) keys.

* A simple key backup mechanism, including a Python script to drive it
  from the client side.

* An RPC multiplexer to allow multiple clients (indepedent processes)
  to talk to the Cryptech Alpha at once.

* Client implenetations of the RPC mechanism in both C and Python.

* Test code for all of the above.

Most of these are fairly well self-contained, although the PBKDF2
implementation uses the hash-core-based HMAC implementation with
fallback to a software implementation if the cores aren't available.

The major exceptions are the RSA and ECDSA implementations, which uses
an external bignum implementation (libtfm) to handle a lot of the
arithmetic.  In the long run, much or all of this may end up being
implemented in Verilog, but for the moment all of the RSA math except
for modular exponentiation is happening in software, as is all of the
math for ECDSA verification; ECDSA math for key generation and signing
on the P-256 and P-384 curves is done in the ECDSA base point
multiplier cores when those are available.


## RSA ##

The RSA implementation includes a compile-time option to bypass the
ModExp core and do everything in software, because the ModExp core is
a tad slow at the moment (others are hard at work fixing this).

The RSA implementation includes optional blinding (enabled by default).


## ECDSA ##

The ECDSA implementation is specific to the NIST prime curves P-256,
P-384, and P-521.

The ECDSA implementation includes a compile-time option to allow test
code to bypass the CSPRNG in order to test against known test vectors.
Do **NOT** enable this in production builds, as ECDSA depends on good
random numbers not just for private keys but for individual
signatures, and an attacker who knows the random number used for a
particular signature can use this to recover the private key.
Arguably, this option should be removed from the code entirely.

The ECDSA software implementation attempts to be constant-time, to
reduce the risk of timing channel attacks.  The algorithms chosen for
the point arithmetic are a tradeoff between speed and code complexity,
and can probably be improved upon even in software; reimplementing at
least the field arithmetic in hardware would probably also help.
Signing and key generation performance is significantly better when
the ECDSA base point multiplier cores are available.

The point addition and point doubling algorithms in the current ECDSA
software implementation come from the [EFD][].  At least at the
moment, we're only interested in ECDSA with the NIST prime curves, so
we use algorithms optimized for a=-3.

The point multiplication algorithm is a straightforward double-and-add
loop, which is not the fastest possible algorithm, but is relatively
easy to confirm by inspection as being constant-time within the limits
imposed by the NIST curves.  Point multiplication could probably be
made faster by using a non-adjacent form (NAF) representation for the
scalar, but the author doesn't understand that well enough to
implement it as a constant-time algorithm.  In theory, changing to a
NAF representation could be done without any change to the public API.

Points stored in keys and curve parameters are in affine format, but
point arithmetic is performed in Jacobian projective coordinates, with
the coordinates themselves in Montgomery form; final mapping back to
affine coordinates also handles the final Montgomery reduction.


## Hash-Based Signatures ##

A hashsig private key is a Merkle tree of one-time signing keys, which can
be used to sign a finite number of messages. Since they don't rely on
"hard math" for security, hashsig schemes are quantum-resistant.

This hashsig code is a clean-room implementation of draft-mcgrew-hash-sigs.
It has been shown to interoperate with the Cisco reference code (each can
verify the other's signatures).

Following the recommendations of the draft, we only store the topmost hash
tree (the "root tree") in the token keystore; lower-level trees are stored
in the volatile keystore, and are regenerated upon a system restart.

This implementation has limitations on the number of keys, size of OTS
keys, and size of signatures, because of the design of the keystore and of
the RPC mechanism:

1. The token keystore is a fairly small flash, partitioned into 2048
8096-byte blocks. Therefore, we can't support LMS algorithm types >
lms_sha256_n32_h10 (a.k.a. h=10, or 1024 keys per tree). In this case,
keygen will return HAL_ERROR_NO_KEY_INDEX_SLOTS.

Additionally, the 8KB key storage size means that we can't support LM-OTS
algorithm type lmots_sha256_n32_w1, which has an OTS key size of 8504
bytes. In this case, keygen will return HAL_ERROR_UNSUPPORTED_KEY.

2. The volatile keystore is currently limited to 1280 keys, so only 2
levels at h=10, but more levels at h=5. One could easily increase the size
of the volatile keystore, but L=2/h=10 gives us a key that can sign 1M
messages, which is sufficient for development and testing purposes.

3. The RPC mechanism currently limits request and response messages to
16KB, so we can't generate or verify signatures greater than that size.
In this case, keygen will return HAL_ERROR_UNSUPPORTED_KEY.

Because the hashsig private key consists of a large number of one-time
signing keys, and because only the root tree is stored in flash, it can
take several minutes to reconstruct the full tree on system restart.
During this time, attempts to generate a hashsig key, delete a hashsig
key, or sign with a hashsig key will return HAL_ERROR_NOT_READY.

A hashsig private key can sign at most 2^(L*h) messages. (System restarts
will cause the lower-level trees to be regenerated, which will need to be
signed with by the root tree, so frequent restarts will rapidly exhaust
the root tree.) When a hashsig key is exhausted, any attempt to use it for
signing will return HAL_ERROR_HASHSIG_KEY_EXHAUSTED.


## Keystore ##

The keystore is basically a light-weight database intended to be run
directly over some kind of block-access device, with an internal
low-level driver interface so that we can use the same API for
multiple keystore devices (eg, flash for "token objects" and RAM for
"session objects", in the PKCS #11 senses of those terms).

The available storage is divided up into "blocks" of a fixed size; for
simplicity, the block size is a multiple of the subsector size of the
flash chip on the Alpha platform, since that's the minimum erasable
unit.  All state stored in the keystore itself follows the conventions
needed for flash devices, whether the device in question is flash or
not.  The basic rule here is that one can only clear bits, never set
them: the only way to set a bit is to erase the whole block and start
over.  So blocks progress from an initial state ("erased") where all
bits are set to one, through several states where the block contains
useful data, and ending in a state where all bits are set to zero
("zeroed"), because that's the way that flash hardware works.

The keystore implementation also applies a light-weight form of wear
leveling to all keystore devices, whether they're flash devices or
not.  The wear-leveling mechanism is not particularly sophisticated,
but should suffice.  The wear-leveling code treats the entirety of a
particular keystore device as a ring buffer of blocks, and keeps track
of which blocks have been used recently by zeroing blocks upon freeing
them rather than erasing them immediately, while also always keeping
the block at the current head of the free list in the erased state.
Taken together, this is enough to recover location of the block at the
head of the free list after a reboot, which is sufficient for a
round-robin wear leveling strategy.

The block format includes a field for a CRC-32 checksum, which covers
the entire block except for a few specific fields which need to be
left out.  On reboot, blocks with bad CRC-32 values are considered
candidates for reuse, but are placed at the end of the free list,
preserve their contents for as long as possible in case the real
problem is a buggy firmware update.

At the moment, the decision about whether to use the CRC-32 mechanism
is up to the individual driver: the flash driver uses it, the RAM
driver (which never stores anything across reboots anyway) does not.

Since the flash-like semantics do not allow setting bits, updates to a
block always consist of allocating a new block and copying the
modified data.  The keystore code uses a trivial lock-step protocol
for this: first:

1. The old block is marked as a "tombstone";
2. The new block (with modified data) is written;
3. The old block is erased.

This protocol is deliberately as simple as possible, so that there is
always a simple recovery path on reboot.

Active blocks within a keystore are named by UUIDs.  With one
exception, these are always type-4 (random) UUIDs, generated directly
from output of the TRNG.  The one exception is the current PIN block,
which always uses the reserved all-zeros UUID, which cannot possibly
conflict with a type-4 UUID (by definition).

The core of the keystore mechanism is the `ks->index[]` array, which
contains nothing but a list of block numbers.  This array is divided
into two parts: the first part is the index of active blocks, which is
kept sorted (by UUID); the second part is the round-robin free list.
Everything else in the keystore is indexed by these block numbers,
which means that the index array is the only data structure which the
keystore code needs to sort or rotate when adding, removing, or
updating a block.  Because the block numbers themselves are small
integers, the index array itself is small enough that shuffling data
within it using `memmove()` is a relatively cheap operation, which in
turn avoids a lot of complexity that would be involved in managing
more sophisticated data structures.

The keystore code includes both caching of recently used keystore
blocks (to avoid unnecessary flash reads) and caching of the location
of the block corresponding to a particular UUID (to avoid unnecessary
index searches).  Aside from whatever direct performance benefits this
might bring, this also frees the pkey layer that sits directly on top
of the keystore code from needing to keep a lot of active state on
particular keystore objects, which is important given that this whole
thing sits under an RPC protocol driven by a client program which can
impose arbitrary delays between any two operations at the pkey layer.


## Key backup ##

The key backup mechanism is a straightforward three-step process,
mediated by a Python script which uses the Python client
implementation of the RPC mechanism.  Steps:

1. Destination HSM (target of key transfer) generates an RSA keypair,
   exports the public key (the "key encryption key encryption key" or
   "KEKEK").

2. Source HSM (origin of the key transfer) wraps keys to be backed up
   using AES keywrap with key encryption keys (KEKs) generated by the
   TRNG; these key encryption keys are in turn encrypted with RSA
   public key (KEKEK) generated by the receipient HSM.

3. Destination HSM receives wrapped keys, unwraps the KEKs using the
   KEKEK then unwraps the wrapped private keys.

Transfer of the wrapped keys between the two HSMs can be by any
convenient mechanism; for simplicity, `cryptech_backup` script bundles
everything up in a text file using JSON and Base64 encoding.


## Multiplexer daemon ##

While the C client library can be built to talk directly to the
Cryptech Alpha board, in most cases it is more convenient to use the
`cryptech_muxd` multiplexer daemon, which is now the default.  Client
code talks to `cryptech_muxd` via a `PF_UNIX` socket; `cryptech_muxd`
handles interleaving of messages between multiple clients, and also
manages access to the Alpha's console port.

The multiplexer requires two external Python libraries, Tornado
(version 4.0 or later) and PySerial (version 3.0 or later).

In the long run, the RPC mechanism will need to be wrapped in some
kind of secure channel protocol, but we're not there yet.


## API ##

Yeah, we ought to document the API, Real Soon Now, perhaps using
[Doxygen][].  For the moment, see the function prototypes in hal.h,
the Python definitions in cryptech.libhal, and and comments in the
code.


[EFD]:		http://www.hyperelliptic.org/EFD/g1p/auto-shortw-jacobian-3.html
[Doxygen]:	http://www.doxygen.org/
