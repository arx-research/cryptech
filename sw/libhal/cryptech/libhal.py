# Copyright (c) 2016, NORDUnet A/S
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

"""
A Python interface to the Cryptech libhal RPC API.
"""

# A lot of this is hand-generated XDR data structure encoding.  If and
# when we ever convert the C library to use data structures processed
# by rpcgen, we may want to rewrite this code to use the output of
# something like https://github.com/floodlight/xdr.git -- in either
# case the generated code would just be for the data structures, we're
# not likely to want to use the full ONC RPC mechanism.

import os
import uuid
import xdrlib
import socket
import logging

logger = logging.getLogger(__name__)


SLIP_END     = chr(0300)        # indicates end of packet
SLIP_ESC     = chr(0333)        # indicates byte stuffing
SLIP_ESC_END = chr(0334)        # ESC ESC_END means END data byte
SLIP_ESC_ESC = chr(0335)        # ESC ESC_ESC means ESC data byte


def slip_encode(buffer):
    return SLIP_END + buffer.replace(SLIP_ESC, SLIP_ESC + SLIP_ESC_ESC).replace(SLIP_END, SLIP_ESC + SLIP_ESC_END) + SLIP_END

def slip_decode(buffer):
    return buffer.strip(SLIP_END).replace(SLIP_ESC + SLIP_ESC_END, SLIP_END).replace(SLIP_ESC + SLIP_ESC_ESC, SLIP_ESC)


HAL_OK = 0

class HALError(Exception):
    "LibHAL error"

    table = [None]

    @classmethod
    def define(cls, **kw):
        assert len(kw) == 1
        name, text = kw.items()[0]
        e = type(name, (cls,), dict(__doc__ = text))
        cls.table.append(e)
        globals()[name] = e

