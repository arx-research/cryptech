# Test vectors from "Suite B Implementer's Guide to FIPS 186-3".
#
# e is given in decimal, all other values are hex, because that's how
# these were given in the paper

p256_d    = 0x70a12c2db16845ed56ff68cfc21a472b3f04d7d6851bf6349f2d7d5b3452b38a
p256_Qx   = 0x8101ece47464a6ead70cf69a6e2bd3d88691a3262d22cba4f7635eaff26680a8
p256_Qy   = 0xd8a12ba61d599235f67d9cb4d58f1783d3ca43e78f0a5abaa624079936c0c3a9
p256_k    = 0x580ec00d856434334cef3f71ecaed4965b12ae37fa47055b1965c7b134ee45d0
p256_kinv = 0x6a664fa115356d33f16331b54c4e7ce967965386c7dcbf2904604d0c132b4a74
p256_Rx   = 0x7214bc9647160bbd39ff2f80533f5dc6ddd70ddf86bb815661e805d5d4e6f27c
p256_Ry   = 0x8b81e3e977597110c7cf2633435b2294b72642987defd3d4007e1cfc5df84541
p256_r    = p256_Rx
p256_M    = 0x54686973206973206f6e6c7920612074657374206d6573736167652e204974206973203438206279746573206c6f6e67
p256_H    = 0x7c3e883ddc8bd688f96eac5e9324222c8f30f9d6bb59e9c5f020bd39ba2b8377
p256_e    = 56197278047627432394583341962843287937266210957576322469816113796290471232375
p256_s    = 0x7d1ff961980f961bdaa3233b6209f4013317d3e3f9e1493592dbeaa1af2bc367
p256_w    = 0xd69be75f67ee5394cabb6c286f3610cf62d722cba9eea70faee770a6b2ed72dc
p256_u1   = 0xbb252401d6fb322bb747184cf2ac52bf8d54b95a1515062a2f6141f2e2092ed8
p256_u2   = 0xaae7d1c7f2c232dfc641948af3dba141d4de8634e571cf84c486301b510cfc04
p256_v    = 0x7214bc9647160bbd39ff2f80533f5dc6ddd70ddf86bb815661e805d5d4e6f27c

p384_d    = 0xc838b85253ef8dc7394fa5808a5183981c7deef5a69ba8f4f2117ffea39cfcd90e95f6cbc854abacab701d50c1f3cf24
p384_Qx   = 0x1fbac8eebd0cbf35640b39efe0808dd774debff20a2a329e91713baf7d7f3c3e81546d883730bee7e48678f857b02ca0
p384_Qy   = 0xeb213103bd68ce343365a8a4c3d4555fa385f5330203bdd76ffad1f3affb95751c132007e1b240353cb0a4cf1693bdf9
p384_k    = 0xdc6b44036989a196e39d1cdac000812f4bdd8b2db41bb33af51372585ebd1db63f0ce8275aa1fd45e2d2a735f8749359
p384_kinv = 0x7436f03088e65c37ba8e7b33887fbc87757514d611f7d1fbdf6d2104a297ad318cdbf7404e4ba37e599666df37b8d8be
p384_Rx   = 0xa0c27ec893092dea1e1bd2ccfed3cf945c8134ed0c9f81311a0f4a05942db8dbed8dd59f267471d5462aa14fe72de856
p384_Ry   = 0x855649409815bb91424eaca5fd76c97375d575d1422ec53d343bd33b847fdf0c11569685b528ab25493015428d7cf72b
p384_r    = p384_Rx
p384_M    = 0x54686973206973206f6e6c7920612074657374206d6573736167652e204974206973203438206279746573206c6f6e67
p384_H    = 0xb9210c9d7e20897ab86597266a9d5077e8db1b06f7220ed6ee75bd8b45db37891f8ba5550304004159f4453dc5b3f5a1
p384_e    = 28493976155450475404302482243066463769180620629462008675793884393889401828800663731864240088367206094074919580333473
p384_s    = 0x20ab3f45b74f10b6e11f96a2c8eb694d206b9dda86d3c7e331c26b22c987b7537726577667adadf168ebbe803794a402
p384_w    = 0x1798845cd0a6cea5327c501a71a4baf2f7be882cfbc303750a7c861af8fe8225467a257f5bf91a4aaa5a79a8637d218a
p384_u1   = 0x6ce25649d42d223e020c11140fe772326612bb11b686d35ee98ed4550e0635d9dd3a2afbca0cf2c4baedcd23313b189e
p384_u2   = 0xf3b240751d5d8ed394a4b5bf8e2a4c0e1e21aa51f2620a08b8c55a2bc334c9689923162648f06e5f4659fc526d9c1fd6
p384_v    = 0xa0c27ec893092dea1e1bd2ccfed3cf945c8134ed0c9f81311a0f4a05942db8dbed8dd59f267471d5462aa14fe72de856

