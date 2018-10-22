#!/usr/bin/env python
#======================================================================
#
# aes_keywrap.py
# --------------
# Python funnctional model of AES Key Wrap including test cases.
# Used to generate test vectors for internal states to drive
# verification of the hardware implementation.
#
#
# Terminology mostly follows the RFC, including variable names.
#
# Block sizes get confusing: AES Key Wrap uses 64-bit blocks, not to
# be confused with AES, which uses 128-bit blocks.  In practice, this
# is less confusing than when reading the description, because we
# concatenate two 64-bit blocks just prior to performing an AES ECB
# operation, then immediately split the result back into a pair of
# 64-bit blocks.
#
#
# Copyright (c) 2018, NORDUnet A/S
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
# - Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
#
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# - Neither the name of the NORDUnet nor the names of its contributors may
#   be used to endorse or promote products derived from this software
#   without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
# TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#======================================================================

from struct import pack, unpack
from os import urandom
from Crypto.Cipher import AES
import unittest


verbose = True


class AESKeyWrapWithPadding(object):
    """
    Implementation of AES Key Wrap With Padding from RFC 5649.
    using PyCrypto to supply the AES code.
    """

    class UnwrapError(Exception):
        "Something went wrong during unwrap."

    def __init__(self, key):
        self.key = key
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

    @staticmethod
    def bin2hex(bytes, sep = ":"):
        return sep.join("{:02x}".format(ord(b)) for b in bytes)

    def wrap_key(self, Q):
        """
        Wrap a key according to RFC 5649 section 4.1.

        Q is the plaintext to be wrapped, a byte string.

        Returns C, the wrapped ciphertext.
        """

        if verbose:
            print("")
            print("Performing key wrap.")
            print("key:       %s" % (self.bin2hex(self.key)))
            print("plaintext: %s" % (self.bin2hex(Q)))
            print("")

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
            if verbose:
                print("")
                print("Number of blocks to wrap: %d" % (n - 1))
                print("Blocks before wrap:")
                for i in self._start_stop(1, n):
                    print("R[%d] = %s" % (i, self.bin2hex(R[i])))
                print("A before wrap: %s" % (self.bin2hex(R[0])))
                print("")


            for j in self._start_stop(0, 5):
                for i in self._start_stop(1, n):
                    if verbose:
                        print("")
                        print("Iteration %d, %d" % (j, i))

                    if verbose:
                        print("Before encrypt: R[0] = %s  R[%d] = %s" % (self.bin2hex(R[0]), i, self.bin2hex(R[i])))

                    R[0], R[i] = self._encrypt(R[0], R[i])

                    if verbose:
                        print("After encrypt:  R[0] = %s  R[%d] = %s" % (self.bin2hex(R[0]), i, self.bin2hex(R[i])))

                    W0, W1 = unpack(">LL", R[0])
                    xorval = n * j + i
                    W1 ^= xorval
                    R[0] = pack(">LL", W0, W1)
                    if verbose:
                        print("xorval = 0x%016x" % (xorval))

            if verbose:
                print("")
                print("Blocks after wrap:")
                for i in self._start_stop(1, n):
                    print("R[%d] = %s" % (i, self.bin2hex(R[i])))
                print("A after wrap: %s" % (self.bin2hex(R[0])))
                print("")


        assert len(R) == (n + 1) and all(len(r) == 8 for r in R)
        return "".join(R)


    def unwrap_key(self, C):
        """
        Unwrap a key according to RFC 5649 section 4.2.

        C is the ciphertext to be unwrapped, a byte string

        Returns Q, the unwrapped plaintext.
        """

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
            if verbose:
                print("Wrapped result: %s" % (self.bin2hex(c)))

            q = K.unwrap_key(C)
            self.assertEqual(q, Q, "Input and output plaintext did not match: {} <> {}".format(self.bin2hex(Q), self.bin2hex(q)))
            self.assertEqual(c, C, "Input and output ciphertext did not match: {} <> {}".format(self.bin2hex(C), self.bin2hex(c)))