HALError.define(HAL_ERROR_BAD_ARGUMENTS             = "Bad arguments given")
HALError.define(HAL_ERROR_UNSUPPORTED_KEY           = "Unsupported key type or key length")
HALError.define(HAL_ERROR_IO_SETUP_FAILED           = "Could not set up I/O with FPGA")
HALError.define(HAL_ERROR_IO_TIMEOUT                = "I/O with FPGA timed out")
HALError.define(HAL_ERROR_IO_UNEXPECTED             = "Unexpected response from FPGA")
HALError.define(HAL_ERROR_IO_OS_ERROR               = "Operating system error talking to FPGA")
HALError.define(HAL_ERROR_IO_BAD_COUNT              = "Bad byte count")
HALError.define(HAL_ERROR_CSPRNG_BROKEN             = "CSPRNG is returning nonsense")
HALError.define(HAL_ERROR_KEYWRAP_BAD_MAGIC         = "Bad magic number while unwrapping key")
HALError.define(HAL_ERROR_KEYWRAP_BAD_LENGTH        = "Length out of range while unwrapping key")
HALError.define(HAL_ERROR_KEYWRAP_BAD_PADDING       = "Non-zero padding detected unwrapping key")
HALError.define(HAL_ERROR_IMPOSSIBLE                = "\"Impossible\" error")
HALError.define(HAL_ERROR_ALLOCATION_FAILURE        = "Memory allocation failed")
HALError.define(HAL_ERROR_RESULT_TOO_LONG           = "Result too long for buffer")
HALError.define(HAL_ERROR_ASN1_PARSE_FAILED         = "ASN.1 parse failed")
HALError.define(HAL_ERROR_KEY_NOT_ON_CURVE          = "EC key is not on its purported curve")
HALError.define(HAL_ERROR_INVALID_SIGNATURE         = "Invalid signature")
HALError.define(HAL_ERROR_CORE_NOT_FOUND            = "Requested core not found")
HALError.define(HAL_ERROR_CORE_BUSY                 = "Requested core busy")
HALError.define(HAL_ERROR_KEYSTORE_ACCESS           = "Could not access keystore")
HALError.define(HAL_ERROR_KEY_NOT_FOUND             = "Key not found")
HALError.define(HAL_ERROR_KEY_NAME_IN_USE           = "Key name in use")
HALError.define(HAL_ERROR_NO_KEY_SLOTS_AVAILABLE    = "No key slots available")
HALError.define(HAL_ERROR_PIN_INCORRECT             = "PIN incorrect")
HALError.define(HAL_ERROR_NO_CLIENT_SLOTS_AVAILABLE = "No client slots available")
HALError.define(HAL_ERROR_FORBIDDEN                 = "Forbidden")
HALError.define(HAL_ERROR_XDR_BUFFER_OVERFLOW       = "XDR buffer overflow")
HALError.define(HAL_ERROR_RPC_TRANSPORT             = "RPC transport error")
HALError.define(HAL_ERROR_RPC_PACKET_OVERFLOW       = "RPC packet overflow")
HALError.define(HAL_ERROR_RPC_BAD_FUNCTION          = "Bad RPC function number")
HALError.define(HAL_ERROR_KEY_NAME_TOO_LONG         = "Key name too long")
HALError.define(HAL_ERROR_MASTERKEY_NOT_SET         = "Master key (Key Encryption Key) not set")
HALError.define(HAL_ERROR_MASTERKEY_FAIL            = "Master key generic failure")
HALError.define(HAL_ERROR_MASTERKEY_BAD_LENGTH      = "Master key of unacceptable length")
HALError.define(HAL_ERROR_KS_DRIVER_NOT_FOUND       = "Keystore driver not found")
HALError.define(HAL_ERROR_KEYSTORE_BAD_CRC          = "Bad CRC in keystore")
HALError.define(HAL_ERROR_KEYSTORE_BAD_BLOCK_TYPE   = "Unsupported keystore block type")
HALError.define(HAL_ERROR_KEYSTORE_LOST_DATA        = "Keystore appears to have lost data")
HALError.define(HAL_ERROR_BAD_ATTRIBUTE_LENGTH      = "Bad attribute length")
HALError.define(HAL_ERROR_ATTRIBUTE_NOT_FOUND       = "Attribute not found")
HALError.define(HAL_ERROR_NO_KEY_INDEX_SLOTS        = "No key index slots available")
HALError.define(HAL_ERROR_KS_INDEX_UUID_MISORDERED  = "Key index UUID misordered")
HALError.define(HAL_ERROR_KEYSTORE_WRONG_BLOCK_TYPE = "Wrong block type in keystore")
HALError.define(HAL_ERROR_RPC_PROTOCOL_ERROR        = "RPC protocol error")
HALError.define(HAL_ERROR_NOT_IMPLEMENTED           = "Not implemented")


class Enum(int):

    def __new__(cls, name, value):
        self = int.__new__(cls, value)
        self._name = name
        setattr(self.__class__, name, self)
        return self

    def __str__(self):
        return self._name

    def __repr__(self):
        return "<Enum:{0.__class__.__name__} {0._name}:{0:d}>".format(self)

    _counter = 0

    @classmethod
    def define(cls, names):
        symbols = []
        for name in names.translate(None, "{}").split(","):
            if "=" in name:
                name, sep, expr = name.partition("=")
                cls._counter = eval(expr.strip())
            if not isinstance(cls._counter, int):
                raise TypeError
            symbols.append(cls(name.strip(), cls._counter))
            cls._counter += 1
        cls.index = dict((int(symbol),  symbol) for symbol in symbols)
        globals().update((symbol._name, symbol) for symbol in symbols)

    def xdr_packer(self, packer):
        packer.pack_uint(self)


class RPCFunc(Enum): pass

