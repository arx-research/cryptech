libtfm
======

This is a trivial port of the Tom's Fast Math (TFM) bignum library to
the Cryptech environment.  We use a git submodule to pull the package
from GitHub, we verify that the SHA-256 digest of what we got from
GitHub matches the version we tested, then we build the library with
the options we want.

See tomsfastmath/doc/tfm.pdf for API details.

In theory, the need for most (perhaps all) of this will go away when
more of the bignum math is implemented in Verilog.  Part of the reason
for using the TFM library is that its extremely modular structure make
it easy for us to link in only the functions we need.
