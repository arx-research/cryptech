# An attempt at a Python interface to PKCS 11 using the scary ctypes
# module from the Python standard library.

# This code is derived from the RSA PKCS #11 C header files, which say:
#
# License to copy and use this software is granted provided that it is
# identified as "RSA Security Inc. PKCS #11 Cryptographic Token Interface
# (Cryptoki)" in all material mentioning or referencing this software.
#
# License is also granted to make and use derivative works provided that
# such works are identified as "derived from the RSA Security Inc. PKCS #11
# Cryptographic Token Interface (Cryptoki)" in all material mentioning or
# referencing the derived work.
#
# RSA Security Inc. makes no representations concerning either the
# merchantability of this software or the suitability of this software for
# any particular purpose. It is provided "as is" without express or implied
# warranty of any kind.

class CKR_Exception(Exception):
  """
  Base class for PKCS #11 exceptions.
  """

  ckr_code = None
  ckr_map  = {}

  def __int__(self):
    return self.ckr_code

  @classmethod
  def raise_on_failure(cls, rv, func, *args):
    if rv != CKR_OK:
      raise cls.ckr_map[rv]

CKR_OK                                                                    = 0x00000000

class CKR_CANCEL        			(CKR_Exception): ckr_code = 0x00000001
class CKR_HOST_MEMORY   			(CKR_Exception): ckr_code = 0x00000002
class CKR_SLOT_ID_INVALI        		(CKR_Exception): ckr_code = 0x00000003
class CKR_GENERAL_ERROR                 	(CKR_Exception): ckr_code = 0x00000005
class CKR_FUNCTION_FAILED			(CKR_Exception): ckr_code = 0x00000006
class CKR_ARGUMENTS_BAD                         (CKR_Exception): ckr_code = 0x00000007
class CKR_NO_EVENT				(CKR_Exception): ckr_code = 0x00000008
class CKR_NEED_TO_CREATE_THREADS		(CKR_Exception): ckr_code = 0x00000009
class CKR_CANT_LOCK				(CKR_Exception): ckr_code = 0x0000000A
class CKR_ATTRIBUTE_READ_ONLY			(CKR_Exception): ckr_code = 0x00000010
class CKR_ATTRIBUTE_SENSITIVE			(CKR_Exception): ckr_code = 0x00000011
class CKR_ATTRIBUTE_TYPE_INVALID		(CKR_Exception): ckr_code = 0x00000012
class CKR_ATTRIBUTE_VALUE_INVALID		(CKR_Exception): ckr_code = 0x00000013
class CKR_DATA_INVALID				(CKR_Exception): ckr_code = 0x00000020
class CKR_DATA_LEN_RANGE			(CKR_Exception): ckr_code = 0x00000021
class CKR_DEVICE_ERROR				(CKR_Exception): ckr_code = 0x00000030
class CKR_DEVICE_MEMORY				(CKR_Exception): ckr_code = 0x00000031
class CKR_DEVICE_REMOVED			(CKR_Exception): ckr_code = 0x00000032
class CKR_ENCRYPTED_DATA_INVALID		(CKR_Exception): ckr_code = 0x00000040
class CKR_ENCRYPTED_DATA_LEN_RANGE		(CKR_Exception): ckr_code = 0x00000041
class CKR_FUNCTION_CANCELED			(CKR_Exception): ckr_code = 0x00000050
class CKR_FUNCTION_NOT_PARALLEL			(CKR_Exception): ckr_code = 0x00000051
class CKR_FUNCTION_NOT_SUPPORTED		(CKR_Exception): ckr_code = 0x00000054
class CKR_KEY_HANDLE_INVALID			(CKR_Exception): ckr_code = 0x00000060
class CKR_KEY_SIZE_RANGE			(CKR_Exception): ckr_code = 0x00000062
class CKR_KEY_TYPE_INCONSISTENT			(CKR_Exception): ckr_code = 0x00000063
class CKR_KEY_NOT_NEEDED			(CKR_Exception): ckr_code = 0x00000064
class CKR_KEY_CHANGED				(CKR_Exception): ckr_code = 0x00000065
class CKR_KEY_NEEDED				(CKR_Exception): ckr_code = 0x00000066
class CKR_KEY_INDIGESTIBLE			(CKR_Exception): ckr_code = 0x00000067
class CKR_KEY_FUNCTION_NOT_PERMITTED		(CKR_Exception): ckr_code = 0x00000068
class CKR_KEY_NOT_WRAPPABLE			(CKR_Exception): ckr_code = 0x00000069
class CKR_KEY_UNEXTRACTABLE			(CKR_Exception): ckr_code = 0x0000006A
class CKR_MECHANISM_INVALID			(CKR_Exception): ckr_code = 0x00000070
class CKR_MECHANISM_PARAM_INVALID		(CKR_Exception): ckr_code = 0x00000071
class CKR_OBJECT_HANDLE_INVALID			(CKR_Exception): ckr_code = 0x00000082
class CKR_OPERATION_ACTIVE			(CKR_Exception): ckr_code = 0x00000090
class CKR_OPERATION_NOT_INITIALIZED		(CKR_Exception): ckr_code = 0x00000091
class CKR_PIN_INCORRECT				(CKR_Exception): ckr_code = 0x000000A0
class CKR_PIN_INVALID				(CKR_Exception): ckr_code = 0x000000A1
class CKR_PIN_LEN_RANGE				(CKR_Exception): ckr_code = 0x000000A2
class CKR_PIN_EXPIRED				(CKR_Exception): ckr_code = 0x000000A3
class CKR_PIN_LOCKED				(CKR_Exception): ckr_code = 0x000000A4
class CKR_SESSION_CLOSED			(CKR_Exception): ckr_code = 0x000000B0
class CKR_SESSION_COUNT				(CKR_Exception): ckr_code = 0x000000B1
class CKR_SESSION_HANDLE_INVALID		(CKR_Exception): ckr_code = 0x000000B3
class CKR_SESSION_PARALLEL_NOT_SUPPORTED	(CKR_Exception): ckr_code = 0x000000B4
class CKR_SESSION_READ_ONLY			(CKR_Exception): ckr_code = 0x000000B5
class CKR_SESSION_EXISTS			(CKR_Exception): ckr_code = 0x000000B6
class CKR_SESSION_READ_ONLY_EXISTS		(CKR_Exception): ckr_code = 0x000000B7
class CKR_SESSION_READ_WRITE_SO_EXISTS		(CKR_Exception): ckr_code = 0x000000B8
class CKR_SIGNATURE_INVALID			(CKR_Exception): ckr_code = 0x000000C0
class CKR_SIGNATURE_LEN_RANGE			(CKR_Exception): ckr_code = 0x000000C1
class CKR_TEMPLATE_INCOMPLETE			(CKR_Exception): ckr_code = 0x000000D0
class CKR_TEMPLATE_INCONSISTENT			(CKR_Exception): ckr_code = 0x000000D1
class CKR_TOKEN_NOT_PRESENT			(CKR_Exception): ckr_code = 0x000000E0
class CKR_TOKEN_NOT_RECOGNIZED			(CKR_Exception): ckr_code = 0x000000E1
class CKR_TOKEN_WRITE_PROTECTED			(CKR_Exception): ckr_code = 0x000000E2
class CKR_UNWRAPPING_KEY_HANDLE_INVALID		(CKR_Exception): ckr_code = 0x000000F0
class CKR_UNWRAPPING_KEY_SIZE_RANGE		(CKR_Exception): ckr_code = 0x000000F1
class CKR_UNWRAPPING_KEY_TYPE_INCONSISTENT	(CKR_Exception): ckr_code = 0x000000F2
class CKR_USER_ALREADY_LOGGED_IN		(CKR_Exception): ckr_code = 0x00000100
class CKR_USER_NOT_LOGGED_IN			(CKR_Exception): ckr_code = 0x00000101
class CKR_USER_PIN_NOT_INITIALIZED		(CKR_Exception): ckr_code = 0x00000102
class CKR_USER_TYPE_INVALID			(CKR_Exception): ckr_code = 0x00000103
class CKR_USER_ANOTHER_ALREADY_LOGGED_IN	(CKR_Exception): ckr_code = 0x00000104
class CKR_USER_TOO_MANY_TYPES			(CKR_Exception): ckr_code = 0x00000105
class CKR_WRAPPED_KEY_INVALID			(CKR_Exception): ckr_code = 0x00000110
class CKR_WRAPPED_KEY_LEN_RANGE			(CKR_Exception): ckr_code = 0x00000112
class CKR_WRAPPING_KEY_HANDLE_INVALID		(CKR_Exception): ckr_code = 0x00000113
class CKR_WRAPPING_KEY_SIZE_RANGE		(CKR_Exception): ckr_code = 0x00000114
class CKR_WRAPPING_KEY_TYPE_INCONSISTENT	(CKR_Exception): ckr_code = 0x00000115
class CKR_RANDOM_SEED_NOT_SUPPORTED		(CKR_Exception): ckr_code = 0x00000120
class CKR_RANDOM_NO_RNG				(CKR_Exception): ckr_code = 0x00000121
class CKR_DOMAIN_PARAMS_INVALID			(CKR_Exception): ckr_code = 0x00000130
class CKR_BUFFER_TOO_SMALL			(CKR_Exception): ckr_code = 0x00000150
class CKR_SAVED_STATE_INVALID			(CKR_Exception): ckr_code = 0x00000160
class CKR_INFORMATION_SENSITIVE			(CKR_Exception): ckr_code = 0x00000170
class CKR_STATE_UNSAVEABLE			(CKR_Exception): ckr_code = 0x00000180
class CKR_CRYPTOKI_NOT_INITIALIZED		(CKR_Exception): ckr_code = 0x00000190
class CKR_CRYPTOKI_ALREADY_INITIALIZED		(CKR_Exception): ckr_code = 0x00000191
class CKR_MUTEX_BAD				(CKR_Exception): ckr_code = 0x000001A0
class CKR_MUTEX_NOT_LOCKED			(CKR_Exception): ckr_code = 0x000001A1
class CKR_NEW_PIN_MODE				(CKR_Exception): ckr_code = 0x000001B0
class CKR_NEXT_OTP				(CKR_Exception): ckr_code = 0x000001B1
class CKR_EXCEEDED_MAX_ITERATIONS		(CKR_Exception): ckr_code = 0x000001B5
class CKR_FIPS_SELF_TEST_FAILED			(CKR_Exception): ckr_code = 0x000001B6
class CKR_LIBRARY_LOAD_FAILED			(CKR_Exception): ckr_code = 0x000001B7
class CKR_PIN_TOO_WEAK  			(CKR_Exception): ckr_code = 0x000001B8
class CKR_PUBLIC_KEY_INVALID			(CKR_Exception): ckr_code = 0x000001B9
class CKR_FUNCTION_REJECTED			(CKR_Exception): ckr_code = 0x00000200
class CKR_VENDOR_DEFINED			(CKR_Exception): ckr_code = 0x80000000

for e in globals().values():
  if isinstance(e, type) and issubclass(e, CKR_Exception) and e is not CKR_Exception:
    CKR_Exception.ckr_map[e.ckr_code] = e
