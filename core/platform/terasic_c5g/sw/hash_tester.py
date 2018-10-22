#!/usr/bin/env python
# -*- coding: utf-8 -*-
#=======================================================================
#
# hash_tester.py
# --------------
# This program sends several commands to the coretest_hashed subsystem
# in order to verify the SHA-1, SHA-256 and SHA-512/x hash function
# cores. The program will use the built in hash implementations in
# Python to do functional comparison and validation.
#
# Note: This program requires the PySerial module.
# http://pyserial.sourceforge.net/
#
# The single and dual block test cases are taken from the
# NIST KAT document:
# http://csrc.nist.gov/groups/ST/toolkit/documents/Examples/SHA_All.pdf
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
import serial
import os
import time
import threading
import hashlib

 
#-------------------------------------------------------------------
# Defines.
#-------------------------------------------------------------------
# Serial port defines.
# CONFIGURE YOUR DEVICE HERE!
SERIAL_DEVICE = '/dev/cu.usbserial-A801SA6T'
BAUD_RATE = 9600
BIT_RATE  = int(50E6 / BAUD_RATE)

BAUD_RATE2 = 256000
BIT_RATE2  = int(50E6 / BAUD_RATE2)

DATA_BITS = 8
STOP_BITS = 1


# Verbose operation on/off
VERBOSE = False

# Delay times we wait
PROC_DELAY_TIME = 0.0001
COMM_DELAY_TIME = 0.01

# Memory map.
SOC                   = '\x55'
EOC                   = '\xaa'
READ_CMD              = '\x10'
WRITE_CMD             = '\x11'

UART_ADDR_PREFIX      = '\x00'
UART_ADDR_BIT_RATE    = '\x10'
UART_ADDR_DATA_BITS   = '\x11'
UART_ADDR_STOP_BITS   = '\x12'

SHA1_ADDR_PREFIX      = '\x10'
SHA1_ADDR_NAME0       = '\x00'
SHA1_ADDR_NAME1       = '\x01'
SHA1_ADDR_VERSION     = '\x02'
SHA1_ADDR_CTRL        = '\x08'
SHA1_CTRL_INIT_CMD    = '\x01'
SHA1_CTRL_NEXT_CMD    = '\x02'
SHA1_ADDR_STATUS      = '\x09'
SHA1_STATUS_READY_BIT = 0
SHA1_STATUS_VALID_BIT = 1
SHA1_ADDR_BLOCK0      = '\x10'
SHA1_ADDR_BLOCK1      = '\x11'
SHA1_ADDR_BLOCK2      = '\x12'
SHA1_ADDR_BLOCK3      = '\x13'
SHA1_ADDR_BLOCK4      = '\x14'
SHA1_ADDR_BLOCK5      = '\x15'
SHA1_ADDR_BLOCK6      = '\x16'
SHA1_ADDR_BLOCK7      = '\x17'
SHA1_ADDR_BLOCK8      = '\x18'
SHA1_ADDR_BLOCK9      = '\x19'
SHA1_ADDR_BLOCK10     = '\x1a'
SHA1_ADDR_BLOCK11     = '\x1b'
SHA1_ADDR_BLOCK12     = '\x1c'
SHA1_ADDR_BLOCK13     = '\x1d'
SHA1_ADDR_BLOCK14     = '\x1e'
SHA1_ADDR_BLOCK15     = '\x1f'
SHA1_ADDR_DIGEST0     = '\x20'
SHA1_ADDR_DIGEST1     = '\x21'
SHA1_ADDR_DIGEST2     = '\x22'
SHA1_ADDR_DIGEST3     = '\x23'
SHA1_ADDR_DIGEST4     = '\x24'

SHA256_ADDR_PREFIX      = '\x20'
SHA256_ADDR_NAME0       = '\x00'
SHA256_ADDR_NAME1       = '\x01'
SHA256_ADDR_VERSION     = '\x02'
SHA256_ADDR_CTRL        = '\x08'
SHA256_CTRL_INIT_CMD    = '\x01'
SHA256_CTRL_NEXT_CMD    = '\x02'
SHA256_ADDR_STATUS      = '\x09'
SHA256_STATUS_READY_BIT = 0
SHA256_STATUS_VALID_BIT = 1
SHA256_ADDR_BLOCK0      = '\x10'
SHA256_ADDR_BLOCK1      = '\x11'
SHA256_ADDR_BLOCK2      = '\x12'
SHA256_ADDR_BLOCK3      = '\x13'
SHA256_ADDR_BLOCK4      = '\x14'
SHA256_ADDR_BLOCK5      = '\x15'
SHA256_ADDR_BLOCK6      = '\x16'
SHA256_ADDR_BLOCK7      = '\x17'
SHA256_ADDR_BLOCK8      = '\x18'
SHA256_ADDR_BLOCK9      = '\x19'
SHA256_ADDR_BLOCK10     = '\x1a'
SHA256_ADDR_BLOCK11     = '\x1b'
SHA256_ADDR_BLOCK12     = '\x1c'
SHA256_ADDR_BLOCK13     = '\x1d'
SHA256_ADDR_BLOCK14     = '\x1e'
SHA256_ADDR_BLOCK15     = '\x1f'
SHA256_ADDR_DIGEST0     = '\x20'
SHA256_ADDR_DIGEST1     = '\x21'
SHA256_ADDR_DIGEST2     = '\x22'
SHA256_ADDR_DIGEST3     = '\x23'
SHA256_ADDR_DIGEST4     = '\x24'
SHA256_ADDR_DIGEST5     = '\x25'
SHA256_ADDR_DIGEST6     = '\x26'
SHA256_ADDR_DIGEST7     = '\x27'

