#!/usr/bin/env python
# -*- coding: utf-8 -*-
#=======================================================================
#
# hash_tester.py
# --------------
# This program sends several commands to the coretest_hashes subsystem
# in order to verify the SHA-1, SHA-256 and SHA-512/x hash function
# cores. The program will use the built in hash implementations in
# Python to do functional comparison and validation.
#
# This version of the program talks to the FPGA over an I2C bus, but
# does not require any additional modules.
#
# The single and dual block test cases are taken from the
# NIST KAT document:
# http://csrc.nist.gov/groups/ST/toolkit/documents/Examples/SHA_All.pdf
#
#
# Authors: Joachim Strömbergson, Paul Selkirk
# Copyright (c) 2014, SUNET
#
# Redistribution and use in source and binary forms, with or
# without modification, are permitted provided that the following
# conditions are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#=======================================================================

#-------------------------------------------------------------------
# Python module imports.
#-------------------------------------------------------------------
import sys
import io
import fcntl

#-------------------------------------------------------------------
# Defines.
#-------------------------------------------------------------------

# addresses and codes common to all hash cores
ADDR_NAME0       = 0x00
ADDR_NAME1       = 0x01
ADDR_VERSION     = 0x02
ADDR_CTRL        = 0x08
CTRL_INIT_CMD    = 1
CTRL_NEXT_CMD    = 2
ADDR_STATUS      = 0x09
STATUS_READY_BIT = 0
STATUS_VALID_BIT = 1

#----------------------------------------------------------------
# NIST sample message blocks and expected digests
#----------------------------------------------------------------

# SHA-1/SHA-256 One Block Message Sample
# Input Message: "abc"
NIST_512_SINGLE = [ 0x61626380, 0x00000000, 0x00000000, 0x00000000,
                    0x00000000, 0x00000000, 0x00000000, 0x00000000,
                    0x00000000, 0x00000000, 0x00000000, 0x00000000,
                    0x00000000, 0x00000000, 0x00000000, 0x00000018 ]

SHA1_SINGLE_DIGEST   = [ 0xa9993e36, 0x4706816a, 0xba3e2571, 0x7850c26c,
                         0x9cd0d89d ]

SHA256_SINGLE_DIGEST = [ 0xBA7816BF, 0x8F01CFEA, 0x414140DE, 0x5DAE2223,
                         0xB00361A3, 0x96177A9C, 0xB410FF61, 0xF20015AD ]

# SHA-1/SHA-256 Two Block Message Sample
# Input Message: "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
NIST_512_DOUBLE0 = [ 0x61626364, 0x62636465, 0x63646566, 0x64656667,
                     0x65666768, 0x66676869, 0x6768696A, 0x68696A6B,
                     0x696A6B6C, 0x6A6B6C6D, 0x6B6C6D6E, 0x6C6D6E6F,
                     0x6D6E6F70, 0x6E6F7071, 0x80000000, 0x00000000 ]
NIST_512_DOUBLE1 = [ 0x00000000, 0x00000000, 0x00000000, 0x00000000,
                     0x00000000, 0x00000000, 0x00000000, 0x00000000,
                     0x00000000, 0x00000000, 0x00000000, 0x00000000,
                     0x00000000, 0x00000000, 0x00000000, 0x000001C0 ]

SHA1_DOUBLE_DIGEST   = [ 0x84983E44, 0x1C3BD26E, 0xBAAE4AA1, 0xF95129E5,
                         0xE54670F1 ]

SHA256_DOUBLE_DIGEST = [ 0x248D6A61, 0xD20638B8, 0xE5C02693, 0x0C3E6039,
                         0xA33CE459, 0x64FF2167, 0xF6ECEDD4, 0x19DB06C1 ]

