#!/usr/bin/env python
# -*- coding: utf-8 -*-
#=======================================================================
#
# entropy_tester.py
# -----------------
# Test SW for the FPGA entropy tester project.
#
# Note: This program requires the PySerial module.
# http://pyserial.sourceforge.net/
#
# 
# Author: Joachim Strömbergson
# Copyright (c) 2014, Secworks Sweden AB
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
import serial
import os
import time
import threading

 
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

# Delay times.
PROC_DELAY_TIME = 0.001
COMM_DELAY_TIME = 0.005


# Verbose operation on/off
VERBOSE = False

# Memory map.
SOC                   = '\x55'
EOC                   = '\xaa'
READ_CMD              = '\x10'
WRITE_CMD             = '\x11'

UART_ADDR_PREFIX      = '\x00'
UART_ADDR_NAME0       = '\x00'
UART_ADDR_NAME1       = '\x01'
UART_ADDR_TYPE        = '\x02'
UART_ADDR_VERSION     = '\x03'
UART_ADDR_BIT_RATE    = '\x10'
UART_ADDR_DATA_BITS   = '\x11'
UART_ADDR_STOP_BITS   = '\x12'

BPENT_ADDR_PREFIX       = '\x10'
BPENT_ADDR_WR_RNG1      = '\x00'
BPENT_ADDR_WR_RNG2      = '\x01'
BPENT_ADDR_RD_RNG1_RNG2 = '\x10'
BPENT_ADDR_RD_P         = '\x11'
BPENT_ADDR_RD_N         = '\x12'


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
# read_word()
#-------------------------------------------------------------------
def read_word(prefix, addr, ser):
    cmd = [SOC, READ_CMD] + [prefix, addr] +  [EOC]
    write_serial_bytes(cmd , ser)


#-------------------------------------------------------------------
# read_rng1_rng2()
#-------------------------------------------------------------------
def read_rng1_rng2(ser):
    if VERBOSE:
        print "Reading rng1 and rng2 three times."
    
    for i in range(3):
        read_word(BPENT_ADDR_PREFIX, BPENT_ADDR_RD_RNG1_RNG2, ser)


#-------------------------------------------------------------------
# read_n_data()
#
# Note we do a lot of read ops here.
#-------------------------------------------------------------------
def read_n_data(ser):
    n = int(1E6)
    if VERBOSE:
        print "Reading n vector %d times." % n

    for i in range(n):
        read_word(BPENT_ADDR_PREFIX, BPENT_ADDR_RD_N, ser)


#-------------------------------------------------------------------
# read_p_data()
#-------------------------------------------------------------------
def read_p_data(ser):
    n = 10
    if VERBOSE:
        print "Reading p vector %d times." % n

    for i in range(n):
        read_word(BPENT_ADDR_PREFIX, BPENT_ADDR_RD_P, ser)


#-------------------------------------------------------------------
# read_uart()
#
# We try to read from the uart to get some read ops working.
#-------------------------------------------------------------------
def read_uart(ser):
        read_word(UART_ADDR_PREFIX, UART_ADDR_NAME0, ser)
        read_word(UART_ADDR_PREFIX, UART_ADDR_NAME1, ser)
        read_word(UART_ADDR_PREFIX, UART_ADDR_TYPE, ser)
        read_word(UART_ADDR_PREFIX, UART_ADDR_VERSION, ser)


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

    # Test the communication by reading name etc from uart.
    read_uart(ser)

    # Perform RNG read ops.
    read_rng1_rng2(ser)
    read_p_data(ser)
    read_n_data(ser)

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