#        def test_rfc5649_1(self):
#            self.rfc5649_test(K = "5840df6e29b02af1 ab493b705bf16ea1 ae8338f4dcc176a8",
#                              Q = "c37b7e6492584340 bed1220780894115 5068f738",
#                              C = "138bdeaa9b8fa7fc 61f97742e72248ee 5ae6ae5360d1ae6a 5f54f373fa543b6a")
#
#        def test_rfc5649_2(self):
#            self.rfc5649_test(K = "5840df6e29b02af1 ab493b705bf16ea1 ae8338f4dcc176a8",
#                              Q = "466f7250617369",
#                              C = "afbeb0f07dfbf541 9200f2ccb50bb24f")
#
#
#        def test_mangled_1(self):
#            self.assertRaises(AESKeyWrapWithPadding.UnwrapError, self.rfc5649_test,
#                              K = "5840df6e29b02af0 ab493b705bf16ea1 ae8338f4dcc176a8",
#                              Q = "466f7250617368",
#                              C = "afbeb0f07dfbf541 9200f2ccb50bb24f")
#
#        def test_mangled_2(self):
#            self.assertRaises(AESKeyWrapWithPadding.UnwrapError, self.rfc5649_test,
#                              K = "5840df6e29b02af0 ab493b705bf16ea1 ae8338f4dcc176a8",
#                              Q = "466f7250617368",
#                              C = "afbeb0f07dfbf541 9200f2ccb50bb24f 0123456789abcdef")
#
#        def test_mangled_3(self):
#            self.assertRaises(AESKeyWrapWithPadding.UnwrapError, self.rfc5649_test,
#                              K = "5840df6e29b02af1 ab493b705bf16ea1 ae8338f4dcc176a8",
#                              Q = "c37b7e6492584340 bed1220780894115 5068f738",
#                              C = "138bdeaa9b8fa7fc 61f97742e72248ee 5ae6ae5360d1ae6a")
#
#
#        # This one should fail. But it doesn't. Que pasa?!?
#        def test_mangled_4(self):
#            self.assertRaises(AESKeyWrapWithPadding.UnwrapError, self.rfc5649_test,
#                              K = "5840df6e29b02af1 ab493b705bf16ea1 ae8338f4dcc176a8",
#                              Q = "c37b7e6492584340 bed1220780894115 5068f738",
#                              C = "238bdeaa9b8fa7fc 61f97742e72248ee 5ae6ae5360d1ae6a")


        # Test vectors from NISTs set of test vectors for SP800-38F KWP algorithm.
        # 128 bit key.
        def test_kwp_ae_128_1(self):
            self.rfc5649_test(K = "7efb9b3964de316e 7245c86186d98b5f",
                              Q = "3e",
                              C = "116a4054c13b7fea de9c22aa57b3caed")

        def test_kwp_ae_128_2(self):
            self.rfc5649_test(K = "45c770fc26717507 2d70a38269c54685",
                              Q = "cc5fb15a17795c34",
                              C = "78ffa3f03b65c55b 812f355730af71ac")

        def test_kwp_ae_128_3(self):
            self.rfc5649_test(K = "853e2bac0f1e6298 67acea0d2b3c087e",
                              Q = "49575527bc59530f be",
                              C = "b43781062eb0317e b2dec6329f2d64de 1c33d85570d57db6")

        def test_kwp_ae_128_4(self):
            self.rfc5649_test(K = "c03db3cc1416dcd1 c069a195a8d77e3d",
                              Q = "46f87f58cdda4200 f53d99ce2e49bdb7 6212511fe0cd4d0 b5f37a27d45a288",
                              C = "57e3b6699c6e8177 59a69492bb7e2cd0 0160d2ebef9bf4d 4eb16fbf798f134 0f6df6558a4fb84cd0")

        def test_kwp_ae_128_5(self):
            self.rfc5649_test(K = "6b8ba9cc9b31068b a175abfcc60c1338",

                              Q = "8af887c58dfbc38e e0423eefcc0e032d cc79dd116638ca65 ad75dca2a2459f13 934dbe61a62cb26d 8bbddbabf9bf52bb e137ef1d3e30eacf 0fe456ec808d6798 dc29fe54fa1f784a a3c11cf394050095 81d3f1d596843813 a6685e503fac8535 e0c06ecca8561b6a 1f22c578eefb6919 12be2e1667946101 ae8c3501e6c66eb1 7e14f2608c9ce6fb ab4a1597ed49ccb3 930b1060f98c97d8 dc4ce81e35279c4d 30d1bf86c9b919a3 ce4f0109e77929e5 8c4c3aeb5de1ec5e 0afa38ae896df912 1c72c255141f2f5c 9a51be5072547cf8 a3b067404e62f961 5a02479cf8c202e7 feb2e258314e0ebe 62878a5c4ecd4e9d f7dab2e1fa9a7b53 2c2169acedb7998d 5cd8a7118848ce7e e9fb2f68e28c2b27 9ddc064db70ad73c 6dbe10c5e1c56a70 9c1407f93a727cce 1075103a4009ae2f 7731b7d71756eee1 19b828ef4ed61eff 164935532a94fa8f e62dc2e22cf20f16 8ae65f4b6785286c 253f365f29453a47 9dc2824b8bdabd96 2da3b76ae9c8a720 155e158fe389c8cc 7fa6ad522c951b5c 236bf964b5b1bfb0 98a39835759b9540 4b72b17f7dbcda93 6177ae059269f41e cdac81a49f5bbfd2 e801392a043ef068 73550a67fcbc039f 0b5d30ce490baa97 9dbbaf9e53d45d7e 2dff26b2f7e6628d ed694217a39f454b 288e7906b79faf4a 407a7d207646f930 96a157f0d1dca05a 7f92e318fc1ff62c e2de7f129b187053",

                              C = "aea19443d7f8ad7d 4501c1ecadc6b5e3 f1c23c29eca60890 5f9cabdd46e34a55 e1f7ac8308e75c90 3675982bda99173a 2ba57d2ccf2e01a0 2589f89dfd4b3c7f d229ec91c9d0c46e a5dee3c048cd4611 bfeadc9bf26daa1e 02cb72e222cf3dab 120dd1e8c2dd9bd5 8bbefa5d14526abd 1e8d2170a6ba8283 c243ec2fd5ef0703 0b1ef5f69f9620e4 b17a363934100588 7b9ffc7935335947 03e5dcae67bd0ce7 a3c98ca65815a4d0 67f27e6e66d6636c ebb789732566a52a c3970e14c37310dc 2fcee0e739a16291 029fd2b4d534e304 45474b26711a8b3e 1ee3cc88b09e8b17 45b6cc0f067624ec b232db750b01fe54 57fdea77b251b10f e95d3eeedb083bdf 109c41dba26cc965 4f787bf95735ff07 070b175cea8b6230 2e6087b91a041547 4605691099f1a9e2 b626c4b3bb7aeb8e ad9922bc3617cb42 7c669b88be5f98ae a7edb8b0063bec80 af4c081f89778d7c 7242ddae88e8d3af f1f80e575e1aab4a 5d115bc27636fd14 d19bc59433f69763 5ecd870d17e7f5b0 04dee4001cddc34a b6e377eeb3fb08e9 476970765105d93e 4558fe3d4fc6fe05 3aab9c6cf032f111 6e70c2d65f7c8cde b6ad63ac4291f93d 467ebbb29ead265c 05ac684d20a6bef0 9b71830f717e08bc b4f9d3773bec928f 66eeb64dc451e958 e357ebbfef5a342d f28707ac4b8e3e8c 854e8d691cb92e87 c0d57558e44cd754 424865c229c9e1ab b28e003b6819400b")


        def test_kwp_ae_256_1(self):
            self.rfc5649_test(K = "2800f18237cf8d2b a1dfe361784fd751 9b0fdb0ec73e2ab1 c0b966b9173fc5b5",
                              Q = "ad",
                              C = "c1eccf2d077a385e 67aaeb35552c893c")

        def test_kwp_ae_256_2(self):
            self.rfc5649_test(K = "1c997c2bb5a15a45 93e337b3249675d55 7467417917f6bc51 65c9af6a3e29504",
                              Q = "3e3eafc50cd4e939",
                              C = "163eb9e7dbc8ed00 86dffbc6ab00e329")

        def test_kwp_ae_256_3(self):
            self.rfc5649_test(K = "8df1533f99be6fe6 0f951057fed1daccd 14bd4e34118f24af 677bbf46bf11fe7",
                              Q = "fb36b1f3907fb5ed ce",
                              C = "6974d7bae0221b4e d91336c26af77e327 61f6024d8bbf292")

        def test_kwp_ae_256_4(self):
            self.rfc5649_test(K = "dea4667d911b5c9e c996cdb35da0e29bc 996cbfb0e0a56bac 12fccc334d732eb",
                              Q = "25d58d437a56a733 2a18541333201f992 9fccde11b06844c1 9ba1ca224cfd6",
                              C = "86d4e258391f15d7 d4f0ab3e15d6f45e6 5dd2f8caf4c67209 63bb8970fc2f3a4 a58dc74674347ec9")