# SHA-512 One Block Message Sample
# Input Message: "abc"
NIST_1024_SINGLE = [ 0x61626380, 0x00000000, 0x00000000, 0x00000000,
                     0x00000000, 0x00000000, 0x00000000, 0x00000000,
                     0x00000000, 0x00000000, 0x00000000, 0x00000000,
                     0x00000000, 0x00000000, 0x00000000, 0x00000000,
                     0x00000000, 0x00000000, 0x00000000, 0x00000000,
                     0x00000000, 0x00000000, 0x00000000, 0x00000000,
                     0x00000000, 0x00000000, 0x00000000, 0x00000000,
                     0x00000000, 0x00000000, 0x00000000, 0x00000018 ]

SHA512_224_SINGLE_DIGEST = [0x4634270f, 0x707b6a54, 0xdaae7530, 0x460842e2,
                            0x0e37ed26, 0x5ceee9a4, 0x3e8924aa]
SHA512_256_SINGLE_DIGEST = [0x53048e26, 0x81941ef9, 0x9b2e29b7, 0x6b4c7dab,
                            0xe4c2d0c6, 0x34fc6d46, 0xe0e2f131, 0x07e7af23]
SHA384_SINGLE_DIGEST     = [0xcb00753f, 0x45a35e8b, 0xb5a03d69, 0x9ac65007,
                            0x272c32ab, 0x0eded163, 0x1a8b605a, 0x43ff5bed,
                            0x8086072b, 0xa1e7cc23, 0x58baeca1, 0x34c825a7]
SHA512_SINGLE_DIGEST     = [0xddaf35a1, 0x93617aba, 0xcc417349, 0xae204131,
                            0x12e6fa4e, 0x89a97ea2, 0x0a9eeee6, 0x4b55d39a,
                            0x2192992a, 0x274fc1a8, 0x36ba3c23, 0xa3feebbd,
                            0x454d4423, 0x643ce80e, 0x2a9ac94f, 0xa54ca49f]

# SHA-512 Two Block Message Sample
# Input Message: "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmn"
# "hijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu"
NIST_1024_DOUBLE0 = [ 0x61626364, 0x65666768, 0x62636465, 0x66676869,
                      0x63646566, 0x6768696a, 0x64656667, 0x68696a6b,
                      0x65666768, 0x696a6b6c, 0x66676869, 0x6a6b6c6d,
                      0x6768696a, 0x6b6c6d6e, 0x68696a6b, 0x6c6d6e6f,
                      0x696a6b6c, 0x6d6e6f70, 0x6a6b6c6d, 0x6e6f7071,
                      0x6b6c6d6e, 0x6f707172, 0x6c6d6e6f, 0x70717273,
                      0x6d6e6f70, 0x71727374, 0x6e6f7071, 0x72737475,
                      0x80000000, 0x00000000, 0x00000000, 0x00000000 ]
NIST_1024_DOUBLE1 = [ 0x00000000, 0x00000000, 0x00000000, 0x00000000,
                      0x00000000, 0x00000000, 0x00000000, 0x00000000,
                      0x00000000, 0x00000000, 0x00000000, 0x00000000,
                      0x00000000, 0x00000000, 0x00000000, 0x00000000,
                      0x00000000, 0x00000000, 0x00000000, 0x00000000,
                      0x00000000, 0x00000000, 0x00000000, 0x00000000,
                      0x00000000, 0x00000000, 0x00000000, 0x00000000,
                      0x00000000, 0x00000000, 0x00000000, 0x00000380 ]

SHA512_224_DOUBLE_DIGEST = [ 0x23fec5bb, 0x94d60b23, 0x30819264, 0x0b0c4533,
                             0x35d66473, 0x4fe40e72, 0x68674af9 ]
SHA512_256_DOUBLE_DIGEST = [ 0x3928e184, 0xfb8690f8, 0x40da3988, 0x121d31be,
                             0x65cb9d3e, 0xf83ee614, 0x6feac861, 0xe19b563a ]
SHA384_DOUBLE_DIGEST     = [ 0x09330c33, 0xf71147e8, 0x3d192fc7, 0x82cd1b47,
                             0x53111b17, 0x3b3b05d2, 0x2fa08086, 0xe3b0f712,
                             0xfcc7c71a, 0x557e2db9, 0x66c3e9fa, 0x91746039 ]
