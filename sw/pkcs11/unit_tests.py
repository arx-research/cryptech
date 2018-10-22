#!/usr/bin/env python

"""
PKCS #11 unit tests, using cryptech.py11 and the Python unit_test framework.
"""

import unittest
import datetime
import sys

from cryptech.py11 import *
from cryptech.py11 import default_so_name as libpkcs11_default
from cryptech.py11.mutex import MutexDB

try:
    from Crypto.Util.number     import inverse
    from Crypto.PublicKey       import RSA
    from Crypto.Signature       import PKCS1_v1_5
    from Crypto.Hash            import SHA256
    pycrypto_loaded = True
except ImportError:
    pycrypto_loaded = False


def log(msg):
    if not args.quiet:
        sys.stderr.write(msg)
        sys.stderr.write("\n")


def main():
    from sys import argv
    global args
    args = parse_arguments(argv[1:])
    argv = argv[:1] + args.only_test
    unittest.main(verbosity = 1 if args.quiet else 2, argv = argv, catchbreak = True, testRunner = TextTestRunner)

def parse_arguments(argv = ()):
    from argparse import ArgumentParser, ArgumentDefaultsHelpFormatter
    parser = ArgumentParser(description = __doc__, formatter_class = ArgumentDefaultsHelpFormatter)
    parser.add_argument("--quiet",      action = "store_true",                          help = "suppress chatter")
    parser.add_argument("--so-pin",     default = "fnord",                              help = "security officer PIN")
    parser.add_argument("--user-pin",   default = "fnord",                              help = "user PIN")
    parser.add_argument("--slot",       default = 0, type = int,                        help = "slot number")
    parser.add_argument("--libpkcs11",  default = libpkcs11_default,                    help = "PKCS #11 library")
    parser.add_argument("--all-tests",  action = "store_true",                          help = "enable tests usually skipped")
    parser.add_argument("--only-test",  default = [], nargs = "+",                      help = "only run tests named here")
    return parser.parse_args(argv)

args = parse_arguments()
p11  = None

def setUpModule():
    global p11

    log("Loading PKCS #11 library {}".format(args.libpkcs11))
    p11 = PKCS11(args.libpkcs11)

    log("Setup complete")


# Subclass a few bits of unittest to add timing reports for individual tests.

class TestCase(unittest.TestCase):

    def setUp(self):
        super(TestCase, self).setUp()
        self.startTime = datetime.datetime.now()

    def tearDown(self):
        self.endTime = datetime.datetime.now()
        super(TestCase, self).tearDown()

class TextTestResult(unittest.TextTestResult):

    def addSuccess(self, test):
        if self.showAll and hasattr(test, "startTime") and hasattr(test, "endTime"):
            self.stream.write("runtime {} ... ".format(test.endTime - test.startTime))
            self.stream.flush()
        super(TextTestResult, self).addSuccess(test)

class TextTestRunner(unittest.TextTestRunner):
    resultclass = TextTestResult


class TestInit(TestCase):
    """
    Test all the flavors of C_Initialize().
    """

    def test_mutex_none(self):
        "Test whether C_Initialize() works in the default case"
        p11.C_Initialize()

    def test_mutex_os(self):
        "Test whether C_Initialize() works with (just) system mutexes"
        p11.C_Initialize(CKF_OS_LOCKING_OK)

    def test_mutex_user(self):
        "Test whether C_Initialize() works with (just) user mutexes"
        mdb = MutexDB()
        p11.C_Initialize(0, mdb.create, mdb.destroy, mdb.lock, mdb.unlock)

    def test_mutex_both(self):
        "Test whether C_Initialize() works with both system and user mutexes"
        mdb = MutexDB()
        p11.C_Initialize(CKF_OS_LOCKING_OK, mdb.create, mdb.destroy, mdb.lock, mdb.unlock)

    def tearDown(self):
        super(TestInit, self).tearDown()
        p11.C_Finalize()