RPCFunc.define('''
    RPC_FUNC_GET_VERSION,
    RPC_FUNC_GET_RANDOM,
    RPC_FUNC_SET_PIN,
    RPC_FUNC_LOGIN,
    RPC_FUNC_LOGOUT,
    RPC_FUNC_LOGOUT_ALL,
    RPC_FUNC_IS_LOGGED_IN,
    RPC_FUNC_HASH_GET_DIGEST_LEN,
    RPC_FUNC_HASH_GET_DIGEST_ALGORITHM_ID,
    RPC_FUNC_HASH_GET_ALGORITHM,
    RPC_FUNC_HASH_INITIALIZE,
    RPC_FUNC_HASH_UPDATE,
    RPC_FUNC_HASH_FINALIZE,
    RPC_FUNC_PKEY_LOAD,
    RPC_FUNC_PKEY_OPEN,
    RPC_FUNC_PKEY_GENERATE_RSA,
    RPC_FUNC_PKEY_GENERATE_EC,
    RPC_FUNC_PKEY_CLOSE,
    RPC_FUNC_PKEY_DELETE,
    RPC_FUNC_PKEY_GET_KEY_TYPE,
    RPC_FUNC_PKEY_GET_KEY_FLAGS,
    RPC_FUNC_PKEY_GET_PUBLIC_KEY_LEN,
    RPC_FUNC_PKEY_GET_PUBLIC_KEY,
    RPC_FUNC_PKEY_SIGN,
    RPC_FUNC_PKEY_VERIFY,
    RPC_FUNC_PKEY_MATCH,
    RPC_FUNC_PKEY_GET_KEY_CURVE,
    RPC_FUNC_PKEY_SET_ATTRIBUTES,
    RPC_FUNC_PKEY_GET_ATTRIBUTES,
    RPC_FUNC_PKEY_EXPORT,
    RPC_FUNC_PKEY_IMPORT,
''')

class HALDigestAlgorithm(Enum): pass

HALDigestAlgorithm.define('''
    HAL_DIGEST_ALGORITHM_NONE,
    HAL_DIGEST_ALGORITHM_SHA1,
    HAL_DIGEST_ALGORITHM_SHA224,
    HAL_DIGEST_ALGORITHM_SHA256,
    HAL_DIGEST_ALGORITHM_SHA512_224,
    HAL_DIGEST_ALGORITHM_SHA512_256,
    HAL_DIGEST_ALGORITHM_SHA384,
    HAL_DIGEST_ALGORITHM_SHA512
''')

class HALKeyType(Enum): pass

HALKeyType.define('''
    HAL_KEY_TYPE_NONE,
    HAL_KEY_TYPE_RSA_PRIVATE,
    HAL_KEY_TYPE_RSA_PUBLIC,
    HAL_KEY_TYPE_EC_PRIVATE,
    HAL_KEY_TYPE_EC_PUBLIC
''')

class HALCurve(Enum): pass

HALCurve.define('''
    HAL_CURVE_NONE,
    HAL_CURVE_P256,
    HAL_CURVE_P384,
    HAL_CURVE_P521
''')

class HALUser(Enum): pass

HALUser.define('''
    HAL_USER_NONE,
    HAL_USER_NORMAL,
    HAL_USER_SO,
    HAL_USER_WHEEL
''')

HAL_KEY_FLAG_USAGE_DIGITALSIGNATURE     = (1 << 0)
HAL_KEY_FLAG_USAGE_KEYENCIPHERMENT      = (1 << 1)
HAL_KEY_FLAG_USAGE_DATAENCIPHERMENT     = (1 << 2)
HAL_KEY_FLAG_TOKEN                      = (1 << 3)
HAL_KEY_FLAG_PUBLIC                     = (1 << 4)
HAL_KEY_FLAG_EXPORTABLE                 = (1 << 5)

HAL_PKEY_ATTRIBUTE_NIL                  = (0xFFFFFFFF)


class UUID(uuid.UUID):

    def xdr_packer(self, packer):
        packer.pack_bytes(self.bytes)


def cached_property(func):

    attr_name = "_" + func.__name__

    def wrapped(self):
        try:
            value = getattr(self, attr_name)
        except AttributeError:
            value = func(self)
            setattr(self, attr_name, value)
        return value

    wrapped.__name__ = func.__name__

    return property(wrapped)


class Handle(object):

    def __int__(self):
        return self.handle

    def __cmp__(self, other):
        return cmp(self.handle, int(other))

    def xdr_packer(self, packer):
        packer.pack_uint(self.handle)