SHA512_DOUBLE_DIGEST     = [ 0x8e959b75, 0xdae313da, 0x8cf4f728, 0x14fc143f,
                             0x8f7779c6, 0xeb9f7fa1, 0x7299aead, 0xb6889018,
                             0x501d289e, 0x4900f7e4, 0x331b99de, 0xc4b5433a,
                             0xc7d329ee, 0xb6dd2654, 0x5e96e55b, 0x874be909 ]

#----------------------------------------------------------------
# I2C class
#----------------------------------------------------------------

# default configuration
I2C_dev = "/dev/i2c-2"
I2C_addr = 0x0f

# from /usr/include/linux/i2c-dev.h
I2C_SLAVE = 0x0703

def hexlist(list):
    return "[ " + ' '.join('%02x' % b for b in list) + " ]"

class I2C:
    # file handle for the i2c device
    file = None

    # constructor: initialize the i2c communications channel
    def __init__(self, dev=I2C_dev, addr=I2C_addr):
        self.dev = dev
        self.addr = addr
        try:
            self.file = io.FileIO(self.dev, 'r+b')
        except IOError as e:
            print "Unable to open %s: %s" % (self.dev, e.strerror)
            sys.exit(1)
        try:
            fcntl.ioctl(self.file, I2C_SLAVE, self.addr)
        except IOError as e:
            print "Unable to set I2C slave device 0x%02x: %s" % (self.addr, e.strerror)
            sys.exit(1)

    # destructor: close the i2c communications channel
    def __del__(self):
        if (self.file):
            self.file.close()

    # write a command to the i2c device
    def write(self, buf):
        if DEBUG:
            print "write %s" % hexlist(buf)
        self.file.write(bytearray(buf))

    # read one response byte from the i2c device
    def read(self):
        # read() on the i2c device will only return one byte at a time,
        # and tc.get_resp() needs to parse the response one byte at a time
        return ord(self.file.read(1))

#----------------------------------------------------------------
# test-case class
#----------------------------------------------------------------

# command codes
SOC                   = 0x55
EOC                   = 0xaa
READ_CMD              = 0x10
WRITE_CMD             = 0x11
RESET_CMD             = 0x01

# response codes
SOR                   = 0xaa
EOR                   = 0x55
READ_OK               = 0x7f
WRITE_OK              = 0x7e
RESET_OK              = 0x7d
UNKNOWN               = 0xfe
ERROR                 = 0xfd

class TcError(Exception):
    pass

class tc:
    def __init__(self, i2c, addr0, addr1):
        self.i2c = i2c
        self.addr0 = addr0
        self.addr1 = addr1

    def send_write_cmd(self, data):
        buf = [SOC, WRITE_CMD, self.addr0, self.addr1]
        for s in (24, 16, 8, 0):
            buf.append(data >> s & 0xff)
        buf.append(EOC)
        self.i2c.write(buf)

    def send_read_cmd(self):
        buf = [SOC, READ_CMD, self.addr0, self.addr1, EOC]
        self.i2c.write(buf)

    def get_resp(self):
        buf = []
        len = 2
        i = 0
        while i < len:
            b = self.i2c.read()
            if ((i == 0) and (b != SOR)):
                # we've gotten out of sync, and there's probably nothing we can do
                print "response byte 0: expected 0x%02x (SOR), got 0x%02x" % (SOR, b)
                raise TcError()
            elif (i == 1):        # response code
                try:
                    # anonymous dictionary of message lengths
                    len = {READ_OK:9, WRITE_OK:5, RESET_OK:3, ERROR:4, UNKNOWN:4}[b]
                except KeyError:  # unknown response code
                    # we've gotten out of sync, and there's probably nothing we can do
                    print "unknown response code 0x%02x" % b
                    raise TcError()
            buf.append(b)
            i += 1
        if DEBUG:
            print "read  %s" % hexlist(buf)
        return buf

    def get_expected(self, expected):
        buf = self.get_resp()
        if (buf != expected):
            print "expected %s,\nreceived %s" % (hexlist(expected), hexlist(buf))
            # raise TcError()

    def get_write_resp(self):
        expected = [SOR, WRITE_OK, self.addr0, self.addr1, EOR]
        self.get_expected(expected)

    def get_read_resp(self, data):
        expected = [SOR, READ_OK, self.addr0, self.addr1]
        for s in (24, 16, 8, 0):
            expected.append(data >> s & 0xff)
        expected.append(EOR)
        self.get_expected(expected)

    def write(self, data):
        self.send_write_cmd(data)
        self.get_write_resp()

    def read(self, data):
        self.send_read_cmd()
        self.get_read_resp(data)

    def read2(self, data):
        self.send_read_cmd()
        self.get_read_resp()

