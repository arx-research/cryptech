"""
Optional Python mutex implementation for cryptech.py11 library,
using the threading.Lock primitive to provide the mutex itself.

Most of the code in this module has to do with mapping between the
Python and PKCS #11 APIs.

If you just want locking, it's probably simpler to let the C code
handle it, by passing CKF_OS_LOCKING_OK to C_Initialize().

The main reason for having a complete implementation in Python is to
test the API.

Sample usage:

  from cryptech.py11       import *
  from cryptech.py11.mutex import MutexDB

  p11 = PKCS11()
  mdb = MutexDB()
  p11.C_Initialize(0, mdb.create, mdb.destroy, mdb.lock, mdb.unlock)

"""

from struct      import pack, unpack
from .types      import *
from .exceptions import *

# This controls how big our mutex handles are.
encoded_format = "=L"

# These are all determined by encoded_format, don't touch.

encoded_length = len(pack(encoded_format, 0))
encoded_type   = CK_BYTE * encoded_length
handle_max     = unpack(encoded_format, chr(0xff) * encoded_length)[0]

class Mutex(object):

  def __init__(self, handle):
    from threading import Lock
    self.encoded = encoded_type(*handle)
    self.lock    = Lock()

def p11_callback(func):
  from threading import ThreadError
  def wrapper(self, arg):
    try:
      func(self, arg)
    except ThreadError, e:
      print "Failed: %s" % e
      return CKR_MUTEX_NOT_LOCKED.ckr_code
    except Exception, e:
      print "Failed: %s" % e
      return CKR_FUNCTION_FAILED.ckr_code
    else:
      return CKR_OK
  return wrapper

class MutexDB(object):

  def __init__(self):
    self.mutexes = {}
    self.next_handle = 0

  def find_free_handle(self):
    if len(self.mutexes) > handle_max:
      raise RuntimeError
    while self.next_handle in self.mutexes:
      self.next_handle = (self.next_handle + 1) & handle_max
    return pack(encoded_format, self.next_handle)

  @p11_callback
  def create(self, handle_ptr):
    handle = self.find_free_handle()
    self.mutexes[handle] = Mutex(handle)
    handle_ptr[0] = self.mutexes[handle].encoded

  @p11_callback
  def destroy(self, handle):
    del self.mutexes[handle[:encoded_length]]

  @p11_callback
  def lock(self, handle):
    self.mutexes[handle[:encoded_length]].lock.acquire()

  @p11_callback
  def unlock(self, handle):
    self.mutexes[handle[:encoded_length]].lock.release()