class TestDevice(TestCase):
    """
    Test basic device stuff like C_GetSlotList(), C_OpenSession(), and C_Login().
    """

    @classmethod
    def setUpClass(cls):
        p11.C_Initialize()

    @classmethod
    def tearDownClass(cls):
        p11.C_Finalize()

    def tearDown(self):
        super(TestDevice, self).tearDown()
        p11.C_CloseAllSessions(args.slot)

    def test_getSlots(self):
        "Test C_GetSlots()"
        self.assertEqual(p11.C_GetSlotList(), (args.slot,))

    def test_getTokenInfo(self):
        "Test C_GetTokenInfo()"
        token_info = p11.C_GetTokenInfo(args.slot)
        self.assertIsInstance(token_info, CK_TOKEN_INFO)
        self.assertEqual(token_info.label.rstrip(), "Cryptech Token")

    def test_sessions_serial(self):
        "Test C_OpenSession() for useful (serial) cases"
        rw_session = p11.C_OpenSession(args.slot, CKF_RW_SESSION | CKF_SERIAL_SESSION)
        ro_session = p11.C_OpenSession(args.slot, CKF_SERIAL_SESSION)

    def test_sessions_parallel(self):
        "Test that C_OpenSession() rejects forbidden (parallel) cases"
        # Cooked API doesn't allow user to make this mistake, must use raw API to test
        from ctypes import byref
        handle = CK_SESSION_HANDLE()
        notify = CK_NOTIFY()
        with self.assertRaises(CKR_SESSION_PARALLEL_NOT_SUPPORTED):
            p11.so.C_OpenSession(args.slot, CKF_RW_SESSION, None, notify, byref(handle))
        with self.assertRaises(CKR_SESSION_PARALLEL_NOT_SUPPORTED):
            p11.so.C_OpenSession(args.slot, 0,              None, notify, byref(handle))

    def test_login_user(self):
        "Test C_Login() with user PIN"
        rw_session = p11.C_OpenSession(args.slot, CKF_RW_SESSION | CKF_SERIAL_SESSION)
        ro_session = p11.C_OpenSession(args.slot, CKF_SERIAL_SESSION)
        p11.C_Login(ro_session, CKU_USER, args.user_pin)
        p11.C_Logout(ro_session)

    def test_login_so(self):
        "Test C_login with SO PIN"
        rw_session = p11.C_OpenSession(args.slot, CKF_RW_SESSION | CKF_SERIAL_SESSION)
        ro_session = p11.C_OpenSession(args.slot, CKF_SERIAL_SESSION)
        self.assertRaises(CKR_SESSION_READ_ONLY_EXISTS, p11.C_Login, ro_session, CKU_SO, args.so_pin)
        p11.C_CloseSession(ro_session)
        p11.C_Login(rw_session, CKU_SO, args.so_pin)
        self.assertRaises(CKR_SESSION_READ_WRITE_SO_EXISTS, p11.C_OpenSession, args.slot, CKF_SERIAL_SESSION)
        p11.C_Logout(rw_session)

    def test_random(self):
        "Test that C_GenerateRandom() returns, um, something"
        # Testing that what this produces really is random seems beyond
        # the scope of a unit test, but feel free to send code.
        session = p11.C_OpenSession(args.slot)
        n = 17
        random = p11.C_GenerateRandom(session, n)
        self.assertIsInstance(random, str)
        self.assertEqual(len(random), n)

    def test_findObjects_operation_state(self):
        "Test C_FindObjects*() CKR_OPERATION_* behavior"
        session = p11.C_OpenSession(args.slot)

        with self.assertRaises(CKR_OPERATION_NOT_INITIALIZED):
            for handle in p11.C_FindObjects(session):
                self.assertIsInstance(handle, (int, long))

        with self.assertRaises(CKR_OPERATION_NOT_INITIALIZED):
            p11.C_FindObjectsFinal(session)

        p11.C_FindObjectsInit(session, CKA_CLASS = CKO_PUBLIC_KEY)

        with self.assertRaises(CKR_OPERATION_ACTIVE):
            p11.C_FindObjectsInit(session, CKA_CLASS = CKO_PRIVATE_KEY)

        for handle in p11.C_FindObjects(session):
            self.assertIsInstance(handle, (int, long))

        p11.C_FindObjectsFinal(session)