def tc_write(i2c, addr0, addr1, data):
    tc(i2c, addr0, addr1).write(data)

def tc_read(i2c, addr0, addr1, data):
    tc(i2c, addr0, addr1).read(data)

def tc_init(i2c, addr0):
    tc(i2c, addr0, ADDR_CTRL).write(CTRL_INIT_CMD)

def tc_next(i2c, addr0):
    tc(i2c, addr0, ADDR_CTRL).write(CTRL_NEXT_CMD)

def tc_wait(i2c, addr0, status):
    t = tc(i2c, addr0, ADDR_STATUS)
    while 1:
        t.send_read_cmd()
        buf = t.get_resp()
        if ((buf[7] & status) == status):
            break

def tc_wait_ready(i2c, addr0):
    tc_wait(i2c, addr0, STATUS_READY_BIT)

def tc_wait_valid(i2c, addr0):
    tc_wait(i2c, addr0, STATUS_VALID_BIT)

#----------------------------------------------------------------
# SHA-1 test cases
#----------------------------------------------------------------
SHA1_ADDR_PREFIX      = 0x10
SHA1_ADDR_BLOCK       = 0x10
SHA1_ADDR_DIGEST      = 0x20

def sha1_read(i2c, addr, data):
    tc_read(i2c, SHA1_ADDR_PREFIX, addr, data)

def sha1_write(i2c, addr, data):
    tc_write(i2c, SHA1_ADDR_PREFIX, addr, data)

def sha1_init(i2c):
    tc_init(i2c, SHA1_ADDR_PREFIX)

def sha1_next(i2c):
    tc_next(i2c, SHA1_ADDR_PREFIX)

def sha1_wait_ready(i2c):
    tc_wait_ready(i2c, SHA1_ADDR_PREFIX)

def sha1_wait_valid(i2c):
    tc_wait_valid(i2c, SHA1_ADDR_PREFIX)

# TC1: Read name and version from SHA-1 core.
def TC1(i2c):
    print "TC1: Reading name, type and version words from SHA-1 core."

    sha1_read(i2c, ADDR_NAME0,   0x73686131)    # "sha1"
    sha1_read(i2c, ADDR_NAME1,   0x20202020)    # "    "
    sha1_read(i2c, ADDR_VERSION, 0x302e3530)    # "0.50"

# TC2: SHA-1 Single block message test as specified by NIST.
def TC2(i2c):
    block = NIST_512_SINGLE
    expected = SHA1_SINGLE_DIGEST

    print "TC2: Single block message test for SHA-1."

    # Write block to SHA-1.
    for i in range(len(block)):
        sha1_write(i2c, SHA1_ADDR_BLOCK + i, block[i])

    # Start initial block hashing, wait and check status.
    sha1_init(i2c)
    sha1_wait_valid(i2c)

    # Extract the digest.
    for i in range(len(expected)):
        sha1_read(i2c, SHA1_ADDR_DIGEST + i, expected[i])

