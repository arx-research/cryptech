#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#=======================================================================
#
# sha512.py
# ---------
# Simple, pure Python model of the SHA-512 hash function. Used as a
# reference for the HW implementation. The code follows the structure
# of the HW implementation as much as possible.
#
#
# Author: Joachim Strömbergson
# Copyright (c) 2014, NORDUnet A/S
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
#=======================================================================

#-------------------------------------------------------------------
# Python module imports.
#-------------------------------------------------------------------
import sys


#-------------------------------------------------------------------
# Constants.
#-------------------------------------------------------------------
MAX_64BIT = 0xffffffffffffffff


#-------------------------------------------------------------------
# ChaCha()
#-------------------------------------------------------------------
class SHA512():
    def __init__(self, mode = 'MODE_SHA_512', verbose = 0):
        assert mode in ['MODE_SHA_512_224', 'MODE_SHA_512_256',
                        'MODE_SHA_512_384', 'MODE_SHA_512']
        self.mode = mode
        self.verbose = verbose
        self.mode
        self.NUM_ROUNDS = 80
        self.H = [0] * 8
        self.t1 = 0
        self.t2 = 0
        self.a = 0
        self.b = 0
        self.c = 0
        self.d = 0
        self.e = 0
        self.f = 0
        self.g = 0
        self.h = 0
        self.w = 0
        self.W = [0] * 16
        self.k = 0
        self.K = [0x428a2f98d728ae22, 0x7137449123ef65cd, 0xb5c0fbcfec4d3b2f,
                  0xe9b5dba58189dbbc, 0x3956c25bf348b538, 0x59f111f1b605d019,
                  0x923f82a4af194f9b, 0xab1c5ed5da6d8118, 0xd807aa98a3030242,
                  0x12835b0145706fbe, 0x243185be4ee4b28c, 0x550c7dc3d5ffb4e2,
                  0x72be5d74f27b896f, 0x80deb1fe3b1696b1, 0x9bdc06a725c71235,
                  0xc19bf174cf692694, 0xe49b69c19ef14ad2, 0xefbe4786384f25e3,
                  0x0fc19dc68b8cd5b5, 0x240ca1cc77ac9c65, 0x2de92c6f592b0275,
                  0x4a7484aa6ea6e483, 0x5cb0a9dcbd41fbd4, 0x76f988da831153b5,
                  0x983e5152ee66dfab, 0xa831c66d2db43210, 0xb00327c898fb213f,
                  0xbf597fc7beef0ee4, 0xc6e00bf33da88fc2, 0xd5a79147930aa725,
                  0x06ca6351e003826f, 0x142929670a0e6e70, 0x27b70a8546d22ffc,
                  0x2e1b21385c26c926, 0x4d2c6dfc5ac42aed, 0x53380d139d95b3df,
                  0x650a73548baf63de, 0x766a0abb3c77b2a8, 0x81c2c92e47edaee6,
                  0x92722c851482353b, 0xa2bfe8a14cf10364, 0xa81a664bbc423001,
                  0xc24b8b70d0f89791, 0xc76c51a30654be30, 0xd192e819d6ef5218,
                  0xd69906245565a910, 0xf40e35855771202a, 0x106aa07032bbd1b8,
                  0x19a4c116b8d2d0c8, 0x1e376c085141ab53, 0x2748774cdf8eeb99,
                  0x34b0bcb5e19b48a8, 0x391c0cb3c5c95a63, 0x4ed8aa4ae3418acb,
                  0x5b9cca4f7763e373, 0x682e6ff3d6b2b8a3, 0x748f82ee5defb2fc,
                  0x78a5636f43172f60, 0x84c87814a1f0ab72, 0x8cc702081a6439ec,
                  0x90befffa23631e28, 0xa4506cebde82bde9, 0xbef9a3f7b2c67915,
                  0xc67178f2e372532b, 0xca273eceea26619c, 0xd186b8c721c0c207,
                  0xeada7dd6cde0eb1e, 0xf57d4f7fee6ed178, 0x06f067aa72176fba,
                  0x0a637dc5a2c898a6, 0x113f9804bef90dae, 0x1b710b35131c471b,
                  0x28db77f523047d84, 0x32caab7b40c72493, 0x3c9ebe0a15c9bebc,
                  0x431d67c49c100d4c, 0x4cc5d4becb3e42b6, 0x597f299cfc657e2a,
                  0x5fcb6fab3ad6faec, 0x6c44198c4a475817]


    def init(self):
        if self.mode == 'MODE_SHA_512_224':
            self.H = [0x8c3d37c819544da2, 0x73e1996689dcd4d6,
                      0x1dfab7ae32ff9c82, 0x679dd514582f9fcf,
                      0x0f6d2b697bd44da8, 0x77e36f7304c48942,
                      0x3f9d85a86a1d36c8, 0x1112e6ad91d692a1]

        elif self.mode == 'MODE_SHA_512_256':
            self.H = [0x22312194fc2bf72c, 0x9f555fa3c84c64c2,
                      0x2393b86b6f53b151, 0x963877195940eabd,
                      0x96283ee2a88effe3, 0xbe5e1e2553863992,
                      0x2b0199fc2c85b8aa, 0x0eb72ddc81c52ca2]

        elif self.mode == 'MODE_SHA_512_384':
            self.H = [0xcbbb9d5dc1059ed8, 0x629a292a367cd507,
                      0x9159015a3070dd17, 0x152fecd8f70e5939,
                      0x67332667ffc00b31, 0x8eb44a8768581511,
                      0xdb0c2e0d64f98fa7, 0x47b5481dbefa4fa4]

        elif self.mode == 'MODE_SHA_512':
            self.H = [0x6a09e667f3bcc908, 0xbb67ae8584caa73b,
                      0x3c6ef372fe94f82b, 0xa54ff53a5f1d36f1,
                      0x510e527fade682d1, 0x9b05688c2b3e6c1f,
                      0x1f83d9abfb41bd6b, 0x5be0cd19137e2179]


    def next(self, block):
        self._W_schedule(block)
        self._copy_digest()
        if self.verbose:
            print("State after init:")
            self._print_state(0)

        for i in range(self.NUM_ROUNDS):
            self._sha512_round(i)
            if self.verbose:
                self._print_state(i)

        self._update_digest()


    def get_digest(self):
        if self.mode == 'MODE_SHA_512_224':
            d = self.H[0:4]
            d[3] = d[3] & 0xffffffff00000000
            return d

        elif self.mode == 'MODE_SHA_512_256':
            return self.H[0:4]

        elif self.mode == 'MODE_SHA_512_384':
            return self.H[0:6]

        elif self.mode == 'MODE_SHA_512':
            return self.H


    def _copy_digest(self):
        self.a = self.H[0]
        self.b = self.H[1]
        self.c = self.H[2]
        self.d = self.H[3]
        self.e = self.H[4]
        self.f = self.H[5]
        self.g = self.H[6]
        self.h = self.H[7]


    def _update_digest(self):
        self.H[0] = (self.H[0] + self.a) & MAX_64BIT
        self.H[1] = (self.H[1] + self.b) & MAX_64BIT
        self.H[2] = (self.H[2] + self.c) & MAX_64BIT
        self.H[3] = (self.H[3] + self.d) & MAX_64BIT
        self.H[4] = (self.H[4] + self.e) & MAX_64BIT
        self.H[5] = (self.H[5] + self.f) & MAX_64BIT
        self.H[6] = (self.H[6] + self.g) & MAX_64BIT
        self.H[7] = (self.H[7] + self.h) & MAX_64BIT


    def _print_state(self, round):
        print("State at round 0x%02x:" % round)
        print("t1 = 0x%016x, t2 = 0x%016x" % (self.t1, self.t2))
        print("k  = 0x%016x, w  = 0x%016x" % (self.k,  self.w))
        print("a  = 0x%016x, b  = 0x%016x" % (self.a,  self.b))
        print("c  = 0x%016x, d  = 0x%016x" % (self.c,  self.d))
        print("e  = 0x%016x, f  = 0x%016x" % (self.e,  self.f))
        print("g  = 0x%016x, h  = 0x%016x" % (self.g,  self.h))
        print()


    def _sha512_round(self, round):
        self.k = self.K[round]
        self.w = self._next_w(round)
        self.t1 = self._T1(self.e, self.f, self.g, self.h, self.k, self.w)
        self.t2 = self._T2(self.a, self.b, self.c)
        self.h = self.g
        self.g = self.f
        self.f = self.e
        self.e = (self.d + self.t1) & MAX_64BIT
        self.d = self.c
        self.c = self.b
        self.b = self.a
        self.a = (self.t1 + self.t2) & MAX_64BIT


    def _next_w(self, round):
        if (round < 16):
            return self.W[round]

        else:
            tmp_w = (self._delta1(self.W[14]) +
                     self.W[9] +
                     self._delta0(self.W[1]) +
                     self.W[0]) & MAX_64BIT
            for i in range(15):
                self.W[i] = self.W[(i+1)]
            self.W[15] = tmp_w
            return tmp_w


    def _W_schedule(self, block):
        for i in range(16):
            self.W[i] = block[i]


    def _Ch(self, x, y, z):
        return (x & y) ^ (~x & z)


    def _Maj(self, x, y, z):
        return (x & y) ^ (x & z) ^ (y & z)

    def _sigma0(self, x):
        return (self._rotr64(x, 28) ^ self._rotr64(x, 34) ^ self._rotr64(x, 39))


    def _sigma1(self, x):
        return (self._rotr64(x, 14) ^ self._rotr64(x, 18) ^ self._rotr64(x, 41))


    def _delta0(self, x):
        return (self._rotr64(x, 1) ^ self._rotr64(x, 8) ^ self._shr64(x, 7))


    def _delta1(self, x):
        return (self._rotr64(x, 19) ^ self._rotr64(x, 61) ^ self._shr64(x, 6))


    def _T1(self, e, f, g, h, k, w):
        T1 = (h + self._sigma1(e) + self._Ch(e, f, g) + k + w) & MAX_64BIT
        if self.verbose:
            print("Inputs, calculations and result for T1:")
            print("e = 0x%016x, f = 0x%016x, g = 0x%016x, h = 0x%016x" % (e, f, g, h))
            print("k = 0x%016x, w = 0x%016x" % (k, w))
            print("Ch = 0x%016x, sigma1 = 0x%016x" % (self._Ch(e, f, g), self._sigma1(e)))
            print("T1 = 0x%016x" % (T1))
            print()
        return T1


    def _T2(self, a, b, c):
        T2 = (self._sigma0(a) + self._Maj(a, b, c)) & MAX_64BIT
        if self.verbose:
            print("Inputs, calculations and result for T2:")
            print("a = 0x%016x, b = 0x%016x, c = 0x%016x" % (a, b, c))
            print("Maj = 0x%016x, sigma0 = 0x%016x" % (self._Maj(a, b, c), self._sigma0(a)))
            print("T2 = 0x%016x" % (T2))
            print()
        return T2


    def _rotr64(self, n, r):
        return ((n >> r) | (n << (64 - r))) & MAX_64BIT


    def _shr64(self, n, r):
        return (n >> r)


