"""
This is a Python interface to PKCS #11, using the ctypes module from
the Python standard library.

This is not (yet?) a complete implementation.  It's intended primarily
to simplify testing of the underlying PKCS #11 shared library.
"""

from ctypes      import *
from .exceptions import *
from .types      import *
from .constants  import *
from .attributes import *

import platform

if platform.system() == "Darwin":
    default_so_name = "/usr/local/lib/libcryptech-pkcs11.dylib"
elif platform.system() == "Linux":
    default_so_name = "/usr/lib/libcryptech-pkcs11.so"
else:
    default_so_name = "/usr/local/lib/libcryptech-pkcs11.so"


class PKCS11 (object):
  """
  PKCS #11 API object, encapsulating the PKCS #11 library itself.
  Sample usage:

    from cryptech.py11 import *

    p11 = PKCS11()
    p11.C_Initialize()
    session = p11.C_OpenSession()
    p11.C_login(session, CK_USER, "secret")
    p11.C_FindObjectsInit(session, {CKA_CLASS: CKO_PRIVATE_KEY, CKA_KEY_TYPE: CKK_EC, CKA_ID: foo})
    keys = list(p11.C_FindObjects(session))
    p11.C_FindObjectsFinal(session)
    if len(keys) != 1:
      raise RuntimeError
    p11.C_SignInit(session, CK_ECDSA_SHA256, keys[0])
    sig = p11.Sign(session, "Your mother was a hamster")
    p11.C_CloseAllSessions(slot)
    p11.C_Finalize()

  The full raw PKCS #11 API is available via the .so attribute, but
  using this can be tricky, both because it requires strict adherence
  to the C API and because one needs to be careful not to run afoul of
  the Python garbage collector.

  The example above uses a set of interface routines built on top of the
  raw PKCS #11 API, which map the API into something a bit more Pythonic.
  """

  # Whether to use C_GetFunctionList() instead of dynamic link symbols.
  use_C_GetFunctionList = False

  def __init__(self, so_name = default_so_name):
    self.so_name = so_name
    self.so = CDLL(so_name)
    self.d = type("Dispatch", (object,), {})()
    for name, args in Prototypes:
      try:
        func = getattr(self.so, name)
      except AttributeError:
        self.use_C_GetFunctionList = True
      else:
        func.restype  = CK_RV
        func.argtypes = args
        func.errcheck = CKR_Exception.raise_on_failure
        setattr(self.d, name, func)
    if self.use_C_GetFunctionList:
      functions = CK_FUNCTION_LIST_PTR()
      self.so.C_GetFunctionList(byref(functions))
      for name, args in Prototypes:
        func = getattr(functions.contents, name)
        func.errcheck = CKR_Exception.raise_on_failure
        setattr(self.d, name, func)
    self.adb = AttributeDB()

  def __getattr__(self, name):
    try:
      return getattr(self.d, name)
    except AttributeError:
      return getattr(self.so, name)

  def C_GetFunctionList(self):
    return self

  @property
  def version(self):
    info = CK_INFO()
    self.d.C_GetInfo(byref(info))
    return info.cryptokiVersion

  # Be very careful if you try to provide your own locking functions.
  # For most purposes, if you really just want locking, you're best
  # off specifying CKF_OS_LOCKING_OK and letting the C code deal with
  # it.  The one case where you might want to provide your own locking
  # is when writing tests to verify behavior of the locking code.
  #
  # We have to stash references to the callback functions passed to
  # C_Initialize() to avoid dumping core when the garbage collector
  # deletes the function pointer instances out from under the C code.

  def C_Initialize(self, flags = 0, create_mutex = None, destroy_mutex = None, lock_mutex = None, unlock_mutex = None):
    if flags == 0 and create_mutex is None and destroy_mutex is None and lock_mutex is None and unlock_mutex is None:
      self._C_Initialize_args = None
      self.d.C_Initialize(None)
    else:
      create_mutex  = CK_CREATEMUTEX()  if create_mutex  is None else CK_CREATEMUTEX(create_mutex)
      destroy_mutex = CK_DESTROYMUTEX() if destroy_mutex is None else CK_DESTROYMUTEX(destroy_mutex)
      lock_mutex    = CK_LOCKMUTEX()    if lock_mutex    is None else CK_LOCKMUTEX(lock_mutex)
      unlock_mutex  = CK_UNLOCKMUTEX()  if unlock_mutex  is None else CK_UNLOCKMUTEX(unlock_mutex)
      self._C_Initialize_args = CK_C_INITIALIZE_ARGS(create_mutex, destroy_mutex,
                                                     lock_mutex, unlock_mutex, flags, None)
      self.d.C_Initialize(cast(byref(self._C_Initialize_args), CK_VOID_PTR))

  def C_Finalize(self):
    self.d.C_Finalize(None)
    self._C_Initialize_args = None

  def C_GetSlotList(self):
    count = CK_ULONG()
    self.d.C_GetSlotList(CK_TRUE, None, byref(count))
    slots = (CK_SLOT_ID * count.value)()
    self.d.C_GetSlotList(CK_TRUE, slots, byref(count))
    return tuple(slots[i] for i in xrange(count.value))

  def C_GetTokenInfo(self, slot_id):
    token_info = CK_TOKEN_INFO()
    self.d.C_GetTokenInfo(slot_id, byref(token_info))
    return token_info

  def C_OpenSession(self, slot, flags = CKF_RW_SESSION, application = None, notify = CK_NOTIFY()):
    flags |= CKF_SERIAL_SESSION
    handle = CK_SESSION_HANDLE()
    self.d.C_OpenSession(slot, flags, application, notify, byref(handle))
    return handle.value

  def C_GenerateRandom(self, session, n):
    buffer = create_string_buffer(n)
    self.d.C_GenerateRandom(session, buffer, sizeof(buffer))
    return buffer.raw

  def C_Login(self, session, user, pin):
    self.d.C_Login(session, user, pin, len(pin))

  def C_GetAttributeValue(self, session_handle, object_handle, *attributes):
    if len(attributes) == 1 and isinstance(attributes[0], (list, tuple)):
      attributes = attributes[0]
    template = self.adb.getvalue_create_template(attributes)
    self.d.C_GetAttributeValue(session_handle, object_handle, template, len(template))
    self.adb.getvalue_allocate_template(template)
    self.d.C_GetAttributeValue(session_handle, object_handle, template, len(template))
    return self.adb.from_ctypes(template)

  def C_FindObjectsInit(self, session, template = None, **kwargs):
    if kwargs:
      assert not template
      template = kwargs
    if template:
      self.d.C_FindObjectsInit(session, self.adb.to_ctypes(template), len(template))
    else:
      self.d.C_FindObjectsInit(session, None, 0)

  def C_FindObjects(self, session, chunk_size = 10):
    objects = (CK_OBJECT_HANDLE * chunk_size)()
    count   = CK_ULONG(1)
    while count.value > 0:
      self.d.C_FindObjects(session, objects, len(objects), byref(count))
      for i in xrange(count.value):
        yield objects[i]

  def FindObjects(self, session, template = None, **kwargs):
    self.C_FindObjectsInit(session, template, **kwargs)
    result = tuple(self.C_FindObjects(session))
    self.C_FindObjectsFinal(session)
    return result

  def _parse_GenerateKeyPair_template(self,
                                      # Attributes common to public and private templates
                                      CKA_ID,
                                      CKA_LABEL         = None,
                                      CKA_TOKEN         = False,
                                      # Attributes only in private template
                                      CKA_SIGN          = False,
                                      CKA_DECRYPT       = False,
                                      CKA_UNWRAP        = False,
                                      CKA_SENSITIVE     = True,
                                      CKA_PRIVATE       = True,
                                      CKA_EXTRACTABLE   = False,
                                      # Finer-grained control for CKA_TOKEN
                                      public_CKA_TOKEN  = False,
                                      private_CKA_TOKEN = False,
                                      # Anything else is only in public template
                                      **kwargs):
    if CKA_LABEL is None:
      CKA_LABEL = CKA_ID
    return (dict(kwargs,
                 CKA_LABEL      = CKA_LABEL,
                 CKA_ID         = CKA_ID,
                 CKA_TOKEN      = public_CKA_TOKEN or CKA_TOKEN),
            dict(CKA_LABEL      = CKA_LABEL,
                 CKA_ID         = CKA_ID,
                 CKA_TOKEN      = private_CKA_TOKEN or CKA_TOKEN,
                 CKA_SIGN       = CKA_SIGN,
                 CKA_DECRYPT    = CKA_DECRYPT,
                 CKA_UNWRAP     = CKA_UNWRAP,
                 CKA_SENSITIVE  = CKA_SENSITIVE,
                 CKA_PRIVATE    = CKA_PRIVATE,
                 CKA_EXTRACTABLE = CKA_EXTRACTABLE))

  def C_GenerateKeyPair(self, session, mechanism_type, public_template = None, private_template = None, **kwargs):
    if kwargs:
      assert not public_template and not private_template
      public_template, private_template = self._parse_GenerateKeyPair_template(**kwargs)
    public_template  = self.adb.to_ctypes(public_template)
    private_template = self.adb.to_ctypes(private_template)
    mechanism = CK_MECHANISM(mechanism_type, None, 0)
    public_handle  = CK_OBJECT_HANDLE()
    private_handle = CK_OBJECT_HANDLE()
    self.d.C_GenerateKeyPair(session, byref(mechanism),
                             public_template,  len(public_template),
                             private_template, len(private_template),
                             byref(public_handle), byref(private_handle))
    return public_handle.value, private_handle.value

  def C_SignInit(self, session, mechanism_type, private_key):
    mechanism = CK_MECHANISM(mechanism_type, None, 0)
    self.d.C_SignInit(session, byref(mechanism), private_key)

  def C_Sign(self, session, data):
    n = CK_ULONG()
    self.d.C_Sign(session, data, len(data), None, byref(n))
    sig = create_string_buffer(n.value)
    self.d.C_Sign(session, data, len(data), sig, byref(n))
    return sig.raw

  def C_SignUpdate(self, session, data):
    self.d.C_SignUpdate(session, data, len(data))

  def C_SignFinal(self, session):
    n = CK_ULONG()
    self.d.C_SignFinal(session, None, byref(n))
    sig = create_string_buffer(n.value)
    self.d.C_SignFinal(session, sig, byref(n))
    return sig.raw

  def C_VerifyInit(self, session, mechanism_type, public_key):
    mechanism = CK_MECHANISM(mechanism_type, None, 0)
    self.d.C_VerifyInit(session, byref(mechanism), public_key)

  def C_Verify(self, session, data, signature):
    self.d.C_Verify(session, data, len(data), signature, len(signature))

  def C_VerifyUpdate(self, session, data):
    self.d.C_VerifyUpdate(session, data, len(data))

  def C_VerifyFinal(self, session, signature):
    self.d.C_VerifyFinal(session, signature, len(signature))

  def C_CreateObject(self, session, template = None, **kwargs):
    if kwargs:
      assert not template
      template = kwargs
    template = self.adb.to_ctypes(template)
    handle = CK_OBJECT_HANDLE()
    self.d.C_CreateObject(session, template, len(template), byref(handle))
    return handle.value

  def C_DigestInit(self, session, mechanism_type):
    mechanism = CK_MECHANISM(mechanism_type, None, 0)
    self.d.C_DigestInit(session, byref(mechanism))

  def C_Digest(self, session, data):
    n = CK_ULONG()
    self.d.C_Digest(session, data, len(data), None, byref(n))
    hash = create_string_buffer(n.value)
    self.d.C_Digest(session, data, len(data), hash, byref(n))
    return hash.raw

  def C_DigestUpdate(self, session, data):
    self.d.C_DigestUpdate(session, data, len(data))

  def C_DigestFinal(self, session):
    n = CK_ULONG()
    self.d.C_DigestFinal(session, None, byref(n))
    hash = create_string_buffer(n.value)
    self.d.C_DigestFinal(session, hash, byref(n))
    return hash.raw

__all__ = ["PKCS11"]
__all__.extend(name for name in globals()
               if name.startswith("CK")
               or name.startswith("CRYPTOKI_"))
