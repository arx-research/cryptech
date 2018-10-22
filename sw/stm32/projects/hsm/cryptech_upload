#!/usr/bin/env python
#
# Copyright (c) 2016-2017, NORDUnet A/S All rights reserved.
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

"""
Utility to upload a new firmware image or FPGA bitstream.
"""

import os
import sys
import time
import struct
import serial
import socket
import getpass
import os.path
import tarfile
import argparse
import platform

from binascii import crc32

FIRMWARE_CHUNK_SIZE = 4096
FPGA_CHUNK_SIZE     = 4096


def parse_args():
    """
    Parse the command line arguments
    """

    share_directory = "/usr/share" if platform.system() == "Linux" else "/usr/local/share"

    default_tarball = os.path.join(share_directory, "cryptech-alpha-firmware.tar.gz")

    if not os.path.exists(default_tarball):
        default_tarball = None

    parser = argparse.ArgumentParser(description = __doc__,
                                     formatter_class = argparse.ArgumentDefaultsHelpFormatter,
                                     )

    parser.add_argument("-d", "--device",
                        default = os.getenv("CRYPTECH_CTY_CLIENT_SERIAL_DEVICE", "/dev/ttyUSB0"),
                        help = "Name of management port USB serial device",
                        )

    parser.add_argument("--socket",
                        default = os.getenv("CRYPTECH_CTY_CLIENT_SOCKET_NAME",
                                            "/tmp/.cryptech_muxd.cty"),
                        help = "Name of cryptech_muxd management port socket",
                        )

    parser.add_argument("--firmware-tarball",
                        type = argparse.FileType("rb"),
                        default = default_tarball,
                        help = "Location of firmware tarball",
                        )

    parser.add_argument("--username",
                        choices = ("so", "wheel"),
                        default = "so",
                        help = "Username to use when logging into the HSM",
                        )

    parser.add_argument("--pin",
                        help = "PIN to use when logging into the HSM",
                        )

    parser.add_argument("--separate-pins",
                        action = "store_true",
                        help = "Prompt separately for each PIN required during firmware upload")

    actions = parser.add_mutually_exclusive_group(required = True)
    actions.add_argument("--fpga",
                         action = "store_true",
                         help = "Upload FPGA bitstream",
                         )
    actions.add_argument("--firmware", "--hsm",
                         action = "store_true",
                         help = "Upload HSM firmware image",
                         )
    actions.add_argument("--bootloader",
                         action = "store_true",
                         help = "Upload bootloader image (dangerous!)",
                         )

    parser.add_argument("--simon-says-whack-my-bootloader",
                        action = "store_true",
                        help = "Confirm that you really want to risk bricking the HSM",
                        )

    parser.add_argument("-i", "--explicit-image",
                        type = argparse.FileType("rb"),
                        help = "Explicit source image file for upload, overrides firmware tarball")

    parser.add_argument("--debug",
                        action = "store_true",
                        help = "Enable debugging of upload protocol",
                        )

    parser.add_argument("-q", "--quiet",
                        action = "store_true",
                        help = "Only report errors",
                        )

    return parser.parse_args()


class ManagementPortAbstract(object):
    """
    Abstract class encapsulating actions on the HSM management port.
    """

    def __init__(self, args):
        self.args = args

    def write(self, data):
        numeric = isinstance(data, (int, long))
        if numeric:
            data = struct.pack("<I", data)
        self.send(data)
        if self.args.debug:
            if numeric:
                print("Wrote 0x{!s}".format(data.encode("hex")))
            else:
                print("Wrote {!r}".format(data))

    def read(self):
        res = ""
        x = self.recv()
        while not x:
            x = self.recv()
        while x:
            res += x
            x = self.recv()
        if self.args.debug:
            print ("Read {!r}".format(res))
        return res

    def execute(self, cmd):
        self.write("\r")
        prompt = self.read()
        #if prompt.endswith("This is the bootloader speaking..."):
        #    prompt = self.read()
        if prompt.endswith("Username: "):
            self.write(self.args.username + "\r")
            prompt = self.read()
            if prompt.endswith("Password: "):
                if not self.args.pin or self.args.separate_pins:
                    self.args.pin = getpass.getpass("{} PIN: ".format(self.args.username))
                self.write(self.args.pin + "\r")
                prompt = self.read()
        if not prompt.endswith(("> ", "# ")):
            print("Device does not seem to be ready for a file transfer (got {!r})".format(prompt))
            return prompt
        self.write(cmd + "\r")
        response = self.read()
        return response


class ManagementPortSerial(ManagementPortAbstract):
    """
    Implmentation of HSM management port abstraction over a direct
    serial connection.
    """

    def __init__(self, args, timeout = 1):
        super(ManagementPortSerial, self).__init__(args)
        self.serial = serial.Serial(args.device, 921600, timeout = timeout)

    def send(self, data):
        self.serial.write(data)
        self.serial.flush()

    def recv(self):
        return self.serial.read(1)

    def set_timeout(self, timeout):
        self.serial.timeout = timeout

    def close(self):
        self.serial.close()