# TC3: SHA-1 Double block message test as specified by NIST.
def TC3(i2c):
    block = [ NIST_512_DOUBLE0, NIST_512_DOUBLE1 ]
    block0_expected = [ 0xF4286818, 0xC37B27AE, 0x0408F581, 0x84677148,
                        0x4A566572 ]
    expected = SHA1_DOUBLE_DIGEST

    print "TC3: Double block message test for SHA-1."

    # Write first block to SHA-1.
    for i in range(len(block[0])):
        sha1_write(i2c, SHA1_ADDR_BLOCK + i, block[0][i])

    # Start initial block hashing, wait and check status.
    sha1_init(i2c)
    sha1_wait_valid(i2c)

    # Extract the first digest.
    for i in range(len(block0_expected)):
        sha1_read(i2c, SHA1_ADDR_DIGEST + i, block0_expected[i])

    # Write second block to SHA-1.
    for i in range(len(block[1])):
        sha1_write(i2c, SHA1_ADDR_BLOCK + i, block[1][i])

    # Start next block hashing, wait and check status.
    sha1_next(i2c)
    sha1_wait_valid(i2c)

    # Extract the second digest.
    for i in range(len(expected)):
        sha1_read(i2c, SHA1_ADDR_DIGEST + i, expected[i])

#----------------------------------------------------------------
# SHA-256 test cases
#----------------------------------------------------------------
SHA256_ADDR_PREFIX      = 0x20
SHA256_ADDR_BLOCK       = 0x10
SHA256_ADDR_DIGEST      = 0x20

def sha256_read(i2c, addr, data):
    tc_read(i2c, SHA256_ADDR_PREFIX, addr, data)

def sha256_write(i2c, addr, data):
    tc_write(i2c, SHA256_ADDR_PREFIX, addr, data)

def sha256_init(i2c):
    tc_init(i2c, SHA256_ADDR_PREFIX)

def sha256_next(i2c):
    tc_next(i2c, SHA256_ADDR_PREFIX)

def sha256_wait_ready(i2c):
    tc_wait_ready(i2c, SHA256_ADDR_PREFIX)

def sha256_wait_valid(i2c):
    tc_wait_valid(i2c, SHA256_ADDR_PREFIX)

# TC4: Read name and version from SHA-256 core.
def TC4(i2c):
    print "TC4: Reading name, type and version words from SHA-256 core."

    sha256_read(i2c, ADDR_NAME0,   0x73686132)  # "sha2"
    sha256_read(i2c, ADDR_NAME1,   0x2d323536)  # "-256"
    sha256_read(i2c, ADDR_VERSION, 0x302e3830)  # "0.80"

# TC5: SHA-256 Single block message test as specified by NIST.
def TC5(i2c):
    block = NIST_512_SINGLE
    expected = SHA256_SINGLE_DIGEST

    print "TC5: Single block message test for SHA-256."

    # Write block to SHA-256.
    for i in range(len(block)):
        sha256_write(i2c, SHA256_ADDR_BLOCK + i, block[i])

    # Start initial block hashing, wait and check status.
    sha256_init(i2c)
    sha256_wait_valid(i2c)

    # Extract the digest.
    for i in range(len(expected)):
        sha256_read(i2c, SHA256_ADDR_DIGEST + i, expected[i])

# TC6: SHA-256 Double block message test as specified by NIST.
def TC6(i2c):
    block = [ NIST_512_DOUBLE0, NIST_512_DOUBLE1 ]
    block0_expected = [ 0x85E655D6, 0x417A1795, 0x3363376A, 0x624CDE5C,
                        0x76E09589, 0xCAC5F811, 0xCC4B32C1, 0xF20E533A ]
    expected = SHA256_DOUBLE_DIGEST

    print "TC6: Double block message test for SHA-256."

    # Write first block to SHA-256.
    for i in range(len(block[0])):
        sha256_write(i2c, SHA256_ADDR_BLOCK + i, block[0][i])

    # Start initial block hashing, wait and check status.
    sha256_init(i2c)
    sha256_wait_valid(i2c)

    # Extract the first digest.
    for i in range(len(block0_expected)):
        sha256_read(i2c, SHA256_ADDR_DIGEST + i, block0_expected[i])

    # Write second block to SHA-256.
    for i in range(len(block[1])):
        sha256_write(i2c, SHA256_ADDR_BLOCK + i, block[1][i])

    # Start next block hashing, wait and check status.
    sha256_next(i2c)
    sha256_wait_valid(i2c)

    # Extract the second digest.
    for i in range(len(expected)):
        sha256_read(i2c, SHA256_ADDR_DIGEST + i, expected[i])

