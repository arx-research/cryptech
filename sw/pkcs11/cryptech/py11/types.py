# An attempt at a Python interface to PKCS 11 using the scary ctypes
# module from the Python standard library.

from ctypes import *
from .types import *
from .exceptions import *

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

# This is not a complete set of PKCS #11 types, because the full set
# with all the optional mechanisms is tediously long and each
# structure type requires hand conversion.  Goal at the moment is to
# cover stuff we care about in the base specification.  Add other
# mechanism-specific stuff later as needed.

# Mapping beween C and Python at the lowest level isn't perfect
# because characters are integer types in C while they're strings in
# Python.  It looks like we want the string handling in most cases
# other than CK_BBOOL and CK_VERSION, map lowest-level PKCS #11 types
# accordingly.

CK_BYTE                                 = c_char
CK_CHAR                                 = CK_BYTE
CK_UTF8CHAR                             = CK_BYTE
CK_BBOOL                                = c_ubyte
CK_ULONG                                = c_ulong
CK_LONG                                 = c_long
CK_FLAGS                                = CK_ULONG
CK_VOID_PTR                             = POINTER(c_char)

CK_BYTE_PTR                             = POINTER(CK_BYTE)
CK_CHAR_PTR                             = POINTER(CK_CHAR)
CK_UTF8CHAR_PTR                         = POINTER(CK_UTF8CHAR)
CK_ULONG_PTR                            = POINTER(CK_ULONG)
CK_VOID_PTR_PTR                         = POINTER(CK_VOID_PTR)

class CK_VERSION (Structure):
  _fields_ = [("major",                 c_ubyte),
              ("minor",                 c_ubyte)]

CK_VERSION_PTR                          = POINTER(CK_VERSION)

class CK_INFO (Structure):
  _fields_ = [("cryptokiVersion",       CK_VERSION),
              ("manufacturerID",        CK_UTF8CHAR * 32),
              ("flags",                 CK_FLAGS),
              ("libraryDescription",    CK_UTF8CHAR * 32),
              ("libraryVersion",        CK_VERSION)]

CK_INFO_PTR                             = POINTER(CK_INFO)

CK_NOTIFICATION                         = CK_ULONG

CK_SLOT_ID                              = CK_ULONG

CK_SLOT_ID_PTR                          = POINTER(CK_SLOT_ID)

class CK_SLOT_INFO (Structure):
  _fields_ = [("slotDescription",       CK_UTF8CHAR * 64),
              ("manufacturerID",        CK_UTF8CHAR * 32),
              ("flags",                 CK_FLAGS),
              ("hardwareVersion",       CK_VERSION),
              ("firmwareVersion",       CK_VERSION)]

CK_SLOT_INFO_PTR                        = POINTER(CK_SLOT_INFO)

class CK_TOKEN_INFO (Structure):
  _fields_ = [("label",                 CK_UTF8CHAR * 32),
              ("manufacturerID",        CK_UTF8CHAR * 32),
              ("model",                 CK_UTF8CHAR * 16),
              ("serialNumber",          CK_CHAR * 16),
              ("flags",                 CK_FLAGS),
              ("ulMaxSessionCount",	CK_ULONG),
              ("ulSessionCount",	CK_ULONG),
              ("ulMaxRwSessionCount",	CK_ULONG),
              ("ulRwSessionCount",	CK_ULONG),
              ("ulMaxPinLen",           CK_ULONG),
              ("ulMinPinLen",           CK_ULONG),
              ("ulTotalPublicMemory",	CK_ULONG),
              ("ulFreePublicMemory",	CK_ULONG),
              ("ulTotalPrivateMemory",	CK_ULONG),
              ("ulFreePrivateMemory",	CK_ULONG),
              ("hardwareVersion",	CK_VERSION),
              ("firmwareVersion",	CK_VERSION),
              ("utcTime",               CK_CHAR * 16)]

CK_TOKEN_INFO_PTR                       = POINTER(CK_TOKEN_INFO)

CK_SESSION_HANDLE                       = CK_ULONG

CK_SESSION_HANDLE_PTR                   = POINTER(CK_SESSION_HANDLE)

CK_USER_TYPE                            = CK_ULONG

CK_STATE                                = CK_ULONG

