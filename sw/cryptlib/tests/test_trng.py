# Trivial test of cryptech trng and noise cores via cryptlib python interface.
# Might upgrade to Python's unittest framework eventually.

import atexit
import os

print "[Loading cryptlib]"
from cryptlib_py import *

if os.getenv("SET_GDB_BREAKPOINTS"):
    print "[Sending SIGINT to self to throw to gdb]"
    os.kill(os.getpid(), 2)
    print "[Continuing]"

print "[Initializing cryptlib]"
cryptInit()
atexit.register(cryptEnd)

print "[Opening device]"
dev = cryptDeviceOpen(CRYPT_UNUSED, CRYPT_DEVICE_HARDWARE, None)
atexit.register(cryptDeviceClose, dev)

use_dev_context = False

def generate_key(i):
    label = "RSA-%04d" % i
    print "[Generating key %s]" % label
    ctx = None
    try:
        if use_dev_context:
            ctx = cryptDeviceCreateContext(dev, CRYPT_ALGO_RSA)
        else:
            ctx = cryptCreateContext(CRYPT_UNUSED, CRYPT_ALGO_RSA)
        ctx.CTXINFO_LABEL = label
        ctx.CTXINFO_KEYSIZE = 2048 / 8
        cryptGenerateKey(ctx)
    finally:
        if ctx is not None:
            cryptDestroyContext(ctx)

have_i2c = os.path.exists("/dev/i2c-2")

if not have_i2c:
    print
    print "[I2C device not found, so testing software only, no hardware cores tested]"

for i in xrange(10):
    generate_key(i)
