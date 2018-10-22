# Initial test script using py11.  About 2/3 testing PKCS #11, 1/3
# doodles figuring out what else py11 should be automating for us.

from cryptech.py11 import *
from struct import pack, unpack

def show_token(i, t, strip = True):
  print "Token in slot #%d:" % i
  fw = max(len(fn) for fn, ft in t.__class__._fields_)
  for fn, ft in t.__class__._fields_:
    fv = getattr(t, fn)
    fn = "%-*s :" % (fw, fn)
    if isinstance(fv, CK_VERSION):
      print " ", fn, "%d.%d" % (fv.major, fv.minor)
    elif isinstance(fv, (int, long)):
      print " ", fn, fv
    else:
      print " ", fn, repr(str(fv).rstrip(" ") if strip else str(fv))


def genkeypair_templates(name = "foo", **kwargs):
  label_id = ((CKA_LABEL, name), (CKA_ID, name))
  return (
    # Public template
    label_id + tuple(kwargs.iteritems()) +
    ((CKA_VERIFY,             True),
     (CKA_ENCRYPT,            False),
     (CKA_WRAP,               False),
     (CKA_TOKEN,              False)),
    # Private template
    label_id +
    ((CKA_SIGN,               True),
     (CKA_DECRYPT,            False),
     (CKA_UNWRAP,             False),
     (CKA_SENSITIVE,          True),
     (CKA_TOKEN,              False),
     (CKA_PRIVATE,            True),
     (CKA_EXTRACTABLE,        False)))


def show_keys(session, key_class, *attributes):
  p11.C_FindObjectsInit(session, {CKA_CLASS: key_class})
  for o in p11.C_FindObjects(session):
    a = p11.C_GetAttributeValue(session, o, attributes + (CKA_CLASS, CKA_KEY_TYPE, CKA_LABEL, CKA_ID, CKA_TOKEN,
                                                          CKA_PRIVATE, CKA_LOCAL, CKA_KEY_GEN_MECHANISM))
    w = max(len(p11.adb.attribute_name(k)) for k in a)
    print "Object:", o
    for k, v in a.iteritems():
      print " %*s (0x%08x): %r" % (w, p11.adb.attribute_name(k), k, v)
  p11.C_FindObjectsFinal(session)


def build_ecpoint(x, y):
  h = (max(x.bit_length(), y.bit_length()) + 15) / 16
  d = chr(0x04) + ("%0*x%0*x" % (h, x, h, y)).decode("hex")
  if len(d) < 128:
    h = "%c%c" % (0x04, len(d))
  else:
    n = len(d).bit_length()
    h = ("%c%c" % (0x04, (n + 7) / 8)) + ("%0*x" % ((n + 15) / 16, len(d))).decode("hex")
  return h + d


ec_curve_oid_p256 = "".join(chr(i) for i in (0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07))
ec_curve_oid_p384 = "".join(chr(i) for i in (0x06, 0x05, 0x2b, 0x81, 0x04, 0x00, 0x22))
ec_curve_oid_p521 = "".join(chr(i) for i in (0x06, 0x05, 0x2b, 0x81, 0x04, 0x00, 0x23))

p11 = PKCS11("./libcryptech-pkcs11.so")

if False:
  print "Not using MUTEXes at all"
  p11.C_Initialize()

elif False:
  print "Using OS MUTEXes"
  p11.C_Initialize(CKF_OS_LOCKING_OK)

else:
  print "Using our own pseudo-MUTEXes"
  from cryptech.py11.mutex import MutexDB
  mdb = MutexDB()
  p11.C_Initialize(0, mdb.create, mdb.destroy, mdb.lock, mdb.unlock)

slots = p11.C_GetSlotList()

print
print "Listing slots and tokens"
for i in slots:
  show_token(i, p11.C_GetTokenInfo(i))

slot = slots[0]

print
print "Generating some random data"
session = p11.C_OpenSession(slot)
random  = p11.C_GenerateRandom(session, 33)
print len(random), "".join("%02x" % ord(i) for i in random)