from textwrap                   import TextWrapper
from os.path                    import basename
from sys                        import argv
from pyasn1.type.univ           import Sequence, Choice, Integer, OctetString, ObjectIdentifier, BitString
from pyasn1.type.namedtype      import NamedTypes, NamedType, OptionalNamedType
from pyasn1.type.namedval       import NamedValues
from pyasn1.type.tag            import Tag, tagClassContext, tagFormatSimple
from pyasn1.type.constraint     import SingleValueConstraint
from pyasn1.codec.der.encoder   import encode as DER_Encode
from pyasn1.codec.der.decoder   import decode as DER_Decode

wrapper = TextWrapper(width = 78, initial_indent = " " * 2, subsequent_indent = " " * 2)

def long_to_bytes(number, order):
  #
  # This is just plain nasty.
  #
  s = "%x" % number
  s = ("0" * (order/8 - len(s))) + s
  return s.decode("hex")

def bytes_to_bits(bytes):
  #
  # This, on the other hand, is not just plain nasty, this is fancy nasty.
  # This is nasty with raisins in it.
  #
  s = bin(long(bytes.encode("hex"), 16))[2:]
  if len(s) % 8:
    s = ("0" * (8 - len(s) % 8)) + s
  return tuple(int(i) for i in s)

###

def encode_sig(r, s, order):
  return long_to_bytes(r, order) + long_to_bytes(s, order)

p256_sig = encode_sig(p256_r, p256_s, 256)
p384_sig = encode_sig(p384_r, p384_s, 384)

###

class ECPrivateKey(Sequence):
  componentType = NamedTypes(
    NamedType("version", Integer(namedValues = NamedValues(("ecPrivkeyVer1", 1))
                                 ).subtype(subtypeSpec = Integer.subtypeSpec + SingleValueConstraint(1))),
    NamedType("privateKey", OctetString()),
    OptionalNamedType("parameters", ObjectIdentifier().subtype(explicitTag = Tag(tagClassContext, tagFormatSimple, 0))),
    OptionalNamedType("publicKey", BitString().subtype(explicitTag = Tag(tagClassContext, tagFormatSimple, 1))))

def encode_key(d, Qx, Qy, order, oid):
  private_key = long_to_bytes(d, order)
  public_key  = bytes_to_bits(chr(0x04) + long_to_bytes(Qx, order) + long_to_bytes(Qy, order))
  parameters = oid
  key = ECPrivateKey()
  key["version"]    = 1
  key["privateKey"] = private_key
  key["parameters"] = parameters
  key["publicKey"]  = public_key
  return DER_Encode(key)

p256_key = encode_key(p256_d, p256_Qx, p256_Qy, 256, "1.2.840.10045.3.1.7")
p384_key = encode_key(p384_d, p384_Qx, p384_Qy, 384, "1.3.132.0.34")

###

print "/*"
print " * ECDSA test data."
print " * File automatically generated by", basename(argv[0])
print " */"

curves = ("p256", "p384")
vars   = set()

for name in dir():
  head, sep, tail = name.partition("_")
  if head in curves:
    vars.add(tail)

vars = sorted(vars)

for curve in curves:
  order = int(curve[1:])
  for var in vars:
    name = curve + "_" + var
    value = globals().get(name, None)
    if isinstance(value, (int, long)):
      value = long_to_bytes(value, order)
    if value is not None:
      print
      print "static const uint8_t %s[] = { /* %d bytes */" % (name, len(value))
      print wrapper.fill(", ".join("0x%02x" % ord(v) for v in value))
      print "};"

print
print "typedef struct {"
print "  hal_curve_name_t curve;"
for var in vars:
  print "  const uint8_t *%8s; size_t %8s_len;" % (var, var)
print "} ecdsa_tc_t;"
print
print "static const ecdsa_tc_t ecdsa_tc[] = {"
for curve in curves:
  print "  { HAL_CURVE_%s," % curve.upper()
  for var in vars:
    name = curve + "_" + var
    if name in globals():
      print "    %-14s sizeof(%s)," % (name + ",", name)
    else:
      print "    %-14s 0," % "NULL,"
  print "  },"
print "};"
