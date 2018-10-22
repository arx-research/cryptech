PKCS #11
========

## Introduction ##

This is an implementation of the [PKCS11][] API for the [Cryptech][]
project.  Like most PKCS #11 implementations, this one is incomplete
and probably always will be: PKCS #11 is very open-ended, and the
specification includes enough rope for an unwary developer to hang not
only himself, but all of his friends, relations, and casual
acquaintances.

Along with the PKCS #11 library itself, the package includes a
companion Python interface ("cryptech.py11"), which uses the ctypes
module from the Python standard library to talk to the PKCS #11
implementation.  The Python implementation is intended primarily to
simplify testing the C code, but can be used for other purposes; while
it seems unlikely that anything could ever make PKCS #11 "fun", the
`cryptech.py11` library attempts to make it a bit less awful by
providing both direct acess to the raw PKCS #11 API and a somewhat
more "pythonic" API layered on top of the raw API.


## Novel design features ##

[PKCS11][]'s data model involves an n-level-deep hierarchy of object
classes, which is somewhat tedious to implement correctly in C,
particularly if one wants the correspondence between specification and
code to be at all obvious.  In order to automate much of the drudge
work involved, this implementation uses an external representation of
the object class hierarchy, which is processed at compile time by a
Python script to generate tables which drive the C code which performs
the necessary type checking.


## Current status ##

As of this writing, the implementation supports only the RSA, ECDSA,
SHA-1, and SHA-2 algorithms, but the design is intended to be
extensible.

The underlying cryptographic support comes from the [Cryptech][]
`libhal` package.

Testing to date has been done using the `bin/pkcs11/` tools from the
BIND9 distribution, the `hsmcheck` and `ods-hsmutil` tools from the
OpenDNSSEC distribution, the `hsmbully` diagnostic tool, the Google
`pkcs11test` test suite, and a somewhat ad hoc set of unit tests using
Python's unittest library along with our own `cryptech.py11` library.

The library is also known to work as an `OpenSSL` engine when used
with the `engine-pkcs11` package spun out of the OpenSC project.  This
has not been tested extensively, but key generation, signature, and
verification all work (with RSA keys -- the engine appears not to
understand ECDSA keys, we have not investigated into details here).


## Copyright status ##

The [PKCS11][] header files are "derived from the RSA Security Inc.
PKCS #11 Cryptographic Token Interface (Cryptoki)".  See the
`pkcs11*.h` header files for details.

Code written for the [Cryptech][] project is under the usual Cryptech
BSD-style license.

[PKCS11]:    http://www.cryptsoft.com/pkcs11doc/STANDARD/       "PKCS #11"
[Cryptech]:  https://cryptech.is/                               "Cryptech"