class CK_SESSION_INFO (Structure):
  _fields_ = [("slotID",                CK_SLOT_ID),
              ("state",                 CK_STATE),
              ("flags",                 CK_FLAGS),
              ("ulDeviceError",         CK_ULONG)]

CK_SESSION_INFO_PTR                     = POINTER(CK_SESSION_INFO)

CK_OBJECT_HANDLE                        = CK_ULONG

CK_OBJECT_HANDLE_PTR                    = POINTER(CK_OBJECT_HANDLE)

CK_OBJECT_CLASS                         = CK_ULONG

CK_OBJECT_CLASS_PTR                     = POINTER(CK_OBJECT_CLASS)

CK_HW_FEATURE_TYPE                      = CK_ULONG

CK_KEY_TYPE                             = CK_ULONG

CK_CERTIFICATE_TYPE                     = CK_ULONG

CK_ATTRIBUTE_TYPE                       = CK_ULONG

class CK_ATTRIBUTE (Structure):
  _fields_ = [("type",                  CK_ATTRIBUTE_TYPE),
              ("pValue",                CK_VOID_PTR),
              ("ulValueLen",            CK_ULONG)]

CK_ATTRIBUTE_PTR                        = POINTER(CK_ATTRIBUTE)

class CK_DATE (Structure):
  _fields_ = [("year",                  CK_CHAR * 4),
              ("month",                 CK_CHAR * 2),
              ("day",                   CK_CHAR * 2)]

CK_MECHANISM_TYPE                       = CK_ULONG

CK_MECHANISM_TYPE_PTR                   = POINTER(CK_MECHANISM_TYPE)

class CK_MECHANISM (Structure):
  _fields_ = [("mechanism",             CK_MECHANISM_TYPE),
              ("pParameter",            CK_VOID_PTR),
              ("ulParameterLen",        CK_ULONG)]

CK_MECHANISM_PTR                        = POINTER(CK_MECHANISM)

class CK_MECHANISM_INFO (Structure):
  _fields_ = [("ulMinKeySize",          CK_ULONG),
              ("ulMaxKeySize",          CK_ULONG),
              ("flags",                 CK_FLAGS)]

CK_MECHANISM_INFO_PTR                   = POINTER(CK_MECHANISM_INFO)

CK_RV                                   = CK_ULONG

CK_NOTIFY                               = CFUNCTYPE(CK_RV, CK_SESSION_HANDLE, CK_NOTIFICATION, CK_VOID_PTR)

CK_CREATEMUTEX                          = CFUNCTYPE(CK_RV, CK_VOID_PTR_PTR)
CK_DESTROYMUTEX                         = CFUNCTYPE(CK_RV, CK_VOID_PTR)
CK_LOCKMUTEX                            = CFUNCTYPE(CK_RV, CK_VOID_PTR)
CK_UNLOCKMUTEX                          = CFUNCTYPE(CK_RV, CK_VOID_PTR)

class CK_C_INITIALIZE_ARGS (Structure):
  _fields_ = [("CreateMutex",           CK_CREATEMUTEX),
              ("DestroyMutex",          CK_DESTROYMUTEX),
              ("LockMutex",             CK_LOCKMUTEX),
              ("UnlockMutex",           CK_UNLOCKMUTEX),
              ("flags",                 CK_FLAGS),
              ("pReserved",             CK_VOID_PTR)]

CK_C_INITIALIZE_ARGS_PTR                = POINTER(CK_C_INITIALIZE_ARGS)

# Prototypes for the PKCS #11 public functions.
#
# These used to be a separate module, but the forward references
# needed to implement CK_FUNCTION_LIST are enough fun without adding a
# gratuitous Python module loop too.

class CK_FUNCTION_LIST (Structure):
  pass

CK_FUNCTION_LIST_PTR                    = POINTER(CK_FUNCTION_LIST)
CK_FUNCTION_LIST_PTR_PTR                = POINTER(CK_FUNCTION_LIST_PTR)

