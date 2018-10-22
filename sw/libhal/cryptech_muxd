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
Implementation of Cryptech RPC protocol multiplexer in Python.

Unlike the original C implementation, this uses SLIP encapsulation
over a SOCK_STREAM channel, because support for SOCK_SEQPACKET is not
what we might wish.  We outsource all the heavy lifting for serial and
network I/O to the PySerial and Tornado libraries, respectively.
"""

import os
import sys
import time
import struct
import atexit
import weakref
import logging
import argparse
import logging.handlers

import serial
import serial.tools.list_ports_posix

import tornado.tcpserver
import tornado.iostream
import tornado.netutil
import tornado.ioloop
import tornado.queues
import tornado.locks
import tornado.gen

from cryptech.libhal import HAL_OK, RPC_FUNC_GET_VERSION, RPC_FUNC_LOGOUT, RPC_FUNC_LOGOUT_ALL


logger = logging.getLogger("cryptech_muxd")


SLIP_END     = chr(0300)        # Indicates end of SLIP packet
SLIP_ESC     = chr(0333)        # Indicates byte stuffing
SLIP_ESC_END = chr(0334)        # ESC ESC_END means END data byte
SLIP_ESC_ESC = chr(0335)        # ESC ESC_ESC means ESC data byte

Control_U    = chr(0025)        # Console: clear line
Control_M    = chr(0015)        # Console: end of line


def slip_encode(buffer):
    "Encode a buffer using SLIP encapsulation."
    return SLIP_END + buffer.replace(SLIP_ESC, SLIP_ESC + SLIP_ESC_ESC).replace(SLIP_END, SLIP_ESC + SLIP_ESC_END) + SLIP_END

def slip_decode(buffer):
    "Decode a SLIP-encapsulated buffer."
    return buffer.strip(SLIP_END).replace(SLIP_ESC + SLIP_ESC_END, SLIP_END).replace(SLIP_ESC + SLIP_ESC_ESC, SLIP_ESC)


def client_handle_get(msg):
    "Extract client_handle field from a Cryptech RPC message."
    return struct.unpack(">L", msg[4:8])[0]

def client_handle_set(msg, handle):
    "Replace client_handle field in a Cryptech RPC message."
    return msg[:4] + struct.pack(">L", handle) + msg[8:]


logout_msg     = struct.pack(">LL", RPC_FUNC_LOGOUT,     0)
logout_all_msg = struct.pack(">LL", RPC_FUNC_LOGOUT_ALL, 0)


class SerialIOStream(tornado.iostream.BaseIOStream):
    """
    Implementation of a Tornado IOStream over a PySerial device.
    """

    # In theory, we want zero (non-blocking mode) for both the read
    # and write timeouts here so that PySerial will let Tornado handle
    # all the select()/poll()/epoll()/kqueue() fun, delivering maximum
    # throughput to all.  In practice, this has always worked for the
    # author, but another developer reports that on some (not all)
    # platforms this fails consistently with Tornado reporting write
    # timeout errors, presumably as the result of receiving an IOError
    # or OSError exception from PySerial.  For reasons we don't really
    # understand, setting a PySerial write timeout on the order of
    # 50-100 ms "solves" this problem.  Again in theory, this will
    # result in lower throughput if PySerial spends too much time
    # blocking on a single serial device when Tornado could be doing
    # something useful elsewhere, but such is life.

    def __init__(self, device):
        self.serial = serial.Serial(device, 921600, timeout = 0, write_timeout = 0.1)
        self.serial_device = device
        super(SerialIOStream, self).__init__()

    def fileno(self):
        return self.serial.fileno()

    def close_fd(self):
        self.serial.close()

    def write_to_fd(self, data):
        return self.serial.write(data)

    def read_from_fd(self):
        return self.serial.read(self.read_chunk_size) or None


class PFUnixServer(tornado.tcpserver.TCPServer):
    """
    Variant on tornado.tcpserver.TCPServer, listening on a PF_UNIX
    (aka PF_LOCAL) socket instead of a TCP socket.
    """

    def __init__(self, serial_stream, socket_filename, mode = 0600):
        super(PFUnixServer, self).__init__()
        self.serial = serial_stream
        self.socket_filename = socket_filename
        self.add_socket(tornado.netutil.bind_unix_socket(socket_filename, mode))
        atexit.register(self.atexit_unlink)

    def atexit_unlink(self):
        try:
            os.unlink(self.socket_filename)
        except:
            pass


class RPCIOStream(SerialIOStream):
    """
    Tornado IOStream for a serial RPC channel.
    """

    def __init__(self, device):
        super(RPCIOStream, self).__init__(device)
        self.queues = weakref.WeakValueDictionary()
        self.rpc_input_lock = tornado.locks.Lock()

    @tornado.gen.coroutine
    def rpc_input(self, query, handle = 0, queue = None):
        "Send a query to the HSM."
        logger.debug("RPC send: %s", ":".join("{:02x}".format(ord(c)) for c in query))
        if queue is not None:
            self.queues[handle] = queue
        with (yield self.rpc_input_lock.acquire()):
            yield self.write(query)
        logger.debug("RPC sent")

    @tornado.gen.coroutine
    def rpc_output_loop(self):
        "Handle reply stream HSM -> network."
        while True:
            try:
                logger.debug("RPC UART read")
                reply = yield self.read_until(SLIP_END)
            except tornado.iostream.StreamClosedError:
                logger.info("RPC UART closed")
                for q in self.queues.itervalues():
                    q.put_nowait(None)
                return
            logger.debug("RPC recv: %s", ":".join("{:02x}".format(ord(c)) for c in reply))
            if reply == SLIP_END:
                continue
            try:
                handle = client_handle_get(slip_decode(reply))
            except:
                logger.debug("RPC skipping bad packet")
                continue
            if handle not in self.queues:
                logger.debug("RPC ignoring response: handle 0x%x", handle)
                continue
            logger.debug("RPC queue put: handle 0x%x, qsize %s", handle, self.queues[handle].qsize())
            self.queues[handle].put_nowait(reply)

    def logout_all(self):
        "Execute an RPC LOGOUT_ALL operation."
        return self.rpc_input(slip_encode(logout_all_msg))


class QueuedStreamClosedError(tornado.iostream.StreamClosedError):
    "Deferred StreamClosedError passed throught a Queue."


class RPCServer(PFUnixServer):
    """
    Serve multiplexed Cryptech RPC over a PF_UNIX socket.
    """

    @tornado.gen.coroutine
    def handle_stream(self, stream, address):
        "Handle one network connection."
        handle = self.next_client_handle()
        queue  = tornado.queues.Queue()
        logger.info("RPC connected %r, handle 0x%x", stream, handle)
        while True:
            try:
                logger.debug("RPC socket read, handle 0x%x", handle)
                query = yield stream.read_until(SLIP_END)
                if len(query) < 9:
                    continue
                query = slip_encode(client_handle_set(slip_decode(query), handle))
                yield self.serial.rpc_input(query, handle, queue)
                logger.debug("RPC queue wait, handle 0x%x", handle)
                reply = yield queue.get()
                if reply is None:
                    raise QueuedStreamClosedError()
                logger.debug("RPC socket write, handle 0x%x", handle)
                yield stream.write(SLIP_END + reply)
            except tornado.iostream.StreamClosedError:
                logger.info("RPC closing %r, handle 0x%x", stream, handle)
                stream.close()
                query = slip_encode(client_handle_set(logout_msg, handle))
                yield self.serial.rpc_input(query, handle)
                return

    client_handle = int(time.time()) << 4

    @classmethod
    def next_client_handle(cls):
        cls.client_handle += 1
        cls.client_handle &= 0xFFFFFFFF
        return cls.client_handle


class CTYIOStream(SerialIOStream):
    """
    Tornado IOStream for a serial console channel.
    """

    def __init__(self, device, console_log = None):
        super(CTYIOStream, self).__init__(device)
        self.attached_cty = None
        self.console_log  = console_log

    @tornado.gen.coroutine
    def cty_output_loop(self):
        while True:
            try:
                buffer = yield self.read_bytes(self.read_chunk_size, partial = True)
            except tornado.iostream.StreamClosedError:
                logger.info("CTY UART closed")
                if self.attached_cty is not None:
                    self.attached_cty.close()
                return
            try:
                futures = []
                if self.console_log is not None:
                    futures.append(self.console_log.write(buffer))
                if self.attached_cty is not None:
                    futures.append(self.attached_cty.write(buffer))
                if futures:
                    yield futures
            except tornado.iostream.StreamClosedError:
                pass


class CTYServer(PFUnixServer):
    """
    Serve Cryptech console over a PF_UNIX socket.
    """

    @tornado.gen.coroutine
    def handle_stream(self, stream, address):
        "Handle one network connection."

        if self.serial.attached_cty is not None:
            yield stream.write("[Console already in use, sorry]\n")
            stream.close()
            return

        logger.info("CTY connected to %r", stream)

        try:
            self.serial.attached_cty = stream
            while self.serial.attached_cty is stream:
                yield self.serial.write((yield stream.read_bytes(1024, partial = True)))
        except tornado.iostream.StreamClosedError:
            stream.close()
        finally:
            logger.info("CTY disconnected from %r", stream)
            if self.serial.attached_cty is stream:
                self.serial.attached_cty = None


class ProbeIOStream(SerialIOStream):
    """
    Tornado IOStream for probing a serial port.  This is nasty.
    """

    def __init__(self, device):
        super(ProbeIOStream, self).__init__(device)

    @classmethod
    @tornado.gen.coroutine
    def run_probes(cls, args):

        if args.rpc_device is not None and args.cty_device is not None:
            return

        if args.probe:
            devs = set(args.probe)
        else:
            devs = set(str(port)
                       for port, desc, hwid in serial.tools.list_ports_posix.comports()
                       if "VID:PID=0403:6014" in hwid)

        devs.discard(args.rpc_device)
        devs.discard(args.cty_device)

        if not devs:
            return

        logging.debug("Probing candidate devices %s", " ".join(devs))

        results = yield dict((dev, ProbeIOStream(dev).run_probe()) for dev in devs)

        for dev, result in results.iteritems():

            if result == "cty" and args.cty_device is None:
                logger.info("Selecting %s as CTY device", dev)
                args.cty_device = dev

            if result == "rpc" and args.rpc_device is None:
                logger.info("Selecting %s as RPC device", dev)
                args.rpc_device = dev

    @tornado.gen.coroutine
    def run_probe(self):

        RPC_query = struct.pack(">LL",  RPC_FUNC_GET_VERSION, 0)
        RPC_reply = struct.pack(">LLL", RPC_FUNC_GET_VERSION, 0, HAL_OK)

        probe_string = SLIP_END + Control_U + SLIP_END + RPC_query + SLIP_END + Control_U + Control_M

        yield self.write(probe_string)
        yield tornado.gen.sleep(0.5)
        response = yield self.read_bytes(self.read_chunk_size, partial = True)

        logger.debug("Probing %s: %r %s", self.serial_device, response, ":".join("{:02x}".format(ord(c)) for c in response))

        is_cty = any(prompt in response for prompt in ("Username:", "Password:", "cryptech>"))

        try:
            is_rpc = response[response.index(SLIP_END + RPC_reply) + len(SLIP_END + RPC_reply) + 4] == SLIP_END
        except ValueError:
            is_rpc = False
        except IndexError:
            is_rpc = False

        assert not is_cty or not is_rpc

        result = None

        if is_cty:
            result = "cty"
            yield self.write(Control_U)

        if is_rpc:
            result = "rpc"
            yield self.write(SLIP_END)

        self.close()
        raise tornado.gen.Return(result)



@tornado.gen.coroutine
def main():
    parser = argparse.ArgumentParser(formatter_class = argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument("-v", "--verbose",
                        action = "count",
                        help = "blather about what we're doing")

    parser.add_argument("-l", "--log-file",
                        help = "log to file instead of stderr")

    parser.add_argument("-L", "--console-log",
                        type = argparse.FileType("a"),
                        help = "log console output to file")

    parser.add_argument("-p", "--probe",
                        nargs = "*",
                        metavar = "DEVICE",
                        help = "probe for device UARTs")

    parser.add_argument("--rpc-device",
                        help    = "RPC serial device name",
                        default = os.getenv("CRYPTECH_RPC_CLIENT_SERIAL_DEVICE"))

    parser.add_argument("--rpc-socket",
                        help    = "RPC PF_UNIX socket name",
                        default = os.getenv("CRYPTECH_RPC_CLIENT_SOCKET_NAME",
                                            "/tmp/.cryptech_muxd.rpc"))

    parser.add_argument("--rpc-socket-mode",
                        help    = "permission bits for RPC socket inode",
                        default = 0600, type = lambda s: int(s, 8))

    parser.add_argument("--cty-device",
                        help    = "CTY serial device name",
                        default = os.getenv("CRYPTECH_CTY_CLIENT_SERIAL_DEVICE"))

    parser.add_argument("--cty-socket",
                        help    = "CTY PF_UNIX socket name",
                        default = os.getenv("CRYPTECH_CTY_CLIENT_SOCKET_NAME",
                                            "/tmp/.cryptech_muxd.cty"))

    parser.add_argument("--cty-socket-mode",
                        help    = "permission bits for CTY socket inode",
                        default = 0600, type = lambda s: int(s, 8))

    args = parser.parse_args()

    if args.log_file is not None:
        logging.getLogger().handlers[:] = [logging.handlers.WatchedFileHandler(args.log_file)]

    logging.getLogger().handlers[0].setFormatter(
        logging.Formatter("%(asctime)-15s %(name)s[%(process)d]:%(levelname)s: %(message)s",
                          "%Y-%m-%d %H:%M:%S"))

    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG if args.verbose > 1 else logging.INFO)

    if args.probe is not None:
        yield ProbeIOStream.run_probes(args)

    if args.console_log is not None:
        console_log = tornado.iostream.PipeIOStream(args.console_log.fileno())
    else:
        console_log = None

    futures = []

    if args.rpc_device is None:
        logger.warn("No RPC device found")
    else:
        rpc_stream = RPCIOStream(device = args.rpc_device)
        rpc_server = RPCServer(rpc_stream, args.rpc_socket, args.rpc_socket_mode)
        futures.append(rpc_stream.rpc_output_loop())
        futures.append(rpc_stream.logout_all())

    if args.cty_device is None:
        logger.warn("No CTY device found")
    else:
        cty_stream = CTYIOStream(device = args.cty_device, console_log = console_log)
        cty_server = CTYServer(cty_stream, args.cty_socket, args.cty_socket_mode)
        futures.append(cty_stream.cty_output_loop())

    # Might want to use WaitIterator(dict(...)) here so we can
    # diagnose and restart output loops if they fail?

    if futures:
        yield futures

if __name__ == "__main__":
    try:
        tornado.ioloop.IOLoop.current().run_sync(main)
    except (SystemExit, KeyboardInterrupt):
        pass
    except:
        logger.exception("Unhandled exception")
    else:
        logger.debug("Main loop exited")