# TC7: SHA-256 Huge message test.
def TC7(i2c):
    block = [ 0xaa55aa55, 0xdeadbeef, 0x55aa55aa, 0xf00ff00f,
              0xaa55aa55, 0xdeadbeef, 0x55aa55aa, 0xf00ff00f,
              0xaa55aa55, 0xdeadbeef, 0x55aa55aa, 0xf00ff00f,
              0xaa55aa55, 0xdeadbeef, 0x55aa55aa, 0xf00ff00f ]
    expected = [ 0x7638f3bc, 0x500dd1a6, 0x586dd4d0, 0x1a1551af,
                 0xd821d235, 0x2f919e28, 0xd5842fab, 0x03a40f2a ]
    n = 1000

    print "TC7: Message with %d blocks test for SHA-256." % n

    # Write first block to SHA-256.
    for i in range(len(block)):
        sha256_write(i2c, SHA256_ADDR_BLOCK + i, block[i])

    # Start initial block hashing, wait and check status.
    sha256_init(i2c)
    sha256_wait_ready(i2c)

    # First block done. Do the rest.
    for i in range(n - 1):
        # Start next block hashing, wait and check status.
        sha256_next(i2c)
        sha256_wait_ready(i2c)

    # XXX valid is probably set at the same time as ready
    sha256_wait_valid(i2c)

    # Extract the final digest.
    for i in range(len(expected)):
        sha256_read(i2c, SHA256_ADDR_DIGEST + i, expected[i])

#----------------------------------------------------------------
# SHA-512 test cases
#----------------------------------------------------------------
SHA512_ADDR_PREFIX      = 0x30
SHA512_ADDR_BLOCK       = 0x10
SHA512_ADDR_DIGEST      = 0x40
SHA512_CTRL_MODE_LOW    = 2
SHA512_CTRL_MODE_HIGH   = 3
MODE_SHA_512_224 = 0
MODE_SHA_512_256 = 1
MODE_SHA_384     = 2
MODE_SHA_512     = 3

def sha512_read(i2c, addr, data):
    tc_read(i2c, SHA512_ADDR_PREFIX, addr, data)

def sha512_write(i2c, addr, data):
    tc_write(i2c, SHA512_ADDR_PREFIX, addr, data)

def sha512_init(i2c, mode):
    tc_write(i2c, SHA512_ADDR_PREFIX, ADDR_CTRL,
             CTRL_INIT_CMD + (mode << SHA512_CTRL_MODE_LOW))

def sha512_next(i2c, mode):
    tc_write(i2c, SHA512_ADDR_PREFIX, ADDR_CTRL,
             CTRL_NEXT_CMD + (mode << SHA512_CTRL_MODE_LOW))

def sha512_wait_ready(i2c):
    tc_wait_ready(i2c, SHA512_ADDR_PREFIX)

def sha512_wait_valid(i2c):
    tc_wait_valid(i2c, SHA512_ADDR_PREFIX)

# TC8: Read name and version from SHA-512 core.
def TC8(i2c):
    print "TC8: Reading name, type and version words from SHA-512 core."

    sha512_read(i2c, ADDR_NAME0,   0x73686132)  # "sha2"
    sha512_read(i2c, ADDR_NAME1,   0x2d353132)  # "-512"
    sha512_read(i2c, ADDR_VERSION, 0x302e3830)  # "0.80"