Prototypes = (
  ("C_Initialize",            [CK_VOID_PTR]),
  ("C_Finalize",              [CK_VOID_PTR]),
  ("C_GetInfo",               [CK_INFO_PTR]),
  ("C_GetFunctionList",       [CK_FUNCTION_LIST_PTR_PTR]),
  ("C_GetSlotList",           [CK_BBOOL, CK_SLOT_ID_PTR, CK_ULONG_PTR]),
  ("C_GetSlotInfo",           [CK_SLOT_ID, CK_SLOT_INFO_PTR]),
  ("C_GetTokenInfo",          [CK_SLOT_ID, CK_TOKEN_INFO_PTR]),
  ("C_GetMechanismList",      [CK_SLOT_ID, CK_MECHANISM_TYPE_PTR, CK_ULONG_PTR]),
  ("C_GetMechanismInfo",      [CK_SLOT_ID, CK_MECHANISM_TYPE, CK_MECHANISM_INFO_PTR]),
  ("C_InitToken",             [CK_SLOT_ID, CK_UTF8CHAR_PTR, CK_ULONG, CK_UTF8CHAR_PTR]),
  ("C_InitPIN",               [CK_SESSION_HANDLE, CK_UTF8CHAR_PTR, CK_ULONG]),
  ("C_SetPIN",                [CK_SESSION_HANDLE, CK_UTF8CHAR_PTR, CK_ULONG, CK_UTF8CHAR_PTR, CK_ULONG]),
  ("C_OpenSession",           [CK_SLOT_ID, CK_FLAGS, CK_VOID_PTR, CK_NOTIFY, CK_SESSION_HANDLE_PTR]),
  ("C_CloseSession",          [CK_SESSION_HANDLE]),
  ("C_CloseAllSessions",      [CK_SLOT_ID]),
  ("C_GetSessionInfo",        [CK_SESSION_HANDLE, CK_SESSION_INFO_PTR]),
  ("C_GetOperationState",     [CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG_PTR]),
  ("C_SetOperationState",     [CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG, CK_OBJECT_HANDLE, CK_OBJECT_HANDLE]),
  ("C_Login",                 [CK_SESSION_HANDLE, CK_USER_TYPE, CK_UTF8CHAR_PTR, CK_ULONG]),
  ("C_Logout",                [CK_SESSION_HANDLE]),
  ("C_CreateObject",          [CK_SESSION_HANDLE, CK_ATTRIBUTE_PTR, CK_ULONG, CK_OBJECT_HANDLE_PTR]),
  ("C_CopyObject",            [CK_SESSION_HANDLE, CK_OBJECT_HANDLE, CK_ATTRIBUTE_PTR, CK_ULONG, CK_OBJECT_HANDLE_PTR]),
  ("C_DestroyObject",         [CK_SESSION_HANDLE, CK_OBJECT_HANDLE]),
  ("C_GetObjectSize",         [CK_SESSION_HANDLE, CK_OBJECT_HANDLE, CK_ULONG_PTR]),
  ("C_GetAttributeValue",     [CK_SESSION_HANDLE, CK_OBJECT_HANDLE, CK_ATTRIBUTE_PTR, CK_ULONG]),
  ("C_SetAttributeValue",     [CK_SESSION_HANDLE, CK_OBJECT_HANDLE, CK_ATTRIBUTE_PTR, CK_ULONG]),
  ("C_FindObjectsInit",       [CK_SESSION_HANDLE, CK_ATTRIBUTE_PTR, CK_ULONG]),
  ("C_FindObjects",           [CK_SESSION_HANDLE, CK_OBJECT_HANDLE_PTR, CK_ULONG, CK_ULONG_PTR]),
  ("C_FindObjectsFinal",      [CK_SESSION_HANDLE]),
  ("C_EncryptInit",           [CK_SESSION_HANDLE, CK_MECHANISM_PTR, CK_OBJECT_HANDLE]),
  ("C_Encrypt",               [CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG, CK_BYTE_PTR, CK_ULONG_PTR]),
  ("C_EncryptUpdate",         [CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG, CK_BYTE_PTR, CK_ULONG_PTR]),
  ("C_EncryptFinal",          [CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG_PTR]),
  ("C_DecryptInit",           [CK_SESSION_HANDLE, CK_MECHANISM_PTR, CK_OBJECT_HANDLE]),
  ("C_Decrypt",               [CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG, CK_BYTE_PTR, CK_ULONG_PTR]),
  ("C_DecryptUpdate",         [CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG, CK_BYTE_PTR, CK_ULONG_PTR]),
  ("C_DecryptFinal",          [CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG_PTR]),
  ("C_DigestInit",            [CK_SESSION_HANDLE, CK_MECHANISM_PTR]),
  ("C_Digest",                [CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG, CK_BYTE_PTR, CK_ULONG_PTR]),
  ("C_DigestUpdate",          [CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG]),
  ("C_DigestKey",             [CK_SESSION_HANDLE, CK_OBJECT_HANDLE]),
  ("C_DigestFinal",           [CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG_PTR]),
  ("C_SignInit",              [CK_SESSION_HANDLE, CK_MECHANISM_PTR, CK_OBJECT_HANDLE]),
  ("C_Sign",                  [CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG, CK_BYTE_PTR, CK_ULONG_PTR]),
  ("C_SignUpdate",            [CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG]),
  ("C_SignFinal",             [CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG_PTR]),
  ("C_SignRecoverInit",       [CK_SESSION_HANDLE, CK_MECHANISM_PTR, CK_OBJECT_HANDLE]),
  ("C_SignRecover",           [CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG, CK_BYTE_PTR, CK_ULONG_PTR]),
  ("C_VerifyInit",            [CK_SESSION_HANDLE, CK_MECHANISM_PTR, CK_OBJECT_HANDLE]),
  ("C_Verify",                [CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG, CK_BYTE_PTR, CK_ULONG]),
  ("C_VerifyUpdate",          [CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG]),
  ("C_VerifyFinal",           [CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG]),
  ("C_VerifyRecoverInit",     [CK_SESSION_HANDLE, CK_MECHANISM_PTR, CK_OBJECT_HANDLE]),
  ("C_VerifyRecover",         [CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG, CK_BYTE_PTR, CK_ULONG_PTR]),
  ("C_DigestEncryptUpdate",   [CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG, CK_BYTE_PTR, CK_ULONG_PTR]),
  ("C_DecryptDigestUpdate",   [CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG, CK_BYTE_PTR, CK_ULONG_PTR]),
  ("C_SignEncryptUpdate",     [CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG, CK_BYTE_PTR, CK_ULONG_PTR]),
  ("C_DecryptVerifyUpdate",   [CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG, CK_BYTE_PTR, CK_ULONG_PTR]),
  ("C_GenerateKey",           [CK_SESSION_HANDLE, CK_MECHANISM_PTR, CK_ATTRIBUTE_PTR, CK_ULONG, CK_OBJECT_HANDLE_PTR]),
  ("C_GenerateKeyPair",       [CK_SESSION_HANDLE, CK_MECHANISM_PTR, CK_ATTRIBUTE_PTR, CK_ULONG,
                               CK_ATTRIBUTE_PTR, CK_ULONG, CK_OBJECT_HANDLE_PTR, CK_OBJECT_HANDLE_PTR]),
  ("C_WrapKey",               [CK_SESSION_HANDLE, CK_MECHANISM_PTR, CK_OBJECT_HANDLE, CK_OBJECT_HANDLE,
                               CK_BYTE_PTR, CK_ULONG_PTR]),
  ("C_UnwrapKey",             [CK_SESSION_HANDLE, CK_MECHANISM_PTR, CK_OBJECT_HANDLE, CK_BYTE_PTR, CK_ULONG,
                               CK_ATTRIBUTE_PTR, CK_ULONG, CK_OBJECT_HANDLE_PTR]),
  ("C_DeriveKey",             [CK_SESSION_HANDLE, CK_MECHANISM_PTR, CK_OBJECT_HANDLE, CK_ATTRIBUTE_PTR, CK_ULONG,
                               CK_OBJECT_HANDLE_PTR]),
  ("C_SeedRandom",            [CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG]),
  ("C_GenerateRandom",        [CK_SESSION_HANDLE, CK_BYTE_PTR, CK_ULONG]),
  ("C_GetFunctionStatus",     [CK_SESSION_HANDLE]),
  ("C_CancelFunction",        [CK_SESSION_HANDLE]),
  ("C_WaitForSlotEvent",      [CK_FLAGS, CK_SLOT_ID_PTR, CK_VOID_PTR]))

CK_FUNCTION_LIST._fields_ = ([("version", CK_VERSION)] +
                             [(_name, CFUNCTYPE(*([CK_RV] + _args)))
                              for _name, _args in Prototypes])