class TestKeys(TestCase):
    """
    Tests involving keys.
    """

    oid_p256 = "".join(chr(i) for i in (0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07))
    oid_p384 = "".join(chr(i) for i in (0x06, 0x05, 0x2b, 0x81, 0x04, 0x00, 0x22))
    oid_p521 = "".join(chr(i) for i in (0x06, 0x05, 0x2b, 0x81, 0x04, 0x00, 0x23))

    @classmethod
    def setUpClass(cls):
        p11.C_Initialize()

    @classmethod
    def tearDownClass(cls):
        p11.C_Finalize()

    def setUp(self):
        self.handles = set()
        self.session = p11.C_OpenSession(args.slot)
        p11.C_Login(self.session, CKU_USER, args.user_pin)
        super(TestKeys, self).setUp()

    def tearDown(self):
        super(TestKeys, self).tearDown()
        for handle in self.handles:
            p11.C_DestroyObject(self.session, handle)
        p11.C_CloseAllSessions(args.slot)
        del self.session

    def assertIsKeypair(self, *keypair):
        public_handle, private_handle = keypair[0] if isinstance(keypair[0], tuple) else keypair
        self.handles.add(public_handle)
        self.handles.add(private_handle)
        self.assertEqual(p11.C_GetAttributeValue(self.session, public_handle,  CKA_CLASS), {CKA_CLASS: CKO_PUBLIC_KEY})
        self.assertEqual(p11.C_GetAttributeValue(self.session, private_handle, CKA_CLASS), {CKA_CLASS: CKO_PRIVATE_KEY})

    def test_keygen_token_vs_session(self):
        "Test C_GenerateKeypair() CKA_TOKEN restrictions"

        self.assertIsKeypair(
          p11.C_GenerateKeyPair(self.session, CKM_EC_KEY_PAIR_GEN, CKA_TOKEN = False,
                                CKA_ID = "EC-P256", CKA_EC_PARAMS = self.oid_p256,
                                CKA_SIGN = True, CKA_VERIFY = True))

        self.assertIsKeypair(
          p11.C_GenerateKeyPair(self.session, CKM_EC_KEY_PAIR_GEN, CKA_TOKEN = True,
                                CKA_ID = "EC-P256", CKA_EC_PARAMS = self.oid_p256,
                                CKA_SIGN = True, CKA_VERIFY = True))

        self.assertIsKeypair(
            p11.C_GenerateKeyPair(self.session, CKM_EC_KEY_PAIR_GEN,
                                  public_CKA_TOKEN = False, private_CKA_TOKEN = True,
                                  CKA_ID = "EC-P256", CKA_EC_PARAMS = self.oid_p256,
                                  CKA_SIGN = True, CKA_VERIFY = True))

        self.assertIsKeypair(
          p11.C_GenerateKeyPair(self.session, CKM_EC_KEY_PAIR_GEN,
                                public_CKA_TOKEN = True, private_CKA_TOKEN = False,
                                CKA_ID = "EC-P256", CKA_EC_PARAMS = self.oid_p256,
                                CKA_SIGN = True, CKA_VERIFY = True))

    def test_gen_sign_verify_ecdsa_p256_sha256(self):
        "Generate/sign/verify with ECDSA-P256-SHA-256"
        public_key, private_key = p11.C_GenerateKeyPair(self.session, CKM_EC_KEY_PAIR_GEN,
                                                        CKA_ID = "EC-P256", CKA_EC_PARAMS = self.oid_p256,
                                                        CKA_SIGN = True, CKA_VERIFY = True, CKA_TOKEN = True)
        self.assertIsKeypair(public_key, private_key)
        hamster = "Your mother was a hamster"
        p11.C_SignInit(self.session, CKM_ECDSA_SHA256, private_key)
        sig = p11.C_Sign(self.session, hamster)
        self.assertIsInstance(sig, str)
        p11.C_VerifyInit(self.session, CKM_ECDSA_SHA256, public_key)
        p11.C_Verify(self.session, hamster, sig)

    def test_gen_sign_verify_ecdsa_p384_sha384(self):
        "Generate/sign/verify with ECDSA-P384-SHA-384"
        #if not args.all_tests: self.skipTest("SHA-384 not available in current build")
        public_key, private_key = p11.C_GenerateKeyPair(self.session, CKM_EC_KEY_PAIR_GEN,
                                                        CKA_ID = "EC-P384", CKA_EC_PARAMS = self.oid_p384,
                                                        CKA_SIGN = True, CKA_VERIFY = True, CKA_TOKEN = True)
        self.assertIsKeypair(public_key, private_key)
        hamster = "Your mother was a hamster"
        p11.C_SignInit(self.session, CKM_ECDSA_SHA384, private_key)
        sig = p11.C_Sign(self.session, hamster)
        self.assertIsInstance(sig, str)
        p11.C_VerifyInit(self.session, CKM_ECDSA_SHA384, public_key)
        p11.C_Verify(self.session, hamster, sig)

    def test_gen_sign_verify_ecdsa_p521_sha512(self):
        "Generate/sign/verify with ECDSA-P521-SHA-512"
        #if not args.all_tests: self.skipTest("SHA-512 not available in current build")
        public_key, private_key = p11.C_GenerateKeyPair(self.session, CKM_EC_KEY_PAIR_GEN,
                                                        CKA_ID = "EC-P521", CKA_EC_PARAMS = self.oid_p521,
                                                        CKA_SIGN = True, CKA_VERIFY = True, CKA_TOKEN = True)
        self.assertIsKeypair(public_key, private_key)
        hamster = "Your mother was a hamster"
        p11.C_SignInit(self.session, CKM_ECDSA_SHA512, private_key)
        sig = p11.C_Sign(self.session, hamster)
        self.assertIsInstance(sig, str)
        p11.C_VerifyInit(self.session, CKM_ECDSA_SHA512, public_key)
        p11.C_Verify(self.session, hamster, sig)

    def test_gen_sign_verify_rsa_1024(self):
        "Generate/sign/verify with RSA-1024-SHA-512"
        "RSA 1024-bit generate/sign/verify test"
        public_key, private_key = p11.C_GenerateKeyPair(
            self.session, CKM_RSA_PKCS_KEY_PAIR_GEN, CKA_MODULUS_BITS = 1024,
            CKA_ID = "RSA-1024", CKA_SIGN = True, CKA_VERIFY = True, CKA_TOKEN = True)
        self.assertIsKeypair(public_key, private_key)
        hamster = "Your mother was a hamster"
        p11.C_SignInit(self.session, CKM_SHA512_RSA_PKCS, private_key)
        sig = p11.C_Sign(self.session, hamster)
        self.assertIsInstance(sig, str)
        p11.C_VerifyInit(self.session, CKM_SHA512_RSA_PKCS, public_key)
        p11.C_Verify(self.session, hamster, sig)

    def test_gen_sign_verify_rsa_2048(self):
        "Generate/sign/verify with RSA-2048-SHA-512"
        #if not args.all_tests: self.skipTest("RSA key generation is still painfully slow")
        public_key, private_key = p11.C_GenerateKeyPair(
            self.session, CKM_RSA_PKCS_KEY_PAIR_GEN, CKA_MODULUS_BITS = 2048,
            CKA_ID = "RSA-2048", CKA_SIGN = True, CKA_VERIFY = True, CKA_TOKEN = True)
        self.assertIsKeypair(public_key, private_key)
        hamster = "Your mother was a hamster"
        p11.C_SignInit(self.session, CKM_SHA512_RSA_PKCS, private_key)
        sig = p11.C_Sign(self.session, hamster)
        self.assertIsInstance(sig, str)
        p11.C_VerifyInit(self.session, CKM_SHA512_RSA_PKCS, public_key)
        p11.C_Verify(self.session, hamster, sig)

    @staticmethod
    def _build_ecpoint(x, y):
        bytes_per_coordinate = (max(x.bit_length(), y.bit_length()) + 15) / 16
        value = chr(0x04) + ("%0*x%0*x" % (bytes_per_coordinate, x, bytes_per_coordinate, y)).decode("hex")
        if len(value) < 128:
            length = chr(len(value))
        else:
            n = len(value).bit_length()
            length = chr((n + 7) / 8) + ("%0*x" % ((n + 15) / 16, len(value))).decode("hex")
        tag = chr(0x04)
        return tag + length + value

    def test_canned_ecdsa_p256_verify(self):
        "EC-P-256 verification test from Suite B Implementer's Guide to FIPS 186-3"
        Q = self._build_ecpoint(0x8101ece47464a6ead70cf69a6e2bd3d88691a3262d22cba4f7635eaff26680a8,
                                0xd8a12ba61d599235f67d9cb4d58f1783d3ca43e78f0a5abaa624079936c0c3a9)
        H = "7c3e883ddc8bd688f96eac5e9324222c8f30f9d6bb59e9c5f020bd39ba2b8377".decode("hex")
        r = "7214bc9647160bbd39ff2f80533f5dc6ddd70ddf86bb815661e805d5d4e6f27c".decode("hex")
        s = "7d1ff961980f961bdaa3233b6209f4013317d3e3f9e1493592dbeaa1af2bc367".decode("hex")
        handle = p11.C_CreateObject(
          session           = self.session,
          CKA_CLASS         = CKO_PUBLIC_KEY,
          CKA_KEY_TYPE      = CKK_EC,
          CKA_LABEL         = "EC-P-256 test case from \"Suite B Implementer's Guide to FIPS 186-3\"",
          CKA_ID            = "EC-P-256",
          CKA_VERIFY        = True,
          CKA_EC_POINT      = Q,
          CKA_EC_PARAMS     = self.oid_p256)
        self.handles.add(handle)
        p11.C_VerifyInit(self.session, CKM_ECDSA, handle)
        p11.C_Verify(self.session, H, r + s)

    def test_canned_ecdsa_p384_verify(self):
        "EC-P-384 verification test from Suite B Implementer's Guide to FIPS 186-3"
        Q = self._build_ecpoint(0x1fbac8eebd0cbf35640b39efe0808dd774debff20a2a329e91713baf7d7f3c3e81546d883730bee7e48678f857b02ca0,
                                0xeb213103bd68ce343365a8a4c3d4555fa385f5330203bdd76ffad1f3affb95751c132007e1b240353cb0a4cf1693bdf9)
        H = "b9210c9d7e20897ab86597266a9d5077e8db1b06f7220ed6ee75bd8b45db37891f8ba5550304004159f4453dc5b3f5a1".decode("hex")
        r = "a0c27ec893092dea1e1bd2ccfed3cf945c8134ed0c9f81311a0f4a05942db8dbed8dd59f267471d5462aa14fe72de856".decode("hex")
        s = "20ab3f45b74f10b6e11f96a2c8eb694d206b9dda86d3c7e331c26b22c987b7537726577667adadf168ebbe803794a402".decode("hex")
        handle = p11.C_CreateObject(
          session           = self.session,
          CKA_CLASS         = CKO_PUBLIC_KEY,
          CKA_KEY_TYPE      = CKK_EC,
          CKA_LABEL         = "EC-P-384 test case from \"Suite B Implementer's Guide to FIPS 186-3\"",
          CKA_ID            = "EC-P-384",
          CKA_VERIFY        = True,
          CKA_EC_POINT      = Q,
          CKA_EC_PARAMS     = self.oid_p384)
        self.handles.add(handle)
        p11.C_VerifyInit(self.session, CKM_ECDSA, handle)
        p11.C_Verify(self.session, H, r + s)

    def test_gen_sign_verify_reload_ecdsa_p256_sha256(self):
        "Generate/sign/verify/destroy/reload/verify with ECDSA-P256-SHA-256"
        public_key, private_key = p11.C_GenerateKeyPair(self.session, CKM_EC_KEY_PAIR_GEN,
                                                        public_CKA_TOKEN = False, private_CKA_TOKEN = True,
                                                        CKA_ID = "EC-P256", CKA_EC_PARAMS = self.oid_p256,
                                                        CKA_SIGN = True, CKA_VERIFY = True)
        self.assertIsKeypair(public_key, private_key)
        hamster = "Your mother was a hamster"
        p11.C_SignInit(self.session, CKM_ECDSA_SHA256, private_key)
        sig = p11.C_Sign(self.session, hamster)
        self.assertIsInstance(sig, str)
        p11.C_VerifyInit(self.session, CKM_ECDSA_SHA256, public_key)
        p11.C_Verify(self.session, hamster, sig)

        a = p11.C_GetAttributeValue(self.session, public_key,
                                    CKA_CLASS, CKA_KEY_TYPE, CKA_VERIFY, CKA_TOKEN,
                                    CKA_EC_PARAMS, CKA_EC_POINT)

        while self.handles:
            p11.C_DestroyObject(self.session, self.handles.pop())
        p11.C_CloseAllSessions(args.slot)
        self.session = p11.C_OpenSession(args.slot)
        p11.C_Login(self.session, CKU_USER, args.user_pin)

        o = p11.C_CreateObject(self.session, a)
        self.handles.add(o)
        p11.C_VerifyInit(self.session, CKM_ECDSA_SHA256, o)
        p11.C_Verify(self.session, hamster, sig)

    def _extract_rsa_public_key(self, handle):
        a = p11.C_GetAttributeValue(self.session, handle, CKA_MODULUS, CKA_PUBLIC_EXPONENT)
        return RSA.construct((a[CKA_MODULUS], a[CKA_PUBLIC_EXPONENT]))

    def assertRawRSASignatureMatches(self, handle, plain, sig):
        pubkey = self._extract_rsa_public_key(handle)
        result = pubkey.encrypt(sig, 0)[0]
        prefix = "\x00\x01" if False else "\x01" # XXX
        expect = prefix + "\xff" * (len(result) - len(plain) - len(prefix) - 1) + "\x00" + plain
        self.assertEqual(result, expect)

    def test_gen_sign_verify_tralala_rsa_1024(self):
        "Generate/sign/verify with RSA-1024 (no hashing, message to be signed not a hash at all)"
        tralala = "tralala-en-hopsasa"
        public_key, private_key = p11.C_GenerateKeyPair(
            self.session, CKM_RSA_PKCS_KEY_PAIR_GEN, CKA_MODULUS_BITS = 1024,
            CKA_ID = "RSA-1024", CKA_SIGN = True, CKA_VERIFY = True, CKA_TOKEN = True)
        self.assertIsKeypair(public_key, private_key)
        p11.C_SignInit(self.session, CKM_RSA_PKCS, private_key)
        sig = p11.C_Sign(self.session, tralala)
        self.assertIsInstance(sig, str)
        self.assertRawRSASignatureMatches(public_key, tralala, sig)
        p11.C_VerifyInit(self.session, CKM_RSA_PKCS, public_key)
        p11.C_Verify(self.session, tralala, sig)

    def test_gen_sign_verify_tralala_rsa_3416(self):
        "Generate/sign/verify with RSA-3416 (no hashing, message to be signed not a hash at all)"
        if not args.all_tests:
            self.skipTest("Key length not a multiple of 32, so expected to fail (very slowly)")
        tralala = "tralala-en-hopsasa"
        public_key, private_key = p11.C_GenerateKeyPair(
            self.session, CKM_RSA_PKCS_KEY_PAIR_GEN, CKA_MODULUS_BITS = 3416,
            CKA_ID = "RSA-3416", CKA_SIGN = True, CKA_VERIFY = True, CKA_TOKEN = True)
        self.assertIsKeypair(public_key, private_key)
        p11.C_SignInit(self.session, CKM_RSA_PKCS, private_key)
        sig = p11.C_Sign(self.session, tralala)
        self.assertIsInstance(sig, str)
        self.assertRawRSASignatureMatches(public_key, tralala, sig)
        p11.C_VerifyInit(self.session, CKM_RSA_PKCS, public_key)
        p11.C_Verify(self.session, tralala, sig)

    def _load_rsa_keypair(self, pem, cka_id = None):
        k = RSA.importKey(pem)
        public = dict(
            CKA_TOKEN = True, CKA_CLASS = CKO_PUBLIC_KEY, CKA_KEY_TYPE = CKK_RSA, CKA_VERIFY = True,
            CKA_MODULUS         = k.n,
            CKA_PUBLIC_EXPONENT = k.e)
        private = dict(
            CKA_TOKEN = True, CKA_CLASS = CKO_PRIVATE_KEY, CKA_KEY_TYPE = CKK_RSA, CKA_SIGN = True,
            CKA_MODULUS         = k.n,
            CKA_PUBLIC_EXPONENT = k.e,
            CKA_PRIVATE_EXPONENT= k.d,
            CKA_PRIME_1         = k.p,
            CKA_PRIME_2         = k.q,
            CKA_COEFFICIENT     = inverse(k.q, k.p),
            CKA_EXPONENT_1      = k.d % (k.p - 1),
            CKA_EXPONENT_2      = k.d % (k.q - 1))
        if cka_id is not None:
            public[CKA_ID] = private[CKA_ID] = cka_id
        public_key  = p11.C_CreateObject(self.session, public)
        private_key = p11.C_CreateObject(self.session, private)
        self.assertIsKeypair(public_key, private_key)
        return public_key, private_key

    @unittest.skipUnless(pycrypto_loaded, "requires PyCrypto")
    def test_load_sign_verify_rsa_1024(self):
        "Load/sign/verify with RSA-1024-SHA-512 and externally-supplied key"
        public_key, private_key = self._load_rsa_keypair(rsa_1024_pem, "RSA-1024")
        hamster = "Your mother was a hamster"
        p11.C_SignInit(self.session, CKM_SHA512_RSA_PKCS, private_key)
        sig = p11.C_Sign(self.session, hamster)
        self.assertIsInstance(sig, str)
        p11.C_VerifyInit(self.session, CKM_SHA512_RSA_PKCS, public_key)
        p11.C_Verify(self.session, hamster, sig)

    @unittest.skipUnless(pycrypto_loaded, "requires PyCrypto")
    def test_load_sign_verify_rsa_2048(self):
        "Load/sign/verify with RSA-2048-SHA-512 and externally-supplied key"
        public_key, private_key = self._load_rsa_keypair(rsa_2048_pem, "RSA-2048")
        hamster = "Your mother was a hamster"
        p11.C_SignInit(self.session, CKM_SHA512_RSA_PKCS, private_key)
        sig = p11.C_Sign(self.session, hamster)
        self.assertIsInstance(sig, str)
        p11.C_VerifyInit(self.session, CKM_SHA512_RSA_PKCS, public_key)
        p11.C_Verify(self.session, hamster, sig)

    @unittest.skipUnless(pycrypto_loaded, "requires PyCrypto")
    def test_load_sign_verify_rsa_3416(self):
        "Load/sign/verify with RSA-3416-SHA-512 and externally-supplied key"
        if not args.all_tests:
            self.skipTest("Key length not a multiple of 32, so expected to fail (fairly quickly)")
        public_key, private_key = self._load_rsa_keypair(rsa_3416_pem, "RSA-3416")
        hamster = "Your mother was a hamster"
        p11.C_SignInit(self.session, CKM_SHA512_RSA_PKCS, private_key)
        sig = p11.C_Sign(self.session, hamster)
        self.assertIsInstance(sig, str)
        p11.C_VerifyInit(self.session, CKM_SHA512_RSA_PKCS, public_key)
        p11.C_Verify(self.session, hamster, sig)

    def test_gen_sign_verify_rsa_3416(self):
        "Generate/sign/verify with RSA-3416-SHA-512"
        if not args.all_tests:
            self.skipTest("Key length not a multiple of 32, so expected to fail (very slowly)")
        public_key, private_key = p11.C_GenerateKeyPair(
            self.session, CKM_RSA_PKCS_KEY_PAIR_GEN, CKA_MODULUS_BITS = 3416,
            CKA_ID = "RSA-3416", CKA_SIGN = True, CKA_VERIFY = True, CKA_TOKEN = True)
        self.assertIsKeypair(public_key, private_key)
        hamster = "Your mother was a hamster"
        p11.C_SignInit(self.session, CKM_SHA512_RSA_PKCS, private_key)
        sig = p11.C_Sign(self.session, hamster)
        self.assertIsInstance(sig, str)
        p11.C_VerifyInit(self.session, CKM_SHA512_RSA_PKCS, public_key)
        p11.C_Verify(self.session, hamster, sig)

    def test_gen_rsa_1028(self):
        "Test that C_GenerateKeyPair() rejects key length not multiple of 8 bits"
        with self.assertRaises(CKR_ATTRIBUTE_VALUE_INVALID):
            p11.C_GenerateKeyPair(
                self.session, CKM_RSA_PKCS_KEY_PAIR_GEN, CKA_MODULUS_BITS = 1028,
                CKA_ID = "RSA-1028", CKA_SIGN = True, CKA_VERIFY = True, CKA_TOKEN = True)

    def test_gen_sign_verify_rsa_1032(self):
        "Generate/sign/verify with RSA-1032-SHA-512 (sic)"
        public_key, private_key = p11.C_GenerateKeyPair(
            self.session, CKM_RSA_PKCS_KEY_PAIR_GEN, CKA_MODULUS_BITS = 1032,
            CKA_ID = "RSA-1032", CKA_SIGN = True, CKA_VERIFY = True, CKA_TOKEN = True)
        self.assertIsKeypair(public_key, private_key)
        hamster = "Your mother was a hamster"
        p11.C_SignInit(self.session, CKM_SHA512_RSA_PKCS, private_key)
        sig = p11.C_Sign(self.session, hamster)
        self.assertIsInstance(sig, str)
        p11.C_VerifyInit(self.session, CKM_SHA512_RSA_PKCS, public_key)
        p11.C_Verify(self.session, hamster, sig)

    @unittest.skipUnless(pycrypto_loaded, "requires PyCrypto")
    def test_load_sign_verify_rsa_1024_with_rpki_data(self):
        "Load/sign/verify with RSA-1024-SHA-256, externally-supplied key"
        public_key, private_key = self._load_rsa_keypair(rsa_1024_pem, "RSA-1024")
        tbs = '''
            31 6B 30 1A 06 09 2A 86 48 86 F7 0D 01 09 03 31
            0D 06 0B 2A 86 48 86 F7 0D 01 09 10 01 1A 30 1C
            06 09 2A 86 48 86 F7 0D 01 09 05 31 0F 17 0D 31
            36 30 37 31 36 30 39 35 32 35 37 5A 30 2F 06 09
            2A 86 48 86 F7 0D 01 09 04 31 22 04 20 11 A2 E6
            0F 1F 86 AF 45 25 4D 8F E1 1F C9 EA B3 83 4A 41
            17 C1 42 B7 43 AD 51 5E F5 A2 F8 E3 25
        '''
        tbs = "".join(chr(int(i, 16)) for i in tbs.split())
        p11.C_SignInit(self.session, CKM_SHA256_RSA_PKCS, private_key)
        p11.C_SignUpdate(self.session, tbs)
        sig = p11.C_SignFinal(self.session)
        self.assertIsInstance(sig, str)
        p11.C_VerifyInit(self.session, CKM_SHA256_RSA_PKCS, public_key)
        p11.C_Verify(self.session, tbs, sig)
        verifier = PKCS1_v1_5.new(RSA.importKey(rsa_1024_pem))
        digest = SHA256.new(tbs)
        self.assertTrue(verifier.verify(digest, sig))
        p11.C_SignInit(self.session, CKM_SHA256_RSA_PKCS, private_key)
        self.assertEqual(sig, p11.C_Sign(self.session, tbs))
        p11.C_VerifyInit(self.session, CKM_SHA256_RSA_PKCS, public_key)
        p11.C_VerifyUpdate(self.session, tbs)
        p11.C_VerifyFinal(self.session, sig)

    def _find_objects(self, chunk_size = 10, template = None, **kwargs):
        p11.C_FindObjectsInit(self.session, template, **kwargs)
        for handle in p11.C_FindObjects(self.session, chunk_size):
            self.assertIsInstance(handle, (int, long))
        p11.C_FindObjectsFinal(self.session)

    @unittest.skipUnless(pycrypto_loaded, "requires PyCrypto")
    def test_findObjects(self):
        self._load_rsa_keypair(rsa_1024_pem, "RSA-1024")
        self._load_rsa_keypair(rsa_2048_pem, "RSA-2048")
        self._load_rsa_keypair(rsa_3416_pem, "RSA-3416")
        self._find_objects(chunk_size = 1,   CKA_CLASS = CKO_PUBLIC_KEY)
        self._find_objects(chunk_size = 1,   CKA_CLASS = CKO_PRIVATE_KEY)
        self._find_objects(chunk_size = 10,  CKA_CLASS = CKO_PUBLIC_KEY)
        self._find_objects(chunk_size = 10,  CKA_CLASS = CKO_PRIVATE_KEY)
        self._find_objects(chunk_size = 100, CKA_CLASS = CKO_PUBLIC_KEY)
        self._find_objects(chunk_size = 100, CKA_CLASS = CKO_PRIVATE_KEY)
        self._find_objects(chunk_size = 1)
        self._find_objects(chunk_size = 1)
        self._find_objects(chunk_size = 10)
        self._find_objects(chunk_size = 10)
        self._find_objects(chunk_size = 100)
        self._find_objects(chunk_size = 100)