# TC9: SHA-512 Single block message test as specified by NIST.
# We do this for all modes.
def TC9(i2c):
    def tc9(i2c, mode, expected):
        block = NIST_1024_SINGLE

        # Write block to SHA-512.
        for i in range(len(block)):
            sha512_write(i2c, SHA512_ADDR_BLOCK + i, block[i])

        # Start initial block hashing, wait and check status.
        sha512_init(i2c, mode)
        sha512_wait_valid(i2c)

        # Extract the digest.
        for i in range(len(expected)):
            sha512_read(i2c, SHA512_ADDR_DIGEST + i, expected[i])

    print "TC9-1: Single block message test for SHA-512/224."
    tc9(i2c, MODE_SHA_512_224, SHA512_224_SINGLE_DIGEST)

    print "TC9-2: Single block message test for SHA-512/256."
    tc9(i2c, MODE_SHA_512_256, SHA512_256_SINGLE_DIGEST)

    print "TC9-3: Single block message test for SHA-384."
    tc9(i2c, MODE_SHA_384, SHA384_SINGLE_DIGEST)

    print "TC9-4: Single block message test for SHA-512."
    tc9(i2c, MODE_SHA_512, SHA512_SINGLE_DIGEST)

# TC10: SHA-512 Single block message test as specified by NIST.
# We do this for all modes.
def TC10(i2c):
    def tc10(i2c, mode, expected):
        block = [ NIST_1024_DOUBLE0, NIST_1024_DOUBLE1 ]

        # Write first block to SHA-512.
        for i in range(len(block[0])):
            sha512_write(i2c, SHA512_ADDR_BLOCK + i, block[0][i])

        # Start initial block hashing, wait and check status.
        sha512_init(i2c, mode)
        sha512_wait_ready(i2c)

        # Write second block to SHA-512.
        for i in range(len(block[1])):
            sha512_write(i2c, SHA512_ADDR_BLOCK + i, block[1][i])

        # Start next block hashing, wait and check status.
        sha512_next(i2c, mode)
        sha512_wait_valid(i2c)

        # Extract the digest.
        for i in range(len(expected)):
            sha512_read(i2c, SHA512_ADDR_DIGEST + i, expected[i])

    print "TC10-1: Double block message test for SHA-512/224."
    tc10(i2c, MODE_SHA_512_224, SHA512_224_DOUBLE_DIGEST)

    print "TC10-2: Double block message test for SHA-512/256."
    tc10(i2c, MODE_SHA_512_256, SHA512_256_DOUBLE_DIGEST)

    print "TC10-3: Double block message test for SHA-384."
    tc10(i2c, MODE_SHA_384, SHA384_DOUBLE_DIGEST)

    print "TC10-4: Double block message test for SHA-512."
    tc10(i2c, MODE_SHA_512, SHA512_DOUBLE_DIGEST)

    tc_wait_valid(i2c, SHA1_ADDR_PREFIX)



# Avalanche entropy tests

AVALANCHE_ADDR_PREFIX      = 0x20
AVALANCHE_NOISE            = 0x20
AVALANCHE_DELTA            = 0x30

def avalanche_read(i2c, addr, data):
    tc_read(i2c, AVALANCHE_ADDR_PREFIX, addr, data)

def avalanche_write(i2c, addr, data):
    tc_write(i2c, AVALANCHE_ADDR_PREFIX, addr, data)


# TC11 Read name and version from Avalanche core.
def TC11(i2c):
    print "TC1: Reading name, type and version words from SHA-1 core."

    avalanche_read(i2c, ADDR_NAME0,   0x73686131)    # "sha1"
    avalanche_read(i2c, ADDR_NAME1,   0x20202020)    # "    "
    avalanche_read(i2c, ADDR_VERSION, 0x302e3530)    # "0.50"


# TC12 Read noise from Avalanche core.
def TC12(i2c):
    for i in range(10):
        avalanche_read(i2c, AVALANCHE_NOISE,   0xffffffff)