SHA512_ADDR_PREFIX      = '\x30'
SHA512_ADDR_NAME0       = '\x00'
SHA512_ADDR_NAME1       = '\x01'
SHA512_ADDR_VERSION     = '\x02'
SHA512_ADDR_CTRL        = '\x08'
SHA512_CTRL_INIT_CMD    = '\x01'
SHA512_CTRL_NEXT_CMD    = '\x02'
SHA512_CTRL_MODE_LOW    = 2
SHA512_CTRL_MODE_HIGH   = 3
SHA512_ADDR_STATUS      = '\x09'
SHA512_STATUS_READY_BIT = 0
SHA512_STATUS_VALID_BIT = 1
SHA512_ADDR_BLOCK0      = '\x10'
SHA512_ADDR_BLOCK1      = '\x11'
SHA512_ADDR_BLOCK2      = '\x12'
SHA512_ADDR_BLOCK3      = '\x13'
SHA512_ADDR_BLOCK4      = '\x14'
SHA512_ADDR_BLOCK5      = '\x15'
SHA512_ADDR_BLOCK6      = '\x16'
SHA512_ADDR_BLOCK7      = '\x17'
SHA512_ADDR_BLOCK8      = '\x18'
SHA512_ADDR_BLOCK9      = '\x19'
SHA512_ADDR_BLOCK10     = '\x1a'
SHA512_ADDR_BLOCK11     = '\x1b'
SHA512_ADDR_BLOCK12     = '\x1c'
SHA512_ADDR_BLOCK13     = '\x1d'
SHA512_ADDR_BLOCK14     = '\x1e'
SHA512_ADDR_BLOCK15     = '\x1f'
SHA512_ADDR_BLOCK16     = '\x20'
SHA512_ADDR_BLOCK17     = '\x21'
SHA512_ADDR_BLOCK18     = '\x22'
SHA512_ADDR_BLOCK19     = '\x23'
SHA512_ADDR_BLOCK20     = '\x24'
SHA512_ADDR_BLOCK21     = '\x25'
SHA512_ADDR_BLOCK22     = '\x26'
SHA512_ADDR_BLOCK23     = '\x27'
SHA512_ADDR_BLOCK24     = '\x28'
SHA512_ADDR_BLOCK25     = '\x29'
SHA512_ADDR_BLOCK26     = '\x2a'
SHA512_ADDR_BLOCK27     = '\x2b'
SHA512_ADDR_BLOCK28     = '\x2c'
SHA512_ADDR_BLOCK29     = '\x2d'
SHA512_ADDR_BLOCK30     = '\x2e'
SHA512_ADDR_BLOCK31     = '\x2f'
SHA512_ADDR_DIGEST0     = '\x40'
SHA512_ADDR_DIGEST1     = '\x41'
SHA512_ADDR_DIGEST2     = '\x42'
SHA512_ADDR_DIGEST3     = '\x43'
SHA512_ADDR_DIGEST4     = '\x44'
SHA512_ADDR_DIGEST5     = '\x45'
SHA512_ADDR_DIGEST6     = '\x46'
SHA512_ADDR_DIGEST7     = '\x47'
SHA512_ADDR_DIGEST8     = '\x48'
SHA512_ADDR_DIGEST9     = '\x49'
SHA512_ADDR_DIGEST10    = '\x4a'
SHA512_ADDR_DIGEST11    = '\x4b'
SHA512_ADDR_DIGEST12    = '\x4c'
SHA512_ADDR_DIGEST13    = '\x4d'
SHA512_ADDR_DIGEST14    = '\x4e'
SHA512_ADDR_DIGEST15    = '\x4f'

MODE_SHA_512_224 = '\x00'
MODE_SHA_512_256 = '\x01'
MODE_SHA_384     = '\x02'
MODE_SHA_512     = '\x03'

sha1_block_addr = [SHA1_ADDR_BLOCK0,  SHA1_ADDR_BLOCK1,
                   SHA1_ADDR_BLOCK2,  SHA1_ADDR_BLOCK3,
                   SHA1_ADDR_BLOCK4,  SHA1_ADDR_BLOCK5,
                   SHA1_ADDR_BLOCK6,  SHA1_ADDR_BLOCK7,
                   SHA1_ADDR_BLOCK8,  SHA1_ADDR_BLOCK9,
                   SHA1_ADDR_BLOCK10, SHA1_ADDR_BLOCK11,
                   SHA1_ADDR_BLOCK12, SHA1_ADDR_BLOCK13,
                   SHA1_ADDR_BLOCK14, SHA1_ADDR_BLOCK15]

sha1_digest_addr = [SHA1_ADDR_DIGEST0, SHA1_ADDR_DIGEST1,
                    SHA1_ADDR_DIGEST2, SHA1_ADDR_DIGEST3,
                    SHA1_ADDR_DIGEST4]

sha256_block_addr = [SHA256_ADDR_BLOCK0,  SHA256_ADDR_BLOCK1,
                     SHA256_ADDR_BLOCK2,  SHA256_ADDR_BLOCK3,
                     SHA256_ADDR_BLOCK4,  SHA256_ADDR_BLOCK5,
                     SHA256_ADDR_BLOCK6,  SHA256_ADDR_BLOCK7,
                     SHA256_ADDR_BLOCK8,  SHA256_ADDR_BLOCK9,
                     SHA256_ADDR_BLOCK10, SHA256_ADDR_BLOCK11,
                     SHA256_ADDR_BLOCK12, SHA256_ADDR_BLOCK13,
                     SHA256_ADDR_BLOCK14, SHA256_ADDR_BLOCK15]

sha256_digest_addr = [SHA256_ADDR_DIGEST0,  SHA256_ADDR_DIGEST1,
                      SHA256_ADDR_DIGEST2,  SHA256_ADDR_DIGEST3,
                      SHA256_ADDR_DIGEST4,  SHA256_ADDR_DIGEST5,
                      SHA256_ADDR_DIGEST6,  SHA256_ADDR_DIGEST7]

