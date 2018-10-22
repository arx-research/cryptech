#!/usr/bin/env python

"""
Python implementation of RFC 5649  AES Key Wrap With Padding,
using PyCrypto to supply the AES code.
"""

# Terminology mostly follows the RFC, including variable names.
#
# Block sizes get confusing: AES Key Wrap uses 64-bit blocks, not to
# be confused with AES, which uses 128-bit blocks.  In practice, this
# is less confusing than when reading the description, because we
# concatenate two 64-bit blocks just prior to performing an AES ECB
# operation, then immediately split the result back into a pair of
# 64-bit blocks.

class AESKeyWrapWithPadding(object):
    """
    Implementation of AES Key Wrap With Padding from RFC 5649.
    """

    class UnwrapError(Exception):
        "Something went wrong during unwrap."

    def __init__(self, key):
        from Crypto.Cipher import AES
        self.ctx = AES.new(key, AES.MODE_ECB)

    def _encrypt(self, b1, b2):
        aes_block = self.ctx.encrypt(b1 + b2)
        return aes_block[:8], aes_block[8:]

    def _decrypt(self, b1, b2):
        aes_block = self.ctx.decrypt(b1 + b2)
        return aes_block[:8], aes_block[8:]

    @staticmethod
    def _start_stop(start, stop):                    # Syntactic sugar
        step = -1 if start > stop else 1
        return xrange(start, stop + step, step)


    def wrap_key(self, Q):
        """
        Wrap a key according to RFC 5649 section 4.1.

        Q is the plaintext to be wrapped, a byte string.

        Returns C, the wrapped ciphertext.
        """

        from struct import pack, unpack

        m = len(Q)                              # Plaintext length
        if m % 8 != 0:                          # Pad Q if needed
            Q += "\x00" * (8 - (m % 8))
        R = [pack(">LL", 0xa65959a6, m)]        # Magic MSB(32,A), build LSB(32,A)
        R.extend(Q[i : i + 8]                   # Append Q
                 for i in xrange(0, len(Q), 8))

        n = len(R) - 1

        if n == 1:
            R[0], R[1] = self._encrypt(R[0], R[1])

        else:
            # RFC 3394 section 2.2.1
            for j in self._start_stop(0, 5):
                for i in self._start_stop(1, n):
                    R[0], R[i] = self._encrypt(R[0], R[i])
                    W0, W1 = unpack(">LL", R[0])
                    W1 ^= n * j + i
                    R[0] = pack(">LL", W0, W1)

        assert len(R) == (n + 1) and all(len(r) == 8 for r in R)
        return "".join(R)


    def unwrap_key(self, C):
        """
        Unwrap a key according to RFC 5649 section 4.2.

        C is the ciphertext to be unwrapped, a byte string

        Returns Q, the unwrapped plaintext.
        """

        from struct import pack, unpack

        if len(C) % 8 != 0:
            raise self.UnwrapError("Ciphertext length {} is not an integral number of blocks"
                                   .format(len(C)))

        n = (len(C) / 8) - 1
        R = [C[i : i + 8] for i in xrange(0, len(C), 8)]

        if n == 1:
            R[0], R[1] = self._decrypt(R[0], R[1])

        else:
            # RFC 3394 section 2.2.2 steps (1), (2), and part of (3)
            for j in self._start_stop(5, 0):
                for i in self._start_stop(n, 1):
                    W0, W1 = unpack(">LL", R[0])
                    W1 ^= n * j + i
                    R[0] = pack(">LL", W0, W1)
                    R[0], R[i] = self._decrypt(R[0], R[i])

        magic, m = unpack(">LL", R[0])

        if magic != 0xa65959a6:
            raise self.UnwrapError("Magic value in AIV should have been 0xa65959a6, was 0x{:02x}"
                              .format(magic))

        if m <= 8 * (n - 1) or m > 8 * n:
            raise self.UnwrapError("Length encoded in AIV out of range: m {}, n {}".format(m, n))

        R = "".join(R[1:])
        assert len(R) ==  8 * n

        if any(r != "\x00" for r in R[m:]):
            raise self.UnwrapError("Nonzero trailing bytes {}".format(R[m:].encode("hex")))

        return R[:m]


