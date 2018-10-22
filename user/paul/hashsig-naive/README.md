hashsig-naive
=============

Reference implementation of of the LMS Hash Based Signature Scheme from
`draft-mcgrew-hash-sigs-10`.

This is a naive implementation, which hews as closely as possible to the
text of the draft, without any regard for performance (or security - keys
are stored unencrypted in the local file system). It is intended to show
that the draft can be implemented as written (except I found a few
omissions in the text), and can interoperate with the official reference
implementation at http://github.com/cisco/hash-sigs.

The user interface is modeled on `demo.c` from the Cisco implementation,
although all code was written independently.
