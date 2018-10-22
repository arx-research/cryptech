# This is not a script in its own right, it's included by the other
# scripts to let us keep all the environment variables in a single place.

# Where the PKCS #11 module lives, usually /usr/lib/libcryptech-pkcs11.so

export PKCS11_MODULE=/usr/lib/libcryptech-pkcs11.so

# PIN for user account on the HSM.  In production you would NOT
# want this in an environment variable!

export PKCS11_PIN=fnord

# Where to find our OpenSSL configuration file.

export OPENSSL_CONF=`pwd`/openssl.conf

# Where to find the engine module this week (its name changes with
# architecture, OpenSSL version, and phase of the moon).

export ENGINE_MODULE=`dpkg -L libengine-pkcs11-openssl | egrep '/(engine_)?pkcs11[.]so$'`

# If USE_PKCS11SPY is set, it should be an absolute path to the OpenSC
# pkcs11-spy.so debugging tool, which we will splice between OpenSSL
# and the real PKCS #11 library.  This is not something you would want
# to do in production, but can be useful when testing.  See
#
#   https://github.com/OpenSC/OpenSC/wiki/Using-OpenSC
#
# By default, pkcs11-spy writes to stderr, but you can override this
# by setting PKCS11SPY_OUTPUT.

if test "X$USE_PKCS11SPY" != "X" && test -r "$USE_PKCS11SPY"
then
    export PKCS11SPY="$PKCS11_MODULE"
    export PKCS11_MODULE="$USE_PKCS11SPY"
fi