if __name__ == "__main__":

    # Test code from here down

    import unittest

    class TestAESKeyWrapWithPadding(unittest.TestCase):

        @staticmethod
        def bin2hex(bytes, sep = ":"):
            return sep.join("{:02x}".format(ord(b)) for b in bytes)

        @staticmethod
        def hex2bin(text):
            return text.translate(None, ": \t\n\r").decode("hex")

        def loopback_test(self, I):
            K = AESKeyWrapWithPadding(self.hex2bin("00:01:02:03:04:05:06:07:08:09:0a:0b:0c:0d:0e:0f"))
            C = K.wrap_key(I)
            O = K.unwrap_key(C)
            self.assertEqual(I, O, "Input and output plaintext did not match: {!r} <> {!r}".format(I, O))

        def rfc5649_test(self, K, Q, C):
            K = AESKeyWrapWithPadding(key = self.hex2bin(K))
            Q = self.hex2bin(Q)
            C = self.hex2bin(C)
            c = K.wrap_key(Q)
            q = K.unwrap_key(C)
            self.assertEqual(q, Q, "Input and output plaintext did not match: {} <> {}".format(self.bin2hex(Q), self.bin2hex(q)))
            self.assertEqual(c, C, "Input and output ciphertext did not match: {} <> {}".format(self.bin2hex(C), self.bin2hex(c)))

        def test_rfc5649_1(self):
            self.rfc5649_test(K = "5840df6e29b02af1 ab493b705bf16ea1 ae8338f4dcc176a8",
                              Q = "c37b7e6492584340 bed1220780894115 5068f738",
                              C = "138bdeaa9b8fa7fc 61f97742e72248ee 5ae6ae5360d1ae6a 5f54f373fa543b6a")

        def test_rfc5649_2(self):
            self.rfc5649_test(K = "5840df6e29b02af1 ab493b705bf16ea1 ae8338f4dcc176a8",
                              Q = "466f7250617369",
                              C = "afbeb0f07dfbf541 9200f2ccb50bb24f")

        def test_mangled_1(self):
            self.assertRaises(AESKeyWrapWithPadding.UnwrapError, self.rfc5649_test,
                              K = "5840df6e29b02af0 ab493b705bf16ea1 ae8338f4dcc176a8",
                              Q = "466f7250617368",
                              C = "afbeb0f07dfbf541 9200f2ccb50bb24f")

        def test_mangled_2(self):
            self.assertRaises(AESKeyWrapWithPadding.UnwrapError, self.rfc5649_test,
                              K = "5840df6e29b02af0 ab493b705bf16ea1 ae8338f4dcc176a8",
                              Q = "466f7250617368",
                              C = "afbeb0f07dfbf541 9200f2ccb50bb24f 0123456789abcdef")

        def test_mangled_3(self):
            self.assertRaises(AESKeyWrapWithPadding.UnwrapError, self.rfc5649_test,
                              K = "5840df6e29b02af1 ab493b705bf16ea1 ae8338f4dcc176a8",
                              Q = "c37b7e6492584340 bed1220780894115 5068f738",
                              C = "138bdeaa9b8fa7fc 61f97742e72248ee 5ae6ae5360d1ae6a")

        def test_loopback_1(self):
            self.loopback_test("!")

        def test_loopback_2(self):
            self.loopback_test("Yo!")

        def test_loopback_3(self):
            self.loopback_test("Hi, Mom")

        def test_loopback_4(self):
            self.loopback_test("1" * (64 / 8))

        def test_loopback_5(self):
            self.loopback_test("2" * (128 / 8))

        def test_loopback_6(self):
            self.loopback_test("3" * (256 / 8))

        def test_loopback_7(self):
            self.loopback_test("3.14159265358979323846264338327950288419716939937510")

        def test_loopback_8(self):
            self.loopback_test("3.14159265358979323846264338327950288419716939937510")

        def test_loopback_9(self):
            self.loopback_test("Hello!  My name is Inigo Montoya. You killed my AES key wrapper. Prepare to die.")

        def test_joachim_loopback(self):
            from os import urandom
            I = "31:32:33"
            K = AESKeyWrapWithPadding(urandom(256/8))
            C = K.wrap_key(I)
            O = K.unwrap_key(C)
            self.assertEqual(I, O, "Input and output plaintext did not match: {!r} <> {!r}".format(I, O))

    unittest.main(verbosity = 9)