class Digest(Handle):

    def __init__(self, hsm, handle, algorithm):
        self.hsm       = hsm
        self.handle    = handle
        self.algorithm = algorithm

    def update(self, data):
        self.hsm.hash_update(self, data)

    def finalize(self, length = None):
        return self.hsm.hash_finalize(self, length or self.digest_length)

    @cached_property
    def algorithm_id(self):
        return self.hsm.hash_get_digest_algorithm_id(self.algorithm)

    @cached_property
    def digest_length(self):
        return self.hsm.hash_get_digest_length(self.algorithm)


class LocalDigest(object):
    """
    Implements same interface as Digest class, but using PyCrypto, to
    support mixed-mode PKey operations.  This only supports algorithms
    that PyCrypto supports, so no SHA512/224 or SHA512/256, sorry.
    """

    def __init__(self, hsm, handle, algorithm, key):
        from Crypto.Hash import HMAC, SHA, SHA224, SHA256, SHA384, SHA512
        self.hsm       = hsm
        self.handle    = handle
        self.algorithm = algorithm
        try:
            h = self._algorithms[algorithm]
        except AttributeError:
            self._algorithms = {
                HAL_DIGEST_ALGORITHM_SHA1   : SHA.SHA1Hash,
                HAL_DIGEST_ALGORITHM_SHA224 : SHA224.SHA224Hash,
                HAL_DIGEST_ALGORITHM_SHA256 : SHA256.SHA256Hash,
                HAL_DIGEST_ALGORITHM_SHA384 : SHA384.SHA384Hash,
                HAL_DIGEST_ALGORITHM_SHA512 : SHA512.SHA512Hash
            }
            h = self._algorithms[algorithm]
        self.digest_length = h.digest_size
        self.algorithm_id  = chr(0x30) + chr(2 + len(h.oid)) + h.oid
        self._context = HMAC.HMAC(key = key, digestmod = h) if key else h()

    def update(self, data):
        self._context.update(data)

    def finalize(self, length = None):
        return self._context.digest()

    def finalize_padded(self, pkey):
        if pkey.key_type not in (HAL_KEY_TYPE_RSA_PRIVATE, HAL_KEY_TYPE_RSA_PUBLIC):
            return self.finalize()
        # PKCS #1.5 requires the digest to be wrapped up in an ASN.1 DigestInfo object.
        from Crypto.Util.asn1 import DerSequence, DerNull, DerOctetString
        return DerSequence([DerSequence([self._context.oid, DerNull().encode()]).encode(),
                            DerOctetString(self.finalize()).encode()]).encode()


class PKey(Handle):

    def __init__(self, hsm, handle, uuid):
        self.hsm     = hsm
        self.handle  = handle
        self.uuid    = uuid
        self.deleted = False

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        if not self.deleted:
            self.close()

    def close(self):
        self.hsm.pkey_close(self)

    def delete(self):
        self.hsm.pkey_delete(self)
        self.deleted = True

    @cached_property
    def key_type(self):
        return self.hsm.pkey_get_key_type(self)

    @cached_property
    def key_curve(self):
        return self.hsm.pkey_get_key_curve(self)

    @cached_property
    def key_flags(self):
        return self.hsm.pkey_get_key_flags(self)

    @cached_property
    def public_key_len(self):
        return self.hsm.pkey_get_public_key_len(self)

    @cached_property
    def public_key(self):
        return self.hsm.pkey_get_public_key(self, self.public_key_len)

    def sign(self, hash = 0, data = "", length = 1024):
        return self.hsm.pkey_sign(self, hash = hash, data = data, length = length)

    def verify(self, hash = 0, data = "", signature = None):
        self.hsm.pkey_verify(self, hash = hash, data = data, signature = signature)

    def set_attributes(self, attributes = None, **kwargs):
        assert attributes is None or not kwargs
        self.hsm.pkey_set_attributes(self, attributes or kwargs)

    def get_attributes(self, attributes):
        attrs = self.hsm.pkey_get_attributes(self, attributes, 0)
        attrs = dict((k, v) for k, v in attrs.iteritems() if v != HAL_PKEY_ATTRIBUTE_NIL)
        result = dict((a, None) for a in attributes)
        result.update(self.hsm.pkey_get_attributes(self, attrs.iterkeys(), sum(attrs.itervalues())))
        return result

    def export_pkey(self, pkey):
        return self.hsm.pkey_export(pkey = pkey, kekek = self, pkcs8_max = 5480, kek_max = 512)

    def import_pkey(self, pkcs8, kek, flags = 0):
        return self.hsm.pkey_import(kekek = self, pkcs8 = pkcs8, kek = kek, flags = flags)