# Keys for preload tests, here rather than inline because they're
# bulky.  These are in PKCS #8 format, see PyCrypto or the "pkey" and
# "genpkey" commands to OpenSSL's command line tool.

rsa_1024_pem = '''\
-----BEGIN PRIVATE KEY-----
MIICdQIBADANBgkqhkiG9w0BAQEFAASCAl8wggJbAgEAAoGBAIwSaEpCTVJvbd4Z
B1P8H9EgFlZqats7PeBIOlC2Q1zla7wBmNJkX5Jkez8tF3l22Sn99c6c6PuhyhzB
dZtifQbZniKCJEzyby5MXZeSr20rPdrqiB9FX13mmtLN7ii4nLyAYFAQ4R8ZvdH2
dRIWtxwhS7d4AyrWYhJkemIvSApfAgMBAAECgYAmL1Zy+AQwNuRSqawPvynFTuQI
Bta+kTXbEJWlLyrKBlkKVb0djfNn6zCWFmrR2A53nh4Gh0wUXRTGJg8znvPKJPcp
45znc7aGQFmDivvl5m/UkbqET6SB6JyCOCKzYa1Rtn3YFMkf/3MgzrWIhFv+UNH/
I5lSjzJcCrN4mgI+AQJBALcTNa0mOWRXX+6jssbf65Cx6wmHsrrptXiP9gKfwdkx
697EzyvPDL8xwL20O+xBFehj866O/f8nOPP47imOPoECQQDD3gU8wD8MeWLqYcXI
AdERIuuk1VnL36QOzn9NoPF01DLJcrcbN24i5/9tcza3Kdec8fexJTh/PMBvR8Zr
w5jfAkAnFgrXtNl7+suYf4qjuxroAZRUrIwUK+F6pAG5/bG9VVMudIZmrAXkrBKi
beB9SEgNHYnhMtY3q4AVVohChwQBAkAR1I5Jf3691fcJOylUEcZEdxdYhAuOoac/
qdCw8mvIpOCSshy1H5CpINGB1zEt72MvaF+SAr9n5dHmz3Pir4WlAkB/ZccJ5QBH
uBP0/flXdmhG5lC3MTMiiE7Rls/3L2t6S4xVDnQ81RYf7Car53WN7qSVSZnhDGsn
BJpghq2nYUH1
-----END PRIVATE KEY-----
'''

