# An attempt at a Python interface to PKCS 11 using the scary ctypes
# module from the Python standard library.

from struct     import pack, unpack
from ctypes     import *
from .types     import *
from .constants import *

class Attribute(object):

  @classmethod
  def new(cls, attribute_name, type_name):
    from . import constants
    from . import types
    assert attribute_name.startswith("CKA_")
    attribute_number = getattr(constants, attribute_name)
    type_class = getattr(types, type_name, str)
    if type_class is CK_BBOOL:
      cls = Attribute_CK_BBOOL
    elif type_class is CK_ULONG:
      cls = Attribute_CK_ULONG
    elif type_name == "biginteger":
      cls = Attribute_biginteger
    return cls(attribute_name, attribute_number, type_name, type_class)

  def __init__(self, attribute_name, attribute_number, type_name, type_class):
    assert attribute_name.startswith("CKA_")
    self.name = attribute_name
    self.code = attribute_number
    self.type_name = type_name
    self.type_class = type_class

  def encode(self, x): return x
  def decode(self, x): return x

class Attribute_CK_BBOOL(Attribute):
  def encode(self, x): return chr(int(x))
  def decode(self, x): return bool(ord(x))

class Attribute_CK_ULONG(Attribute):
  def encode(self, x): return pack("L", x)
  def decode(self, x): return unpack("L", x)[0]

class Attribute_biginteger(Attribute):
  def encode(self, x): return "\x00" if x == 0 else ("%0*x" % (((x.bit_length() + 7) / 8) * 2, x)).decode("hex")
  def decode(self, x): return long(x.encode("hex"), 16)


class AttributeDB(object):

  def __init__(self):
    from .attribute_map import attribute_map
    self.db = {}
    for attribute_name, type_name in attribute_map.iteritems():
      a = Attribute.new(attribute_name, type_name)
      self.db[a.name] = a
      self.db[a.code] = a

  def encode(self, k, v):
    return self.db[k].encode(v) if k in self.db else v

  def decode(self, k, v):
    return self.db[k].decode(v) if k in self.db else v

  def getvalue_create_template(self, attributes):
    attributes = tuple(self.db[a].code for a in attributes)
    template = (CK_ATTRIBUTE * len(attributes))()
    for i in xrange(len(attributes)):
      template[i].type = attributes[i]
      template[i].pValue = None
      template[i].ulValueLen = 0
    return template

  def getvalue_allocate_template(self, template):
    for t in template:
      t.pValue = create_string_buffer(t.ulValueLen)

  def from_ctypes(self, template):
    return dict((t.type, self.decode(t.type, t.pValue[:t.ulValueLen]))
                for t in template)

  def to_ctypes(self, attributes):
    attributes = tuple(attributes.iteritems()
                       if isinstance(attributes, dict) else
                       attributes)
    template = (CK_ATTRIBUTE * len(attributes))()
    for i, kv in enumerate(attributes):
      k, v = kv
      if k in self.db:
        a = self.db[k]
        k, v = a.code, a.encode(v)
      template[i].type = k
      template[i].pValue = create_string_buffer(v)
      template[i].ulValueLen = len(v)
    return template

  def attribute_name(self, code):
    return self.db[code].name

  def attribute_code(self, name):
    return self.db[name].code