class ContextManagedUnpacker(xdrlib.Unpacker):

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.done()


class HSM(object):

    mixed_mode = False
    debug_io = False

    def _raise_if_error(self, status):
        if status != 0:
            raise HALError.table[status]()

    def __init__(self, sockname = os.getenv("CRYPTECH_RPC_CLIENT_SOCKET_NAME",
                                            "/tmp/.cryptech_muxd.rpc")):
        self.socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.socket.connect(sockname)
        self.sockfile = self.socket.makefile("rb")

    def _send(self, msg):       # Expects an xdrlib.Packer
        msg = slip_encode(msg.get_buffer())
        if self.debug_io:
            logger.debug("send: %s", ":".join("{:02x}".format(ord(c)) for c in msg))
        self.socket.sendall(msg)

    def _recv(self, code):      # Returns a ContextManagedUnpacker
        closed = False
        while True:
            msg = [self.sockfile.read(1)]
            while msg[-1] != SLIP_END:
                if msg[-1] == "":
                    raise HAL_ERROR_RPC_TRANSPORT()
                msg.append(self.sockfile.read(1))
            if self.debug_io:
                logger.debug("recv: %s", ":".join("{:02x}".format(ord(c)) for c in msg))
            msg = slip_decode("".join(msg))
            if not msg:
                continue
            msg = ContextManagedUnpacker("".join(msg))
            if msg.unpack_uint() != code:
                continue
            return msg

    _pack_builtin = (((int, long),        "_pack_uint"),
                     (str,                "_pack_bytes"),
                     ((list, tuple, set), "_pack_array"),
                     (dict,               "_pack_items"))

    def _pack_arg(self, packer, arg):
        if hasattr(arg, "xdr_packer"):
            return arg.xdr_packer(packer)
        for cls, method in self._pack_builtin:
            if isinstance(arg, cls):
                return getattr(self, method)(packer, arg)
        raise RuntimeError("Don't know how to pack {!r} ({!r})".format(arg, type(arg)))

    def _pack_args(self, packer, args):
        for arg in args:
            self._pack_arg(packer, arg)

    def _pack_uint(self, packer, arg):
        packer.pack_uint(arg)

    def _pack_bytes(self, packer, arg):
        packer.pack_bytes(arg)

    def _pack_array(self, packer, arg):
        packer.pack_uint(len(arg))
        self._pack_args(packer, arg)

    def _pack_items(self, packer, arg):
        packer.pack_uint(len(arg))
        for name, value in arg.iteritems():
            self._pack_arg(packer, name)
            self._pack_arg(packer, HAL_PKEY_ATTRIBUTE_NIL if value is None else value)

    def rpc(self, code, *args, **kwargs):
        client = kwargs.get("client", 0)
        packer = xdrlib.Packer()
        packer.pack_uint(code)
        packer.pack_uint(client)
        self._pack_args(packer, args)
        self._send(packer)
        unpacker = self._recv(code)
        client = unpacker.unpack_uint()
        self._raise_if_error(unpacker.unpack_uint())
        return unpacker

    def get_version(self):
        with self.rpc(RPC_FUNC_GET_VERSION) as r:
            return r.unpack_uint()

    def get_random(self, n):
        with self.rpc(RPC_FUNC_GET_RANDOM, n) as r:
            return r.unpack_bytes()

    def set_pin(self, user, pin, client = 0):
        with self.rpc(RPC_FUNC_SET_PIN, user, pin, client = client):
            return

    def login(self, user, pin, client = 0):
        with self.rpc(RPC_FUNC_LOGIN, user, pin, client = client):
            return

    def logout(self, client = 0):
        with self.rpc(RPC_FUNC_LOGOUT, client = client):
            return

    def logout_all(self):
        with self.rpc(RPC_FUNC_LOGOUT_ALL):
            return

    def is_logged_in(self, user, client = 0):
        with self.rpc(RPC_FUNC_IS_LOGGED_IN, user, client = client):
            return

    def hash_get_digest_length(self, alg):
        with self.rpc(RPC_FUNC_HASH_GET_DIGEST_LEN, alg) as r:
            return r.unpack_uint()

    def hash_get_digest_algorithm_id(self, alg, max_len = 256):
        with self.rpc(RPC_FUNC_HASH_GET_DIGEST_ALGORITHM_ID, alg, max_len) as r:
            return r.unpack_bytes()

    def hash_get_algorithm(self, handle):
        with self.rpc(RPC_FUNC_HASH_GET_ALGORITHM, handle) as r:
            return HALDigestAlgorithm.index[r.unpack_uint()]

    def hash_initialize(self, alg, key = None, client = 0, session = 0, mixed_mode = None):
        if key is None:
            key = ""
        if mixed_mode is None:
            mixed_mode = self.mixed_mode
        if mixed_mode:
            return LocalDigest(self, 0, alg, key)
        else:
            with self.rpc(RPC_FUNC_HASH_INITIALIZE, session, alg, key, client = client) as r:
                return Digest(self, r.unpack_uint(), alg)

    def hash_update(self, handle, data):
        with self.rpc(RPC_FUNC_HASH_UPDATE, handle, data):
            return

    def hash_finalize(self, handle, length = None):
        if length is None:
            length = self.hash_get_digest_length(self.hash_get_algorithm(handle))
        with self.rpc(RPC_FUNC_HASH_FINALIZE, handle, length) as r:
            return r.unpack_bytes()

    def pkey_load(self, der, flags = 0, client = 0, session = 0):
        with self.rpc(RPC_FUNC_PKEY_LOAD, session, der, flags, client = client) as r:
            pkey = PKey(self, r.unpack_uint(), UUID(bytes = r.unpack_bytes()))
            logger.debug("Loaded pkey %s", pkey.uuid)
            return pkey

    def pkey_open(self, uuid, client = 0, session = 0):
        with self.rpc(RPC_FUNC_PKEY_OPEN, session, uuid, client = client) as r:
            pkey = PKey(self, r.unpack_uint(), uuid)
            logger.debug("Opened pkey %s", pkey.uuid)
            return pkey

    def pkey_generate_rsa(self, keylen, flags = 0, exponent = "\x01\x00\x01", client = 0, session = 0):
        with self.rpc(RPC_FUNC_PKEY_GENERATE_RSA, session, keylen, exponent, flags, client = client) as r:
            pkey = PKey(self, r.unpack_uint(), UUID(bytes = r.unpack_bytes()))
            logger.debug("Generated RSA pkey %s", pkey.uuid)
            return pkey

    def pkey_generate_ec(self, curve, flags = 0, client = 0, session = 0):
        with self.rpc(RPC_FUNC_PKEY_GENERATE_EC, session, curve, flags, client = client) as r:
            pkey = PKey(self, r.unpack_uint(), UUID(bytes = r.unpack_bytes()))
            logger.debug("Generated EC pkey %s", pkey.uuid)
            return pkey

    def pkey_close(self, pkey):
        try:
            logger.debug("Closing pkey %s", pkey.uuid)
        except AttributeError:
            pass
        with self.rpc(RPC_FUNC_PKEY_CLOSE, pkey):
            return

    def pkey_delete(self, pkey):
        try:
            logger.debug("Deleting pkey %s", pkey.uuid)
        except AttributeError:
            pass
        with self.rpc(RPC_FUNC_PKEY_DELETE, pkey):
            return

    def pkey_get_key_type(self, pkey):
        with self.rpc(RPC_FUNC_PKEY_GET_KEY_TYPE, pkey) as r:
            return HALKeyType.index[r.unpack_uint()]

    def pkey_get_key_curve(self, pkey):
        with self.rpc(RPC_FUNC_PKEY_GET_KEY_CURVE, pkey) as r:
            return HALCurve.index[r.unpack_uint()]

    def pkey_get_key_flags(self, pkey):
        with self.rpc(RPC_FUNC_PKEY_GET_KEY_FLAGS, pkey) as r:
            return r.unpack_uint()

    def pkey_get_public_key_len(self, pkey):
        with self.rpc(RPC_FUNC_PKEY_GET_PUBLIC_KEY_LEN, pkey) as r:
            return r.unpack_uint()

    def pkey_get_public_key(self, pkey, length = None):
        if length is None:
            length = self.pkey_get_public_key_len(pkey)
        with self.rpc(RPC_FUNC_PKEY_GET_PUBLIC_KEY, pkey, length) as r:
            return r.unpack_bytes()

    def pkey_sign(self, pkey, hash = 0, data = "", length = 1024):
        assert not hash or not data
        if isinstance(hash, LocalDigest):
            hash, data = 0, hash.finalize_padded(pkey)
        with self.rpc(RPC_FUNC_PKEY_SIGN, pkey, hash, data, length) as r:
            return r.unpack_bytes()

    def pkey_verify(self, pkey, hash = 0, data = "", signature = None):
        assert not hash or not data
        if isinstance(hash, LocalDigest):
            hash, data = 0, hash.finalize_padded(pkey)
        with self.rpc(RPC_FUNC_PKEY_VERIFY, pkey, hash, data, signature):
            return

    def pkey_match(self, type = 0, curve = 0, mask = 0, flags = 0,
                   attributes = {}, length = 64, client = 0, session = 0):
        u = UUID(int = 0)
        n = length
        s = 0
        while n == length:
            with self.rpc(RPC_FUNC_PKEY_MATCH, session, type, curve, mask, flags,
                          attributes, s, length, u, client = client) as r:
                s = r.unpack_uint()
                n = r.unpack_uint()
                for i in xrange(n):
                    u = UUID(bytes = r.unpack_bytes())
                    yield u

    def pkey_set_attributes(self, pkey, attributes):
        with self.rpc(RPC_FUNC_PKEY_SET_ATTRIBUTES, pkey, attributes):
            return

    def pkey_get_attributes(self, pkey, attributes, attributes_buffer_len = 2048):
        attributes = tuple(attributes)
        with self.rpc(RPC_FUNC_PKEY_GET_ATTRIBUTES, pkey, attributes, attributes_buffer_len) as r:
            n = r.unpack_uint()
            if n != len(attributes):
                raise HAL_ERROR_RPC_PROTOCOL_ERROR
            if attributes_buffer_len > 0:
                return dict((r.unpack_uint(), r.unpack_bytes()) for i in xrange(n))
            else:
                return dict((r.unpack_uint(), r.unpack_uint()) for i in xrange(n))

    def pkey_export(self, pkey, kekek, pkcs8_max = 2560, kek_max = 512):
        with self.rpc(RPC_FUNC_PKEY_EXPORT, pkey, kekek, pkcs8_max, kek_max) as r:
            pkcs8, kek = r.unpack_bytes(), r.unpack_bytes()
            logger.debug("Exported pkey %s", pkey.uuid)
            return pkcs8, kek

    def pkey_import(self, kekek, pkcs8, kek, flags = 0, client = 0, session = 0):
        with self.rpc(RPC_FUNC_PKEY_IMPORT, session, kekek, pkcs8, kek, flags, client = client) as r:
            pkey = PKey(self, r.unpack_uint(), UUID(bytes = r.unpack_bytes()))
            logger.debug("Imported pkey %s", pkey.uuid)
            return pkey