print
print "Logging in"
p11.C_Login(session, CKU_USER, "fnord")

if False:
  print
  print "Generating RSA keypair"
  rsa_templates = genkeypair_templates("RSA", CKA_MODULUS_BITS = 1024)
  rsa_keypair   = p11.C_GenerateKeyPair(session, CKM_RSA_PKCS_KEY_PAIR_GEN, rsa_templates[0], rsa_templates[1])
  print rsa_keypair

print
print "Generating EC P256 keypair"
ec_templates = genkeypair_templates("EC-P256", CKA_EC_PARAMS = ec_curve_oid_p256)
ec_p256_keypair   = p11.C_GenerateKeyPair(session, CKM_EC_KEY_PAIR_GEN, ec_templates[0], ec_templates[1])
print ec_p256_keypair

print
print "Generating EC P384 keypair"
ec_templates = genkeypair_templates("EC-P384", CKA_EC_PARAMS = ec_curve_oid_p384)
ec_p384_keypair   = p11.C_GenerateKeyPair(session, CKM_EC_KEY_PAIR_GEN, ec_templates[0], ec_templates[1])
print ec_p384_keypair

print
print "Generating EC P521 keypair"
ec_templates = genkeypair_templates("EC-P521", CKA_EC_PARAMS = ec_curve_oid_p521)
ec_p521_keypair   = p11.C_GenerateKeyPair(session, CKM_EC_KEY_PAIR_GEN, ec_templates[0], ec_templates[1])
print ec_p521_keypair

print
print "Listing keys"
show_keys(session, CKO_PUBLIC_KEY,  CKA_VERIFY, CKA_ENCRYPT, CKA_WRAP)
show_keys(session, CKO_PRIVATE_KEY, CKA_SIGN,   CKA_DECRYPT, CKA_UNWRAP)

hamster = "Your mother was a hamster"

print
print "Testing P-256 signature"
p11.C_SignInit(session, CKM_ECDSA_SHA256, ec_p256_keypair[1])
sig_p256 = p11.C_Sign(session, hamster)
print "Signature:", sig_p256.encode("hex")

print
print "Testing P256 verification"
p11.C_VerifyInit(session, CKM_ECDSA_SHA256, ec_p256_keypair[0])
p11.C_Verify(session, hamster, sig_p256)
print "OK"                      # Verification failure throws an exception

print
print "Testing C_CreateObject()"
handle = p11.C_CreateObject(session, dict(
    CKA_CLASS           = CKO_PUBLIC_KEY,
    CKA_KEY_TYPE        = CKK_EC,
    CKA_LABEL           = "EC-P-256 test case from \"Suite B Implementer's Guide to FIPS 186-3\"",
    CKA_ID              = "TC-P-256",
    CKA_VERIFY          = True,
    CKA_ENCRYPT         = False,
    CKA_WRAP            = False,
    CKA_TOKEN           = False,
    CKA_EC_POINT        = build_ecpoint(0x8101ece47464a6ead70cf69a6e2bd3d88691a3262d22cba4f7635eaff26680a8,
                                        0xd8a12ba61d599235f67d9cb4d58f1783d3ca43e78f0a5abaa624079936c0c3a9),
    CKA_EC_PARAMS       = ec_curve_oid_p256))
print handle

print
print "Verifying canned signature with public key installed via C_CreateObject()"
p11.C_VerifyInit(session, CKM_ECDSA, handle)
p11.C_Verify(session,
             "7c3e883ddc8bd688f96eac5e9324222c8f30f9d6bb59e9c5f020bd39ba2b8377".decode("hex"),
             "7214bc9647160bbd39ff2f80533f5dc6ddd70ddf86bb815661e805d5d4e6f27c".decode("hex") +
             "7d1ff961980f961bdaa3233b6209f4013317d3e3f9e1493592dbeaa1af2bc367".decode("hex"))
print "OK"

p11.C_CloseAllSessions(slot)
p11.C_Finalize()