#-------------------------------------------------------------------
# compare_digests()
#
# Compare if two given digests are equal or not.
#-------------------------------------------------------------------
def compare_digests(digest, expected):
    if (digest != expected):
        print("Error:")
        print("Got:")
        for i in digest:
            print("0x%016x " % (i), end="")
        print()
        print("Expected:")
        for i in expected:
            print("0x%016x " % (i), end="")
        print()
    else:
        print("Test case ok.")
    print()


#-------------------------------------------------------------------
# double_block_tests()
#
# NIST tests with dual block messages.
#-------------------------------------------------------------------
def double_block_tests():
    print("Running double block message tests.")

    TC_BLOCK1 = [0x6162636465666768, 0x6263646566676869, 0x636465666768696a, 0x6465666768696a6b,
                 0x65666768696a6b6c, 0x666768696a6b6c6d, 0x6768696a6b6c6d6e, 0x68696a6b6c6d6e6f,
                 0x696a6b6c6d6e6f70, 0x6a6b6c6d6e6f7071, 0x6b6c6d6e6f707172, 0x6c6d6e6f70717273,
                 0x6d6e6f7071727374, 0x6e6f707172737475, 0x8000000000000000, 0x0000000000000000]

    TC_BLOCK2 = [0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
                 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
                 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
                 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000380]

    print("Test case for SHA-512-224.")
    TC256_expected = [0x23fec5bb94d60b23, 0x308192640b0c4533, 0x35d664734fe40e72, 0x68674af900000000]
    my_sha512 = SHA512(mode = 'MODE_SHA_512_224', verbose = 0)
    my_sha512.init()
    my_sha512.next(TC_BLOCK1)
    my_sha512.next(TC_BLOCK2)
    my_digest = my_sha512.get_digest()
    compare_digests(my_digest, TC256_expected)

    print("Test case for SHA-512-256.")
    TC256_expected = [0x3928e184fb8690f8, 0x40da3988121d31be, 0x65cb9d3ef83ee614, 0x6feac861e19b563a]
    my_sha512 = SHA512(mode = 'MODE_SHA_512_256', verbose = 0)
    my_sha512.init()
    my_sha512.next(TC_BLOCK1)
    my_sha512.next(TC_BLOCK2)
    my_digest = my_sha512.get_digest()
    compare_digests(my_digest, TC256_expected)

    print("Test case for SHA-512-384.")
    TC384_expected = [0x09330c33f71147e8, 0x3d192fc782cd1b47, 0x53111b173b3b05d2, 0x2fa08086e3b0f712,
                      0xfcc7c71a557e2db9, 0x66c3e9fa91746039]
    my_sha512 = SHA512(mode = 'MODE_SHA_512_384', verbose = 0)
    my_sha512.init()
    my_sha512.next(TC_BLOCK1)
    my_sha512.next(TC_BLOCK2)
    my_digest = my_sha512.get_digest()
    compare_digests(my_digest, TC384_expected)

    print("Test case for SHA-512.")
    TC512_expected = [0x8e959b75dae313da, 0x8cf4f72814fc143f, 0x8f7779c6eb9f7fa1, 0x7299aeadb6889018,
                      0x501d289e4900f7e4, 0x331b99dec4b5433a, 0xc7d329eeb6dd2654, 0x5e96e55b874be909]
    my_sha512 = SHA512(mode = 'MODE_SHA_512', verbose = 0)
    my_sha512.init()
    my_sha512.next(TC_BLOCK1)
    my_sha512.next(TC_BLOCK2)
    my_digest = my_sha512.get_digest()
    compare_digests(my_digest, TC512_expected)