rsa_2048_pem = '''\
-----BEGIN PRIVATE KEY-----
MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQCzvgb90hKxeDJy
zeWz/F4JGZ3Acl1i3url3VPXHyoldyhuNC+8jf4iM7TGBYGLH+sYkBXWu9GD0erl
KBMJMTBO8OdXulSAJh8r1Z8qNPSVNguvNgGQlRDGc7tZJ6gWFlzM2g5flED24bN9
6Ir9O1cZi7xMc0Nzkn9Rms5IwPW8OB4IZZlbFC6Ih9vUSp06Tm3rQ/eQJkhLFbzM
ejc9OH1LSpYtji44ohmy/jPJsmSlzwK5JSchZqbxl/msVw1t/nZS3loqKUMvzn9F
iARLiaIrUKNCmSmL8HqEt2qKt0ESHG0vX07h5W5iHIJOuKhqcX3li8nFcwsOV3AB
RsCRgeppAgMBAAECggEANEeTVQRjN4dUdRv6Me23lEIFJlKdYwKfpBhKKIoCAj+0
XMmFEPzj7CLJ88bqNQMlqFFQaNLcT9Eg12Jelw/dkzhysYuaxGNSMbfCwc4BTd0Y
bO/yaJFS/cXvujDUrQf4GgVapOZENwrS4E5hDuLRpLaGIF5uQhFcQuoaEgM99m6H
TzOIhtu3DjdbfSsmkGVQ7xUVFcvaCrMVoq06dvUH4HpYTKeeqgcVv++XjIe83Nzv
+oN5U2oFOzrYpGGHN6jrekmmbEaxy8UHOySC6Y+UyRrPEy9q1ZgkkC9VCrM7E28/
4PETw8MI7uNoosuFXofctKjtRvC5Sn9zW3dyv9NkAQKBgQDSkdDy4+xN2kA7gdry
4eLKUzKRNSbnoPrCu2TENUR8oTjbJ52LQEbGa9HIKaM33701hJsc34C6adJgoY9c
PoBcJgxOI7N40A/rI3c8krKS5X/ViBY3TzJsP3LaBKDdfioaTDVTjhYD6v2eu7Lt
4eIqa8sVA4PhLSVGRW53ZSYjwQKBgQDahY6cR8WgPseQJTkCbKyfKCSuz6nioZpC
stkpKhJepzE6AKZTLLWvNIPNT/khl40aby5seLNkY3Tb9cYBX2iShGv4cguPMiAl
hb7Uljz19i6YaX74gkTjsYpv44ddLSZp+/FTOl0C0I8WocQpb8B2d4BgvfxgHrHb
KCHnyihQqQKBgQC7PKPixt7hnzdMcrxhCpDiPbaSPgQZJSC1NXJ1sbPzalynKwPA
xefpGgiRBs02qsGRLBfNRcQuflhuSlqyuHTk+4Qnm0FEJSZyfLfS6dLWIjJYikjO
56I7dPPIfyMXsM75UVh9srNKypK4qciCFEBKXk1XoyeKe91QLf77NbsDQQKBgDnY
CLwNs56Lf8AEWmbt5XPr6GntxoabSH5HYXyoClzL3RgBfAWgXCeYuxrqBISD3XIV
5DAKc1IrkY94K4XJf6DpNLt7VNv+5MuJ783ORyzEkej+ZAHcWef74y1jCT386aI8
ctEZLe3Ez1uqToa5cjTpxS3WnKvE9EeTBAabWLihAoGBAKft+PG+j+QhZeHSGxRz
mY6AQHH4z7IU94fmXiT2kTsG8/svhPNfsmVt7UCSJbCaYsrcuR2NonVV8wn/U793
LcnLAV+WObeduMTBCasJw8IniFwFhhfkxOtmlevExE1I3ENwkRlJ7NdROky4pnd5
LGmN2EOOlFijEGxBfw+Wb1rQ
-----END PRIVATE KEY-----
'''

rsa_3416_pem = '''\
-----BEGIN PRIVATE KEY-----
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
BvUE7g/dCNw3ISPEAgVJZUHJ
-----END PRIVATE KEY-----
'''


if __name__ == "__main__":
    main()