sha512_block_addr = [SHA512_ADDR_BLOCK0,  SHA512_ADDR_BLOCK1,
                     SHA512_ADDR_BLOCK2,  SHA512_ADDR_BLOCK3,
                     SHA512_ADDR_BLOCK4,  SHA512_ADDR_BLOCK5,
                     SHA512_ADDR_BLOCK6,  SHA512_ADDR_BLOCK7,
                     SHA512_ADDR_BLOCK8,  SHA512_ADDR_BLOCK9,
                     SHA512_ADDR_BLOCK10, SHA512_ADDR_BLOCK11,
                     SHA512_ADDR_BLOCK12, SHA512_ADDR_BLOCK13,
                     SHA512_ADDR_BLOCK14, SHA512_ADDR_BLOCK15,
                     SHA512_ADDR_BLOCK16, SHA512_ADDR_BLOCK17,
                     SHA512_ADDR_BLOCK18, SHA512_ADDR_BLOCK19,
                     SHA512_ADDR_BLOCK20, SHA512_ADDR_BLOCK21,
                     SHA512_ADDR_BLOCK22, SHA512_ADDR_BLOCK23,
                     SHA512_ADDR_BLOCK24, SHA512_ADDR_BLOCK25,
                     SHA512_ADDR_BLOCK26, SHA512_ADDR_BLOCK27,
                     SHA512_ADDR_BLOCK28, SHA512_ADDR_BLOCK29,
                     SHA512_ADDR_BLOCK30, SHA512_ADDR_BLOCK31]

sha512_digest_addr = [SHA512_ADDR_DIGEST0,  SHA512_ADDR_DIGEST1,
                      SHA512_ADDR_DIGEST2,  SHA512_ADDR_DIGEST3,
                      SHA512_ADDR_DIGEST4,  SHA512_ADDR_DIGEST5,
                      SHA512_ADDR_DIGEST6,  SHA512_ADDR_DIGEST7,
                      SHA512_ADDR_DIGEST8,  SHA512_ADDR_DIGEST9,
                      SHA512_ADDR_DIGEST10, SHA512_ADDR_DIGEST11,
                      SHA512_ADDR_DIGEST12, SHA512_ADDR_DIGEST13,
                      SHA512_ADDR_DIGEST14, SHA512_ADDR_DIGEST15]

NIST_512_SINGLE = ['\x61', '\x62', '\x63', '\x80', '\x00', '\x00', '\x00', '\x00',
                   '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                   '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                   '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                   '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                   '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                   '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                   '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x18']