class ManagementPortSocket(ManagementPortAbstract):
    """
    Implmentation of HSM management port abstraction over a PF_UNIX
    socket connection to the cryptech_muxd management socket.
    """

    def __init__(self, args, timeout = 1):
        super(ManagementPortSocket, self).__init__(args)
        self.socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.socket.connect(args.socket)
        self.socket.settimeout(timeout)

    def send(self, data):
        self.socket.sendall(data)

    def recv(self):
        try:
            return self.socket.recv(1)
        except socket.timeout:
            return ""

    def set_timeout(self, timeout):
        self.socket.settimeout(timeout)

    def close(self):
        self.socket.close()


def send_file(src, size, args, dst):
    """
    Upload an image from some file-like source to the management port.
    Details depend on what kind of image it is.
    """

    if args.fpga:
        chunk_size = FPGA_CHUNK_SIZE
        response = dst.execute("fpga bitstream upload")
    elif args.firmware:
        chunk_size = FIRMWARE_CHUNK_SIZE
        response = dst.execute("firmware upload")
        if "Rebooting" in response:
            response = dst.execute("firmware upload")
    elif args.bootloader:
        chunk_size = FIRMWARE_CHUNK_SIZE
        response = dst.execute("bootloader upload")
    if "Access denied" in response:
        print "Access denied"
        return False
    if not "OK" in response:
        print("Device did not accept the upload command (got {!r})".format(response))
        return False

    dst.set_timeout(0.001)
    crc = 0
    counter = 0
    # 1. Write size of file (4 bytes)
    dst.write(struct.pack("<I", size))
    response = dst.read()
    if not response.startswith("Send "):
        print response
        return False

    # 2. Write file contents while calculating CRC-32
    chunks = int((size + chunk_size - 1) / chunk_size)
    for counter in xrange(chunks):
        data = src.read(chunk_size)
        dst.write(data)
        if not args.quiet:
            print("Wrote {!s} bytes (chunk {!s}/{!s})".format(len(data), counter + 1, chunks))
        # read ACK (a counter of number of 4k chunks received)
        ack_bytes = ""
        while len(ack_bytes) < 4:
            ack_bytes += dst.read()
        ack = struct.unpack("<I", ack_bytes[:4])[0]
        if ack != counter + 1:
            print("ERROR: Did not receive the expected counter as ACK (got {!r}/{!r}, not {!r})".format(ack, ack_bytes, counter))
            return False
        counter += 1

        crc = crc32(data, crc) & 0xffffffff

    # 3. Write CRC-32 (4 bytes)
    dst.write(struct.pack("<I", crc))
    response = dst.read()
    if not args.quiet:
        print response

    src.close()

    if args.fpga:
        # tell the fpga to read its new configuration
        dst.execute("fpga reset")
        # log out of the CLI
        # (firmware/bootloader upgrades reboot, don't need an exit)
        dst.execute("exit")

    return True


dire_bootloader_warning = '''
                            WARNING

Updating the bootloader risks bricking your HSM!  If something goes
badly wrong here, or you upload a bad bootloader image, you will not
be able to recover without an ST-LINK programmer.

In most cases a normal "--firmware" upgrade should be all that is
necessary to bring your HSM up to date, there is seldom any real need
to update the bootloader.

Do not proceed with this unless you REALLY know what you are doing.

If you got here by accident, ^C now, without answering the PIN prompt.
'''


def main():
    global args
    args = parse_args()


    if args.bootloader:
        if not args.simon_says_whack_my_bootloader:
            sys.exit("You didn't say \"Simon says\"")
        print dire_bootloader_warning
        args.pin = None

    if args.explicit_image is None and args.firmware_tarball is None:
        sys.exit("No source file specified for upload and firmware tarball not found")

    if args.explicit_image:
        src = args.explicit_image       # file-like object, thanks to argparse
        size = os.fstat(src.fileno()).st_size
        if size == 0:                   # Flashing from stdin won't work, sorry
            sys.exit("Can't flash from a pipe or zero-length file")
        if not args.quiet:
            print "Uploading from explicitly-specified file {}".format(args.explicit_image.name)

    else:
        tar = tarfile.open(fileobj = args.firmware_tarball)
        if not args.quiet:
            print "Firmware tarball {} content:".format(args.firmware_tarball.name)
        tar.list(True)
        if args.fpga:
            name = "alpha_fmc.bit"
        elif args.firmware:
            name = "hsm.bin"
        elif args.bootloader:
            name = "bootloader.bin"
        else:
            # Somebody updated other part of this script without updating this part :(
            sys.exit("Don't know which component to select from firmware tarball, sorry")
        try:
            size = tar.getmember(name).size
        except KeyError:
            sys.exit("Expected component {} missing from firmware tarball {}".format(name, args.firmware_tarball.name))
        src = tar.extractfile(name)
        if not args.quiet:
            print "Uploading {} from {}".format(name, args.firmware_tarball.name)

    if not args.quiet:
        print "Initializing management port and synchronizing with HSM, this may take a few seconds"
    try:
        dst = ManagementPortSocket(args, timeout = 1)
    except socket.error as e:
        dst = ManagementPortSerial(args, timeout = 1)
    send_file(src, size, args, dst)
    dst.close()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        pass