# TRNG tests
TRNG_PREFIX       = 0x00
TRNG_ADDR_NAME0   = 0x00
TRNG_ADDR_NAME1   = 0x01
TRNG_ADDR_VERSION = 0x02
ENT1_PREFIX       = 0x05
ENT2_PREFIX       = 0x06
CSPRNG_PREFIX     = 0x0b
CSPRNG_DATA       = 0x20
ENT1_DATA         = 0x20
ENT2_DATA         = 0x20

def general_read(i2c, prefix, addr, data):
    tc_read(i2c, prefix, addr, data)

def general_write(i2c, prefix, addr, data):
    tc_write(i2c, prefix, addr, data)


# TC13 Read name and version from TRNG core.
def TC13(i2c):
    print "TC13: Reading name, type and version words from TRNG."

    general_read(i2c, TRNG_PREFIX, TRNG_ADDR_NAME0,   0x74726e67)
    general_read(i2c, TRNG_PREFIX, TRNG_ADDR_NAME1,   0x20202020)
    general_read(i2c, TRNG_PREFIX, TRNG_ADDR_VERSION, 0x302e3031)


# TC14 Read random numbers from TRNG core.
def TC14(i2c):
    print "TC14: Reading TRNG data."
    for i in range(20):
        general_read(i2c, CSPRNG_PREFIX, CSPRNG_DATA,   0xffffffff)

# TC15 Read entropy numbers from TRNG core avalanche noise entropy provider.
def TC15(i2c):
    print "TC15: Reading avalanche data."
    for i in range(20):
        general_read(i2c, ENT1_PREFIX, ENT1_DATA,   0xffffffff)

# TC16 Read entropy numbers from TRNG core ROSC noise entropy provider.
def TC16(i2c):
    print "TC15: Reading rosc data."
    for i in range(20):
        general_read(i2c, ENT2_PREFIX, ENT2_DATA,   0xffffffff)

#----------------------------------------------------------------
# main
#----------------------------------------------------------------
if __name__ == '__main__':
    import argparse

    all_tests = [TC13, TC14, TC15, TC16]
#    all_tests = [TC12]
    all_tests2 = [ TC1, TC2, TC3, TC4, TC5, TC6, TC7, TC8, TC9, TC10 ]
    sha1_tests = all_tests2[0:3]
    sha256_tests = all_tests2[3:7]
    sha512_tests = all_tests2[7:]

    parser = argparse.ArgumentParser()
    parser.add_argument('-d', '--debug', action='store_true',
                        help='add debugging/trace information')
    parser.add_argument('-i', dest='dev', default=I2C_dev,
                        help='I2C device name (default ' + I2C_dev + ')')
    parser.add_argument('-a', dest='addr', type=lambda x:int(x,0), default=I2C_addr,
                        help='I2C device address (default ' + hex(I2C_addr) + ')')
    parser.add_argument('test_cases', metavar='TC', nargs='*',
                        help='test case number, "sha1", "sha256", "sha512", or "all"')
    args = parser.parse_args()
    DEBUG = args.debug
    i = I2C(args.dev, args.addr)

    if (not args.test_cases):
        for TC in all_tests:
            TC(i)
    else:
        for t in args.test_cases:
            if (t == 'sha1'):
                for TC in sha1_tests:
                    TC(i)
            elif (t == 'sha256'):
                for TC in sha256_tests:
                    TC(i)
            elif (t == 'sha512'):
                for TC in sha512_tests:
                    TC(i)
            elif (t == 'all'):
                for TC in all_tests:
                    TC(i)
            else:
                try:
                    n = int(t)
                except:
                    print 'invalid test case %s' % t
                else:
                    if ((n < 1) or (n > len(all_tests))):
                        print 'invalid test case %d' % n
                    else:
                        all_tests[n-1](i)

#=======================================================================
# EOF hash_tester.py
#=======================================================================