NIST_512_DOUBLE0 = ['\x61', '\x62', '\x63', '\x64', '\x62', '\x63', '\x64', '\x65',
                    '\x63', '\x64', '\x65', '\x66', '\x64', '\x65', '\x66', '\x67',
                    '\x65', '\x66', '\x67', '\x68', '\x66', '\x67', '\x68', '\x69',
                    '\x67', '\x68', '\x69', '\x6A', '\x68', '\x69', '\x6A', '\x6B',
                    '\x69', '\x6A', '\x6B', '\x6C', '\x6A', '\x6B', '\x6C', '\x6D',
                    '\x6B', '\x6C', '\x6D', '\x6E', '\x6C', '\x6D', '\x6E', '\x6F',
                    '\x6D', '\x6E', '\x6F', '\x70', '\x6E', '\x6F', '\x70', '\x71',
                    '\x80', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00']

NIST_512_DOUBLE1 = ['\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x01', '\xC0']

NIST_1024_SINGLE = ['\x61', '\x62', '\x63', '\x80', '\x00', '\x00', '\x00', '\x00',
                    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                    '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x18']

NIST_1024_DOUBLE0 = ['\x61', '\x62', '\x63', '\x64', '\x65', '\x66', '\x67', '\x68',
                     '\x62', '\x63', '\x64', '\x65', '\x66', '\x67', '\x68', '\x69',
                     '\x63', '\x64', '\x65', '\x66', '\x67', '\x68', '\x69', '\x6a',
                     '\x64', '\x65', '\x66', '\x67', '\x68', '\x69', '\x6a', '\x6b',
                     '\x65', '\x66', '\x67', '\x68', '\x69', '\x6a', '\x6b', '\x6c',
                     '\x66', '\x67', '\x68', '\x69', '\x6a', '\x6b', '\x6c', '\x6d',
                     '\x67', '\x68', '\x69', '\x6a', '\x6b', '\x6c', '\x6d', '\x6e',
                     '\x68', '\x69', '\x6a', '\x6b', '\x6c', '\x6d', '\x6e', '\x6f',
                     '\x69', '\x6a', '\x6b', '\x6c', '\x6d', '\x6e', '\x6f', '\x70',
                     '\x6a', '\x6b', '\x6c', '\x6d', '\x6e', '\x6f', '\x70', '\x71',
                     '\x6b', '\x6c', '\x6d', '\x6e', '\x6f', '\x70', '\x71', '\x72',
                     '\x6c', '\x6d', '\x6e', '\x6f', '\x70', '\x71', '\x72', '\x73',
                     '\x6d', '\x6e', '\x6f', '\x70', '\x71', '\x72', '\x73', '\x74',
                     '\x6e', '\x6f', '\x70', '\x71', '\x72', '\x73', '\x74', '\x75',
                     '\x80', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                     '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00']

NIST_1024_DOUBLE1 = ['\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                     '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                     '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                     '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                     '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                     '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                     '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                     '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                     '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                     '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                     '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                     '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                     '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                     '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                     '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
                     '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x03', '\x80']


#-------------------------------------------------------------------
# print_response()
#
# Parses a received buffer and prints the response.
#-------------------------------------------------------------------
def print_response(buffer):
    if VERBOSE:
        print "Length of response: %d" % len(buffer)
        if buffer[0] == '\xaa':
            print "Response contains correct Start of Response (SOR)"
        if buffer[-1] == '\x55':
            print "Response contains correct End of Response (EOR)"

    response_code = ord(buffer[1])

    if response_code == 0xfe:
        print "UNKNOWN response code received."

    elif response_code == 0xfd:
        print "ERROR response code received."

    elif response_code == 0x7f:
        read_addr = ord(buffer[2]) * 256 + ord(buffer[3])
        read_data = (ord(buffer[4]) * 16777216) + (ord(buffer[5]) * 65536) +\
                    (ord(buffer[6]) * 256) + ord(buffer[7])
        print "READ_OK. address 0x%02x = 0x%08x." % (read_addr, read_data)

    elif response_code == 0x7e:
        read_addr = ord(buffer[2]) * 256 + ord(buffer[3])
        print "WRITE_OK. address 0x%02x." % (read_addr)

    elif response_code == 0x7d:
        print "RESET_OK."

    else:
        print "Response 0x%02x is unknown." % response_code
        print buffer
        

#-------------------------------------------------------------------
# read_serial_thread()
#
# Function used in a thread to read from the serial port and
# collect response from coretest.
#-------------------------------------------------------------------
def read_serial_thread(serialport):
    if VERBOSE:
        print "Serial port response thread started. Waiting for response..."
        
    buffer = []
    while True:
        if serialport.isOpen():
            response = serialport.read()
            buffer.append(response)
            if ((response == '\x55') and len(buffer) > 7):
                print_response(buffer)
                buffer = []
        else:
            print "No open device yet."
            time.sleep(COMM_DELAY_TIME)
            

#-------------------------------------------------------------------
# write_serial_bytes()
#
# Send the bytes in the buffer to coretest over the serial port.
#-------------------------------------------------------------------
def write_serial_bytes(tx_cmd, serialport):
    if VERBOSE:
        print "Command to be sent:", tx_cmd
    
    for tx_byte in tx_cmd:
        serialport.write(tx_byte)

    # Allow the device to complete the transaction.
    time.sleep(COMM_DELAY_TIME)


#-------------------------------------------------------------------
# single_block_test_sha512x()
#
# Write a given block to SHA-512/x and perform single block
# processing for the given mode.
#-------------------------------------------------------------------
def single_block_test_sha512x(block, mode, ser):
    # Write block to SHA-512.
    for i in range(len(block) / 4):
        message = [SOC, WRITE_CMD, SHA512_ADDR_PREFIX,] + [sha512_block_addr[i]] +\
                  block[(i * 4) : ((i * 4 ) + 4)] + [EOC]
        write_serial_bytes(message, ser)

    # Start initial block hashing, wait and check status.
    mode_cmd = chr(ord(SHA512_CTRL_INIT_CMD) + (ord(mode) << SHA512_CTRL_MODE_LOW))
    write_serial_bytes([SOC, WRITE_CMD, SHA512_ADDR_PREFIX, SHA512_ADDR_CTRL,
                        '\x00', '\x00', '\x00', mode_cmd, EOC], ser)
    time.sleep(PROC_DELAY_TIME)
    write_serial_bytes([SOC, READ_CMD, SHA512_ADDR_PREFIX, SHA512_ADDR_STATUS, EOC], ser)

    # Select the correct number of digest addresses to read.
    if (mode == MODE_SHA_512_224):
        mode_digest_addr = sha512_digest_addr[0 : 7]
    elif (mode == MODE_SHA_512_256):
        mode_digest_addr = sha512_digest_addr[0 : 8]
    elif (mode == MODE_SHA_384):
        mode_digest_addr = sha512_digest_addr[0 : 12]
    elif (mode == MODE_SHA_512):
        mode_digest_addr = sha512_digest_addr

    # Extract the digest.
    for digest_addr in mode_digest_addr:
        message = [SOC, READ_CMD, SHA512_ADDR_PREFIX] + [digest_addr] + [EOC]
        write_serial_bytes(message, ser)
    print""


#-------------------------------------------------------------------
# dual_block_test_sha512x()
#
# Write a given block to SHA-512/x and perform single block
# processing for the given mode.
#-------------------------------------------------------------------
def dual_block_test_sha512x(block0, block1, mode, ser):
    # Write block0 to SHA-512.
    for i in range(len(block0) / 4):
        message = [SOC, WRITE_CMD, SHA512_ADDR_PREFIX,] + [sha512_block_addr[i]] +\
                  block0[(i * 4) : ((i * 4 ) + 4)] + [EOC]
        write_serial_bytes(message, ser)

    # Start initial block hashing, wait and check status.
    mode_cmd = chr(ord(SHA512_CTRL_INIT_CMD) + (ord(mode) << SHA512_CTRL_MODE_LOW))
    write_serial_bytes([SOC, WRITE_CMD, SHA512_ADDR_PREFIX, SHA512_ADDR_CTRL,
                        '\x00', '\x00', '\x00', mode_cmd, EOC], ser)
    time.sleep(PROC_DELAY_TIME)
    write_serial_bytes([SOC, READ_CMD, SHA512_ADDR_PREFIX, SHA512_ADDR_STATUS, EOC], ser)

    # Write block1 to SHA-512.
    for i in range(len(block1) / 4):
        message = [SOC, WRITE_CMD, SHA512_ADDR_PREFIX,] + [sha512_block_addr[i]] +\
                  block1[(i * 4) : ((i * 4 ) + 4)] + [EOC]
        write_serial_bytes(message, ser)

    # Start next block hashing, wait and check status.
    mode_cmd = chr(ord(SHA512_CTRL_NEXT_CMD) + (ord(mode) << SHA512_CTRL_MODE_LOW))
    write_serial_bytes([SOC, WRITE_CMD, SHA512_ADDR_PREFIX, SHA512_ADDR_CTRL,
                        '\x00', '\x00', '\x00', mode_cmd, EOC], ser)
    time.sleep(PROC_DELAY_TIME)
    write_serial_bytes([SOC, READ_CMD, SHA512_ADDR_PREFIX, SHA512_ADDR_STATUS, EOC], ser)

    # Select the correct number of digest addresses to read.
    if (mode == MODE_SHA_512_224):
        mode_digest_addr = sha512_digest_addr[0 : 7]
    elif (mode == MODE_SHA_512_256):
        mode_digest_addr = sha512_digest_addr[0 : 8]
    elif (mode == MODE_SHA_384):
        mode_digest_addr = sha512_digest_addr[0 : 12]
    elif (mode == MODE_SHA_512):
        mode_digest_addr = sha512_digest_addr

    # Extract the digest.
    for digest_addr in mode_digest_addr:
        message = [SOC, READ_CMD, SHA512_ADDR_PREFIX] + [digest_addr] + [EOC]
        write_serial_bytes(message, ser)
    print""


#-------------------------------------------------------------------
# single_block_test_sha256()
#
# Write a given block to SHA-256 and perform single block
# processing.
#-------------------------------------------------------------------
def single_block_test_sha256(block, ser):
    # Write block to SHA-2.
    for i in range(len(block) / 4):
        message = [SOC, WRITE_CMD, SHA256_ADDR_PREFIX,] + [sha256_block_addr[i]] +\
                  block[(i * 4) : ((i * 4 ) + 4)] + [EOC]
        write_serial_bytes(message, ser)

    # Start initial block hashing, wait and check status.
    write_serial_bytes([SOC, WRITE_CMD, SHA256_ADDR_PREFIX, SHA256_ADDR_CTRL,
                        '\x00', '\x00', '\x00', SHA256_CTRL_INIT_CMD, EOC], ser)
    time.sleep(PROC_DELAY_TIME)
    write_serial_bytes([SOC, READ_CMD, SHA256_ADDR_PREFIX, SHA256_ADDR_STATUS, EOC], ser)

    # Extract the digest.
    for digest_addr in sha256_digest_addr:
        message = [SOC, READ_CMD, SHA256_ADDR_PREFIX] + [digest_addr] + [EOC]
        write_serial_bytes(message, ser)
    print""


#-------------------------------------------------------------------
# double_block_test_sha256()
#
# Run double block message test.
#-------------------------------------------------------------------
def double_block_test_sha256(block1, block2, ser):
    # Write block1 to SHA-256.
    for i in range(len(block1) / 4):
        message = [SOC, WRITE_CMD, SHA256_ADDR_PREFIX,] + [sha256_block_addr[i]] +\
                  block1[(i * 4) : ((i * 4 ) + 4)] + [EOC]
        write_serial_bytes(message, ser)

    # Start initial block hashing, wait and check status.
    write_serial_bytes([SOC, WRITE_CMD, SHA256_ADDR_PREFIX, SHA256_ADDR_CTRL,
                        '\x00', '\x00', '\x00', SHA256_CTRL_INIT_CMD, EOC], ser)
    time.sleep(PROC_DELAY_TIME)
    write_serial_bytes([SOC, READ_CMD, SHA256_ADDR_PREFIX, SHA256_ADDR_STATUS, EOC], ser)

    # Extract the first digest.
    for digest_addr in sha256_digest_addr:
        message = [SOC, READ_CMD, SHA256_ADDR_PREFIX] + [digest_addr] + [EOC]
        write_serial_bytes(message, ser)
    print""

    # Write block2 to SHA-256.
    for i in range(len(block2) / 4):
        message = [SOC, WRITE_CMD, SHA256_ADDR_PREFIX,] + [sha256_block_addr[i]] +\
                  block2[(i * 4) : ((i * 4 ) + 4)] + [EOC]
        write_serial_bytes(message, ser)

    # Start next block hashing, wait and check status.
    write_serial_bytes([SOC, WRITE_CMD, SHA256_ADDR_PREFIX, SHA256_ADDR_CTRL,
                        '\x00', '\x00', '\x00', SHA256_CTRL_NEXT_CMD, EOC], ser)
    time.sleep(PROC_DELAY_TIME)
    write_serial_bytes([SOC, READ_CMD, SHA256_ADDR_PREFIX, SHA256_ADDR_STATUS, EOC], ser)

    # Extract the second digest.
    for digest_addr in sha256_digest_addr:
        message = [SOC, READ_CMD, SHA256_ADDR_PREFIX] + [digest_addr] + [EOC]
        write_serial_bytes(message, ser)
    print""


#-------------------------------------------------------------------
# huge_message_test_sha256()
#
# Test with a message with a huge number (n) number of blocks.
#-------------------------------------------------------------------
def huge_message_test_sha256(block, n, ser):
    # Write block to SHA-256.
    for i in range(len(block) / 4):
        message = [SOC, WRITE_CMD, SHA256_ADDR_PREFIX,] + [sha256_block_addr[i]] +\
                  block[(i * 4) : ((i * 4 ) + 4)] + [EOC]
        write_serial_bytes(message, ser)

    # Start initial block hashing, wait and check status.
    write_serial_bytes([SOC, WRITE_CMD, SHA256_ADDR_PREFIX, SHA256_ADDR_CTRL,
                        '\x00', '\x00', '\x00', SHA256_CTRL_INIT_CMD, EOC], ser)
    time.sleep(PROC_DELAY_TIME)
    write_serial_bytes([SOC, READ_CMD, SHA256_ADDR_PREFIX, SHA256_ADDR_STATUS, EOC], ser)

    # Extract the first digest.
    print "Digest for block 0000:"
    for digest_addr in sha256_digest_addr:
        message = [SOC, READ_CMD, SHA256_ADDR_PREFIX] + [digest_addr] + [EOC]
        write_serial_bytes(message, ser)
    print""

    # First block done. Do the rest.
    for i in range(n - 1):
        # Start next block hashing, wait and check status.
        write_serial_bytes([SOC, WRITE_CMD, SHA256_ADDR_PREFIX, SHA256_ADDR_CTRL,
                            '\x00', '\x00', '\x00', SHA256_CTRL_NEXT_CMD, EOC], ser)
        time.sleep(PROC_DELAY_TIME)
        write_serial_bytes([SOC, READ_CMD, SHA256_ADDR_PREFIX, SHA256_ADDR_STATUS, EOC], ser)

        # Extract the second digest.
        print "Digest for block %04d" % (i + 1)
        for digest_addr in sha256_digest_addr:
            message = [SOC, READ_CMD, SHA256_ADDR_PREFIX] + [digest_addr] + [EOC]
            write_serial_bytes(message, ser)
        print""


#-------------------------------------------------------------------
# single_block_test_sha1()
#
# Write a given block to SHA-1 and perform single block
# processing.
#-------------------------------------------------------------------
def single_block_test_sha1(block, ser):
    # Write block to SHA-1.
    for i in range(len(block) / 4):
        message = [SOC, WRITE_CMD, SHA1_ADDR_PREFIX,] + [sha1_block_addr[i]] +\
                  block[(i * 4) : ((i * 4 ) + 4)] + [EOC]
        write_serial_bytes(message, ser)

    # Start initial block hashing, wait and check status.
    write_serial_bytes([SOC, WRITE_CMD, SHA1_ADDR_PREFIX, SHA1_ADDR_CTRL,
                        '\x00', '\x00', '\x00', SHA1_CTRL_INIT_CMD, EOC], ser)
    time.sleep(PROC_DELAY_TIME)
    write_serial_bytes([SOC, READ_CMD, SHA1_ADDR_PREFIX, SHA1_ADDR_STATUS,   EOC], ser)

    # Extract the digest.
    for digest_addr in sha1_digest_addr:
        message = [SOC, READ_CMD, SHA1_ADDR_PREFIX] + [digest_addr] + [EOC]
        write_serial_bytes(message, ser)
    print""
    

#-------------------------------------------------------------------
# double_block_test_sha1
#
# Run double block message test for SHA-1.
#-------------------------------------------------------------------
def double_block_test_sha1(block1, block2, ser):
    # Write block1 to SHA-1.
    for i in range(len(block1) / 4):
        message = [SOC, WRITE_CMD, SHA1_ADDR_PREFIX,] + [sha1_block_addr[i]] +\
                  block1[(i * 4) : ((i * 4 ) + 4)] + [EOC]
        write_serial_bytes(message, ser)

    # Start initial block hashing, wait and check status.
    write_serial_bytes([SOC, WRITE_CMD, SHA1_ADDR_PREFIX, SHA1_ADDR_CTRL,
                        '\x00', '\x00', '\x00', SHA1_CTRL_INIT_CMD, EOC], ser)
    time.sleep(PROC_DELAY_TIME)
    write_serial_bytes([SOC, READ_CMD, SHA1_ADDR_PREFIX, SHA1_ADDR_STATUS,   EOC], ser)

    # Extract the first digest.
    for digest_addr in sha1_digest_addr:
        message = [SOC, READ_CMD, SHA1_ADDR_PREFIX] + [digest_addr] + [EOC]
        write_serial_bytes(message, ser)
    print""

    # Write block2 to SHA-1.
    for i in range(len(block2) / 4):
        message = [SOC, WRITE_CMD, SHA1_ADDR_PREFIX,] + [sha1_block_addr[i]] +\
                  block2[(i * 4) : ((i * 4 ) + 4)] + [EOC]
        write_serial_bytes(message, ser)

    # Start next block hashing, wait and check status.
    write_serial_bytes([SOC, WRITE_CMD, SHA1_ADDR_PREFIX, SHA1_ADDR_CTRL,
                        '\x00', '\x00', '\x00', SHA1_CTRL_NEXT_CMD, EOC], ser)
    time.sleep(PROC_DELAY_TIME)
    write_serial_bytes([SOC, READ_CMD, SHA1_ADDR_PREFIX, SHA1_ADDR_STATUS,   EOC], ser)

    # Extract the second digest.
    for digest_addr in sha1_digest_addr:
        message = [SOC, READ_CMD, SHA1_ADDR_PREFIX] + [digest_addr] + [EOC]
        write_serial_bytes(message, ser)
    print""


#-------------------------------------------------------------------
# TC1: Read name and version from SHA-1 core.
#-------------------------------------------------------------------
def tc1(ser):
    print "TC1: Reading name, type and version words from SHA-1 core."
    write_serial_bytes([SOC, READ_CMD, SHA1_ADDR_PREFIX, SHA1_ADDR_NAME0, EOC], ser)
    write_serial_bytes([SOC, READ_CMD, SHA1_ADDR_PREFIX, SHA1_ADDR_NAME1, EOC], ser)
    write_serial_bytes([SOC, READ_CMD, SHA1_ADDR_PREFIX, SHA1_ADDR_VERSION, EOC], ser)
    print""


#-------------------------------------------------------------------
# TC2: SHA-1 Single block message test as specified by NIST.
#-------------------------------------------------------------------
def tc2(ser):
    print "TC2: Single block message test for SHA-1."

    tc2_sha1_expected = [0xa9993e36, 0x4706816a, 0xba3e2571,
                         0x7850c26c, 0x9cd0d89d]

    print "TC2: Expected digest values as specified by NIST:"
    for i in tc2_sha1_expected:
        print("0x%08x " % i)
    print("")
    single_block_test_sha1(NIST_512_SINGLE, ser)
    

#-------------------------------------------------------------------
# TC3: SHA-1 Double block message test as specified by NIST.
#-------------------------------------------------------------------
def tc3(ser):
    print "TC3: Double block message test for SHA-1."

    tc3_1_sha1_expected = [0xF4286818, 0xC37B27AE, 0x0408F581,
                           0x84677148, 0x4A566572]

    tc3_2_sha1_expected = [0x84983E44, 0x1C3BD26E, 0xBAAE4AA1,
                           0xF95129E5, 0xE54670F1]

    print "TC3: Expected digest values for first block as specified by NIST:"
    for i in tc3_1_sha1_expected:
        print("0x%08x " % i)
    print("")
    print "TC3: Expected digest values for second block as specified by NIST:"
    for i in tc3_2_sha1_expected:
        print("0x%08x " % i)
    print("")
    double_block_test_sha1(NIST_512_DOUBLE0, NIST_512_DOUBLE1, ser)


#-------------------------------------------------------------------
# TC4: Read name and version from SHA-256 core.
#-------------------------------------------------------------------
def tc4(ser):
    print "TC4: Reading name, type and version words from SHA-256 core."
    my_cmd = [SOC, READ_CMD, SHA256_ADDR_PREFIX, SHA256_ADDR_NAME0, EOC]
    write_serial_bytes(my_cmd, ser)
    my_cmd = [SOC, READ_CMD, SHA256_ADDR_PREFIX, SHA256_ADDR_NAME1, EOC]
    write_serial_bytes(my_cmd, ser)
    my_cmd = [SOC, READ_CMD, SHA256_ADDR_PREFIX, SHA256_ADDR_VERSION, EOC]
    write_serial_bytes(my_cmd, ser)
    print""


#-------------------------------------------------------------------
# TC5: SHA-256 Single block message test as specified by NIST.
#-------------------------------------------------------------------
def tc5(ser):
    print "TC5: Single block message test for SHA-256."

    tc5_sha256_expected = [0xBA7816BF, 0x8F01CFEA, 0x414140DE, 0x5DAE2223,
                           0xB00361A3, 0x96177A9C, 0xB410FF61, 0xF20015AD]

    print "TC5: Expected digest values as specified by NIST:"
    for i in tc5_sha256_expected:
        print("0x%08x " % i)
    print("")
    single_block_test_sha256(NIST_512_SINGLE, ser)


#-------------------------------------------------------------------
# TC6: SHA-256 Double block message test as specified by NIST.
#-------------------------------------------------------------------
def tc6(ser):
    print "TC6: Double block message test for SHA-256."

    tc6_1_sha256_expected = [0x85E655D6, 0x417A1795, 0x3363376A, 0x624CDE5C,
                             0x76E09589, 0xCAC5F811, 0xCC4B32C1, 0xF20E533A]

    tc6_2_sha256_expected = [0x248D6A61, 0xD20638B8, 0xE5C02693, 0x0C3E6039,
                             0xA33CE459, 0x64FF2167, 0xF6ECEDD4, 0x19DB06C1]

    print "TC6: Expected digest values for first block as specified by NIST:"
    for i in tc6_1_sha256_expected:
        print("0x%08x " % i)
    print("")
    print "TC6: Expected digest values for second block as specified by NIST:"
    for i in tc6_2_sha256_expected:
        print("0x%08x " % i)
    print("")
    double_block_test_sha256(NIST_512_DOUBLE0, NIST_512_DOUBLE1, ser)


#-------------------------------------------------------------------
# TC7: SHA-256 Huge message test.
#-------------------------------------------------------------------
def tc7(ser):
    n = 1000
    print "TC7: Message with %d blocks test for SHA-256." % n
    tc7_block = ['\xaa', '\x55', '\xaa', '\x55', '\xde', '\xad', '\xbe', '\xef',
                 '\x55', '\xaa', '\x55', '\xaa', '\xf0', '\x0f', '\xf0', '\x0f',

                 '\xaa', '\x55', '\xaa', '\x55', '\xde', '\xad', '\xbe', '\xef',
                 '\x55', '\xaa', '\x55', '\xaa', '\xf0', '\x0f', '\xf0', '\x0f',

                 '\xaa', '\x55', '\xaa', '\x55', '\xde', '\xad', '\xbe', '\xef',
                 '\x55', '\xaa', '\x55', '\xaa', '\xf0', '\x0f', '\xf0', '\x0f',

                 '\xaa', '\x55', '\xaa', '\x55', '\xde', '\xad', '\xbe', '\xef',
                 '\x55', '\xaa', '\x55', '\xaa', '\xf0', '\x0f', '\xf0', '\x0f']

    tc7_expected = [0x7638f3bc, 0x500dd1a6, 0x586dd4d0, 0x1a1551af,
                    0xd821d235, 0x2f919e28, 0xd5842fab, 0x03a40f2a]

    huge_message_test_sha256(tc7_block, n, ser)

    print "TC7: Expected digest values after %d blocks:" %n
    for i in tc7_expected:
        print("0x%08x " % i)
    print("")


#-------------------------------------------------------------------
# TC8: Read name and version from SHA-512 core.
#-------------------------------------------------------------------
def tc8(ser):
    print "TC8: Reading name, type and version words from SHA-512 core."
    my_cmd = [SOC, READ_CMD, SHA512_ADDR_PREFIX, SHA512_ADDR_NAME0, EOC]
    write_serial_bytes(my_cmd, ser)
    my_cmd = [SOC, READ_CMD, SHA512_ADDR_PREFIX, SHA512_ADDR_NAME1, EOC]
    write_serial_bytes(my_cmd, ser)
    my_cmd = [SOC, READ_CMD, SHA512_ADDR_PREFIX, SHA512_ADDR_VERSION, EOC]
    write_serial_bytes(my_cmd, ser)
    print""


#-------------------------------------------------------------------
# TC9: Single block tests of SHA-512/x
#
# We do this for all modes.
#-------------------------------------------------------------------
def tc9(ser):
    print "TC9: Single block message test for SHA-512/x."

    tc9_224_expected = [0x4634270f, 0x707b6a54, 0xdaae7530, 0x460842e2,
                        0x0e37ed26, 0x5ceee9a4, 0x3e8924aa]

    tc9_256_expected = [0x53048e26, 0x81941ef9, 0x9b2e29b7, 0x6b4c7dab,
                        0xe4c2d0c6, 0x34fc6d46, 0xe0e2f131, 0x07e7af23]

    tc9_384_expected = [0xcb00753f, 0x45a35e8b, 0xb5a03d69, 0x9ac65007,
                        0x272c32ab, 0x0eded163, 0x1a8b605a, 0x43ff5bed,
                        0x8086072b, 0xa1e7cc23, 0x58baeca1, 0x34c825a7]

    tc9_512_expected = [0xddaf35a1, 0x93617aba, 0xcc417349, 0xae204131,
                        0x12e6fa4e, 0x89a97ea2, 0x0a9eeee6, 0x4b55d39a,
                        0x2192992a, 0x274fc1a8, 0x36ba3c23, 0xa3feebbd,
                        0x454d4423, 0x643ce80e, 0x2a9ac94f, 0xa54ca49f]

    print "TC9-1: Expected digest values for SHA-512/224 as specified by NIST:"
    for i in tc9_224_expected:
        print("0x%08x " % i)
    single_block_test_sha512x(NIST_1024_SINGLE, MODE_SHA_512_224, ser)
    print("")

    print "TC9-2: Expected digest values for SHA-512/256 as specified by NIST:"
    for i in tc9_256_expected:
        print("0x%08x " % i)
    single_block_test_sha512x(NIST_1024_SINGLE, MODE_SHA_512_256, ser)
    print("")

    print "TC9-3: Expected digest values for SHA-384 as specified by NIST:"
    for i in tc9_384_expected:
        print("0x%08x " % i)
    single_block_test_sha512x(NIST_1024_SINGLE, MODE_SHA_384, ser)
    print("")

    print "TC9-4: Expected digest values for SHA-512 as specified by NIST:"
    for i in tc9_512_expected:
        print("0x%08x " % i)
    single_block_test_sha512x(NIST_1024_SINGLE, MODE_SHA_512, ser)
    print("")


#-------------------------------------------------------------------
# TC10: Dual block tests of SHA-512/x
#
# We do this for all modes.
#-------------------------------------------------------------------
def tc10(ser):
    print "TC9: Single block message test for SHA-512/x."

    tc10_224_expected = [0x23fec5bb, 0x94d60b23, 0x30819264, 0x0b0c4533,
                         0x35d66473, 0x4fe40e72, 0x68674af9]

    tc10_256_expected = [0x3928e184, 0xfb8690f8, 0x40da3988, 0x121d31be,
                         0x65cb9d3e, 0xf83ee614, 0x6feac861, 0xe19b563a]

    tc10_384_expected = [0x09330c33, 0xf71147e8, 0x3d192fc7, 0x82cd1b47,
                         0x53111b17, 0x3b3b05d2, 0x2fa08086, 0xe3b0f712,
                         0xfcc7c71a, 0x557e2db9, 0x66c3e9fa, 0x91746039]

    tc10_512_expected = [0x8e959b75, 0xdae313da, 0x8cf4f728, 0x14fc143f,
                         0x8f7779c6, 0xeb9f7fa1, 0x7299aead, 0xb6889018,
                         0x501d289e, 0x4900f7e4, 0x331b99de, 0xc4b5433a,
                         0xc7d329ee, 0xb6dd2654, 0x5e96e55b, 0x874be909]

    print "TC10-1: Expected digest values for SHA-512/224 as specified by NIST:"
    for i in tc10_224_expected:
        print("0x%08x " % i)
    dual_block_test_sha512x(NIST_1024_DOUBLE0, NIST_1024_DOUBLE1, MODE_SHA_512_224, ser)
    print("")

    print "TC10-2: Expected digest values for SHA-512/256 as specified by NIST:"
    for i in tc10_256_expected:
        print("0x%08x " % i)
    dual_block_test_sha512x(NIST_1024_DOUBLE0, NIST_1024_DOUBLE1, MODE_SHA_512_256, ser)
    print("")

    print "TC10-3: Expected digest values for SHA-384 as specified by NIST:"
    for i in tc10_384_expected:
        print("0x%08x " % i)
    dual_block_test_sha512x(NIST_1024_DOUBLE0, NIST_1024_DOUBLE1, MODE_SHA_384, ser)
    print("")

    print "TC10-4: Expected digest values for SHA-512 as specified by NIST:"
    for i in tc10_512_expected:
        print("0x%08x " % i)
    dual_block_test_sha512x(NIST_1024_DOUBLE0, NIST_1024_DOUBLE1, MODE_SHA_512, ser)
    print("")


#-------------------------------------------------------------------
# main()
#
# Parse any arguments and run the tests.
#-------------------------------------------------------------------
def main():
    # Open device
    ser = serial.Serial()
    ser.port=SERIAL_DEVICE
    ser.baudrate=BAUD_RATE
    ser.bytesize=DATA_BITS
    ser.parity='N'
    ser.stopbits=STOP_BITS
    ser.timeout=1
    ser.writeTimeout=0

    if VERBOSE:
        print "Setting up a serial port and starting a receive thread."

    try:
        ser.open()
    except:
        print "Error: Can't open serial device."
        sys.exit(1)

    # Try and switch baud rate in the FPGA and then here.
    bit_rate_high = chr((BIT_RATE2 >> 8) & 0xff)
    bit_rate_low = chr(BIT_RATE2 & 0xff)

    if VERBOSE:
        print("Changing to new baud rate.")
        print("Baud rate: %d" % BAUD_RATE2)
        print("Bit rate high byte: 0x%02x" % ord(bit_rate_high))
        print("Bit rate low byte:  0x%02x" % ord(bit_rate_low))

    write_serial_bytes([SOC, WRITE_CMD, UART_ADDR_PREFIX, UART_ADDR_BIT_RATE,
                        '\x00', '\x00', bit_rate_high, bit_rate_low, EOC], ser)
    ser.baudrate=BAUD_RATE2

    try:
        my_thread = threading.Thread(target=read_serial_thread, args=(ser,))
    except:
        print "Error: Can't start thread."
        sys.exit()
        
    my_thread.daemon = True
    my_thread.start()

    # Run the enabled test cases.
    tc_list = [(tc1, False), (tc2, False), (tc3, False), (tc4, False),
               (tc5, False), (tc6, False), (tc7, True), (tc8, False),
               (tc9, False), (tc10, False)]
    for (test_case, action) in tc_list:
        if action:
            test_case(ser)

    # Exit nicely.
    time.sleep(50 * COMM_DELAY_TIME)
    if VERBOSE:
        print "Done. Closing device."
    ser.close()


#-------------------------------------------------------------------
# __name__
# Python thingy which allows the file to be run standalone as
# well as parsed from within a Python interpreter.
#-------------------------------------------------------------------
if __name__=="__main__": 
    # Run the main function.
    sys.exit(main())


#=======================================================================
# EOF hash_tester.py
#=======================================================================