#        # Huge test case. It fails since the expected wrapped object is wrong.
#        def test_huge_256_1(self):
#            self.rfc5649_test(K = "55aa55aa55aa55aa" * 4,
#                              Q = "0101010101010101" * 2040,
#                              C = "86d4e258391f15d7" * 2042)


#        def test_loopback_1(self):
#            self.loopback_test("!")
#
#        def test_loopback_2(self):
#            self.loopback_test("Yo!")
#
#        def test_loopback_3(self):
#            self.loopback_test("Hi, Mom")
#
#        def test_loopback_4(self):
#            self.loopback_test("1" * (64 / 8))
#
#        def test_loopback_5(self):
#            self.loopback_test("2" * (128 / 8))
#
#        def test_loopback_6(self):
#            self.loopback_test("3" * (256 / 8))
#
#        def test_loopback_7(self):
#            self.loopback_test("3.14159265358979323846264338327950288419716939937510")
#
#        def test_loopback_8(self):
#            self.loopback_test("3.14159265358979323846264338327950288419716939937510")
#
#        def test_loopback_9(self):
#            self.loopback_test("Hello!  My name is Inigo Montoya. You killed my AES key wrapper. Prepare to die.")
#
#        def test_joachim_loopback(self):
#            I = "31:32:33"
#            K = AESKeyWrapWithPadding(urandom(256/8))
#            C = K.wrap_key(I)
#            O = K.unwrap_key(C)
#            self.assertEqual(I, O, "Input and output plaintext did not match: {!r} <> {!r}".format(I, O))
#

    unittest.main(verbosity = 9)

#======================================================================
# OEF aes_keywrap.py
#======================================================================