#-------------------------------------------------------------------
# single_block_tests()
#
# NIST test with single block messages for the different
# versions of SHA-512.
#-------------------------------------------------------------------
def single_block_tests():
    print("Running single block message tests.")

    TC_BLOCK = [0x6162638000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
                0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
                0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
                0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000018]

    print("Test case for SHA-512-224.")
    TC224_expected = [0x4634270f707b6a54, 0xdaae7530460842e2, 0x0e37ed265ceee9a4, 0x3e8924aa00000000]
    my_sha512 = SHA512(mode = 'MODE_SHA_512_224', verbose = 0)
    my_sha512.init()
    my_sha512.next(TC_BLOCK)
    my_digest = my_sha512.get_digest()
    compare_digests(my_digest, TC224_expected)

    print("Test case for SHA-512-256.")
    TC256_expected = [0x53048e2681941ef9, 0x9b2e29b76b4c7dab, 0xe4c2d0c634fc6d46, 0xe0e2f13107e7af23]
    my_sha512 = SHA512(mode = 'MODE_SHA_512_256', verbose = 0)
    my_sha512.init()
    my_sha512.next(TC_BLOCK)
    my_digest = my_sha512.get_digest()
    compare_digests(my_digest, TC256_expected)

    print("Test case for SHA-512-384.")
    TC384_expected = [0xcb00753f45a35e8b, 0xb5a03d699ac65007, 0x272c32ab0eded163,
                      0x1a8b605a43ff5bed, 0x8086072ba1e7cc23, 0x58baeca134c825a7]
    my_sha512 = SHA512(mode = 'MODE_SHA_512_384', verbose = 0)
    my_sha512.init()
    my_sha512.next(TC_BLOCK)
    my_digest = my_sha512.get_digest()
    compare_digests(my_digest, TC384_expected)

    print("Test case for SHA-512.")
    TC512_expected = [0xddaf35a193617aba, 0xcc417349ae204131, 0x12e6fa4e89a97ea2, 0x0a9eeee64b55d39a,
                      0x2192992a274fc1a8, 0x36ba3c23a3feebbd, 0x454d4423643ce80e, 0x2a9ac94fa54ca49f]
    my_sha512 = SHA512(mode = 'MODE_SHA_512', verbose = 0)
    my_sha512.init()
    my_sha512.next(TC_BLOCK)
    my_digest = my_sha512.get_digest()
    compare_digests(my_digest, TC512_expected)
    print()


#-------------------------------------------------------------------
# main()
#
# If executed tests the ChaCha class using known test vectors.
#-------------------------------------------------------------------
def main():
    print("Testing the SHA-512 Python model.")
    print("---------------------------------")
    print

    single_block_tests()
    double_block_tests()


#-------------------------------------------------------------------
# __name__
# Python thingy which allows the file to be run standalone as
# well as parsed from within a Python interpreter.
#-------------------------------------------------------------------
if __name__=="__main__":
    # Run the main function.
    sys.exit(main())

#=======================================================================
# EOF sha512.py
#=======================================================================
