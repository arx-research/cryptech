# Trivial test of cryptech hash cores via cryptlib python interface.
# Might upgrade to Python's unittest framework eventually.

import atexit, os.path
from cryptlib_py import *

cryptInit()
atexit.register(cryptEnd)

hwdev = cryptDeviceOpen(CRYPT_UNUSED, CRYPT_DEVICE_HARDWARE, None)
atexit.register(cryptDeviceClose, hwdev)

# Usual NIST sample messages.

def hextext(text):
    return "".join(text.split()).lower()

NIST_512_SINGLE      = "abc"
SHA1_SINGLE_DIGEST   = hextext("A9993E36 4706816A BA3E2571 7850C26C 9CD0D89D")
SHA256_SINGLE_DIGEST = hextext("BA7816BF 8F01CFEA 414140DE 5DAE2223 B00361A3 96177A9C B410FF61 F20015AD")

NIST_512_DOUBLE      = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
SHA1_DOUBLE_DIGEST   = hextext("84983E44 1C3BD26E BAAE4AA1 F95129E5 E54670F1")
SHA256_DOUBLE_DIGEST = hextext("248D6A61 D20638B8 E5C02693 0C3E6039 A33CE459 64FF2167 F6ECEDD4 19DB06C1")

NIST_1024_SINGLE     = "abc"
SHA384_SINGLE_DIGEST = hextext("CB00753F 45A35E8B B5A03D69 9AC65007 272C32AB 0EDED163"
                               "1A8B605A 43FF5BED 8086072B A1E7CC23 58BAECA1 34C825A7")
SHA512_SINGLE_DIGEST = hextext("DDAF35A1 93617ABA CC417349 AE204131 12E6FA4E 89A97EA2 0A9EEEE6 4B55D39A"
                               "2192992A 274FC1A8 36BA3C23 A3FEEBBD 454D4423 643CE80E 2A9AC94F A54CA49F")

NIST_1024_DOUBLE     = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmn" \
                       "hijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu"
SHA384_DOUBLE_DIGEST = hextext("09330C33 F71147E8 3D192FC7 82CD1B47 53111B17 3B3B05D2"
                               "2FA08086 E3B0F712 FCC7C71A 557E2DB9 66C3E9FA 91746039")
SHA512_DOUBLE_DIGEST = hextext("8E959B75 DAE313DA 8CF4F728 14FC143F 8F7779C6 EB9F7FA1 7299AEAD B6889018"
                               "501D289E 4900F7E4 331B99DE C4B5433A C7D329EE B6DD2654 5E96E55B 874BE909")

def do_hash(dev, algorithm, text, blocksize = None):
    ctx = None
    try:
        if dev is None:
            ctx = cryptCreateContext(CRYPT_UNUSED, algorithm)
        else:
            ctx = cryptDeviceCreateContext(dev, algorithm)
        if blocksize is not None:
            ctx.CTXINFO_BLOCKSIZE = blocksize
        cryptEncrypt(ctx, array("c", text))
        cryptEncrypt(ctx, array("c", ""))
        result = ctx.CRYPT_CTXINFO_HASHVALUE
        return result.encode("hex")
    finally:
        if ctx is not None:
            cryptDestroyContext(ctx)

def sha1(dev, text):
    return do_hash(dev, CRYPT_ALGO_SHA1, text)

def sha256(dev, text):
    return do_hash(dev, CRYPT_ALGO_SHA2, text)

def sha384(dev, text):
    return do_hash(dev, CRYPT_ALGO_SHA2, text, 48)

def sha512(dev, text):
    return do_hash(dev, CRYPT_ALGO_SHA2, text, 64)

have_i2c = os.path.exists("/dev/i2c-2")

if not have_i2c:
    print
    print "I2C device not found, so testing software only, no hardware cores tested"

def test(digest, text, expect):
    print
    print "Testing %s(%r)" % (digest.__name__, text)
    hashes = [digest(None, text)]
    if have_i2c:
        hashes.append(digest(hwdev, text))
    for hash in hashes:
        if hash == expect:
            print "+", hash
        else:
            print "-", hash
            print "!", expect

test(sha1,   NIST_512_SINGLE,  SHA1_SINGLE_DIGEST)
test(sha1,   NIST_512_DOUBLE,  SHA1_DOUBLE_DIGEST)
test(sha256, NIST_512_SINGLE,  SHA256_SINGLE_DIGEST)
test(sha256, NIST_512_DOUBLE,  SHA256_DOUBLE_DIGEST)
test(sha384, NIST_1024_SINGLE, SHA384_SINGLE_DIGEST)
test(sha384, NIST_1024_DOUBLE, SHA384_DOUBLE_DIGEST)
test(sha512, NIST_1024_SINGLE, SHA512_SINGLE_DIGEST)
test(sha512, NIST_1024_DOUBLE, SHA512_DOUBLE_DIGEST)
