/*
 * pkcs11.c
 * --------
 *
 * This is a partial implementation of PKCS #11 on top of the Cryptech
 * libhal library connecting to the Cryptech FPGA cores.
 *
 * Author: Rob Austein
 * Copyright (c) 2015-2016, NORDUnet A/S
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the NORDUnet nor the names of its contributors may
 *   be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include <hal.h>

/*
 * Magic PKCS #11 macros that must be defined before including
 * pkcs11.h.  For now these are only the Unix versions, add others
 * later (which may require minor refactoring).
 */

#define CK_PTR                                          *
#define CK_DEFINE_FUNCTION(returnType, name)            returnType name
#define CK_DECLARE_FUNCTION(returnType, name)           returnType name
#define CK_DECLARE_FUNCTION_POINTER(returnType, name)   returnType (* name)
#define CK_CALLBACK_FUNCTION(returnType, name)          returnType (* name)
#ifndef NULL_PTR
#define NULL_PTR                                        NULL
#endif

#include "pkcs11.h"

#include "attributes.h"

/*
 * This PKCS #11 implementation is hardwired with one slot, the token
 * for which is always present (so we return the same answer
 * regardless of the value of tokenPresent).
 */

#define P11_ONE_AND_ONLY_SLOT   0

/*
 * How many sessions and object handles to allow.  We could do this
 * with dynamic memory, but static arrays are simpler and faster.  We
 * don't expect all that many sessions, and slots in the object table
 * are cheap.
 */

#ifndef P11_MAX_SESSION_HANDLES
#define P11_MAX_SESSION_HANDLES (64)
#endif

#ifndef P11_MAX_OBJECT_HANDLES
#define P11_MAX_OBJECT_HANDLES  (4096)
#endif

/*
 * Manufacturer ID, version numbers (hardware, firmware, software), etc.
 * Some of this really should be coming from RPC queries.
 */

#define P11_MANUFACTURER_ID     "Cryptech Project"
#define P11_TOKEN_LABEL		"Cryptech Token"
#define P11_BOARD_MODEL		"Alpha Board"
#define P11_BOARD_SERIAL	"007"
#define P11_LIBRARY_DESCRIPTION	"libcryptech-pkcs11.so"
#define	P11_SLOT_DESCRIPTION	"Cryptech Alpha slot"
#define P11_VERSION_HW_MAJOR    0
#define P11_VERSION_HW_MINOR    3
#define P11_VERSION_FW_MAJOR    3
#define P11_VERSION_FW_MINOR    0
#define P11_VERSION_SW_MAJOR    3
#define P11_VERSION_SW_MINOR    0

/*
 * Debugging control.
 */

#ifndef DEBUG_HAL
#define DEBUG_HAL       0
#endif

#ifndef DEBUG_PKCS11
#define DEBUG_PKCS11    0
#endif

/*
 * Whether to include POSIX-specific features.
 */

#ifndef USE_POSIX
#define USE_POSIX 1
#endif

/*
 * Whether to use POSIX threads.
 */

#ifndef USE_PTHREADS
#define USE_PTHREADS USE_POSIX
#endif

#if USE_PTHREADS && !USE_POSIX
#error Can not use POSIX threads without using POSIX
#endif

#if USE_POSIX
#include <unistd.h>
#include <errno.h>
#endif

#if USE_PTHREADS
#include <pthread.h>
#endif



/*
 * PKCS #11 sessions.  General idea is that we have separate
 * descriptors/handles/state for each operation that we're allowed to
 * do in parallel, so sign, verify, digest, encrypt, decrypt, wrapkey,
 * and unwrapkey all need separate slots in the session structure.
 * Add these as we go.
 */

typedef struct p11_session {
  CK_SESSION_HANDLE handle;             /* Session handle */
  CK_STATE state;                       /* State (CKS_*) of this session */
  CK_NOTIFY notify;                     /* Notification callback */
  CK_VOID_PTR application;              /* Application data */
  hal_pkey_attribute_t *find_query;     /* FindObject*() query state */
  unsigned find_query_token : 1;        /* Find query for token objects in progress */
  unsigned find_query_session : 1;      /* Find query for session objects in progress */
  unsigned find_query_n : 30;           /* Number of entries in find_query */
  hal_uuid_t find_query_previous_uuid;  /* Previous UUID for find queries */
  unsigned find_query_state;            /* hal_rpc_pkey_match() internal state */
  hal_digest_algorithm_t
    digest_algorithm,                   /* Hash algorithm for C_Digest*() */
    sign_digest_algorithm,              /* Hash algorithm for C_Sign*() */
    verify_digest_algorithm;            /* Hash algorithm for C_Verify*() */
  CK_OBJECT_HANDLE
    sign_key_handle,                    /* Private key for C_Sign*() */
    verify_key_handle;                  /* Public  key for C_Verify() */
  hal_hash_handle_t
    digest_handle,                      /* Hash state for C_Digest*() */
    sign_digest_handle,                 /* Hash state for C_Sign*() */
    verify_digest_handle;               /* Hash state for C_Verify*() */
} p11_session_t;

/*
 * PKCS #11 objects.  These are pretty simple, as they're really just
 * mappings from PKCS #11's naming scheme to libhal UUIDs, with a little
 * extra fun for PKCS #11 "session" objects.
 */

typedef struct p11_object {
  CK_OBJECT_HANDLE  handle;             /* Object handle */
  CK_SESSION_HANDLE session;            /* Associated session (if any) */
  hal_uuid_t        uuid;               /* libhal key UUID */
} p11_object_t;

/*
 * PKCS #11 handle management.  PKCS #11 has two kinds of handles:
 * session handles and object handles.  We subdivide object handles
 * into token object handles (handles for objects which live on the
 * token, ie, in non-volatile storage) and session object handles
 * (handles for objects which live only as long as the session does),
 * and we steal two bits of the handle as as flags to distinguish
 * between these three kinds handles.  We sub-divide the rest of a
 * handle into a nonce (well, a lame one -- for now this is just a
 * counter, if this becomes an issue we could do better) and an array
 * index into the relevant table.
 */

typedef enum {
  handle_flavor_none            = 0, /* Matches CK_INVALID_HANDLE */
  handle_flavor_session         = 1,
  handle_flavor_token_object    = 2,
  handle_flavor_session_object  = 3
} handle_flavor_t;

#define HANDLE_MASK_FLAVOR      (0xc0000000)
#define HANDLE_MASK_NONCE       (0x3fff0000)
#define HANDLE_MASK_INDEX       (0x0000ffff)



/*
 * Current logged-in user.
 */

static enum {
  not_logged_in,
  logged_in_as_user,
  logged_in_as_so
} logged_in_as = not_logged_in;

/*
 * PKCS #11 sessions and object handles for this application.
 */

static p11_session_t p11_sessions     [P11_MAX_SESSION_HANDLES];
static p11_object_t  p11_objects      [P11_MAX_OBJECT_HANDLES];
static unsigned      p11_object_uuids [P11_MAX_OBJECT_HANDLES];

static unsigned p11_sessions_in_use, p11_objects_in_use;

/*
 * Mutex callbacks.
 */

static CK_CREATEMUTEX  mutex_cb_create;
static CK_DESTROYMUTEX mutex_cb_destroy;
static CK_LOCKMUTEX    mutex_cb_lock;
static CK_UNLOCKMUTEX  mutex_cb_unlock;

/*
 * Global mutex.  We may want something finer grained later, but this
 * will suffice to comply with the API requirements.
 */

static CK_VOID_PTR p11_global_mutex;

/*
 * (POSIX-specific) process which last called C_Initialize().
 */

#if USE_POSIX
static pid_t initialized_pid;
#endif



/*
 * Syntactic sugar for functions returning CK_RV complex enough to
 * need cleanup actions on failure.  Also does very basic logging for
 * debug-by-printf().
 *
 * NB: This uses a variable ("rv") and a goto target ("fail") which
 * must be defined in the calling environment.  We could make these
 * arguments to the macro, but doing so would make the code less
 * readable without significantly reducing the voodoo factor.
 */

#if DEBUG_PKCS11

#define lose(_ck_rv_code_)                                                      \
  do {                                                                          \
    rv = (_ck_rv_code_);                                                        \
    fprintf(stderr, "\n%s:%u: %s\n", __FILE__, __LINE__, #_ck_rv_code_);        \
    goto fail;                                                                  \
  } while (0)

#else  /* DEBUG_PKCS11 */

#define lose(_ck_rv_code_)                                                      \
  do {                                                                          \
    rv = (_ck_rv_code_);                                                        \
    goto fail;                                                                  \
  } while (0)

#endif  /* DEBUG_PKCS11 */

/*
 * More debug-by-printf() support.  One would like to consider this a
 * relic of the previous millenium, but, sadly, broken debugging
 * environments are still all too common.
 */

#if DEBUG_PKCS11 > 1

#define ENTER_PUBLIC_FUNCTION(_name_) \
  fprintf(stderr, "\nEntering function %s\n", #_name_)

#else  /* DEBUG_PKCS11 > 1 */

#define ENTER_PUBLIC_FUNCTION(_name_)

#endif  /* DEBUG_PKCS11 > 1 */

/*
 * Error checking for libhal calls.
 */

#define hal_whine(_expr_)            (_hal_whine((_expr_), #_expr_, __FILE__, __LINE__, HAL_OK))
#define hal_whine_allow(_expr_, ...) (_hal_whine((_expr_), #_expr_, __FILE__, __LINE__, __VA_ARGS__, HAL_OK))
#define hal_check(_expr_)            (hal_whine(_expr_) == HAL_OK)

#if DEBUG_HAL

static inline hal_error_t _hal_whine(const hal_error_t err,
                                     const char * const expr,
                                     const char * const file,
                                     const unsigned line, ...)
{
  va_list ap;
  int ok = 0;
  hal_error_t code;

  va_start(ap, line);
  do {
    code = va_arg(ap, hal_error_t);
    ok |= (err == code);
  } while (code != HAL_OK);
  va_end(ap);

  if (!ok)
    fprintf(stderr, "\n%s:%u: %s returned %s\n", file, line, expr, hal_error_string(err));

  return err;
}

#else /* DEBUG_HAL */

#define _hal_whine(_expr_, ...) (_expr_)

#endif /* DEBUG_HAL */

/*
 * Error translation fun for the entire family!
 */

#if DEBUG_PKCS11 || DEBUG_HAL

#define hal_p11_error_case(_hal_err_, _p11_err_) \
  case _hal_err_: fprintf(stderr, "\n%s:%u: Mapping %s to %s\n", file, line, #_hal_err_, #_p11_err_); return _p11_err_;

#else

#define hal_p11_error_case(_hal_err_, _p11_err_) \
  case _hal_err_: return _p11_err_;

#endif

#define p11_error_from_hal(_hal_err_) \
  (_p11_error_from_hal((_hal_err_), __FILE__, __LINE__))

#define p11_whine_from_hal(_expr_) \
  (_p11_error_from_hal(_hal_whine((_expr_), #_expr_, __FILE__, __LINE__, HAL_OK), __FILE__, __LINE__))

static CK_RV _p11_error_from_hal(const hal_error_t err, const char * const file, const unsigned line)
{
  switch (err) {
    hal_p11_error_case(HAL_ERROR_PIN_INCORRECT,         CKR_PIN_INCORRECT);
    hal_p11_error_case(HAL_ERROR_INVALID_SIGNATURE,     CKR_SIGNATURE_INVALID);

    /*
     * More here later, first see if this compiles.
     */

  case HAL_OK:
    return CKR_OK;

  default:
#if DEBUG_PKCS11 || DEBUG_HAL
    fprintf(stderr, "\n%s:%u: Mapping unhandled HAL error to CKR_FUNCTION_FAILED\n", file, line);
#endif
    return CKR_FUNCTION_FAILED;
  }
}

#undef hal_p11_error_case

/*
 * All (?) public functions should test whether we've been initialized or not.
 * Where practical, we bury this check in other boilerplate.
 */

#if USE_POSIX

#define p11_uninitialized()     (!initialized_pid)

#else

#define p11_uninitialized()     (0)

#endif

/*
 * Handle unsupported functions.
 */

#define UNSUPPORTED_FUNCTION(_name_)            \
  do {                                          \
    ENTER_PUBLIC_FUNCTION(_name_);              \
    if (p11_uninitialized())                    \
      return CKR_CRYPTOKI_NOT_INITIALIZED;      \
    return CKR_FUNCTION_NOT_SUPPORTED;          \
  } while (0)



/*
 * Thread mutex utilities.  We need to handle three separate cases:
 *
 * 1) User doesn't care about mutexes;
 * 2) User wants us to use "OS" mutexes;
 * 3) User wants us to use user-specified mutexs.
 *
 * For "OS" mutexes, read POSIX Threads mutexes, at least for now.
 *
 * PKCS #11 sort of has a fourth case, but it's really just license
 * for us to pick either the second or third case at whim.
 *
 * To simplify the rest of the API, we provide a POSIX-based
 * implementation which uses the same API an user-provided mutex
 * implementation would be required to use, use null function pointers
 * to represent the case where the user doesn't need mutexes at all,
 * and wrap the whole thing in trivial macros to insulate the rest of
 * the code from the grotty details.
 */

/*
 * Basic macros.
 */

#define mutex_create(_m_)   (mutex_cb_create  == NULL ? CKR_OK : mutex_cb_create(_m_))
#define mutex_destroy(_m_)  (mutex_cb_destroy == NULL ? CKR_OK : mutex_cb_destroy(_m_))
#define mutex_lock(_m_)     (mutex_cb_lock    == NULL ? CKR_OK : mutex_cb_lock(_m_))
#define mutex_unlock(_m_)   (mutex_cb_unlock  == NULL ? CKR_OK : mutex_cb_unlock(_m_))

/*
 * Slightly higher-level macros for common operations.
 *
 * Since the locking code depends on initialization,
 * attempting to lock anything when not initialized
 * is a failure, by definition.
 */

#define mutex_lock_or_return_failure(_m_)       \
  do {                                          \
    if (p11_uninitialized())                    \
      return CKR_CRYPTOKI_NOT_INITIALIZED;      \
    CK_RV _rv = mutex_lock(_m_);                \
    if (_rv != CKR_OK)                          \
      return _rv;                               \
  } while (0)

#define mutex_unlock_return_with_rv(_rv_, _m_)  \
  do {                                          \
    CK_RV _rv1 = _rv_;                          \
    CK_RV _rv2 = mutex_unlock(_m_);             \
    return _rv1 == CKR_OK ? _rv2 : _rv1;        \
  } while (0)

/*
 * Mutex implementation using POSIX mutexes.
 */

#if USE_PTHREADS

static CK_RV posix_mutex_create(CK_VOID_PTR_PTR ppMutex)
{
  pthread_mutex_t *m = NULL;
  CK_RV rv;

  if (ppMutex == NULL)
    lose(CKR_GENERAL_ERROR);

  if ((m = malloc(sizeof(*m))) == NULL)
    lose(CKR_HOST_MEMORY);

  switch (pthread_mutex_init(m, NULL)) {

  case 0:
    *ppMutex = m;
    return CKR_OK;

  case ENOMEM:
    lose(CKR_HOST_MEMORY);

  default:
    lose(CKR_GENERAL_ERROR);
  }

 fail:
  if (m != NULL)
    free(m);
  return rv;
}

static CK_RV posix_mutex_destroy(CK_VOID_PTR pMutex)
{
  CK_RV rv;

  if (pMutex == NULL)
    lose(CKR_MUTEX_BAD);

  switch (pthread_mutex_destroy(pMutex)) {

  case 0:
    free(pMutex);
    return CKR_OK;

  case EINVAL:
    lose(CKR_MUTEX_BAD);

  case EBUSY:
    /*
     * PKCS #11 mutex semantics are a bad match for POSIX here,
     * leaving us only the nuclear option.  Feh.  Fall through.
     */

  default:
    lose(CKR_GENERAL_ERROR);
  }

 fail:
  return rv;
}

static CK_RV posix_mutex_lock(CK_VOID_PTR pMutex)
{
  CK_RV rv;

  if (pMutex == NULL)
    lose(CKR_MUTEX_BAD);

  switch (pthread_mutex_lock(pMutex)) {

  case 0:
    return CKR_OK;

  case EINVAL:
    lose(CKR_MUTEX_BAD);

  default:
    lose(CKR_GENERAL_ERROR);
  }

 fail:
  return rv;
}

static CK_RV posix_mutex_unlock(CK_VOID_PTR pMutex)
{
  CK_RV rv;

  if (pMutex == NULL)
    lose(CKR_MUTEX_BAD);

  switch (pthread_mutex_unlock(pMutex)) {

  case 0:
    return CKR_OK;

  case EINVAL:
    lose(CKR_MUTEX_BAD);

  case EPERM:
    lose(CKR_MUTEX_NOT_LOCKED);

  default:
    lose(CKR_GENERAL_ERROR);
  }

 fail:
  return rv;
}

#endif /* USE_PTHREADS */



/*
 * Bit mask twiddling utilities.
 */

static inline CK_ULONG mask_pos(const CK_ULONG mask)
{
  return mask & ~(mask - 1);    /* Finds least significant bit set in mask */
}

static inline CK_ULONG mask_ldb(const CK_ULONG mask, const CK_ULONG value)
{
  return (value & mask) / mask_pos(mask);
}

static inline CK_ULONG mask_dpb(const CK_ULONG mask, const CK_ULONG value)
{
  return (value * mask_pos(mask)) & mask;
}

/*
 * Translate between libhal EC curve names and OIDs.
 */
#warning Perhaps this should be  a utility routine in libhal instead of here

static int ec_curve_oid_to_name(const uint8_t * const oid, const size_t oid_len, hal_curve_name_t *curve)
{
  static uint8_t ec_curve_oid_p256[] = { 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07 };
  static uint8_t ec_curve_oid_p384[] = { 0x06, 0x05, 0x2b, 0x81, 0x04, 0x00, 0x22 };
  static uint8_t ec_curve_oid_p521[] = { 0x06, 0x05, 0x2b, 0x81, 0x04, 0x00, 0x23 };

  if (oid == NULL || curve == NULL)
    return 0;

  else if (oid_len == sizeof(ec_curve_oid_p256) && memcmp(oid, ec_curve_oid_p256, oid_len) == 0)
    *curve = HAL_CURVE_P256;

  else if (oid_len == sizeof(ec_curve_oid_p384) && memcmp(oid, ec_curve_oid_p384, oid_len) == 0)
    *curve = HAL_CURVE_P384;

  else if (oid_len == sizeof(ec_curve_oid_p521) && memcmp(oid, ec_curve_oid_p521, oid_len) == 0)
    *curve = HAL_CURVE_P521;

  else
    return 0;

  return 1;
}

/*
 * Extract libhal-compatible client and session identifiers from a session.
 *
 * libhal's session identifiers are deliberately chosen to be in the same
 * numeric range as PKCS #11's, so we can just use them directly.
 *
 * libhal's client identifiers are multiplexing extension handled elsewhere,
 * for our purposes using constant client identifier of zero will do.
 */

static inline hal_client_handle_t p11_session_hal_client(const p11_session_t * const session)
{
  hal_client_handle_t handle = {0};
  return handle;
}

static inline hal_session_handle_t p11_session_hal_session(const p11_session_t * const session)
{
  hal_session_handle_t handle = {session->handle};
  return handle;
}

/*
 * Handle utilities.
 */

static inline CK_ULONG handle_compose(const handle_flavor_t flavor,
                                      const unsigned nonce,
                                      const unsigned index)
{
  return (mask_dpb(HANDLE_MASK_FLAVOR, flavor) |
          mask_dpb(HANDLE_MASK_NONCE,  nonce)  |
          mask_dpb(HANDLE_MASK_INDEX,  index));
}

static inline handle_flavor_t handle_flavor(const CK_ULONG handle)
{
  return (handle_flavor_t) mask_ldb(HANDLE_MASK_FLAVOR, handle);
}

static inline unsigned handle_index(const CK_ULONG handle)
{
  return mask_ldb(HANDLE_MASK_INDEX, handle);
}



/*
 * Descriptor methods.  Descriptors are generated at compile time by
 * an auxiliary Python script, see attributes.* for details.
 */

/*
 * Return the descriptor associated with a particular object class and
 * key type.
 */

static const p11_descriptor_t *p11_descriptor_from_key_type(const CK_OBJECT_CLASS object_class,
                                                            const CK_KEY_TYPE key_type)
{
  int i;

  for (i = 0; i < sizeof(p11_descriptor_keyclass_map)/sizeof(*p11_descriptor_keyclass_map); i++) {
    const p11_descriptor_keyclass_map_t * const m = &p11_descriptor_keyclass_map[i];
    if (m->object_class == object_class && m->key_type == key_type)
      return m->descriptor;
  }

  return NULL;
}

/*
 * Find the entry for a particular attribute in a descriptor.
 */

static const p11_attribute_descriptor_t *p11_find_attribute_in_descriptor(const p11_descriptor_t *descriptor,
                                                                          const CK_ATTRIBUTE_TYPE type)
{
  int i;

  if (descriptor != NULL && descriptor->attributes != NULL)
    for (i = 0; i < descriptor->n_attributes; i++)
      if (descriptor->attributes[i].type == type)
        return &descriptor->attributes[i];

  return NULL;
}

/*
 * Check whether an attribute is marked as sensitive.  If we don't
 * recognize the attribute, report it as sensitive (safer than the
 * alternative).
 */

static int p11_attribute_is_sensitive(const p11_descriptor_t *descriptor,
                                      const CK_ATTRIBUTE_TYPE type)
{
  const p11_attribute_descriptor_t *a = p11_find_attribute_in_descriptor(descriptor, type);
  return a == NULL || (a->flags & P11_DESCRIPTOR_SENSITIVE) != 0;
}



/*
 * Attribute methods.
 */

/*
 * Find an attribute in a CK_ATTRIBUTE_PTR template.  Returns index
 * into template, or -1 if not found.
 */

static int p11_attribute_find_in_template(const CK_ATTRIBUTE_TYPE type,
                                          const CK_ATTRIBUTE_PTR template,
                                          const CK_ULONG length)
{
  if (template != NULL)
    for (int i = 0; i < length; i++)
      if (template[i].type == type)
        return i;

  return -1;
}

/*
 * Find an attribute in a CK_ATTRIBUTE_PTR template.  Returns pointer
 * to attribute value, or NULL if not found.
 */

static void *p11_attribute_find_value_in_template(const CK_ATTRIBUTE_TYPE type,
                                                  const CK_ATTRIBUTE_PTR template,
                                                  const CK_ULONG length)
{
  const int i = p11_attribute_find_in_template(type, template, length);
  return i < 0 ? NULL : template[i].pValue;
}

/*
 * Idiom for combination of p11_attribute_find_value_in_template() and
 * p11_find_attribute_in_descriptor().
 */

static const void *
p11_attribute_find_value_in_template_or_descriptor(const p11_descriptor_t *descriptor,
                                                   const CK_ATTRIBUTE_TYPE type,
                                                   const CK_ATTRIBUTE_PTR template,
                                                   const CK_ULONG length)
{
  const int i = p11_attribute_find_in_template(type, template, length);
  if (i >= 0)
    return template[i].pValue;
  const p11_attribute_descriptor_t * const atd = p11_find_attribute_in_descriptor(descriptor, type);
  assert(atd != NULL);
  return atd->value;
}

/*
 * Set attributes for a newly-created or newly-uploaded HSM key.
 */

static int p11_attributes_set(const hal_pkey_handle_t pkey,
                              const CK_ATTRIBUTE_PTR template,
                              const CK_ULONG template_length,
                              const p11_descriptor_t * const descriptor,
                              const hal_pkey_attribute_t *extra,
                              const unsigned extra_length)
{
  assert(template != NULL && descriptor != NULL && (extra_length == 0 || extra != NULL));

  /*
   * Populate attributes, starting with the application's template,
   * which we assume has already been blessed by the API function that
   * called this method.
   *
   * If the attribute is flagged as sensitive in the descriptor, we
   * don't store it as an attribute.  Generally, this only arises for
   * private key components of objects created with C_CreateObject(),
   * but in theory there are some corner cases in which a user could
   * choose to mark a private key as extractable and not sensitive, so
   * we might have to back-fill missing values in those cases if
   * anyone ever thinks up a sane reason for supporting them.  For
   * now, assume that private keys are bloody well supposed to be
   * private.
   */

  hal_pkey_attribute_t attributes[template_length + descriptor->n_attributes + extra_length];
  unsigned n = 0;

  for (int i = 0; i < template_length; i++) {
    const CK_ATTRIBUTE_TYPE type = template[i].type;
    const void *             val = template[i].pValue;
    const int                len = template[i].ulValueLen;

    if (p11_attribute_is_sensitive(descriptor, type))
      continue;

    if (n >= sizeof(attributes) / sizeof(*attributes))
      return 0;

    attributes[n].type   = type;
    attributes[n].value  = val;
    attributes[n].length = len;
    n++;
  }

  /*
   * Next, add defaults from the descriptor.
   */

  for (int i = 0; i < descriptor->n_attributes; i++) {
    const CK_ATTRIBUTE_TYPE type = descriptor->attributes[i].type;
    const void *             val = descriptor->attributes[i].value;
    const int                len = descriptor->attributes[i].length;
    const unsigned         flags = descriptor->attributes[i].flags;

    if (val == NULL && (flags & P11_DESCRIPTOR_DEFAULT_VALUE) != 0)
      val = "";

    if (val == NULL || p11_attribute_find_in_template(type, template, template_length) >= 0)
      continue;

    if (n >= sizeof(attributes) / sizeof(*attributes))
      return 0;

    attributes[n].type   = type;
    attributes[n].value  = val;
    attributes[n].length = len;
    n++;
  }

  /*
   * Finally, add any attributes provided by the calling function itself.
   */

  for (int i = 0; i < extra_length; i++) {
    if (n >= sizeof(attributes) / sizeof(*attributes))
      return 0;

    attributes[n].type   = extra[i].type;
    attributes[n].value  = extra[i].value;
    attributes[n].length = extra[i].length;
    n++;
  }

  /*
   * Set all those attributes.
   */

  if (!hal_check(hal_rpc_pkey_set_attributes(pkey, attributes, n)))
    return 0;

  return 1;
}

/*
 * Map a keyusage-related attribute to a keyusage bit flag.
 *
 * Assumes that calling code has already checked whether this
 * attribute is legal for this object class, that attribute which
 * should be CK_BBOOLs are of the correct length, etcetera.
 *
 * To handle all the possible permutations of specified and default
 * values, it may be necessary to defer calling this method until
 * after the default and mandatory values have been merged into the
 * values supplied by the application-supplied template.
 *
 * Semantics of the flags follow RFC 5280 4.2.1.3.  Numeric values
 * don't matter particularly as we only use them internally so we
 * can simplify things a bit by reusing libhal's flag values.
 */

static void p11_attribute_apply_keyusage(hal_key_flags_t *keyusage, const CK_ATTRIBUTE_TYPE type, const CK_BBOOL *value)
{
  unsigned flag;

  assert(keyusage != NULL && value != NULL);

  switch (type) {
  case CKA_SIGN:                /* Generate signature */
  case CKA_VERIFY:              /* Verify signature */
    flag = HAL_KEY_FLAG_USAGE_DIGITALSIGNATURE;
    break;
  case CKA_ENCRYPT:             /* Encrypt bulk data (seldom used) */
  case CKA_DECRYPT:             /* Bulk decryption (seldom used) */
    flag = HAL_KEY_FLAG_USAGE_DATAENCIPHERMENT;
    break;
  case CKA_WRAP:                /* Wrap key (normal way of doing encryption) */
  case CKA_UNWRAP:              /* Unwrap key (normal way of doing decryption) */
    flag = HAL_KEY_FLAG_USAGE_KEYENCIPHERMENT;
    break;
  default:
    return;                     /* Attribute not related to key usage */
  }

  if (*value)
    *keyusage |=  flag;
  else
    *keyusage &= ~flag;
}



/*
 * Access rights.
 */

static CK_RV p11_check_read_access(const p11_session_t *session,
                                   const CK_BBOOL cka_private,
                                   const CK_BBOOL cka_token)
{
  if (session == NULL)
    return CKR_SESSION_HANDLE_INVALID;

  switch (session->state) {

  case CKS_RO_PUBLIC_SESSION:
    /* RO access to public token objects, RW access to public session objects */
    return (cka_private) ? CKR_OBJECT_HANDLE_INVALID : CKR_OK;

  case CKS_RO_USER_FUNCTIONS:
    /* RO access to all token objects, RW access to all session objects */
    return CKR_OK;

  case CKS_RW_PUBLIC_SESSION:
    /* RW access all public objects */
    return (cka_private) ? CKR_OBJECT_HANDLE_INVALID : CKR_OK;

  case CKS_RW_USER_FUNCTIONS:
    /* RW acess to all objects */
    return CKR_OK;

  case CKS_RW_SO_FUNCTIONS:
    /* RW access to public token objects only */
    return (cka_private || ! cka_token) ? CKR_OBJECT_HANDLE_INVALID : CKR_OK;
  }

  return CKR_SESSION_HANDLE_INVALID;
}

static CK_RV p11_check_write_access(const p11_session_t *session,
                                    const CK_BBOOL cka_private,
                                    const CK_BBOOL cka_token)
{
  if (session == NULL)
    return CKR_SESSION_HANDLE_INVALID;

  switch (session->state) {

  case CKS_RO_PUBLIC_SESSION:
    /* RO access to public token objects, RW access to public session objects */
    return (cka_private || cka_token) ? CKR_USER_NOT_LOGGED_IN : CKR_OK;

  case CKS_RO_USER_FUNCTIONS:
    /* RO access to all token objects, RW access to all session objects */
    return (cka_token) ? CKR_SESSION_READ_ONLY : CKR_OK;

  case CKS_RW_PUBLIC_SESSION:
    /* RW access all public objects */
    return (cka_private) ? CKR_USER_NOT_LOGGED_IN : CKR_OK;

  case CKS_RW_USER_FUNCTIONS:
    /* RW acess to all objects */
    return CKR_OK;

  case CKS_RW_SO_FUNCTIONS:
    /* RW access to public token objects only */
    return (cka_private || ! cka_token) ? CKR_USER_NOT_LOGGED_IN : CKR_OK;
  }

  return CKR_SESSION_HANDLE_INVALID;
}



/*
 * Object methods.
 */

/*
 * Look up an object's UUID in the object index table, return
 * indication of whether it's present or not and the position it
 * should occupy within the index table in either case.
 *
 * NB: *where is a position in p11_object_uuids[], not p11_objects[].
 */

static int p11_object_uuid_bsearch(const hal_uuid_t * const uuid, int *where)
{
  assert(uuid != NULL && where != NULL);

  int lo = -1;
  int hi = p11_objects_in_use;

  for (;;) {
    int m = (lo + hi) / 2;
    if (hi == 0 || m == lo) {
      *where = hi;
      return 0;
    }
    const int cmp = hal_uuid_cmp(uuid, &p11_objects[p11_object_uuids[m]].uuid);
    if (cmp < 0)
      hi = m;
    else if (cmp > 0)
      lo = m;
    else {
      *where = m;
      return 1;
    }
  }
}

/*
 * Allocate a new object.
 */

static CK_OBJECT_HANDLE p11_object_allocate(const handle_flavor_t flavor,
                                            const hal_uuid_t *uuid,
                                            const p11_session_t *session)
{
  if (uuid == NULL)
    return CK_INVALID_HANDLE;

  if (flavor != handle_flavor_token_object && flavor != handle_flavor_session_object)
    return CK_INVALID_HANDLE;

  int where;

  if (p11_object_uuid_bsearch(uuid, &where)) {
    assert(where >= 0 && where < p11_objects_in_use);
    const CK_OBJECT_HANDLE handle = p11_objects[p11_object_uuids[where]].handle;
    return handle_flavor(handle) == flavor ? handle : CK_INVALID_HANDLE;
  }

  if (p11_objects_in_use >= sizeof(p11_objects) / sizeof(*p11_objects))
    return CK_INVALID_HANDLE;

  static unsigned next_index, nonce;
  const unsigned  last_index = next_index;
  p11_object_t *object = NULL;

  do {

    next_index = (next_index + 1) % (sizeof(p11_objects) / sizeof(*p11_objects));

    if (next_index == last_index)
      return CK_INVALID_HANDLE;

    if (next_index == 0)
      ++nonce;

    object = &p11_objects[next_index];

  } while (object->handle != CK_INVALID_HANDLE);

  object->handle = handle_compose(flavor, nonce, next_index);
  object->uuid = *uuid;
  object->session = flavor == handle_flavor_session_object ? session->handle : CK_INVALID_HANDLE;

  if (where < p11_objects_in_use)
    memmove(&p11_object_uuids[where + 1], &p11_object_uuids[where],
            (p11_objects_in_use - where) * sizeof(*p11_object_uuids));

  p11_object_uuids[where] = next_index;

  p11_objects_in_use++;

  return object->handle;
}

/*
 * Free an object.
 */

static void p11_object_free(p11_object_t *object)
{
  if (object == NULL)
    return;

  int where;

  if (p11_objects_in_use > 0 &&
      p11_object_uuid_bsearch(&object->uuid, &where) &&
      --p11_objects_in_use > where)
    memmove(&p11_object_uuids[where], &p11_object_uuids[where + 1],
            (p11_objects_in_use - where) * sizeof(*p11_object_uuids));

  memset(object, 0, sizeof(*object));
}

/*
 * Find an object given its UUID.
 */

static p11_object_t *p11_object_by_uuid(const hal_uuid_t * const uuid)
{
  int where;

  if (uuid == NULL || !p11_object_uuid_bsearch(uuid, &where))
    return NULL;

  assert(where >= 0 && where < p11_objects_in_use);

  p11_object_t *object = &p11_objects[p11_object_uuids[where]];

  if (handle_flavor(object->handle) != handle_flavor_session_object &&
      handle_flavor(object->handle) != handle_flavor_token_object)
    return NULL;

  return object;
}

/*
 * Find an object given its handle.
 */

static p11_object_t *p11_object_by_handle(const CK_OBJECT_HANDLE object_handle)
{
  if (handle_flavor(object_handle) != handle_flavor_session_object &&
      handle_flavor(object_handle) != handle_flavor_token_object)
    return NULL;

  const unsigned index = handle_index(object_handle);

  if (index >= sizeof(p11_objects) / sizeof(*p11_objects))
    return NULL;

  p11_object_t *object = &p11_objects[index];

  if (object->handle != object_handle)
    return NULL;

  return object;
}

/*
 * Translate CKA_TOKEN value to handle flavor.
 */

static inline handle_flavor_t p11_object_flavor_from_cka_token(const CK_BBOOL *bbool)
{
  return (bbool != NULL && *bbool) ? handle_flavor_token_object : handle_flavor_session_object;
}

/*
 * Open the HSM pkey object (if any) corresponding to the PKCS #11 handle.
 */

static int p11_object_pkey_open(const p11_session_t *session,
                                const CK_OBJECT_HANDLE object_handle,
                                hal_pkey_handle_t *pkey)
{
  const p11_object_t *object = p11_object_by_handle(object_handle);

  return (session != NULL && pkey != NULL && object != NULL &&
          hal_check(hal_rpc_pkey_open(p11_session_hal_client(session),
                                      p11_session_hal_session(session),
                                      pkey, &object->uuid)));
}

/*
 * Create pkeys to go with PKCS #11 key objects loaded by C_CreateObject().
 */

static int p11_object_create_rsa_public_key(const p11_session_t * const session,
                                            const handle_flavor_t flavor,
                                            const CK_ATTRIBUTE_PTR template,
                                            const CK_ULONG template_len,
                                            const p11_descriptor_t * const descriptor,
                                            CK_OBJECT_HANDLE_PTR phObject,
                                            const hal_key_flags_t flags)
{
  const hal_pkey_attribute_t extra[] = {
    {.type = CKA_LOCAL, .value = &const_CK_FALSE, .length = sizeof(const_CK_FALSE)}
  };

  hal_pkey_handle_t pkey = {HAL_HANDLE_NONE};
  uint8_t keybuf[hal_rsa_key_t_size];
  hal_rsa_key_t *key = NULL;
  hal_uuid_t uuid;

  const uint8_t *cka_modulus = NULL;
  size_t cka_modulus_len = 0;
  const uint8_t *cka_public_exponent = const_0x010001;
  size_t cka_public_exponent_len = sizeof(const_0x010001);

  for (int i = 0; i < template_len; i++) {
    const void * const val = template[i].pValue;
    const size_t       len = template[i].ulValueLen;
    switch (template[i].type) {
    case CKA_MODULUS:          cka_modulus          = val; cka_modulus_len          = len; break;
    case CKA_PUBLIC_EXPONENT:  cka_public_exponent  = val; cka_public_exponent_len  = len; break;
    }
  }

  int ok = hal_check(hal_rsa_key_load_public(&key, keybuf, sizeof(keybuf),
                                             cka_modulus, cka_modulus_len,
                                             cka_public_exponent, cka_public_exponent_len));

  if (ok) {
    uint8_t der[hal_rsa_public_key_to_der_len(key)];
    ok = (hal_check(hal_rsa_public_key_to_der(key, der, NULL, sizeof(der))) &&
          hal_check(hal_rpc_pkey_load(p11_session_hal_client(session),
                                      p11_session_hal_session(session),
                                      &pkey, &uuid, der, sizeof(der), flags)));
  }

  if (ok)
    ok = p11_attributes_set(pkey, template, template_len, descriptor,
                            extra, sizeof(extra)/sizeof(*extra));

  if (ok) {
    *phObject = p11_object_allocate(flavor, &uuid, session);
    ok = *phObject != CK_INVALID_HANDLE;
  }

  if (!ok && pkey.handle != HAL_HANDLE_NONE)
    (void) hal_rpc_pkey_delete(pkey);
  else
    (void) hal_rpc_pkey_close(pkey);

  return ok;
}

static int p11_object_create_ec_public_key(const p11_session_t * const session,
                                           const handle_flavor_t flavor,
                                           const CK_ATTRIBUTE_PTR template,
                                           const CK_ULONG template_len,
                                           const p11_descriptor_t * const descriptor,
                                           CK_OBJECT_HANDLE_PTR phObject,
                                           const hal_key_flags_t flags)
{
  const hal_pkey_attribute_t extra[] = {
    {.type = CKA_LOCAL, .value = &const_CK_FALSE, .length = sizeof(const_CK_FALSE)}
  };

  hal_pkey_handle_t pkey = {HAL_HANDLE_NONE};
  uint8_t keybuf[hal_ecdsa_key_t_size];
  hal_ecdsa_key_t *key = NULL;
  hal_curve_name_t curve;
  hal_uuid_t uuid;

  const uint8_t *cka_ec_point  = NULL;  size_t cka_ec_point_len  = 0;
  const uint8_t *cka_ec_params = NULL;  size_t cka_ec_params_len = 0;

  for (int i = 0; i < template_len; i++) {
    const void * const val = template[i].pValue;
    const size_t       len = template[i].ulValueLen;
    switch (template[i].type) {
    case CKA_EC_POINT:  cka_ec_point  = val; cka_ec_point_len  = len; break;
    case CKA_EC_PARAMS: cka_ec_params = val; cka_ec_params_len = len; break;
    }
  }

  int ok
    = (ec_curve_oid_to_name(cka_ec_params, cka_ec_params_len, &curve) &&
       hal_check(hal_ecdsa_key_from_ecpoint(&key, keybuf, sizeof(keybuf),
                                            cka_ec_point, cka_ec_point_len,
                                            curve)));

  if (ok) {
    uint8_t der[hal_ecdsa_public_key_to_der_len(key)];
    ok = (hal_check(hal_ecdsa_public_key_to_der(key, der, NULL, sizeof(der))) &&
          hal_check(hal_rpc_pkey_load(p11_session_hal_client(session),
                                      p11_session_hal_session(session),
                                      &pkey, &uuid, der, sizeof(der), flags)));
  }

  if (ok)
    ok = p11_attributes_set(pkey, template, template_len, descriptor,
                            extra, sizeof(extra)/sizeof(*extra));

  if (ok) {
    *phObject = p11_object_allocate(flavor, &uuid, session);
    ok = *phObject != CK_INVALID_HANDLE;
  }

  if (!ok && pkey.handle != HAL_HANDLE_NONE)
    (void) hal_rpc_pkey_delete(pkey);
  else
    (void) hal_rpc_pkey_close(pkey);

  return ok;
}

static int p11_object_create_rsa_private_key(const p11_session_t * const session,
                                             const handle_flavor_t flavor,
                                             const CK_ATTRIBUTE_PTR template,
                                             const CK_ULONG template_len,
                                             const p11_descriptor_t * const descriptor,
                                             CK_OBJECT_HANDLE_PTR phObject,
                                             const hal_key_flags_t flags)
{
  const hal_pkey_attribute_t extra[] = {
    {.type = CKA_LOCAL,             .value = &const_CK_FALSE, .length = sizeof(const_CK_FALSE)},
    {.type = CKA_ALWAYS_SENSITIVE,  .value = &const_CK_FALSE, .length = sizeof(const_CK_FALSE)},
    {.type = CKA_NEVER_EXTRACTABLE, .value = &const_CK_FALSE, .length = sizeof(const_CK_FALSE)}
  };

  hal_pkey_handle_t pkey = {HAL_HANDLE_NONE};
  uint8_t keybuf[hal_rsa_key_t_size];
  hal_rsa_key_t *key = NULL;
  hal_uuid_t uuid;

  const uint8_t *cka_modulus          = NULL;   size_t cka_modulus_len          = 0;
  const uint8_t *cka_private_exponent = NULL;   size_t cka_private_exponent_len = 0;
  const uint8_t *cka_prime_1          = NULL;   size_t cka_prime_1_len          = 0;
  const uint8_t *cka_prime_2          = NULL;   size_t cka_prime_2_len          = 0;
  const uint8_t *cka_exponent_1       = NULL;   size_t cka_exponent_1_len       = 0;
  const uint8_t *cka_exponent_2       = NULL;   size_t cka_exponent_2_len       = 0;
  const uint8_t *cka_coefficient      = NULL;   size_t cka_coefficient_len      = 0;

  const uint8_t *cka_public_exponent = const_0x010001;
  size_t cka_public_exponent_len = sizeof(const_0x010001);

  for (int i = 0; i < template_len; i++) {
    const void * const val = template[i].pValue;
    const size_t       len = template[i].ulValueLen;
    switch (template[i].type) {
    case CKA_MODULUS:          cka_modulus          = val; cka_modulus_len          = len; break;
    case CKA_PUBLIC_EXPONENT:  cka_public_exponent  = val; cka_public_exponent_len  = len; break;
    case CKA_PRIVATE_EXPONENT: cka_private_exponent = val; cka_private_exponent_len = len; break;
    case CKA_PRIME_1:          cka_prime_1          = val; cka_prime_1_len          = len; break;
    case CKA_PRIME_2:          cka_prime_2          = val; cka_prime_2_len          = len; break;
    case CKA_EXPONENT_1:       cka_exponent_1       = val; cka_exponent_1_len       = len; break;
    case CKA_EXPONENT_2:       cka_exponent_2       = val; cka_exponent_2_len       = len; break;
    case CKA_COEFFICIENT:      cka_coefficient      = val; cka_coefficient_len      = len; break;
    }
  }

  int ok = hal_check(hal_rsa_key_load_private(&key, keybuf, sizeof(keybuf),
                                              cka_modulus,                  cka_modulus_len,
                                              cka_public_exponent,          cka_public_exponent_len,
                                              cka_private_exponent,         cka_private_exponent_len,
                                              cka_prime_1,                  cka_prime_1_len,
                                              cka_prime_2,                  cka_prime_2_len,
                                              cka_coefficient,              cka_coefficient_len,
                                              cka_exponent_1,               cka_exponent_1_len,
                                              cka_exponent_2,               cka_exponent_2_len));
  if (ok) {
    uint8_t der[hal_rsa_private_key_to_der_len(key)];
    ok = (hal_check(hal_rsa_private_key_to_der(key, der, NULL, sizeof(der))) &&
          hal_check(hal_rpc_pkey_load(p11_session_hal_client(session),
                                      p11_session_hal_session(session),
                                      &pkey, &uuid, der, sizeof(der), flags)));
    memset(der, 0, sizeof(der));
  }

  memset(keybuf, 0, sizeof(keybuf));

  if (ok)
    ok = p11_attributes_set(pkey, template, template_len, descriptor,
                            extra, sizeof(extra)/sizeof(*extra));

  if (ok) {
    *phObject = p11_object_allocate(flavor, &uuid, session);
    ok = *phObject != CK_INVALID_HANDLE;
  }

  if (!ok && pkey.handle != HAL_HANDLE_NONE)
    (void) hal_rpc_pkey_delete(pkey);
  else
    (void) hal_rpc_pkey_close(pkey);

  return ok;
}

static int p11_object_create_ec_private_key(const p11_session_t * const session,
                                            const handle_flavor_t flavor,
                                            const CK_ATTRIBUTE_PTR template,
                                            const CK_ULONG template_len,
                                            const p11_descriptor_t * const descriptor,
                                            CK_OBJECT_HANDLE_PTR phObject,
                                            const hal_key_flags_t flags)
{
  const hal_pkey_attribute_t extra[] = {
    {.type = CKA_LOCAL,             .value = &const_CK_FALSE, .length = sizeof(const_CK_FALSE)},
    {.type = CKA_ALWAYS_SENSITIVE,  .value = &const_CK_FALSE, .length = sizeof(const_CK_FALSE)},
    {.type = CKA_NEVER_EXTRACTABLE, .value = &const_CK_FALSE, .length = sizeof(const_CK_FALSE)}
  };

  hal_pkey_handle_t pkey = {HAL_HANDLE_NONE};
  uint8_t keybuf[hal_ecdsa_key_t_size];
  hal_ecdsa_key_t *key = NULL;
  hal_curve_name_t curve;
  hal_uuid_t uuid;

  const uint8_t *cka_value     = NULL;  size_t cka_value_len     = 0;
  const uint8_t *cka_ec_point  = NULL;  size_t cka_ec_point_len  = 0;
  const uint8_t *cka_ec_params = NULL;  size_t cka_ec_params_len = 0;

  for (int i = 0; i < template_len; i++) {
    const void * const val = template[i].pValue;
    const size_t       len = template[i].ulValueLen;
    switch (template[i].type) {
    case CKA_VALUE:     cka_value     = val; cka_value_len     = len; break;
    case CKA_EC_POINT:  cka_ec_point  = val; cka_ec_point_len  = len; break;
    case CKA_EC_PARAMS: cka_ec_params = val; cka_ec_params_len = len; break;
    }
  }

  int ok
    = (ec_curve_oid_to_name(cka_ec_params, cka_ec_params_len, &curve) &&
       hal_check(hal_ecdsa_key_load_private(&key, keybuf, sizeof(keybuf), curve,
                                            cka_ec_point + 1 + 0 * cka_ec_point_len / 2,
                                            cka_ec_point_len / 2,
                                            cka_ec_point + 1 + 1 * cka_ec_point_len / 2,
                                            cka_ec_point_len / 2,
                                            cka_value,
                                            cka_value_len)));

  if (ok) {
    uint8_t der[hal_ecdsa_private_key_to_der_len(key)];
    ok = (hal_check(hal_ecdsa_private_key_to_der(key, der, NULL, sizeof(der))) &&
          hal_check(hal_rpc_pkey_load(p11_session_hal_client(session),
                                      p11_session_hal_session(session),
                                      &pkey, &uuid, der, sizeof(der), flags)));
    memset(der, 0, sizeof(der));
  }

  memset(keybuf, 0, sizeof(keybuf));

  if (ok)
    ok = p11_attributes_set(pkey, template, template_len, descriptor,
                            extra, sizeof(extra)/sizeof(*extra));

  if (ok) {
    *phObject = p11_object_allocate(flavor, &uuid, session);
    ok = *phObject != CK_INVALID_HANDLE;
  }

  if (!ok && pkey.handle != HAL_HANDLE_NONE)
    (void) hal_rpc_pkey_delete(pkey);
  else
    (void) hal_rpc_pkey_close(pkey);

  return ok;
}



/*
 * Session methods.
 */

/*
 * Create a new session.
 */

static p11_session_t *p11_session_allocate(void)
{
  static unsigned next_index, nonce;
  const unsigned  last_index = next_index;
  p11_session_t *session = NULL;

  if (p11_sessions_in_use >= sizeof(p11_sessions) / sizeof(*p11_sessions))
    return NULL;

  do {

    next_index = (next_index + 1) % (sizeof(p11_sessions) / sizeof(*p11_sessions));

    if (next_index == last_index)
      return NULL;

    if (next_index == 0)
      ++nonce;

    session = &p11_sessions[next_index];

  } while (session->handle != CK_INVALID_HANDLE);

  memset(session, 0, sizeof(*session));
  session->handle = handle_compose(handle_flavor_session, nonce, next_index);
  p11_sessions_in_use++;
  return session;
}

/*
 * Free a session.
 */

static void p11_session_free(p11_session_t *session)
{
  if (session == NULL)
    return;

  assert(p11_sessions_in_use > 0);

  if (session->find_query)
    free(session->find_query);

  (void) hal_rpc_hash_finalize(session->digest_handle, NULL, 0);
  (void) hal_rpc_hash_finalize(session->sign_digest_handle, NULL, 0);
  (void) hal_rpc_hash_finalize(session->verify_digest_handle, NULL, 0);

  memset(session, 0, sizeof(*session));

  if (--p11_sessions_in_use == 0)
    logged_in_as = not_logged_in;
}

/*
 * Find a session.
 */

static p11_session_t *p11_session_find(const CK_SESSION_HANDLE session_handle)
{
  if (handle_flavor(session_handle) != handle_flavor_session)
    return NULL;

  const unsigned index = handle_index(session_handle);

  if (index >= sizeof(p11_sessions) / sizeof(*p11_sessions))
    return NULL;

  p11_session_t *session = &p11_sessions[index];

  if (session->handle != session_handle)
    return NULL;

  return session;
}

/*
 * Iterate over session handles.  Start with CK_INVALID_HANDLE,
 * returns CK_INVALID_HANDLE when done.
 *
 * This does not verify the provided session handle, because we want
 * to be able to modify the sessions this finds, including deleting
 * them (which invalidates the session handle).  Don't trust the
 * returned handle until it has been blessed by p11_session_find().
 */

static CK_SESSION_HANDLE p11_session_handle_iterate(const CK_SESSION_HANDLE session_handle)
{
  unsigned index;

  if (session_handle == CK_INVALID_HANDLE)
    index = 0;

  else if (handle_flavor(session_handle) == handle_flavor_session)
    index = handle_index(session_handle) + 1;

  else
    return CK_INVALID_HANDLE;

  for (; index < sizeof(p11_sessions) / sizeof(*p11_sessions); index++)
    if (handle_flavor(p11_sessions[index].handle) == handle_flavor_session)
      return p11_sessions[index].handle;

  return CK_INVALID_HANDLE;
}

/*
 * Same thing, but return session objects instead of session handles.
 * This is just syntactic sugar around a common idiom.
 */

static p11_session_t *p11_session_iterate(p11_session_t *session)
{
  const CK_SESSION_HANDLE handle = session == NULL ? CK_INVALID_HANDLE : session->handle;
  return p11_session_find(p11_session_handle_iterate(handle));
}

/*
 * Delete all sessions.  Have to use p11_session_handle_iterate() here.
 */

static void p11_session_free_all(void)
{
  for (CK_SESSION_HANDLE handle = p11_session_handle_iterate(CK_INVALID_HANDLE);
       handle != CK_INVALID_HANDLE; handle = p11_session_handle_iterate(handle))
    p11_session_free(p11_session_find(handle));
}

/*
 * Check session database against login state for consistency.
 *
 * This is mostly useful in assertions.
 */

static int p11_session_consistent_login(void)
{
  switch (logged_in_as) {

  case not_logged_in:
    for (p11_session_t *session = p11_session_iterate(NULL);
         session != NULL; session = p11_session_iterate(session))
      if (session->state != CKS_RO_PUBLIC_SESSION && session->state != CKS_RW_PUBLIC_SESSION)
        return 0;
    return 1;

  case logged_in_as_user:
    for (p11_session_t *session = p11_session_iterate(NULL);
         session != NULL; session = p11_session_iterate(session))
      if (session->state != CKS_RO_USER_FUNCTIONS && session->state != CKS_RW_USER_FUNCTIONS)
        return 0;
    return 1;

  case logged_in_as_so:
    for (p11_session_t *session = p11_session_iterate(NULL);
         session != NULL; session = p11_session_iterate(session))
      if (session->state != CKS_RW_SO_FUNCTIONS)
        return 0;
    return 1;

  default:
    return 0;
  }
}



/*
 * PKCS #11 likes space-padded rather than null-terminated strings.
 * This requires minor antics so that we can use a printf()-like API
 * while neither overflowing the caller's buffer nor truncating the
 * output if it happens to be exactly the target length.
 */

static int psnprintf(void *buffer_, size_t size, const char *format, ...)
{
  char buffer[size + 1];
  size_t n;
  va_list ap;

  va_start(ap, format);
  n = vsnprintf(buffer, sizeof(buffer), format, ap);
  va_end(ap);

  for (size_t i = n; i < size; i++)
    buffer[i] = ' ';

  memcpy(buffer_, buffer, size);

  return n;
}



/*
 * Template checking and key generation.
 */

/*
 * First pass: called once per template entry during initial pass over
 * template to handle generic checks that apply regardless of
 * attribute type.
 */

static CK_RV p11_template_check_1(const CK_ATTRIBUTE_TYPE type,
                                  const void * const val,
                                  const size_t len,
                                  const p11_descriptor_t * const descriptor,
                                  unsigned long forbidden_flag)
{
  const p11_attribute_descriptor_t * const atd = p11_find_attribute_in_descriptor(descriptor, type);
  CK_RV rv;

  /* Attribute not allowed or not allowed for key generation */
  if (atd == NULL || (atd->flags & forbidden_flag) != 0)
    lose(CKR_ATTRIBUTE_TYPE_INVALID);

  /* NULL or wrong-sized attribute values */
  if (val == NULL || (atd->size != 0 && len != atd->size))
    lose(CKR_ATTRIBUTE_VALUE_INVALID);

  /* Attributes which only the SO user is allowed to set to CK_TRUE */
  if ((atd->flags & P11_DESCRIPTOR_ONLY_SO_USER_CAN_SET) != 0 && logged_in_as != logged_in_as_so && *(CK_BBOOL *) val)
      lose(CKR_ATTRIBUTE_VALUE_INVALID);

  /* Attributes which don't match mandatory values */
  if (atd->value != NULL && (atd->flags & P11_DESCRIPTOR_DEFAULT_VALUE) == 0 && memcmp(val, atd->value, atd->length) != 0)
    lose(CKR_TEMPLATE_INCONSISTENT);

#warning Add _LATCH checks here?

  rv = CKR_OK;

 fail:
#if DEBUG_PKCS11
  if (rv != CKR_OK)
    fprintf(stderr, "\np11_template_check_1() rejected attribute 0x%08lx\n", (unsigned long) type);
#endif
  return rv;
}

/*
 * Second pass: called once per template to check that each attribute
 * required for that template has been specified exactly once and that
 * the session's current login state allows access with this template.
 */

static CK_RV p11_template_check_2(const p11_session_t *session,
                                  const p11_descriptor_t * const descriptor,
                                  const CK_ATTRIBUTE_PTR template,
                                  const CK_ULONG template_length,
                                  unsigned long required_flag,
                                  unsigned long forbidden_flag)
{
  const CK_BBOOL * const cka_private
    = p11_attribute_find_value_in_template_or_descriptor(descriptor, CKA_PRIVATE,
                                                         template, template_length);
  const CK_BBOOL * const cka_token
    = p11_attribute_find_value_in_template_or_descriptor(descriptor, CKA_TOKEN,
                                                         template, template_length);
  CK_RV rv;

  assert(cka_private != NULL && cka_token != NULL);

  /*
   * Morass of session-state-specific restrictions on which objects we
   * can even see, much less modify.  Callers of this function need RW
   * acecss to the object in question, which simplifies this a bit.
   */

  if ((rv = p11_check_write_access(session, *cka_private, *cka_token)) != CKR_OK)
    goto fail;

  for (int i = 0; i < descriptor->n_attributes; i++) {
    const p11_attribute_descriptor_t * const atd = &descriptor->attributes[i];
    const int required_by_api  = (atd->flags & required_flag) != 0;
    const int forbidden_by_api = (atd->flags & forbidden_flag) != 0;
    const int in_descriptor    = (atd->flags & P11_DESCRIPTOR_DEFAULT_VALUE) != 0 || atd->value != NULL;
    const int pos_in_template  = p11_attribute_find_in_template(atd->type, template, template_length);

    /* Multiple entries for same attribute */
    if (pos_in_template >= 0)
      for (int j = pos_in_template + 1; j < template_length; j++)
        if (template[j].type == atd->type)
          lose(CKR_TEMPLATE_INCONSISTENT);

    /* Required attribute missing from template */
    if (!forbidden_by_api && (required_by_api || !in_descriptor) && pos_in_template < 0) {
#if DEBUG_PKCS11
      fprintf(stderr, "\n[Missing attribute 0x%lx]\n", atd->type);
#endif
      lose(CKR_TEMPLATE_INCOMPLETE);
    }
  }

  rv = CKR_OK;

 fail:
  return rv;
}

/*
 * Mechanism-independent checks for templates and descriptors when
 * generating new keypairs.
 *
 * PKCS #11 gives the application far too much rope (including but not
 * limited to the ability to supply completely unrelated templates for
 * public and private keys in a keypair), so we need to do a fair
 * amount of checking.  We automate as much of the dumb stuff as
 * possible through the object descriptor.
 *
 * Key usage handling here is based on RFC 5280 4.2.1.3.
 */

static CK_RV p11_check_keypair_attributes(const p11_session_t *session,
                                          const CK_ATTRIBUTE_PTR pPublicKeyTemplate,
                                          const CK_ULONG ulPublicKeyAttributeCount,
                                          const p11_descriptor_t * const public_descriptor,
                                          hal_key_flags_t *public_flags,
                                          const CK_ATTRIBUTE_PTR pPrivateKeyTemplate,
                                          const CK_ULONG ulPrivateKeyAttributeCount,
                                          const p11_descriptor_t * const private_descriptor,
                                          hal_key_flags_t *private_flags)
{
  CK_RV rv = CKR_OK;
  int i;

  assert(session             != NULL &&
         pPublicKeyTemplate  != NULL && public_descriptor  != NULL && public_flags  != NULL &&
         pPrivateKeyTemplate != NULL && private_descriptor != NULL && private_flags != NULL);

  *public_flags = *private_flags = 0;

  const CK_BBOOL * public_cka_private = NULL;
  const CK_BBOOL *private_cka_private = NULL;
  const CK_BBOOL *private_cka_extractable = NULL;

  /*
   * Check values provided in the public and private templates.
   */

  for (i = 0; i < ulPublicKeyAttributeCount; i++) {
    const CK_ATTRIBUTE_TYPE type = pPublicKeyTemplate[i].type;
    const void * const       val = pPublicKeyTemplate[i].pValue;
    const size_t             len = pPublicKeyTemplate[i].ulValueLen;

    if ((rv = p11_template_check_1(type, val, len, public_descriptor,
                                   P11_DESCRIPTOR_FORBIDDEN_BY_GENERATE)) != CKR_OK)
      goto fail;

    if (type == CKA_PRIVATE)
      public_cka_private = val;

    p11_attribute_apply_keyusage(public_flags, type, val);
  }

  for (i = 0; i < ulPrivateKeyAttributeCount; i++) {
    const CK_ATTRIBUTE_TYPE type = pPrivateKeyTemplate[i].type;
    const void * const       val = pPrivateKeyTemplate[i].pValue;
    const size_t             len = pPrivateKeyTemplate[i].ulValueLen;

    if ((rv = p11_template_check_1(type, val, len, private_descriptor,
                                   P11_DESCRIPTOR_FORBIDDEN_BY_GENERATE)) != CKR_OK)
      goto fail;

    if (type == CKA_PRIVATE)
      private_cka_private = val;

    if (type == CKA_EXTRACTABLE)
      private_cka_extractable = val;

    p11_attribute_apply_keyusage(private_flags, type, val);
  }

  /*
   * We insist that keyusage be specified for both public and private
   * key, and that they match.  May not need to be this strict.
   */

  if (*public_flags != *private_flags || *public_flags == 0)
    lose(CKR_TEMPLATE_INCONSISTENT);

  /*
   * Pass PKCS #11's weird notion of "public" objects through to HSM.
   */

  if (public_cka_private != NULL && ! *public_cka_private)
    *public_flags |= HAL_KEY_FLAG_PUBLIC;

  if (private_cka_private != NULL && ! *private_cka_private)
    *private_flags |= HAL_KEY_FLAG_PUBLIC;

  /*
   * Pass extractability through to HSM.  Public keys are always extractable.
   */

  *public_flags |= HAL_KEY_FLAG_EXPORTABLE;

  if (private_cka_extractable != NULL && *private_cka_extractable)
    *private_flags |= HAL_KEY_FLAG_EXPORTABLE;

  /*
   * Check that all required attributes have been specified,
   * and that our current session state allows this access.
   */

  if ((rv = p11_template_check_2(session,
                                 public_descriptor,
                                 pPublicKeyTemplate,
                                 ulPublicKeyAttributeCount,
                                 P11_DESCRIPTOR_REQUIRED_BY_GENERATE,
                                 P11_DESCRIPTOR_FORBIDDEN_BY_GENERATE))  != CKR_OK ||
      (rv = p11_template_check_2(session,
                                 private_descriptor,
                                 pPrivateKeyTemplate,
                                 ulPrivateKeyAttributeCount,
                                 P11_DESCRIPTOR_REQUIRED_BY_GENERATE,
                                 P11_DESCRIPTOR_FORBIDDEN_BY_GENERATE)) != CKR_OK)
    goto fail;

  /*
   * If we get this far, we're happy.  Maybe.
   */

  rv = CKR_OK;

 fail:
  return rv;
}

/*
 * CKM_RSA_PKCS_KEY_PAIR_GEN key pair generation handler.
 */

static CK_RV generate_keypair_rsa_pkcs(p11_session_t *session,
                                       const handle_flavor_t public_handle_flavor,
                                       const CK_ATTRIBUTE_PTR pPublicKeyTemplate,
                                       const CK_ULONG ulPublicKeyAttributeCount,
                                       const p11_descriptor_t *public_descriptor,
                                       CK_OBJECT_HANDLE_PTR phPublicKey,
                                       const hal_key_flags_t public_flags,
                                       const handle_flavor_t private_handle_flavor,
                                       const CK_ATTRIBUTE_PTR pPrivateKeyTemplate,
                                       const CK_ULONG ulPrivateKeyAttributeCount,
                                       const p11_descriptor_t *private_descriptor,
                                       CK_OBJECT_HANDLE_PTR phPrivateKey,
                                       const hal_key_flags_t private_flags,
                                       const CK_MECHANISM_PTR pMechanism)
{
  hal_pkey_handle_t public_pkey = {HAL_HANDLE_NONE}, private_pkey = {HAL_HANDLE_NONE};
  const uint8_t *public_exponent = const_0x010001;
  size_t public_exponent_len = sizeof(const_0x010001);
  hal_uuid_t public_uuid, private_uuid;
  CK_ULONG keysize = 0;
  CK_RV rv;

  assert(pPublicKeyTemplate != NULL && pPrivateKeyTemplate != NULL &&
         public_descriptor  != NULL && private_descriptor  != NULL &&
         phPublicKey        != NULL && phPrivateKey        != NULL &&
         session            != NULL && pMechanism          != NULL);

  for (int i = 0; i < ulPublicKeyAttributeCount; i++) {
    const CK_ATTRIBUTE_TYPE type = pPublicKeyTemplate[i].type;
    const void * const       val = pPublicKeyTemplate[i].pValue;
    const size_t             len = pPublicKeyTemplate[i].ulValueLen;

    switch (type) {

    case CKA_MODULUS_BITS:      /* Keysize in bits -- only allow multiples of 8 */
      keysize = *(CK_ULONG *) val;
      if ((keysize & 7) != 0)
        return CKR_ATTRIBUTE_VALUE_INVALID;
      continue;

    case CKA_PUBLIC_EXPONENT:
      public_exponent = val;
      public_exponent_len = len;
      continue;
    }
  }

  if (keysize == 0)
    return CKR_TEMPLATE_INCOMPLETE;

  {
    if (!hal_check(hal_rpc_pkey_generate_rsa(p11_session_hal_client(session),
                                             p11_session_hal_session(session),
                                             &private_pkey, &private_uuid, keysize,
                                             public_exponent, public_exponent_len,
                                             private_flags)))
      lose(CKR_FUNCTION_FAILED);

    uint8_t der[hal_rpc_pkey_get_public_key_len(private_pkey)], keybuf[hal_rsa_key_t_size];
    size_t der_len, modulus_len;
    hal_rsa_key_t *key = NULL;

    if (!hal_check(hal_rpc_pkey_get_public_key(private_pkey, der, &der_len, sizeof(der)))       ||
        !hal_check(hal_rsa_public_key_from_der(&key, keybuf, sizeof(keybuf), der, der_len))     ||
        !hal_check(hal_rpc_pkey_load(p11_session_hal_client(session),
                                     p11_session_hal_session(session),
                                     &public_pkey, &public_uuid, der, der_len, public_flags))   ||
        !hal_check(hal_rsa_key_get_modulus(key, NULL, &modulus_len, 0)))
      lose(CKR_FUNCTION_FAILED);

    uint8_t modulus[modulus_len];

    if (!hal_check(hal_rsa_key_get_modulus(key, modulus, NULL, sizeof(modulus))))
      lose(CKR_FUNCTION_FAILED);

    const hal_pkey_attribute_t extra[] = {
      {.type  = CKA_LOCAL,
       .value = &const_CK_TRUE,         .length = sizeof(const_CK_TRUE)},
      {.type  = CKA_KEY_GEN_MECHANISM,
       .value = &pMechanism->mechanism, .length = sizeof(pMechanism->mechanism)},
      {.type  = CKA_MODULUS,
       .value  = modulus,               .length = modulus_len}
    };

    if (!p11_attributes_set(private_pkey, pPrivateKeyTemplate, ulPrivateKeyAttributeCount,
                            private_descriptor, extra, sizeof(extra)/sizeof(*extra))            ||
        !p11_attributes_set(public_pkey, pPublicKeyTemplate, ulPublicKeyAttributeCount,
                            public_descriptor, extra, sizeof(extra)/sizeof(*extra)))
      lose(CKR_FUNCTION_FAILED);

    *phPrivateKey = p11_object_allocate(private_handle_flavor, &private_uuid, session);
    *phPublicKey  = p11_object_allocate(public_handle_flavor,  &public_uuid,  session);

    if (*phPrivateKey == CK_INVALID_HANDLE || *phPublicKey == CK_INVALID_HANDLE)
      lose(CKR_FUNCTION_FAILED);
  }

  rv = CKR_OK;

 fail:
  hal_rpc_pkey_close(private_pkey);
  hal_rpc_pkey_close(public_pkey);
  return rv;
}

/*
 * CKM_EC_KEY_PAIR_GEN key pair generation handler.
 */

static CK_RV generate_keypair_ec(p11_session_t *session,
                                 const handle_flavor_t public_handle_flavor,
                                 const CK_ATTRIBUTE_PTR pPublicKeyTemplate,
                                 const CK_ULONG ulPublicKeyAttributeCount,
                                 const p11_descriptor_t *public_descriptor,
                                 CK_OBJECT_HANDLE_PTR phPublicKey,
                                 const hal_key_flags_t public_flags,
                                 const handle_flavor_t private_handle_flavor,
                                 const CK_ATTRIBUTE_PTR pPrivateKeyTemplate,
                                 const CK_ULONG ulPrivateKeyAttributeCount,
                                 const p11_descriptor_t *private_descriptor,
                                 CK_OBJECT_HANDLE_PTR phPrivateKey,
                                 const hal_key_flags_t private_flags,
                                 const CK_MECHANISM_PTR pMechanism)
{
  hal_pkey_handle_t public_pkey = {HAL_HANDLE_NONE}, private_pkey = {HAL_HANDLE_NONE};
  const CK_BYTE *params = NULL;
  hal_curve_name_t curve;
  size_t params_len;
  hal_uuid_t public_uuid, private_uuid;
  CK_RV rv;

  assert(session != NULL && pPublicKeyTemplate != NULL && pPrivateKeyTemplate != NULL);

  for (int i = 0; i < ulPublicKeyAttributeCount; i++) {
    const CK_ATTRIBUTE_TYPE type = pPublicKeyTemplate[i].type;
    const void * const       val = pPublicKeyTemplate[i].pValue;
    const size_t             len = pPublicKeyTemplate[i].ulValueLen;

    switch (type) {

    case CKA_EC_PARAMS:
      params = val;
      params_len = len;
      continue;
    }
  }

  if (!ec_curve_oid_to_name(params, params_len, &curve))
    return CKR_TEMPLATE_INCOMPLETE;

  {

    if (!hal_check(hal_rpc_pkey_generate_ec(p11_session_hal_client(session),
                                            p11_session_hal_session(session),
                                            &private_pkey, &private_uuid,
                                            curve, private_flags)))
      lose(CKR_FUNCTION_FAILED);

    uint8_t der[hal_rpc_pkey_get_public_key_len(private_pkey)], keybuf[hal_ecdsa_key_t_size];
    hal_ecdsa_key_t *key = NULL;
    size_t der_len;

    if (!hal_check(hal_rpc_pkey_get_public_key(private_pkey, der, &der_len, sizeof(der)))       ||
        !hal_check(hal_ecdsa_public_key_from_der(&key, keybuf, sizeof(keybuf), der, der_len))   ||
        !hal_check(hal_rpc_pkey_load(p11_session_hal_client(session),
                                     p11_session_hal_session(session),
                                     &public_pkey, &public_uuid, der, der_len, public_flags)))
      lose(CKR_FUNCTION_FAILED);

    uint8_t point[hal_ecdsa_key_to_ecpoint_len(key)];

    if (!hal_check(hal_ecdsa_key_to_ecpoint(key, point, NULL, sizeof(point))))
      lose(CKR_FUNCTION_FAILED);

    const hal_pkey_attribute_t extra[] = {
      {.type  = CKA_LOCAL,
       .value = &const_CK_TRUE,         .length = sizeof(const_CK_TRUE)},
      {.type  = CKA_KEY_GEN_MECHANISM,
       .value = &pMechanism->mechanism, .length = sizeof(pMechanism->mechanism)},
      {.type  = CKA_EC_PARAMS,
       .value  = params,                .length = params_len},
      {.type  = CKA_EC_POINT,
       .value = point,                  .length = sizeof(point)}
    };

    if (!p11_attributes_set(private_pkey, pPrivateKeyTemplate, ulPrivateKeyAttributeCount,
                            private_descriptor, extra, sizeof(extra)/sizeof(*extra) - 1)         ||
        !p11_attributes_set(public_pkey, pPublicKeyTemplate, ulPublicKeyAttributeCount,
                            public_descriptor, extra, sizeof(extra)/sizeof(*extra)))
      lose(CKR_FUNCTION_FAILED);

    *phPrivateKey = p11_object_allocate(private_handle_flavor, &private_uuid, session);
    *phPublicKey  = p11_object_allocate(public_handle_flavor,  &public_uuid,  session);

    if (*phPrivateKey == CK_INVALID_HANDLE || *phPublicKey == CK_INVALID_HANDLE)
      lose(CKR_FUNCTION_FAILED);
  }

  rv = CKR_OK;

 fail:
  hal_rpc_pkey_close(private_pkey);
  hal_rpc_pkey_close(public_pkey);
  return rv;
}

/*
 * Key pair generation.  This needs a mechanism-specific function to
 * do the inner bits, but there's a lot of boilerplate.
 */

static CK_RV generate_keypair(p11_session_t *session,
                              const CK_MECHANISM_PTR pMechanism,
                              CK_RV (*mechanism_handler)(p11_session_t *session,
                                                         const handle_flavor_t public_handle_flavor,
                                                         const CK_ATTRIBUTE_PTR pPublicKeyTemplate,
                                                         const CK_ULONG ulPublicKeyAttributeCount,
                                                         const p11_descriptor_t *public_descriptor,
                                                         CK_OBJECT_HANDLE_PTR phPublicKey,
                                                         const hal_key_flags_t public_flags,
                                                         const handle_flavor_t private_handle_flavor,
                                                         const CK_ATTRIBUTE_PTR pPrivateKeyTemplate,
                                                         const CK_ULONG ulPrivateKeyAttributeCount,
                                                         const p11_descriptor_t *private_descriptor,
                                                         CK_OBJECT_HANDLE_PTR phPrivateKey,
                                                         const hal_key_flags_t private_flags,
                                                         const CK_MECHANISM_PTR pMechanism),
                              const CK_ATTRIBUTE_PTR pPublicKeyTemplate,
                              const CK_ULONG ulPublicKeyAttributeCount,
                              const p11_descriptor_t * const public_descriptor,
                              CK_OBJECT_HANDLE_PTR phPublicKey,
                              const CK_ATTRIBUTE_PTR pPrivateKeyTemplate,
                              const CK_ULONG ulPrivateKeyAttributeCount,
                              const p11_descriptor_t * const private_descriptor,
                              CK_OBJECT_HANDLE_PTR phPrivateKey)
{
  handle_flavor_t public_handle_flavor  = handle_flavor_session_object;
  handle_flavor_t private_handle_flavor = handle_flavor_session_object;
  hal_key_flags_t public_flags  = 0;
  hal_key_flags_t private_flags = 0;
  CK_RV rv;

  rv = p11_check_keypair_attributes(session,
                                    pPublicKeyTemplate,  ulPublicKeyAttributeCount,
                                    public_descriptor,   &public_flags,
                                    pPrivateKeyTemplate, ulPrivateKeyAttributeCount,
                                    private_descriptor,  &private_flags);
  if (rv != CKR_OK)
    return rv;

  assert(session             != NULL && pMechanism   != NULL &&
         pPublicKeyTemplate  != NULL && phPublicKey  != NULL &&
         pPrivateKeyTemplate != NULL && phPrivateKey != NULL);

  for (int i = 0; i < ulPublicKeyAttributeCount; i++)
    if (pPublicKeyTemplate[i].type == CKA_TOKEN)
      public_handle_flavor = p11_object_flavor_from_cka_token(pPublicKeyTemplate[i].pValue);

  for (int i = 0; i < ulPrivateKeyAttributeCount; i++)
    if (pPrivateKeyTemplate[i].type == CKA_TOKEN)
      private_handle_flavor = p11_object_flavor_from_cka_token(pPrivateKeyTemplate[i].pValue);

  if (public_handle_flavor == handle_flavor_token_object)
    public_flags  |= HAL_KEY_FLAG_TOKEN;

  if (private_handle_flavor == handle_flavor_token_object)
    private_flags |= HAL_KEY_FLAG_TOKEN;

  return mechanism_handler(session,
                           public_handle_flavor,  pPublicKeyTemplate,  ulPublicKeyAttributeCount,
                           public_descriptor,     phPublicKey,         public_flags,
                           private_handle_flavor, pPrivateKeyTemplate, ulPrivateKeyAttributeCount,
                           private_descriptor,    phPrivateKey,        private_flags,
                           pMechanism);
}

/*
 * Mechanism-independent checks for templates and descriptors when
 * import objects via C_CreateObject().
 *
 * Fun question exactly how calling code knows what descriptor to
 * pass.  p11_descriptor_from_key_type() will suffice for key objects.
 * Drive off that bridge when we get to it.
 */

static CK_RV p11_check_create_attributes(const p11_session_t *session,
                                         const CK_ATTRIBUTE_PTR pTemplate,
                                         const CK_ULONG ulCount,
                                         const p11_descriptor_t * const descriptor)
{
  CK_RV rv = CKR_OK;
  int i;

  assert(session != NULL && pTemplate != NULL && descriptor != NULL);

  /*
   * Check values provided in the template.
   */

  for (i = 0; i < ulCount; i++) {
    const CK_ATTRIBUTE_TYPE type = pTemplate[i].type;
    const void * const       val = pTemplate[i].pValue;
    const size_t             len = pTemplate[i].ulValueLen;

    if ((rv = p11_template_check_1(type, val, len, descriptor,
                                   P11_DESCRIPTOR_FORBIDDEN_BY_CREATEOBJECT)) != CKR_OK)
      goto fail;
  }

  /*
   * Check that all required attributes have been specified,
   * and that our current session state allows this access.
   */

  if ((rv = p11_template_check_2(session, descriptor, pTemplate, ulCount,
                                 P11_DESCRIPTOR_REQUIRED_BY_CREATEOBJECT,
                                 P11_DESCRIPTOR_FORBIDDEN_BY_CREATEOBJECT)) != CKR_OK)
    goto fail;

  /*
   * If we get this far, we're happy.  Maybe.
   */

  rv = CKR_OK;

 fail:
  return rv;
}




/*
 * Add data to a digest.
 */

static CK_RV digest_update(const p11_session_t * const session,
                           const hal_digest_algorithm_t algorithm,
                           hal_hash_handle_t *handle,
                           const uint8_t * const data, const size_t data_len)
{
  assert(algorithm != HAL_DIGEST_ALGORITHM_NONE && handle != NULL && data != NULL);

  if (handle->handle == HAL_HANDLE_NONE) {
    switch (hal_rpc_hash_initialize(p11_session_hal_client(session),
                                    p11_session_hal_session(session),
                                    handle, algorithm, NULL, 0)) {
    case HAL_OK:
      break;
    case HAL_ERROR_ALLOCATION_FAILURE:
      return CKR_HOST_MEMORY;
    default:
      return CKR_FUNCTION_FAILED;
    }
  }

  if (!hal_check(hal_rpc_hash_update(*handle, data, data_len)))
    return CKR_FUNCTION_FAILED;

  return CKR_OK;
}

/*
 * Finish using a digest context, if we haven't already.
 */

static void digest_cleanup(hal_hash_handle_t *handle)
{
  assert(handle != NULL);
  if (handle->handle == HAL_HANDLE_NONE)
    return;
  (void) hal_rpc_hash_finalize(*handle, NULL, 0);
  handle->handle = HAL_HANDLE_NONE;
}

/*
 * Compute the length of a signature based on the key.
 */

static int get_signature_len(const hal_pkey_handle_t pkey,
                             size_t *signature_len)
{
  assert(signature_len != NULL);

  hal_pkey_attribute_t attribute;
  uint8_t attribute_buffer[sizeof(CK_KEY_TYPE)];
  hal_curve_name_t curve;
  CK_BYTE oid[20];

  attribute.type = CKA_KEY_TYPE;
  if (!hal_check(hal_rpc_pkey_get_attributes(pkey, &attribute, 1,
                                             attribute_buffer, sizeof(attribute_buffer))))
    return 0;

  switch (*(CK_KEY_TYPE*)attribute.value) {

  case CKK_RSA:
    attribute.type = CKA_MODULUS;
    if (!hal_check(hal_rpc_pkey_get_attributes(pkey, &attribute, 1, NULL, 0)) ||
        attribute.length == HAL_PKEY_ATTRIBUTE_NIL)
      return 0;
    *signature_len = attribute.length;
    return 1;

  case CKK_EC:
    attribute.type = CKA_EC_PARAMS;
    if (!hal_check(hal_rpc_pkey_get_attributes(pkey, &attribute, 1, oid, sizeof(oid))) ||
        !ec_curve_oid_to_name(attribute.value, attribute.length, &curve))
      return 0;
    switch (curve) {
    case HAL_CURVE_P256: *signature_len = 64;  return 1;
    case HAL_CURVE_P384: *signature_len = 96;  return 1;
    case HAL_CURVE_P521: *signature_len = 132; return 1;
    default:                                   return 0;
   }
  }

  return 0;
}

/*
 * Generate a signature using the libhal RPC API.
 */

static CK_RV sign_hal_rpc(p11_session_t *session,
                          CK_BYTE_PTR pData,
                          CK_ULONG ulDataLen,
                          CK_BYTE_PTR pSignature,
                          CK_ULONG_PTR pulSignatureLen)
{
  hal_pkey_handle_t pkey = {HAL_HANDLE_NONE};
  size_t signature_len;
  CK_RV rv;

  assert(session != NULL && pulSignatureLen != NULL);

  if (!p11_object_pkey_open(session, session->sign_key_handle, &pkey))
    lose(CKR_FUNCTION_FAILED);

  if (!get_signature_len(pkey, &signature_len))
    lose(CKR_FUNCTION_FAILED);

  rv = pSignature != NULL && signature_len > *pulSignatureLen ? CKR_BUFFER_TOO_SMALL : CKR_OK;

  *pulSignatureLen = signature_len;

  if (pSignature != NULL && rv == CKR_OK)
    rv = p11_whine_from_hal(hal_rpc_pkey_sign(pkey, session->sign_digest_handle, pData, ulDataLen,
                                              pSignature, &signature_len, signature_len));
  /* Fall through */

 fail:
  hal_rpc_pkey_close(pkey);
  return rv;
}

/*
 * Verify a signature using the libhal RPC API.
 */

static CK_RV verify_hal_rpc(p11_session_t *session,
                            CK_BYTE_PTR pData,
                            CK_ULONG ulDataLen,
                            CK_BYTE_PTR pSignature,
                            CK_ULONG ulSignatureLen)
{
  hal_pkey_handle_t pkey = {HAL_HANDLE_NONE};
  CK_RV rv;

  assert(session != NULL);

  if (!p11_object_pkey_open(session, session->verify_key_handle, &pkey))
    lose(CKR_FUNCTION_FAILED);

  rv = p11_whine_from_hal(hal_rpc_pkey_verify(pkey, session->verify_digest_handle, pData, ulDataLen,
                                              pSignature, ulSignatureLen));
  /* Fall through */

 fail:
  hal_rpc_pkey_close(pkey);
  return rv;
}

#warning May need to do something about truncating oversized hashes for ECDSA, see PKCS11 spec



/*
 * PKCS #11 API functions.
 */

CK_RV C_Initialize(CK_VOID_PTR pInitArgs)
{
  ENTER_PUBLIC_FUNCTION(C_Initialize);

  CK_C_INITIALIZE_ARGS_PTR a = pInitArgs;
  CK_RV rv;

  /*
   * We'd like to detect the error of calling this method more than
   * once in a single process without an intervening call to
   * C_Finalize(), but there's no completely portable way to do that
   * when faced with things like the POSIX fork() system call.  For
   * the moment, we use a POSIX-specific check, but may need to
   * generalize this for other platforms.
   */

#if USE_POSIX
  if (initialized_pid == getpid())
    lose(CKR_CRYPTOKI_ALREADY_INITIALIZED);
#endif

  /*
   * Sort out what the user wants to do about mutexes.  Default is not
   * to use mutexes at all.
   *
   * There's a chicken and egg problem here: setting up the global
   * mutex and mutex function pointers creates a race condition, and
   * there's no obvious action we can take which is robust in the face
   * of pathological behavior by the caller such as simultaneous calls
   * to this method with incompatible mutex primitives.
   *
   * Given that (a) it's an error to call this method more than once
   * in the same process without an intervening F_Finalize() call, and
   * given that (b) we haven't actually promised to do any kind of
   * locking at all until this method returns CKR_OK, we punt
   * responsibility for this pathological case back to the caller.
   */

  mutex_cb_create  = NULL;
  mutex_cb_destroy = NULL;
  mutex_cb_lock    = NULL;
  mutex_cb_unlock  = NULL;

  if (a != NULL) {

    const int functions_provided = ((a->CreateMutex  != NULL) +
                                    (a->DestroyMutex != NULL) +
                                    (a->LockMutex    != NULL) +
                                    (a->UnlockMutex  != NULL));

    /*
     * Reserved is, um, reserved.
     * Mutex parameters must either all be present or all be absent.
     */

    if (a->pReserved != NULL || (functions_provided & 3) != 0)
      lose(CKR_ARGUMENTS_BAD);

    /*
     * If the user provided mutex functions, use them.  Otherwise, if
     * the user wants locking, use POSIX mutexes or return an error
     * depending on whether we have POSIX mutexes available.
     * Otherwise, we don't need to use mutexes.
     */

    if (functions_provided) {
      mutex_cb_create  = a->CreateMutex;
      mutex_cb_destroy = a->DestroyMutex;
      mutex_cb_lock    = a->LockMutex;
      mutex_cb_unlock  = a->UnlockMutex;
    }

    else if ((a->flags & CKF_OS_LOCKING_OK) != 0) {
#if USE_PTHREADS
      mutex_cb_create  = posix_mutex_create;
      mutex_cb_destroy = posix_mutex_destroy;
      mutex_cb_lock    = posix_mutex_lock;
      mutex_cb_unlock  = posix_mutex_unlock;
#else
      lose(CKR_CANT_LOCK);
#endif
    }
  }

  /*
   * Now that we know which mutex implementation to use, set up a
   * global mutex.  We may want something finer grained later, but
   * this is enough to preserve the basic API semantics.
   *
   * Open question whether we should lock at this point, given that
   * until we return we haven't promised to do locking.  Skip for now
   * as it's simpler, fix later if it turns out to be a problem.
   */

  if ((rv = mutex_create(&p11_global_mutex)) != CKR_OK)
    goto fail;

  /*
   * Initialize libhal RPC channel.
   */

  if (!hal_check(hal_rpc_client_init()))
    lose(CKR_GENERAL_ERROR);

#if USE_POSIX
  initialized_pid = getpid();
#endif

  return CKR_OK;

 fail:
  return rv;
}

CK_RV C_Finalize(CK_VOID_PTR pReserved)
{
  ENTER_PUBLIC_FUNCTION(C_Finalize);

  CK_RV rv = CKR_OK;

  if (pReserved != NULL)
    return CKR_ARGUMENTS_BAD;

  mutex_lock_or_return_failure(p11_global_mutex);

  /*
   * Destroy all current sessions.
   */

  p11_session_free_all();

  /*
   * At this point we're pretty well committed to shutting down, so
   * there's not much to be done if any of the rest of this fails.
   */

  hal_rpc_client_close();

  rv =  mutex_unlock(p11_global_mutex);
  (void) mutex_destroy(p11_global_mutex);
  p11_global_mutex = NULL;

#if USE_POSIX
  initialized_pid = 0;
#endif

  return rv;
}

CK_RV C_GetFunctionList(CK_FUNCTION_LIST_PTR_PTR ppFunctionList)
{
  ENTER_PUBLIC_FUNCTION(C_GetFunctionList);

  /*
   * Use pkcs11f.h to build dispatch vector for C_GetFunctionList().
   * This should be const, but that's not what PKCS #11 says, oh well.
   *
   * This doesn't touch anything requiring locks, nor should it.
   */

  static CK_FUNCTION_LIST ck_function_list = {
    { CRYPTOKI_VERSION_MAJOR, CRYPTOKI_VERSION_MINOR },
#define CK_PKCS11_FUNCTION_INFO(name) name,
#include "pkcs11f.h"
#undef  CK_PKCS11_FUNCTION_INFO
  };

  if (ppFunctionList == NULL)
    return CKR_ARGUMENTS_BAD;

  *ppFunctionList = &ck_function_list;

  return CKR_OK;
}

CK_RV C_GetSlotList(CK_BBOOL tokenPresent,
                    CK_SLOT_ID_PTR pSlotList,
                    CK_ULONG_PTR pulCount)
{
  ENTER_PUBLIC_FUNCTION(C_GetSlotList);

  /*
   * We only have one slot, and it's hardwired.
   * No locking required here as long as this holds.
   */

  if (pulCount == NULL)
    return CKR_ARGUMENTS_BAD;

  if (pSlotList != NULL && *pulCount < 1)
    return CKR_BUFFER_TOO_SMALL;

  if (p11_uninitialized())
    return CKR_CRYPTOKI_NOT_INITIALIZED;

  *pulCount = 1;

  if (pSlotList != NULL)
    pSlotList[0] = P11_ONE_AND_ONLY_SLOT;

  return CKR_OK;
}

CK_RV C_GetTokenInfo(CK_SLOT_ID slotID,
                     CK_TOKEN_INFO_PTR pInfo)
{
  ENTER_PUBLIC_FUNCTION(C_GetTokenInfo);

  /*
   * No locking required here as long as we're just returning constants.
   *
   * Some of the values below are nonsensical, because they don't map
   * particularly well to what the HSM is really doing.  In some cases
   * (particularly for some of the flags) we hard-wire whatever client
   * software insists that we say before it will talk to us.  Feh.
   *
   * Eventually we expect Cryptech devices to have their own hardware
   * clocks, in which case we'd set CKF_CLOCK_ON_TOKEN and
   * pInfo->utcTime.  Hardware not implemented yet, so not here either.
   */

  if (pInfo == NULL)
    return CKR_ARGUMENTS_BAD;

  if (slotID != P11_ONE_AND_ONLY_SLOT)
    return CKR_SLOT_ID_INVALID;

  if (p11_uninitialized())
    return CKR_CRYPTOKI_NOT_INITIALIZED;

  memset(pInfo, 0, sizeof(*pInfo));

  psnprintf(pInfo->label,          sizeof(pInfo->label),          P11_TOKEN_LABEL);
  psnprintf(pInfo->manufacturerID, sizeof(pInfo->manufacturerID), P11_MANUFACTURER_ID);
  psnprintf(pInfo->model,          sizeof(pInfo->model),          P11_BOARD_MODEL);
  psnprintf(pInfo->serialNumber,   sizeof(pInfo->serialNumber),   P11_BOARD_SERIAL);

  pInfo->flags = CKF_RNG | CKF_LOGIN_REQUIRED | CKF_USER_PIN_INITIALIZED | CKF_TOKEN_INITIALIZED;

  pInfo->ulMaxSessionCount      = CK_EFFECTIVELY_INFINITE;
  pInfo->ulSessionCount         = CK_UNAVAILABLE_INFORMATION;
  pInfo->ulMaxRwSessionCount    = CK_EFFECTIVELY_INFINITE;
  pInfo->ulRwSessionCount       = CK_UNAVAILABLE_INFORMATION;
  pInfo->ulMaxPinLen            = (CK_ULONG) hal_rpc_min_pin_length;
  pInfo->ulMinPinLen            = (CK_ULONG) hal_rpc_max_pin_length;
  pInfo->ulTotalPublicMemory    = CK_UNAVAILABLE_INFORMATION;
  pInfo->ulFreePublicMemory     = CK_UNAVAILABLE_INFORMATION;
  pInfo->ulTotalPrivateMemory   = CK_UNAVAILABLE_INFORMATION;
  pInfo->ulFreePrivateMemory    = CK_UNAVAILABLE_INFORMATION;
  pInfo->hardwareVersion.major  = P11_VERSION_HW_MAJOR;
  pInfo->hardwareVersion.minor  = P11_VERSION_HW_MINOR;
  pInfo->firmwareVersion.major  = P11_VERSION_FW_MAJOR;
  pInfo->firmwareVersion.minor  = P11_VERSION_FW_MINOR;

  return CKR_OK;
}

CK_RV C_OpenSession(CK_SLOT_ID slotID,
                    CK_FLAGS flags,
                    CK_VOID_PTR pApplication,
                    CK_NOTIFY Notify,
                    CK_SESSION_HANDLE_PTR phSession)
{
  ENTER_PUBLIC_FUNCTION(C_OpenSession);

  const int parallel_session = (flags & CKF_SERIAL_SESSION) == 0;
  const int read_only_session = (flags & CKF_RW_SESSION) == 0;
  p11_session_t *session = NULL;
  CK_RV rv;

  mutex_lock_or_return_failure(p11_global_mutex);

  if (slotID != P11_ONE_AND_ONLY_SLOT)
    lose(CKR_SLOT_ID_INVALID);

  if (phSession == NULL)
    lose(CKR_ARGUMENTS_BAD);

  if (parallel_session)
    lose(CKR_SESSION_PARALLEL_NOT_SUPPORTED);

  if ((session = p11_session_allocate()) == NULL)
    lose(CKR_HOST_MEMORY);

  switch (logged_in_as) {

  case not_logged_in:
    session->state = read_only_session ? CKS_RO_PUBLIC_SESSION : CKS_RW_PUBLIC_SESSION;
    break;

  case logged_in_as_user:
    session->state = read_only_session ? CKS_RO_USER_FUNCTIONS : CKS_RW_USER_FUNCTIONS;
    break;

  case logged_in_as_so:
    if (read_only_session)
      lose(CKR_SESSION_READ_WRITE_SO_EXISTS);
    session->state = CKS_RW_SO_FUNCTIONS;
    break;
  }

  session->notify = Notify;
  session->application = pApplication;

  assert(p11_session_consistent_login());

  if ((rv = mutex_unlock(p11_global_mutex)) != CKR_OK)
    goto fail;

  *phSession = session->handle;
  return CKR_OK;

 fail:
  p11_session_free(session);
  (void) mutex_unlock(p11_global_mutex);
  return rv;
}

CK_RV C_CloseSession(CK_SESSION_HANDLE hSession)
{
  ENTER_PUBLIC_FUNCTION(C_CloseSession);

  p11_session_t *session;
  CK_RV rv = CKR_OK;

  mutex_lock_or_return_failure(p11_global_mutex);

  if ((session = p11_session_find(hSession)) == NULL)
    lose(CKR_SESSION_HANDLE_INVALID);

  p11_session_free(session);

 fail:
  mutex_unlock_return_with_rv(rv, p11_global_mutex);
}

CK_RV C_CloseAllSessions(CK_SLOT_ID slotID)
{
  ENTER_PUBLIC_FUNCTION(C_CloseAllSessions);

  if (slotID != P11_ONE_AND_ONLY_SLOT)
    return CKR_SLOT_ID_INVALID;

  mutex_lock_or_return_failure(p11_global_mutex);

  p11_session_free_all();

  return mutex_unlock(p11_global_mutex);
}

CK_RV C_Login(CK_SESSION_HANDLE hSession,
              CK_USER_TYPE userType,
              CK_UTF8CHAR_PTR pPin,
              CK_ULONG ulPinLen)
{
  ENTER_PUBLIC_FUNCTION(C_Login);

  const hal_client_handle_t client = {HAL_HANDLE_NONE};
  hal_user_t user = HAL_USER_NONE;
  CK_RV rv = CKR_OK;

  mutex_lock_or_return_failure(p11_global_mutex);

  if (pPin == NULL)
    lose(CKR_ARGUMENTS_BAD);

  /*
   * Mind, I don't really know why this function takes a session
   * handle, given that the semantics don't seem to call upon us to do
   * anything special for "this" session.
   */

  if (p11_session_find(hSession) == NULL)
    lose(CKR_SESSION_HANDLE_INVALID);

  /*
   * Figure out which PIN we're checking.
   * We don't (yet?) support CKU_CONTEXT_SPECIFIC.
   *
   * We don't currently support re-login without an intervening
   * logout, so reject the login attempt if we're already logged in.
   *
   * Read-only SO is an illegal state, so reject the login attempt if
   * we have any read-only sessions and we're trying to log in as SO.
   */

  switch (userType) {
  case CKU_USER:
    switch (logged_in_as) {
    case not_logged_in:         break;
    case logged_in_as_user:     lose(CKR_USER_ALREADY_LOGGED_IN);
    case logged_in_as_so:       lose(CKR_USER_ANOTHER_ALREADY_LOGGED_IN);
    }
    user = HAL_USER_NORMAL;
    break;
  case CKU_SO:
    switch (logged_in_as) {
    case not_logged_in:         break;
    case logged_in_as_so:       lose(CKR_USER_ALREADY_LOGGED_IN);
    case logged_in_as_user:     lose(CKR_USER_ANOTHER_ALREADY_LOGGED_IN);
    }
    for (p11_session_t *session = p11_session_iterate(NULL);
         session != NULL; session = p11_session_iterate(session))
      if (session->state == CKS_RO_PUBLIC_SESSION)
        lose(CKR_SESSION_READ_ONLY_EXISTS);
    user = HAL_USER_SO;
    break;
  case CKU_CONTEXT_SPECIFIC:
    lose(CKR_OPERATION_NOT_INITIALIZED);
  default:
    lose(CKR_USER_TYPE_INVALID);
  }

  /*
   * Try to log in the HSM.
   */

  if ((rv = p11_whine_from_hal(hal_rpc_login(client, user, (char *) pPin, ulPinLen))) != CKR_OK)
    goto fail;

  /*
   * If we get here, the PIN was OK.  Update global login state, then
   * whack every session into the correct new state.
   */

  assert(p11_session_consistent_login());

  logged_in_as = userType == CKU_SO ? logged_in_as_so : logged_in_as_user;

  for (p11_session_t *session = p11_session_iterate(NULL);
       session != NULL; session = p11_session_iterate(session)) {
    switch (session->state) {

    case CKS_RO_PUBLIC_SESSION:
      assert(userType == CKU_USER);
      session->state = CKS_RO_USER_FUNCTIONS;
      continue;

    case CKS_RW_PUBLIC_SESSION:
      session->state = userType == CKU_SO ? CKS_RW_SO_FUNCTIONS : CKS_RW_USER_FUNCTIONS;
      continue;

    }
  }

  assert(p11_session_consistent_login());

 fail:
  mutex_unlock_return_with_rv(rv, p11_global_mutex);
}

CK_RV C_Logout(CK_SESSION_HANDLE hSession)
{
  ENTER_PUBLIC_FUNCTION(C_Logout);

  const hal_client_handle_t client = {HAL_HANDLE_NONE};
  p11_session_t *session = NULL;
  CK_RV rv = CKR_OK;

  mutex_lock_or_return_failure(p11_global_mutex);

  /*
   * Mind, I don't really know why this function takes a session
   * handle, given that the semantics don't seem to call upon us to do
   * anything special for "this" session.
   */

  if ((session = p11_session_find(hSession)) == NULL)
    lose(CKR_SESSION_HANDLE_INVALID);

  if (logged_in_as == not_logged_in)
    lose(CKR_USER_NOT_LOGGED_IN);

  /*
   * Delete any private session objects, clear handles for all private
   * objects, and whack every existing session into the right state.
   */

  {
    assert(p11_session_consistent_login());

    const hal_pkey_attribute_t attrs[] = {
      {.type = CKA_PRIVATE, .value = &const_CK_TRUE, .length = sizeof(const_CK_TRUE)}
    };

    hal_uuid_t uuids[64];
    unsigned n, state;

    for (p11_session_t *session = p11_session_iterate(NULL);
         session != NULL; session = p11_session_iterate(session)) {

      memset(uuids, 0, sizeof(uuids));
      state = 0;
      do {

        rv = p11_whine_from_hal(hal_rpc_pkey_match(p11_session_hal_client(session),
                                                   p11_session_hal_session(session),
                                                   HAL_KEY_TYPE_NONE, HAL_CURVE_NONE,
                                                   HAL_KEY_FLAG_TOKEN, 0,
                                                   attrs, sizeof(attrs)/sizeof(*attrs), &state,
                                                   uuids, &n, sizeof(uuids)/sizeof(*uuids),
                                                   &uuids[sizeof(uuids)/sizeof(*uuids) - 1]));
        if (rv != CKR_OK)
          goto fail;

        for (int i = 0; i < n; i++) {
          p11_object_free(p11_object_by_uuid(&uuids[i]));
          hal_pkey_handle_t pkey;
          rv = p11_whine_from_hal(hal_rpc_pkey_open(p11_session_hal_client(session),
                                                    p11_session_hal_session(session),
                                                    &pkey, &uuids[i]));
          if (rv != CKR_OK)
            goto fail;
          if ((rv = p11_whine_from_hal(hal_rpc_pkey_delete(pkey))) != CKR_OK) {
            (void) hal_rpc_pkey_close(pkey);
            goto fail;
          }
        }

      } while (n == sizeof(uuids)/sizeof(*uuids));
    }

    memset(uuids, 0, sizeof(uuids));
    state = 0;
    do {

      rv = p11_whine_from_hal(hal_rpc_pkey_match(p11_session_hal_client(session),
                                                 p11_session_hal_session(session),
                                                 HAL_KEY_TYPE_NONE, HAL_CURVE_NONE,
                                                 HAL_KEY_FLAG_TOKEN, HAL_KEY_FLAG_TOKEN,
                                                 attrs, sizeof(attrs)/sizeof(*attrs), &state,
                                                 uuids, &n, sizeof(uuids)/sizeof(*uuids),
                                                 &uuids[sizeof(uuids)/sizeof(*uuids) - 1]));
      if (rv != CKR_OK)
        goto fail;

      for (int i = 0; i < n; i++)
        p11_object_free(p11_object_by_uuid(&uuids[i]));

    } while (n == sizeof(uuids)/sizeof(*uuids));

    for (p11_session_t *session = p11_session_iterate(NULL);
         session != NULL; session = p11_session_iterate(session)) {
      switch (session->state) {

      case CKS_RO_USER_FUNCTIONS:
        session->state = CKS_RO_PUBLIC_SESSION;
        continue;

      case CKS_RW_USER_FUNCTIONS:
      case CKS_RW_SO_FUNCTIONS:
        session->state = CKS_RW_PUBLIC_SESSION;
        continue;

      }
    }

    if ((rv = p11_whine_from_hal(hal_rpc_logout(client))) != CKR_OK)
      goto fail;

    logged_in_as = not_logged_in;

    assert(p11_session_consistent_login());
  }

 fail:
  mutex_unlock_return_with_rv(rv, p11_global_mutex);
}

CK_RV C_CreateObject(CK_SESSION_HANDLE hSession,
                     CK_ATTRIBUTE_PTR pTemplate,
                     CK_ULONG ulCount,
                     CK_OBJECT_HANDLE_PTR phObject)
{
  ENTER_PUBLIC_FUNCTION(C_CreateObject);

  p11_session_t *session;
  CK_RV rv;

  mutex_lock_or_return_failure(p11_global_mutex);

  if ((session = p11_session_find(hSession)) == NULL)
    lose(CKR_SESSION_HANDLE_INVALID);

  if (pTemplate == NULL || phObject == NULL)
    lose(CKR_ARGUMENTS_BAD);

  const CK_OBJECT_CLASS * const cka_class       = p11_attribute_find_value_in_template(CKA_CLASS,       pTemplate, ulCount);
  const CK_KEY_TYPE     * const cka_key_type    = p11_attribute_find_value_in_template(CKA_KEY_TYPE,    pTemplate, ulCount);
  const CK_BBOOL        * const cka_token       = p11_attribute_find_value_in_template(CKA_TOKEN,       pTemplate, ulCount);
  const CK_BBOOL        * const cka_private     = p11_attribute_find_value_in_template(CKA_PRIVATE,     pTemplate, ulCount);
  const CK_BBOOL        * const cka_extractable = p11_attribute_find_value_in_template(CKA_EXTRACTABLE, pTemplate, ulCount);

  if (cka_class == NULL)
    lose(CKR_TEMPLATE_INCOMPLETE);

  switch (*cka_class) {
  case CKO_PUBLIC_KEY:
  case CKO_PRIVATE_KEY:
  case CKO_SECRET_KEY:
    break;
  default:
    lose(CKR_TEMPLATE_INCONSISTENT);
  }

  if (cka_key_type == NULL)
    lose(CKR_TEMPLATE_INCOMPLETE);

  const p11_descriptor_t * const
    descriptor = p11_descriptor_from_key_type(*cka_class, *cka_key_type);

  if (descriptor == NULL)
    lose(CKR_TEMPLATE_INCONSISTENT);

  if ((rv = p11_check_create_attributes(session, pTemplate, ulCount, descriptor)) != CKR_OK)
    goto fail;

  const handle_flavor_t flavor = p11_object_flavor_from_cka_token(cka_token);

  switch (session->state) {
  case CKS_RO_PUBLIC_SESSION:
  case CKS_RO_USER_FUNCTIONS:
    if (flavor == handle_flavor_token_object)
      lose(CKR_SESSION_READ_ONLY);
  }

  hal_key_flags_t flags = flavor == handle_flavor_token_object ? HAL_KEY_FLAG_TOKEN : 0;

  for (int i = 0; i < ulCount; i++)
    p11_attribute_apply_keyusage(&flags, pTemplate[i].type, pTemplate[i].pValue);

  if (cka_private != NULL && ! *cka_private)
    flags |= HAL_KEY_FLAG_PUBLIC;

  if (*cka_class == CKO_PUBLIC_KEY || (cka_extractable != NULL && *cka_extractable))
    flags |= HAL_KEY_FLAG_EXPORTABLE;

  int (*handler)(const p11_session_t *session,
                 const handle_flavor_t flavor,
                 const CK_ATTRIBUTE_PTR pTemplate,
                 const CK_ULONG ulCount,
                 const p11_descriptor_t * const descriptor,
                 CK_OBJECT_HANDLE_PTR phObject,
                 const hal_key_flags_t flags) = NULL;

  if (*cka_class == CKO_PUBLIC_KEY && *cka_key_type == CKK_RSA)
    handler = p11_object_create_rsa_public_key;

  if (*cka_class == CKO_PUBLIC_KEY && *cka_key_type == CKK_EC)
    handler = p11_object_create_ec_public_key;

  if (*cka_class == CKO_PRIVATE_KEY && *cka_key_type == CKK_RSA)
    handler = p11_object_create_rsa_private_key;

  if (*cka_class == CKO_PRIVATE_KEY && *cka_key_type == CKK_EC)
    handler = p11_object_create_ec_private_key;

  if (handler == NULL)
    lose(CKR_FUNCTION_FAILED);

  if (!handler(session, flavor, pTemplate, ulCount, descriptor, phObject, flags))
    lose(CKR_FUNCTION_FAILED);

  return mutex_unlock(p11_global_mutex);

 fail:
  mutex_unlock_return_with_rv(rv, p11_global_mutex);
}

CK_RV C_DestroyObject(CK_SESSION_HANDLE hSession,
                      CK_OBJECT_HANDLE hObject)
{
  ENTER_PUBLIC_FUNCTION(C_DestroyObject);

  uint8_t attributes_buffer[2 * sizeof(CK_BBOOL)];
  hal_pkey_handle_t pkey = {HAL_HANDLE_NONE};
  hal_pkey_attribute_t attributes[] = {
    [0].type = CKA_PRIVATE,
    [1].type = CKA_TOKEN
  };
  CK_BBOOL cka_private;
  CK_BBOOL cka_token;
  p11_session_t *session;
  CK_RV rv = CKR_OK;

  mutex_lock_or_return_failure(p11_global_mutex);

  session = p11_session_find(hSession);

  if (!p11_object_pkey_open(session, hObject, &pkey))
    lose(CKR_FUNCTION_FAILED);

  if (!hal_check(hal_rpc_pkey_get_attributes(pkey, attributes, sizeof(attributes)/sizeof(*attributes),
                                             attributes_buffer, sizeof(attributes_buffer))))
    lose(CKR_KEY_HANDLE_INVALID);

  cka_private  = *(CK_BBOOL*) attributes[0].value;
  cka_token    = *(CK_BBOOL*) attributes[1].value;

  if ((rv = p11_check_write_access(session, cka_private, cka_token)) != CKR_OK)
    goto fail;

  if (!hal_check(hal_rpc_pkey_delete(pkey)))
    lose(CKR_FUNCTION_FAILED);

  p11_object_free(p11_object_by_handle(hObject));

 fail:
  if (pkey.handle != HAL_HANDLE_NONE)
    (void) hal_rpc_pkey_close(pkey);
  mutex_unlock_return_with_rv(rv, p11_global_mutex);
}

CK_RV C_GetAttributeValue(CK_SESSION_HANDLE hSession,
                          CK_OBJECT_HANDLE hObject,
                          CK_ATTRIBUTE_PTR pTemplate,
                          CK_ULONG ulCount)
{
  ENTER_PUBLIC_FUNCTION(C_GetAttributeValue);

  hal_pkey_handle_t pkey = {HAL_HANDLE_NONE};
  const p11_descriptor_t *descriptor = NULL;
  CK_BBOOL cka_extractable, cka_sensitive;
  CK_OBJECT_CLASS cka_class;
  CK_KEY_TYPE cka_key_type;
  CK_BBOOL cka_private;
  CK_BBOOL cka_token;
  int sensitive_object = 0;
  p11_session_t *session;
  CK_RV rv;

  mutex_lock_or_return_failure(p11_global_mutex);

  if (pTemplate == NULL)
    lose(CKR_ARGUMENTS_BAD);

  session = p11_session_find(hSession);

  if (!p11_object_pkey_open(session, hObject, &pkey))
    lose(CKR_OBJECT_HANDLE_INVALID);

  {
    hal_pkey_attribute_t attributes[] = {
      [0].type = CKA_CLASS,
      [1].type = CKA_PRIVATE,
      [2].type = CKA_TOKEN,
      [3].type = CKA_KEY_TYPE
    };
    uint8_t attributes_buffer[sizeof(CK_OBJECT_CLASS) + 2 * sizeof(CK_BBOOL) + sizeof(CK_KEY_TYPE)];

    if (!hal_check(hal_rpc_pkey_get_attributes(pkey,
                                               attributes, sizeof(attributes)/sizeof(*attributes),
                                               attributes_buffer, sizeof(attributes_buffer))))
      lose(CKR_OBJECT_HANDLE_INVALID);

    cka_class    = *(CK_OBJECT_CLASS*) attributes[0].value;
    cka_private  = *(CK_BBOOL*)        attributes[1].value;
    cka_token    = *(CK_BBOOL*)        attributes[2].value;
    cka_key_type = *(CK_KEY_TYPE*)     attributes[3].value;

    if ((rv = p11_check_read_access(session, cka_private, cka_token)) != CKR_OK)
      goto fail;

    descriptor = p11_descriptor_from_key_type(cka_class, cka_key_type);
  }

  if (cka_class == CKO_PRIVATE_KEY || cka_class == CKO_SECRET_KEY) {
    hal_pkey_attribute_t attributes[] = {
      [0].type = CKA_EXTRACTABLE,
      [1].type = CKA_SENSITIVE
    };
    uint8_t attributes_buffer[sizeof(CK_OBJECT_CLASS) + sizeof(CK_KEY_TYPE)];

    if (!hal_check(hal_rpc_pkey_get_attributes(pkey,
                                               attributes, sizeof(attributes)/sizeof(*attributes),
                                               attributes_buffer, sizeof(attributes_buffer))))
      lose(CKR_OBJECT_HANDLE_INVALID);

    cka_extractable = *(CK_BBOOL*) attributes[0].value;
    cka_sensitive   = *(CK_BBOOL*) attributes[1].value;

    sensitive_object = cka_sensitive || !cka_extractable;
  }

  {
    hal_pkey_attribute_t attributes[ulCount];

    memset(attributes, 0, sizeof(attributes));

    for (int i = 0; i < ulCount; i++)
      attributes[i].type = pTemplate[i].type;

    if (!hal_check(hal_rpc_pkey_get_attributes(pkey,
                                               attributes, sizeof(attributes)/sizeof(*attributes),
                                               NULL, 0)))
      lose(CKR_OBJECT_HANDLE_INVALID);

    rv = CKR_OK;

    size_t attributes_buffer_len = 0;

    for (int i = 0; i < ulCount; i++) {
      if (sensitive_object && p11_attribute_is_sensitive(descriptor, pTemplate[i].type)) {
        pTemplate[i].ulValueLen = -1;
        rv = CKR_ATTRIBUTE_SENSITIVE;
        continue;
      }
      if (attributes[i].length == HAL_PKEY_ATTRIBUTE_NIL) {
        pTemplate[i].ulValueLen = -1;
        rv = CKR_ATTRIBUTE_TYPE_INVALID;
        continue;
      }
      if (pTemplate[i].pValue == NULL) {
        pTemplate[i].ulValueLen = attributes[i].length;
        continue;
      }
      if (pTemplate[i].ulValueLen < attributes[i].length) {
        pTemplate[i].ulValueLen = -1;
        rv = CKR_BUFFER_TOO_SMALL;
        continue;
      }
      attributes_buffer_len += attributes[i].length;
    }

    if (attributes_buffer_len == 0)
      goto fail;

    uint8_t attributes_buffer[attributes_buffer_len];
    unsigned n = 0;

    for (int i = 0; i < ulCount; i++)
      if (pTemplate[i].pValue != NULL && pTemplate[i].ulValueLen != -1)
        attributes[n++].type = pTemplate[i].type;

    if (!hal_check(hal_rpc_pkey_get_attributes(pkey, attributes, n,
                                               attributes_buffer, sizeof(attributes_buffer))))
      lose(CKR_OBJECT_HANDLE_INVALID);

    for (int i = 0; i < n; i++) {
      int j = p11_attribute_find_in_template(attributes[i].type, pTemplate, ulCount);

      if (j < 0 || pTemplate[j].ulValueLen == -1 || pTemplate[j].ulValueLen < attributes[i].length)
        lose(CKR_FUNCTION_FAILED);

      memcpy(pTemplate[j].pValue, attributes[i].value, attributes[i].length);
      pTemplate[j].ulValueLen = attributes[i].length;
    }
  }

 fail:
  if (pkey.handle != HAL_HANDLE_NONE) {
    if (rv == CKR_OK)
      rv = p11_whine_from_hal(hal_rpc_pkey_close(pkey));
    else
      (void) hal_rpc_pkey_close(pkey);
  }
  mutex_unlock_return_with_rv(rv, p11_global_mutex);
}

CK_RV C_FindObjectsInit(CK_SESSION_HANDLE hSession,
                        CK_ATTRIBUTE_PTR pTemplate,
                        CK_ULONG ulCount)
{
  ENTER_PUBLIC_FUNCTION(C_FindObjectsInit);

  const size_t attributes_len = sizeof(hal_pkey_attribute_t) * (ulCount + 1);
  size_t len = attributes_len;
  CK_BBOOL *cka_private = NULL;
  CK_BBOOL *cka_token = NULL;
  p11_session_t *session;
  CK_RV rv = CKR_OK;
  uint8_t *mem;

  mutex_lock_or_return_failure(p11_global_mutex);

  if ((session = p11_session_find(hSession)) == NULL)
    lose(CKR_SESSION_HANDLE_INVALID);

  if (ulCount > 0 && pTemplate == NULL)
    lose(CKR_ARGUMENTS_BAD);

  if (session->find_query != NULL)
    lose(CKR_OPERATION_ACTIVE);

  assert(!session->find_query_token && !session->find_query_session && !session->find_query_state);

  for (int i = 0; i < ulCount; i++) {
    if (pTemplate[i].pValue == NULL || pTemplate[i].ulValueLen == 0)
      lose(CKR_ARGUMENTS_BAD);
    len += pTemplate[i].ulValueLen;
  }

  if ((mem = malloc(len)) == NULL)
    lose(CKR_HOST_MEMORY);

  session->find_query = (hal_pkey_attribute_t *) mem;
  mem += attributes_len;

  for (int i = 0; i < ulCount; i++) {
    len = pTemplate[i].ulValueLen;
    session->find_query[i].type   = pTemplate[i].type;
    session->find_query[i].value  = mem;
    session->find_query[i].length = len;
    memcpy(mem, pTemplate[i].pValue, len);
    mem += len;
  }

  cka_private = p11_attribute_find_value_in_template(CKA_PRIVATE, pTemplate, ulCount);
  cka_token   = p11_attribute_find_value_in_template(CKA_TOKEN,   pTemplate, ulCount);

  session->find_query_n       = ulCount;
  session->find_query_token   = cka_token == NULL ||  *cka_token;
  session->find_query_session = cka_token == NULL || !*cka_token;
  session->find_query_state   = 0;
  memset(&session->find_query_previous_uuid, 0, sizeof(session->find_query_previous_uuid));

  /*
   * Quietly enforce object privacy even if template tries to bypass,
   * per PCKS #11 specification.
   */

  if (logged_in_as != logged_in_as_user && cka_private == NULL) {
    session->find_query[ulCount].type   = CKA_PRIVATE;
    session->find_query[ulCount].value  = &const_CK_FALSE;
    session->find_query[ulCount].length = sizeof(const_CK_FALSE);
    session->find_query_n++;
  }

  if (logged_in_as != logged_in_as_user && cka_private != NULL && *cka_private) {
    int i = p11_attribute_find_in_template(CKA_PRIVATE, pTemplate, ulCount);
    assert(i >= 0 && i < ulCount);
    session->find_query[i].value  = &const_CK_FALSE;
    session->find_query[i].length = sizeof(const_CK_FALSE);
  }

 fail:
  mutex_unlock_return_with_rv(rv, p11_global_mutex);
}

CK_RV C_FindObjects(CK_SESSION_HANDLE hSession,
                    CK_OBJECT_HANDLE_PTR phObject,
                    CK_ULONG ulMaxObjectCount,
                    CK_ULONG_PTR pulObjectCount)
{
  ENTER_PUBLIC_FUNCTION(C_FindObjects);

  p11_session_t *session;
  CK_RV rv = CKR_OK;

  mutex_lock_or_return_failure(p11_global_mutex);

  if ((session = p11_session_find(hSession)) == NULL)
    lose(CKR_SESSION_HANDLE_INVALID);

  if (session->find_query == NULL)
    lose(CKR_OPERATION_NOT_INITIALIZED);

  if (phObject == NULL || pulObjectCount == NULL)
    lose(CKR_ARGUMENTS_BAD);

  *pulObjectCount = 0;

  while (*pulObjectCount < ulMaxObjectCount &&
         (session->find_query_token || session->find_query_session)) {
    hal_uuid_t uuids[ulMaxObjectCount - *pulObjectCount];
    handle_flavor_t flavor;
    hal_key_flags_t flags;
    unsigned n;

    if (session->find_query_token) {
      flavor = handle_flavor_token_object;
      flags  = HAL_KEY_FLAG_TOKEN;
    }
    else {
      flavor = handle_flavor_session_object;
      flags  = 0;
    }

    rv = p11_whine_from_hal(hal_rpc_pkey_match(p11_session_hal_client(session),
                                               p11_session_hal_session(session),
                                               HAL_KEY_TYPE_NONE, HAL_CURVE_NONE,
                                               HAL_KEY_FLAG_TOKEN, flags,
                                               session->find_query, session->find_query_n,
                                               &session->find_query_state,
                                               uuids, &n, sizeof(uuids)/sizeof(*uuids),
                                               &session->find_query_previous_uuid));
      if (rv != CKR_OK)
        goto fail;

      for (int i = 0; i < n; i++) {
        phObject[*pulObjectCount] = p11_object_allocate(flavor, &uuids[i], session);
        if (phObject[*pulObjectCount] == CK_INVALID_HANDLE)
          lose(CKR_FUNCTION_FAILED);
        ++*pulObjectCount;
      }

      if (n == sizeof(uuids)/sizeof(*uuids)) {
        memcpy(&session->find_query_previous_uuid, &uuids[n - 1],
               sizeof(session->find_query_previous_uuid));
      }

      else {
        memset(&session->find_query_previous_uuid, 0, sizeof(session->find_query_previous_uuid));
        session->find_query_state = 0;

        if (session->find_query_token)
          session->find_query_token = 0;
        else
          session->find_query_session = 0;
      }
  }

 fail:
  mutex_unlock_return_with_rv(rv, p11_global_mutex);
}

CK_RV C_FindObjectsFinal(CK_SESSION_HANDLE hSession)
{
  ENTER_PUBLIC_FUNCTION(C_FindObjectsFinal);

  p11_session_t *session;
  CK_RV rv = CKR_OK;

  mutex_lock_or_return_failure(p11_global_mutex);

  if ((session = p11_session_find(hSession)) == NULL)
    lose(CKR_SESSION_HANDLE_INVALID);

  if (session->find_query == NULL)
    lose(CKR_OPERATION_NOT_INITIALIZED);

  free(session->find_query);

  session->find_query = NULL;
  session->find_query_n = 0;
  session->find_query_token = 0;
  session->find_query_session = 0;
  session->find_query_state = 0;
  memset(&session->find_query_previous_uuid, 0, sizeof(session->find_query_previous_uuid));

 fail:
  mutex_unlock_return_with_rv(rv, p11_global_mutex);
}

CK_RV C_DigestInit(CK_SESSION_HANDLE hSession,
                   CK_MECHANISM_PTR pMechanism)
{
  ENTER_PUBLIC_FUNCTION(C_DigestInit);

  hal_digest_algorithm_t algorithm;
  p11_session_t *session;
  CK_RV rv = CKR_OK;

  mutex_lock_or_return_failure(p11_global_mutex);

  if ((session = p11_session_find(hSession)) == NULL)
    lose(CKR_SESSION_HANDLE_INVALID);

  if (pMechanism == NULL)
    lose(CKR_ARGUMENTS_BAD);

  if (session->digest_algorithm != HAL_DIGEST_ALGORITHM_NONE)
    lose(CKR_OPERATION_ACTIVE);

  switch (pMechanism->mechanism) {
  case CKM_SHA_1:       algorithm = HAL_DIGEST_ALGORITHM_SHA1;   break;
  case CKM_SHA224:      algorithm = HAL_DIGEST_ALGORITHM_SHA224; break;
  case CKM_SHA256:      algorithm = HAL_DIGEST_ALGORITHM_SHA256; break;
  case CKM_SHA384:      algorithm = HAL_DIGEST_ALGORITHM_SHA384; break;
  case CKM_SHA512:      algorithm = HAL_DIGEST_ALGORITHM_SHA512; break;
  default:              lose(CKR_MECHANISM_INVALID);
  }

  session->digest_algorithm = algorithm;
  return mutex_unlock(p11_global_mutex);

 fail:
  mutex_unlock_return_with_rv(rv, p11_global_mutex);
}

CK_RV C_Digest(CK_SESSION_HANDLE hSession,
               CK_BYTE_PTR pData,
               CK_ULONG ulDataLen,
               CK_BYTE_PTR pDigest,
               CK_ULONG_PTR pulDigestLen)
{
  ENTER_PUBLIC_FUNCTION(C_Digest);

  p11_session_t *session;
  size_t digest_len;
  CK_RV rv = CKR_OK;

  mutex_lock_or_return_failure(p11_global_mutex);

  if ((session = p11_session_find(hSession)) == NULL)
    lose(CKR_SESSION_HANDLE_INVALID);

  if (pData == NULL || pulDigestLen == NULL)
    lose(CKR_ARGUMENTS_BAD);

  if (session->digest_algorithm == HAL_DIGEST_ALGORITHM_NONE)
    lose(CKR_OPERATION_NOT_INITIALIZED);

  if (session->digest_handle.handle != HAL_HANDLE_NONE)
    lose(CKR_OPERATION_ACTIVE);

  if (!hal_check(hal_rpc_hash_get_digest_length(session->digest_algorithm, &digest_len)))
    lose(CKR_FUNCTION_FAILED);

  rv = pDigest != NULL && *pulDigestLen < digest_len ? CKR_BUFFER_TOO_SMALL : CKR_OK;

  *pulDigestLen = digest_len;

  if (pDigest == NULL || rv == CKR_BUFFER_TOO_SMALL)
    mutex_unlock_return_with_rv(rv, p11_global_mutex);

  if ((rv = digest_update(session, session->digest_algorithm,
                          &session->digest_handle, pData, ulDataLen)) != CKR_OK)
    goto fail;

  if (!hal_check(hal_rpc_hash_finalize(session->digest_handle, pDigest, *pulDigestLen)))
    lose(CKR_FUNCTION_FAILED);

  rv = CKR_OK;                  /* Fall through */

 fail:
  if (session != NULL) {
    digest_cleanup(&session->digest_handle);
    session->digest_algorithm = HAL_DIGEST_ALGORITHM_NONE;
  }
  mutex_unlock_return_with_rv(rv, p11_global_mutex);
}

CK_RV C_DigestUpdate(CK_SESSION_HANDLE hSession,
                     CK_BYTE_PTR pPart,
                     CK_ULONG ulPartLen)
{
  ENTER_PUBLIC_FUNCTION(C_DigestUpdate);

  p11_session_t *session;
  CK_RV rv = CKR_OK;

  mutex_lock_or_return_failure(p11_global_mutex);

  if ((session = p11_session_find(hSession)) == NULL)
    lose(CKR_SESSION_HANDLE_INVALID);

  if (pPart == NULL)
    lose(CKR_ARGUMENTS_BAD);

  if (session->digest_algorithm == HAL_DIGEST_ALGORITHM_NONE)
    lose(CKR_OPERATION_NOT_INITIALIZED);

  if ((rv = digest_update(session, session->digest_algorithm,
                          &session->digest_handle, pPart, ulPartLen)) != CKR_OK)
    goto fail;

  return mutex_unlock(p11_global_mutex);

 fail:
  if (session != NULL) {
    digest_cleanup(&session->digest_handle);
    session->digest_algorithm = HAL_DIGEST_ALGORITHM_NONE;
  }
  mutex_unlock_return_with_rv(rv, p11_global_mutex);
}

CK_RV C_DigestFinal(CK_SESSION_HANDLE hSession,
                    CK_BYTE_PTR pDigest,
                    CK_ULONG_PTR pulDigestLen)
{
  ENTER_PUBLIC_FUNCTION(C_DigestFinal);

  p11_session_t *session;
  size_t digest_len;
  CK_RV rv = CKR_OK;

  mutex_lock_or_return_failure(p11_global_mutex);

  if ((session = p11_session_find(hSession)) == NULL)
    lose(CKR_SESSION_HANDLE_INVALID);

  if (pulDigestLen == NULL)
    lose(CKR_ARGUMENTS_BAD);

  if (session->digest_algorithm == HAL_DIGEST_ALGORITHM_NONE || session->digest_handle.handle == HAL_HANDLE_NONE)
    lose(CKR_OPERATION_NOT_INITIALIZED);

  if (!hal_check(hal_rpc_hash_get_digest_length(session->digest_algorithm, &digest_len)))
    lose(CKR_FUNCTION_FAILED);

  rv = pDigest != NULL && *pulDigestLen < digest_len ? CKR_BUFFER_TOO_SMALL : CKR_OK;

  *pulDigestLen = digest_len;

  if (pDigest == NULL || rv == CKR_BUFFER_TOO_SMALL)
    mutex_unlock_return_with_rv(rv, p11_global_mutex);

  if (!hal_check(hal_rpc_hash_finalize(session->digest_handle, pDigest, *pulDigestLen)))
    lose(CKR_FUNCTION_FAILED);

  rv = CKR_OK;                  /* Fall through */

 fail:
  if (session != NULL) {
    digest_cleanup(&session->digest_handle);
    session->digest_algorithm = HAL_DIGEST_ALGORITHM_NONE;
  }
  mutex_unlock_return_with_rv(rv, p11_global_mutex);
}

CK_RV C_SignInit(CK_SESSION_HANDLE hSession,
                 CK_MECHANISM_PTR pMechanism,
                 CK_OBJECT_HANDLE hKey)
{
  ENTER_PUBLIC_FUNCTION(C_SignInit);

  uint8_t attributes_buffer[sizeof(CK_OBJECT_CLASS) + sizeof(CK_KEY_TYPE) + 3 * sizeof(CK_BBOOL)];
  hal_pkey_handle_t pkey = {HAL_HANDLE_NONE};
  hal_pkey_attribute_t attributes[] = {
    [0].type = CKA_KEY_TYPE,
    [1].type = CKA_SIGN,
    [2].type = CKA_PRIVATE,
    [3].type = CKA_TOKEN
  };
  CK_KEY_TYPE cka_key_type;
  CK_BBOOL cka_sign;
  CK_BBOOL cka_private;
  CK_BBOOL cka_token;
  p11_session_t *session;
  CK_RV rv = CKR_OK;

  mutex_lock_or_return_failure(p11_global_mutex);

  if ((session = p11_session_find(hSession)) == NULL)
    lose(CKR_SESSION_HANDLE_INVALID);

  if (pMechanism == NULL)
    lose(CKR_ARGUMENTS_BAD);

  if (session->sign_key_handle != CK_INVALID_HANDLE ||
      session->sign_digest_algorithm != HAL_DIGEST_ALGORITHM_NONE)
    lose(CKR_OPERATION_ACTIVE);

  if (!p11_object_pkey_open(session, hKey, &pkey))
    lose(CKR_KEY_HANDLE_INVALID);

  if (!hal_check(hal_rpc_pkey_get_attributes(pkey, attributes, sizeof(attributes)/sizeof(*attributes),
                                             attributes_buffer, sizeof(attributes_buffer))))
    lose(CKR_KEY_HANDLE_INVALID);

  cka_key_type = *(CK_KEY_TYPE*) attributes[0].value;
  cka_sign     = *(CK_BBOOL*)    attributes[1].value;
  cka_private  = *(CK_BBOOL*)    attributes[2].value;
  cka_token    = *(CK_BBOOL*)    attributes[3].value;

  if ((rv = p11_check_read_access(session, cka_private, cka_token)) != CKR_OK)
    goto fail;

  if (!cka_sign)
    lose(CKR_KEY_FUNCTION_NOT_PERMITTED);

  switch (pMechanism->mechanism) {
  case CKM_RSA_PKCS:
  case CKM_SHA1_RSA_PKCS:
  case CKM_SHA224_RSA_PKCS:
  case CKM_SHA256_RSA_PKCS:
  case CKM_SHA384_RSA_PKCS:
  case CKM_SHA512_RSA_PKCS:
    if (cka_key_type != CKK_RSA)
      lose(CKR_KEY_TYPE_INCONSISTENT);
    break;
  case CKM_ECDSA:
  case CKM_ECDSA_SHA224:
  case CKM_ECDSA_SHA256:
  case CKM_ECDSA_SHA384:
  case CKM_ECDSA_SHA512:
    if (cka_key_type != CKK_EC)
      lose(CKR_KEY_TYPE_INCONSISTENT);
    break;
  default:
    return CKR_MECHANISM_INVALID;
  }

  session->sign_key_handle = hKey;

  switch (pMechanism->mechanism) {
  case CKM_RSA_PKCS:
  case CKM_ECDSA:
    session->sign_digest_algorithm = HAL_DIGEST_ALGORITHM_NONE;
    break;
  case CKM_SHA1_RSA_PKCS:
    session->sign_digest_algorithm = HAL_DIGEST_ALGORITHM_SHA1;
    break;
  case CKM_SHA224_RSA_PKCS:
  case CKM_ECDSA_SHA224:
    session->sign_digest_algorithm = HAL_DIGEST_ALGORITHM_SHA224;
    break;
  case CKM_SHA256_RSA_PKCS:
  case CKM_ECDSA_SHA256:
    session->sign_digest_algorithm = HAL_DIGEST_ALGORITHM_SHA256;
    break;
  case CKM_SHA384_RSA_PKCS:
  case CKM_ECDSA_SHA384:
    session->sign_digest_algorithm = HAL_DIGEST_ALGORITHM_SHA384;
    break;
  case CKM_SHA512_RSA_PKCS:
  case CKM_ECDSA_SHA512:
    session->sign_digest_algorithm = HAL_DIGEST_ALGORITHM_SHA512;
    break;
  default:
    return CKR_MECHANISM_INVALID;
  }

  rv = CKR_OK;

 fail:
  if (pkey.handle != HAL_HANDLE_NONE)
    (void) hal_rpc_pkey_close(pkey);
  if (rv != CKR_OK && session != NULL) {
    session->sign_key_handle = CK_INVALID_HANDLE;
    session->sign_digest_algorithm = HAL_DIGEST_ALGORITHM_NONE;
  }
  mutex_unlock_return_with_rv(rv, p11_global_mutex);
}

CK_RV C_Sign(CK_SESSION_HANDLE hSession,
             CK_BYTE_PTR pData,
             CK_ULONG ulDataLen,
             CK_BYTE_PTR pSignature,
             CK_ULONG_PTR pulSignatureLen)
{
  ENTER_PUBLIC_FUNCTION(C_Sign);

  p11_session_t *session;
  CK_RV rv;

  mutex_lock_or_return_failure(p11_global_mutex);

  if ((session = p11_session_find(hSession)) == NULL)
    lose(CKR_SESSION_HANDLE_INVALID);

  if (pData == NULL || pulSignatureLen == NULL)
    lose(CKR_ARGUMENTS_BAD);

  if (session->sign_key_handle == CK_INVALID_HANDLE)
    lose(CKR_OPERATION_NOT_INITIALIZED);

  if (session->sign_digest_handle.handle != HAL_HANDLE_NONE)
    lose(CKR_OPERATION_ACTIVE);

  if (session->sign_digest_algorithm != HAL_DIGEST_ALGORITHM_NONE && pSignature != NULL) {
    if ((rv = digest_update(session, session->sign_digest_algorithm,
                            &session->sign_digest_handle, pData, ulDataLen)) != CKR_OK)
      goto fail;
    pData = NULL;
    ulDataLen = 0;
  }

  rv = sign_hal_rpc(session, pData, ulDataLen, pSignature, pulSignatureLen);

                                /* Fall through */
 fail:
  if (session != NULL && pSignature != NULL && rv != CKR_BUFFER_TOO_SMALL) {
    session->sign_key_handle = CK_INVALID_HANDLE;
    session->sign_digest_algorithm = HAL_DIGEST_ALGORITHM_NONE;
    digest_cleanup(&session->sign_digest_handle);
  }

  mutex_unlock_return_with_rv(rv, p11_global_mutex);
}

CK_RV C_SignUpdate(CK_SESSION_HANDLE hSession,
                   CK_BYTE_PTR pPart,
                   CK_ULONG ulPartLen)
{
  ENTER_PUBLIC_FUNCTION(C_SignUpdate);

  p11_session_t *session;
  CK_RV rv;

  mutex_lock_or_return_failure(p11_global_mutex);

  if ((session = p11_session_find(hSession)) == NULL)
    lose(CKR_SESSION_HANDLE_INVALID);

  if (pPart == NULL)
    lose(CKR_ARGUMENTS_BAD);

  if (session->sign_key_handle == CK_INVALID_HANDLE)
    lose(CKR_OPERATION_NOT_INITIALIZED);

  if (session->sign_digest_algorithm == HAL_DIGEST_ALGORITHM_NONE)
    lose(CKR_FUNCTION_FAILED);

  if ((rv = digest_update(session, session->sign_digest_algorithm,
                          &session->sign_digest_handle, pPart, ulPartLen)) != CKR_OK)
    goto fail;

  return mutex_unlock(p11_global_mutex);

 fail:
  if (session != NULL) {
    session->sign_key_handle = CK_INVALID_HANDLE;
    session->sign_digest_algorithm = HAL_DIGEST_ALGORITHM_NONE;
    digest_cleanup(&session->sign_digest_handle);
  }

  mutex_unlock_return_with_rv(rv, p11_global_mutex);
}

CK_RV C_SignFinal(CK_SESSION_HANDLE hSession,
                  CK_BYTE_PTR pSignature,
                  CK_ULONG_PTR pulSignatureLen)
{
  ENTER_PUBLIC_FUNCTION(C_SignFinal);

  p11_session_t *session;
  CK_RV rv;

  mutex_lock_or_return_failure(p11_global_mutex);

  if ((session = p11_session_find(hSession)) == NULL)
    lose(CKR_SESSION_HANDLE_INVALID);

  if (pulSignatureLen == NULL)
    lose(CKR_ARGUMENTS_BAD);

  if (session->sign_key_handle == CK_INVALID_HANDLE ||
      session->sign_digest_handle.handle == HAL_HANDLE_NONE)
    lose(CKR_OPERATION_NOT_INITIALIZED);

  rv = sign_hal_rpc(session, NULL, 0, pSignature, pulSignatureLen);

                                /* Fall through */
 fail:
  if (session != NULL && pSignature != NULL && rv != CKR_BUFFER_TOO_SMALL) {
    session->sign_key_handle = CK_INVALID_HANDLE;
    session->sign_digest_algorithm = HAL_DIGEST_ALGORITHM_NONE;
    digest_cleanup(&session->sign_digest_handle);
  }

  mutex_unlock_return_with_rv(rv, p11_global_mutex);
}

CK_RV C_VerifyInit(CK_SESSION_HANDLE hSession,
                   CK_MECHANISM_PTR pMechanism,
                   CK_OBJECT_HANDLE hKey )
{
  ENTER_PUBLIC_FUNCTION(C_VerifyInit);

  uint8_t attributes_buffer[sizeof(CK_OBJECT_CLASS) + sizeof(CK_KEY_TYPE) + 3 * sizeof(CK_BBOOL)];
  hal_pkey_handle_t pkey = {HAL_HANDLE_NONE};
  hal_pkey_attribute_t attributes[] = {
    [0].type = CKA_KEY_TYPE,
    [1].type = CKA_VERIFY,
    [2].type = CKA_PRIVATE,
    [3].type = CKA_TOKEN
  };
  CK_KEY_TYPE cka_key_type;
  CK_BBOOL cka_verify;
  CK_BBOOL cka_private;
  CK_BBOOL cka_token;

  p11_session_t *session;
  CK_RV rv = CKR_OK;

  mutex_lock_or_return_failure(p11_global_mutex);

  if ((session = p11_session_find(hSession)) == NULL)
    lose(CKR_SESSION_HANDLE_INVALID);

  if (pMechanism == NULL)
    lose(CKR_ARGUMENTS_BAD);

  if (session->verify_key_handle != CK_INVALID_HANDLE ||
      session->verify_digest_algorithm != HAL_DIGEST_ALGORITHM_NONE)
    lose(CKR_OPERATION_ACTIVE);

  if (!p11_object_pkey_open(session, hKey, &pkey))
    lose(CKR_KEY_HANDLE_INVALID);

  if (!hal_check(hal_rpc_pkey_get_attributes(pkey, attributes, sizeof(attributes)/sizeof(*attributes),
                                             attributes_buffer, sizeof(attributes_buffer))))
    lose(CKR_KEY_HANDLE_INVALID);

  cka_key_type = *(CK_KEY_TYPE*) attributes[0].value;
  cka_verify   = *(CK_BBOOL*)    attributes[1].value;
  cka_private  = *(CK_BBOOL*)    attributes[2].value;
  cka_token    = *(CK_BBOOL*)    attributes[3].value;

  if ((rv = p11_check_read_access(session, cka_private, cka_token)) != CKR_OK)
    goto fail;

  if (!cka_verify)
    lose(CKR_KEY_FUNCTION_NOT_PERMITTED);

  switch (pMechanism->mechanism) {
  case CKM_RSA_PKCS:
  case CKM_SHA1_RSA_PKCS:
  case CKM_SHA224_RSA_PKCS:
  case CKM_SHA256_RSA_PKCS:
  case CKM_SHA384_RSA_PKCS:
  case CKM_SHA512_RSA_PKCS:
    if (cka_key_type != CKK_RSA)
      lose(CKR_KEY_TYPE_INCONSISTENT);
    break;
  case CKM_ECDSA:
  case CKM_ECDSA_SHA224:
  case CKM_ECDSA_SHA256:
  case CKM_ECDSA_SHA384:
  case CKM_ECDSA_SHA512:
    if (cka_key_type != CKK_EC)
      lose(CKR_KEY_TYPE_INCONSISTENT);
    break;
  default:
    return CKR_MECHANISM_INVALID;
  }

  session->verify_key_handle = hKey;

  switch (pMechanism->mechanism) {
  case CKM_RSA_PKCS:
  case CKM_ECDSA:
    session->verify_digest_algorithm = HAL_DIGEST_ALGORITHM_NONE;
    break;
  case CKM_SHA1_RSA_PKCS:
    session->verify_digest_algorithm = HAL_DIGEST_ALGORITHM_SHA1;
    break;
  case CKM_SHA224_RSA_PKCS:
  case CKM_ECDSA_SHA224:
    session->verify_digest_algorithm = HAL_DIGEST_ALGORITHM_SHA224;
    break;
  case CKM_SHA256_RSA_PKCS:
  case CKM_ECDSA_SHA256:
    session->verify_digest_algorithm = HAL_DIGEST_ALGORITHM_SHA256;
    break;
  case CKM_SHA384_RSA_PKCS:
  case CKM_ECDSA_SHA384:
    session->verify_digest_algorithm = HAL_DIGEST_ALGORITHM_SHA384;
    break;
  case CKM_SHA512_RSA_PKCS:
  case CKM_ECDSA_SHA512:
    session->verify_digest_algorithm = HAL_DIGEST_ALGORITHM_SHA512;
    break;
  default:
    return CKR_MECHANISM_INVALID;
  }

  rv = CKR_OK;

 fail:
  if (pkey.handle != HAL_HANDLE_NONE)
    (void) hal_rpc_pkey_close(pkey);
  if (rv != CKR_OK && session != NULL) {
    session->verify_key_handle = CK_INVALID_HANDLE;
    session->verify_digest_algorithm = HAL_DIGEST_ALGORITHM_NONE;
  }
  mutex_unlock_return_with_rv(rv, p11_global_mutex);
}

CK_RV C_Verify(CK_SESSION_HANDLE hSession,
               CK_BYTE_PTR pData,
               CK_ULONG ulDataLen,
               CK_BYTE_PTR pSignature,
               CK_ULONG ulSignatureLen)
{
  ENTER_PUBLIC_FUNCTION(C_Verify);

  p11_session_t *session;
  CK_RV rv;

  mutex_lock_or_return_failure(p11_global_mutex);

  if ((session = p11_session_find(hSession)) == NULL)
    lose(CKR_SESSION_HANDLE_INVALID);

  if (pData == NULL || pSignature == NULL)
    lose(CKR_ARGUMENTS_BAD);

  if (session->verify_key_handle == CK_INVALID_HANDLE)
    lose(CKR_OPERATION_NOT_INITIALIZED);

  if (session->verify_digest_algorithm != HAL_DIGEST_ALGORITHM_NONE) {
    if ((rv = digest_update(session, session->verify_digest_algorithm,
                            &session->verify_digest_handle, pData, ulDataLen)) != CKR_OK)
      goto fail;
    pData = NULL;
    ulDataLen = 0;
  }

  rv = verify_hal_rpc(session, pData, ulDataLen, pSignature, ulSignatureLen);

 fail:                          /* Fall through */

  if (session != NULL) {
    session->verify_key_handle = CK_INVALID_HANDLE;
    session->verify_digest_algorithm = HAL_DIGEST_ALGORITHM_NONE;
    digest_cleanup(&session->verify_digest_handle);
  }

  mutex_unlock_return_with_rv(rv, p11_global_mutex);
}

CK_RV C_VerifyUpdate(CK_SESSION_HANDLE hSession,
                     CK_BYTE_PTR pPart,
                     CK_ULONG ulPartLen)
{
  ENTER_PUBLIC_FUNCTION(C_VerifyUpdate);

  p11_session_t *session;
  CK_RV rv;

  mutex_lock_or_return_failure(p11_global_mutex);

  if ((session = p11_session_find(hSession)) == NULL)
    lose(CKR_SESSION_HANDLE_INVALID);

  if (pPart == NULL)
    lose(CKR_ARGUMENTS_BAD);

  if (session->verify_key_handle == CK_INVALID_HANDLE)
    lose(CKR_OPERATION_NOT_INITIALIZED);

  if (session->verify_digest_algorithm == HAL_DIGEST_ALGORITHM_NONE)
    lose(CKR_FUNCTION_FAILED);

  if ((rv = digest_update(session, session->verify_digest_algorithm,
                          &session->verify_digest_handle, pPart, ulPartLen)) != CKR_OK)
    goto fail;

  return mutex_unlock(p11_global_mutex);

 fail:
  if (session != NULL) {
    session->verify_key_handle = CK_INVALID_HANDLE;
    session->verify_digest_algorithm = HAL_DIGEST_ALGORITHM_NONE;
    digest_cleanup(&session->verify_digest_handle);
  }

  mutex_unlock_return_with_rv(rv, p11_global_mutex);
}

CK_RV C_VerifyFinal(CK_SESSION_HANDLE hSession,
                    CK_BYTE_PTR pSignature,
                    CK_ULONG ulSignatureLen)
{
  ENTER_PUBLIC_FUNCTION(C_VerifyFinal);

  p11_session_t *session;
  CK_RV rv;

  mutex_lock_or_return_failure(p11_global_mutex);

  if ((session = p11_session_find(hSession)) == NULL)
    lose(CKR_SESSION_HANDLE_INVALID);

  if (pSignature == NULL)
    lose(CKR_ARGUMENTS_BAD);

  if (session->verify_key_handle == CK_INVALID_HANDLE ||
      session->verify_digest_handle.handle == HAL_HANDLE_NONE)
    lose(CKR_OPERATION_NOT_INITIALIZED);

  rv = verify_hal_rpc(session, NULL, 0, pSignature, ulSignatureLen);

 fail:                          /* Fall through */

  if (session != NULL) {
    session->verify_key_handle = CK_INVALID_HANDLE;
    session->verify_digest_algorithm = HAL_DIGEST_ALGORITHM_NONE;
    digest_cleanup(&session->verify_digest_handle);
  }

  mutex_unlock_return_with_rv(rv, p11_global_mutex);
}

/*
 * If there's any method in this entire package which really needs a
 * more complex mutex structure than the single global mutex, it's
 * probably this one.  Key generation can take a looooong time.
 * Drive off that bridge when we get to it.
 */

CK_RV C_GenerateKeyPair(CK_SESSION_HANDLE hSession,
                        CK_MECHANISM_PTR pMechanism,
                        CK_ATTRIBUTE_PTR pPublicKeyTemplate,
                        CK_ULONG ulPublicKeyAttributeCount,
                        CK_ATTRIBUTE_PTR pPrivateKeyTemplate,
                        CK_ULONG ulPrivateKeyAttributeCount,
                        CK_OBJECT_HANDLE_PTR phPublicKey,
                        CK_OBJECT_HANDLE_PTR phPrivateKey)
{
  ENTER_PUBLIC_FUNCTION(C_GenerateKeyPair);

  p11_session_t *session;
  CK_RV rv;

  mutex_lock_or_return_failure(p11_global_mutex);

  if ((session = p11_session_find(hSession)) == NULL)
    lose(CKR_SESSION_HANDLE_INVALID);

  if (pMechanism          == NULL ||
      pPublicKeyTemplate  == NULL || phPublicKey  == NULL ||
      pPrivateKeyTemplate == NULL || phPrivateKey == NULL)
    lose(CKR_ARGUMENTS_BAD);

  switch (pMechanism->mechanism) {

  case CKM_RSA_PKCS_KEY_PAIR_GEN:
    rv = generate_keypair(session, pMechanism, generate_keypair_rsa_pkcs,
                          pPublicKeyTemplate,  ulPublicKeyAttributeCount,  &p11_descriptor_rsa_public_key,  phPublicKey,
                          pPrivateKeyTemplate, ulPrivateKeyAttributeCount, &p11_descriptor_rsa_private_key, phPrivateKey);
    break;

  case CKM_EC_KEY_PAIR_GEN:
    rv = generate_keypair(session, pMechanism, generate_keypair_ec,
                          pPublicKeyTemplate,  ulPublicKeyAttributeCount,  &p11_descriptor_ec_public_key,  phPublicKey,
                          pPrivateKeyTemplate, ulPrivateKeyAttributeCount, &p11_descriptor_ec_private_key, phPrivateKey);
    break;

  default:
    lose(CKR_MECHANISM_INVALID);
  }

 fail:
  mutex_unlock_return_with_rv(rv, p11_global_mutex);
}

CK_RV C_GenerateRandom(CK_SESSION_HANDLE hSession,
                       CK_BYTE_PTR RandomData,
                       CK_ULONG ulRandomLen)
{
  ENTER_PUBLIC_FUNCTION(C_GenerateRandom);

  p11_session_t *session;
  CK_RV rv = CKR_OK;

  mutex_lock_or_return_failure(p11_global_mutex);

  if ((session = p11_session_find(hSession)) == NULL)
    lose(CKR_SESSION_HANDLE_INVALID);

  if (RandomData == NULL)
    lose(CKR_ARGUMENTS_BAD);

  if (!hal_check(hal_rpc_get_random(RandomData, ulRandomLen)))
    lose(CKR_FUNCTION_FAILED);

 fail:
  mutex_unlock_return_with_rv(rv, p11_global_mutex);
}

/*
 * Supply information about a particular mechanism.  We may want a
 * more generic structure for this, for the moment, just answer the
 * questions that applications we care about are asking.
 */

CK_RV C_GetMechanismInfo(CK_SLOT_ID slotID,
                         CK_MECHANISM_TYPE type,
                         CK_MECHANISM_INFO_PTR pInfo)
{
  ENTER_PUBLIC_FUNCTION(C_GetMechanismInfo);

  const CK_ULONG rsa_key_min = 1024;
  const CK_ULONG rsa_key_max = 8192;
  const CK_ULONG ec_key_min  = 256;
  const CK_ULONG ec_key_max  = 521;

  /*
   * No locking here, no obvious need for it.
   */

  if (pInfo == NULL)
    return CKR_ARGUMENTS_BAD;

  if (slotID != P11_ONE_AND_ONLY_SLOT)
    return CKR_SLOT_ID_INVALID;

  if (p11_uninitialized())
    return CKR_CRYPTOKI_NOT_INITIALIZED;

  switch (type) {

  case CKM_RSA_PKCS_KEY_PAIR_GEN:
    pInfo->ulMinKeySize = rsa_key_min;
    pInfo->ulMaxKeySize = rsa_key_max;
    pInfo->flags = CKF_HW | CKF_GENERATE_KEY_PAIR;
    break;

  case CKM_EC_KEY_PAIR_GEN:
    pInfo->ulMinKeySize = ec_key_min;
    pInfo->ulMaxKeySize = ec_key_max;
    pInfo->flags = CKF_HW | CKF_GENERATE_KEY_PAIR | CKF_EC_F_P | CKF_EC_NAMEDCURVE | CKF_EC_UNCOMPRESS;
    break;

  case CKM_RSA_PKCS:
  case CKM_SHA1_RSA_PKCS:
  case CKM_SHA224_RSA_PKCS:
  case CKM_SHA256_RSA_PKCS:
  case CKM_SHA384_RSA_PKCS:
  case CKM_SHA512_RSA_PKCS:
    pInfo->ulMinKeySize = rsa_key_min;
    pInfo->ulMaxKeySize = rsa_key_max;
    pInfo->flags = CKF_HW | CKF_SIGN | CKF_VERIFY;
    break;

  case CKM_ECDSA:
  case CKM_ECDSA_SHA224:
  case CKM_ECDSA_SHA256:
  case CKM_ECDSA_SHA384:
  case CKM_ECDSA_SHA512:
    pInfo->ulMinKeySize = ec_key_min;
    pInfo->ulMaxKeySize = ec_key_max;
    pInfo->flags = CKF_HW | CKF_SIGN | CKF_VERIFY | CKF_EC_F_P | CKF_EC_NAMEDCURVE | CKF_EC_UNCOMPRESS;
    break;

  case CKM_SHA_1:
  case CKM_SHA224:
  case CKM_SHA256:
  case CKM_SHA384:
  case CKM_SHA512:
    pInfo->ulMinKeySize = 0;
    pInfo->ulMaxKeySize = 0;
    pInfo->flags = CKF_HW | CKF_DIGEST;
    break;

#if 0
    /*
     * libhal supports HMAC, but we have no PKCS #11 HMAC support (yet).
     *
     * HMAC in PKCS #11 is a bit weird (what a surprise).  It uses the
     * C_Sign*()/C_Verify*() API, with "generic secret key" objects
     * (CKO_SECRET_KEY, CKK_GENERIC_SECRET): these can be created with
     * C_CreateObject() (user-supplied HMAC key) or C_GenerateKey()
     * (HSM-generated HMAC key, probably from TRNG).  The CKM_*_HMAC
     * mechanisms have fixed-length output; the CKM_*_HMAC_GENERAL
     * mechanisms are variable-width output.
     */
  case CKM_SHA_1_HMAC:
  case CKM_SHA224_HMAC:
  case CKM_SHA256_HMAC:
  case CKM_SHA384_HMAC:
  case CKM_SHA512_HMAC:
#endif

  default:
    return CKR_MECHANISM_INVALID;
  }

  return CKR_OK;
}

CK_RV C_GetSessionInfo(CK_SESSION_HANDLE hSession,
                       CK_SESSION_INFO_PTR pInfo)
{
  ENTER_PUBLIC_FUNCTION(C_GetSessionInfo);

  p11_session_t *session;
  CK_RV rv = CKR_OK;

  mutex_lock_or_return_failure(p11_global_mutex);

  if (pInfo == NULL)
    lose(CKR_ARGUMENTS_BAD);

  if ((session = p11_session_find(hSession)) == NULL)
    lose(CKR_SESSION_HANDLE_INVALID);

  pInfo->slotID = P11_ONE_AND_ONLY_SLOT;
  pInfo->state = session->state;
  pInfo->flags = CKF_SERIAL_SESSION;
  pInfo->ulDeviceError = 0;

  switch (session->state) {
  case CKS_RW_PUBLIC_SESSION:
  case CKS_RW_SO_FUNCTIONS:
  case CKS_RW_USER_FUNCTIONS:
    pInfo->flags |= CKF_RW_SESSION;
  default:
    break;
  }

 fail:
  mutex_unlock_return_with_rv(rv, p11_global_mutex);
}

CK_RV C_GetInfo(CK_INFO_PTR pInfo)
{
  ENTER_PUBLIC_FUNCTION(C_GetInfo);

  if (pInfo == NULL)
    return CKR_ARGUMENTS_BAD;

  if (p11_uninitialized())
    return CKR_CRYPTOKI_NOT_INITIALIZED;

  memset(pInfo, 0, sizeof(*pInfo));
  pInfo->cryptokiVersion.major = 2;
  pInfo->cryptokiVersion.minor = 30;
  psnprintf(pInfo->manufacturerID,     sizeof(pInfo->manufacturerID),     P11_MANUFACTURER_ID);
  psnprintf(pInfo->libraryDescription, sizeof(pInfo->libraryDescription), P11_LIBRARY_DESCRIPTION);
  pInfo->libraryVersion.major = P11_VERSION_SW_MAJOR;
  pInfo->libraryVersion.minor = P11_VERSION_SW_MINOR;

  return CKR_OK;
}

CK_RV C_GetSlotInfo(CK_SLOT_ID slotID,
                    CK_SLOT_INFO_PTR pInfo)
{
  ENTER_PUBLIC_FUNCTION(C_GetSlotInfo);

  if (pInfo == NULL)
    return CKR_ARGUMENTS_BAD;

  if (slotID != P11_ONE_AND_ONLY_SLOT)
    return CKR_SLOT_ID_INVALID;

  if (p11_uninitialized())
    return CKR_CRYPTOKI_NOT_INITIALIZED;

  memset(pInfo, 0, sizeof(*pInfo));
  psnprintf(pInfo->slotDescription, sizeof(pInfo->slotDescription), P11_SLOT_DESCRIPTION);
  psnprintf(pInfo->manufacturerID,  sizeof(pInfo->manufacturerID),  P11_MANUFACTURER_ID);
  pInfo->flags = CKF_TOKEN_PRESENT | CKF_HW_SLOT;
  pInfo->hardwareVersion.major = P11_VERSION_HW_MAJOR;
  pInfo->hardwareVersion.minor = P11_VERSION_HW_MINOR;
  pInfo->firmwareVersion.major = P11_VERSION_FW_MAJOR;
  pInfo->firmwareVersion.minor = P11_VERSION_FW_MINOR;
  return CKR_OK;
}

CK_RV C_GetMechanismList(CK_SLOT_ID slotID,
                         CK_MECHANISM_TYPE_PTR pMechanismList,
                         CK_ULONG_PTR pulCount)
{
  static const CK_MECHANISM_TYPE mechanisms[] = {
                       CKM_ECDSA_SHA224,       CKM_ECDSA_SHA256,    CKM_ECDSA_SHA384,    CKM_ECDSA_SHA512,    CKM_ECDSA,    CKM_EC_KEY_PAIR_GEN,
    CKM_SHA1_RSA_PKCS, CKM_SHA224_RSA_PKCS,    CKM_SHA256_RSA_PKCS, CKM_SHA384_RSA_PKCS, CKM_SHA512_RSA_PKCS, CKM_RSA_PKCS, CKM_RSA_PKCS_KEY_PAIR_GEN,
    CKM_SHA_1,         CKM_SHA224, 	       CKM_SHA256,          CKM_SHA384,          CKM_SHA512,
#if 0
    /* libhal support these but pkcs11 doesn't, yet */
    CKM_SHA_1_HMAC,    CKM_SHA224_HMAC,        CKM_SHA256_HMAC,     CKM_SHA384_HMAC,     CKM_SHA512_HMAC,
#endif
  };
  const CK_ULONG mechanisms_len = sizeof(mechanisms)/sizeof(*mechanisms);

  ENTER_PUBLIC_FUNCTION(C_GetMechanismList);

  if (pulCount == NULL)
    return CKR_ARGUMENTS_BAD;

  if (slotID != P11_ONE_AND_ONLY_SLOT)
    return CKR_SLOT_ID_INVALID;

  if (p11_uninitialized())
    return CKR_CRYPTOKI_NOT_INITIALIZED;

  CK_RV rv = CKR_OK;

  if (pMechanismList != NULL && *pulCount < mechanisms_len)
    rv = CKR_BUFFER_TOO_SMALL;

  else if (pMechanismList != NULL)
    memcpy(pMechanismList, mechanisms, sizeof(mechanisms));

  *pulCount = mechanisms_len;

  return rv;
}

CK_RV C_SeedRandom(CK_SESSION_HANDLE hSession,
                   CK_BYTE_PTR pSeed,
                   CK_ULONG ulSeedLen)
{
  ENTER_PUBLIC_FUNCTION(C_SeedRandom);

  if (p11_uninitialized())
    return CKR_CRYPTOKI_NOT_INITIALIZED;

  return CKR_RANDOM_SEED_NOT_SUPPORTED;
}



/*
 * Legacy functions.  These are basically just unimplemented functions
 * which return a different error code to keep test suites happy.
 */

CK_RV C_GetFunctionStatus(CK_SESSION_HANDLE hSession)
{
  ENTER_PUBLIC_FUNCTION(C_GetFunctionStatus);

  if (p11_uninitialized())
    return CKR_CRYPTOKI_NOT_INITIALIZED;

  return CKR_FUNCTION_NOT_PARALLEL;
}

CK_RV C_CancelFunction(CK_SESSION_HANDLE hSession)
{
  ENTER_PUBLIC_FUNCTION(C_CancelFunction);

  if (p11_uninitialized())
    return CKR_CRYPTOKI_NOT_INITIALIZED;

  return CKR_FUNCTION_NOT_PARALLEL;
}



/*
 * Stubs for unsupported functions below here.  Per the PKCS #11
 * specification, it's OK to skip implementing almost any function in
 * the API, but if one does so, one must provide a stub which returns
 * CKR_FUNCTION_NOT_SUPPORTED, because every slot in the dispatch
 * vector must be populated.  We could reuse a single stub for all the
 * unimplemented slots, but the type signatures wouldn't match, which
 * would require some nasty casts I'd rather avoid.
 *
 * Many of these functions would be straightforward to implement, but
 * there are enough bald yaks in this saga already.
 */

CK_RV C_GenerateKey(CK_SESSION_HANDLE hSession,
                    CK_MECHANISM_PTR pMechanism,
                    CK_ATTRIBUTE_PTR pTemplate,
                    CK_ULONG ulCount,
                    CK_OBJECT_HANDLE_PTR phKey)
{
  UNSUPPORTED_FUNCTION(C_GenerateKey);
}

CK_RV C_InitToken(CK_SLOT_ID slotID,
                  CK_UTF8CHAR_PTR pPin,
                  CK_ULONG ulPinLen,
                  CK_UTF8CHAR_PTR pLabel)
{
  UNSUPPORTED_FUNCTION(C_InitToken);
}

CK_RV C_InitPIN(CK_SESSION_HANDLE hSession,
                CK_UTF8CHAR_PTR pPin,
                CK_ULONG ulPinLen)
{
  UNSUPPORTED_FUNCTION(C_InitPIN);
}

CK_RV C_SetPIN(CK_SESSION_HANDLE hSession,
               CK_UTF8CHAR_PTR pOldPin,
               CK_ULONG ulOldLen,
               CK_UTF8CHAR_PTR pNewPin,
               CK_ULONG ulNewLen)
{
  UNSUPPORTED_FUNCTION(C_SetPIN);
}

CK_RV C_GetOperationState(CK_SESSION_HANDLE hSession,
                          CK_BYTE_PTR pOperationState,
                          CK_ULONG_PTR pulOperationStateLen)
{
  UNSUPPORTED_FUNCTION(C_GetOperationState);
}

CK_RV C_SetOperationState(CK_SESSION_HANDLE hSession,
                          CK_BYTE_PTR pOperationState,
                          CK_ULONG ulOperationStateLen,
                          CK_OBJECT_HANDLE hEncryptionKey,
                          CK_OBJECT_HANDLE hAuthenticationKey)
{
  UNSUPPORTED_FUNCTION(C_SetOperationState);
}

CK_RV C_CopyObject(CK_SESSION_HANDLE hSession,
                   CK_OBJECT_HANDLE hObject,
                   CK_ATTRIBUTE_PTR pTemplate,
                   CK_ULONG ulCount,
                   CK_OBJECT_HANDLE_PTR phNewObject)
{
  UNSUPPORTED_FUNCTION(C_CopyObject);
}

CK_RV C_GetObjectSize(CK_SESSION_HANDLE hSession,
                      CK_OBJECT_HANDLE hObject,
                      CK_ULONG_PTR pulSize)
{
  UNSUPPORTED_FUNCTION(C_GetObjectSize);
}

CK_RV C_SetAttributeValue(CK_SESSION_HANDLE hSession,
                          CK_OBJECT_HANDLE hObject,
                          CK_ATTRIBUTE_PTR pTemplate,
                          CK_ULONG ulCount)
{
  UNSUPPORTED_FUNCTION(C_SetAttributeValue);
}

CK_RV C_EncryptInit(CK_SESSION_HANDLE hSession,
                    CK_MECHANISM_PTR pMechanism,
                    CK_OBJECT_HANDLE hKey)
{
  UNSUPPORTED_FUNCTION(C_EncryptInit);
}

CK_RV C_Encrypt(CK_SESSION_HANDLE hSession,
                CK_BYTE_PTR pData,
                CK_ULONG ulDataLen,
                CK_BYTE_PTR pEncryptedData,
                CK_ULONG_PTR pulEncryptedDataLen)
{
  UNSUPPORTED_FUNCTION(C_Encrypt);
}

CK_RV C_EncryptUpdate(CK_SESSION_HANDLE hSession,
                      CK_BYTE_PTR pPart,
                      CK_ULONG ulPartLen,
                      CK_BYTE_PTR pEncryptedPart,
                      CK_ULONG_PTR pulEncryptedPartLen)
{
  UNSUPPORTED_FUNCTION(C_EncryptUpdate);
}

CK_RV C_EncryptFinal(CK_SESSION_HANDLE hSession,
                     CK_BYTE_PTR pLastEncryptedPart,
                     CK_ULONG_PTR pulLastEncryptedPartLen)
{
  UNSUPPORTED_FUNCTION(C_EncryptFinal);
}

CK_RV C_DecryptInit(CK_SESSION_HANDLE hSession,
                    CK_MECHANISM_PTR pMechanism,
                    CK_OBJECT_HANDLE hKey)
{
  UNSUPPORTED_FUNCTION(C_DecryptInit);
}

CK_RV C_Decrypt(CK_SESSION_HANDLE hSession,
                CK_BYTE_PTR pEncryptedData,
                CK_ULONG ulEncryptedDataLen,
                CK_BYTE_PTR pData,
                CK_ULONG_PTR pulDataLen)
{
  UNSUPPORTED_FUNCTION(C_Decrypt);
}

CK_RV C_DecryptUpdate(CK_SESSION_HANDLE hSession,
                      CK_BYTE_PTR pEncryptedPart,
                      CK_ULONG ulEncryptedPartLen,
                      CK_BYTE_PTR pPart,
                      CK_ULONG_PTR pulPartLen)
{
  UNSUPPORTED_FUNCTION(C_DecryptUpdate);
}

CK_RV C_DecryptFinal(CK_SESSION_HANDLE hSession,
                     CK_BYTE_PTR pLastPart,
                     CK_ULONG_PTR pulLastPartLen)
{
  UNSUPPORTED_FUNCTION(C_DecryptFinal);
}

CK_RV C_DigestKey(CK_SESSION_HANDLE hSession,
                  CK_OBJECT_HANDLE hKey)
{
  UNSUPPORTED_FUNCTION(C_DigestKey);
}

CK_RV C_SignRecoverInit(CK_SESSION_HANDLE hSession,
                        CK_MECHANISM_PTR pMechanism,
                        CK_OBJECT_HANDLE hKey)
{
  UNSUPPORTED_FUNCTION(C_SignRecoverInit);
}

CK_RV C_SignRecover(CK_SESSION_HANDLE hSession,
                    CK_BYTE_PTR pData,
                    CK_ULONG ulDataLen,
                    CK_BYTE_PTR pSignature,
                    CK_ULONG_PTR pulSignatureLen)
{
  UNSUPPORTED_FUNCTION(C_SignRecover);
}

CK_RV C_VerifyRecoverInit(CK_SESSION_HANDLE hSession,
                          CK_MECHANISM_PTR pMechanism,
                          CK_OBJECT_HANDLE hKey)
{
  UNSUPPORTED_FUNCTION(C_VerifyRecoverInit);
}

CK_RV C_VerifyRecover(CK_SESSION_HANDLE hSession,
                      CK_BYTE_PTR pSignature,
                      CK_ULONG ulSignatureLen,
                      CK_BYTE_PTR pData,
                      CK_ULONG_PTR pulDataLen)
{
  UNSUPPORTED_FUNCTION(C_VerifyRecover);
}

CK_RV C_DigestEncryptUpdate(CK_SESSION_HANDLE hSession,
                            CK_BYTE_PTR pPart,
                            CK_ULONG ulPartLen,
                            CK_BYTE_PTR pEncryptedPart,
                            CK_ULONG_PTR pulEncryptedPartLen)
{
  UNSUPPORTED_FUNCTION(C_DigestEncryptUpdate);
}

CK_RV C_DecryptDigestUpdate(CK_SESSION_HANDLE hSession,
                            CK_BYTE_PTR pEncryptedPart,
                            CK_ULONG ulEncryptedPartLen,
                            CK_BYTE_PTR pPart,
                            CK_ULONG_PTR pulPartLen)
{
  UNSUPPORTED_FUNCTION(C_DecryptDigestUpdate);
}

CK_RV C_SignEncryptUpdate(CK_SESSION_HANDLE hSession,
                          CK_BYTE_PTR pPart,
                          CK_ULONG ulPartLen,
                          CK_BYTE_PTR pEncryptedPart,
                          CK_ULONG_PTR pulEncryptedPartLen)
{
  UNSUPPORTED_FUNCTION(C_SignEncryptUpdate);
}

CK_RV C_DecryptVerifyUpdate(CK_SESSION_HANDLE hSession,
                            CK_BYTE_PTR pEncryptedPart,
                            CK_ULONG ulEncryptedPartLen,
                            CK_BYTE_PTR pPart,
                            CK_ULONG_PTR pulPartLen)
{
  UNSUPPORTED_FUNCTION(C_DecryptVerifyUpdate);
}

CK_RV C_WrapKey(CK_SESSION_HANDLE hSession,
                CK_MECHANISM_PTR pMechanism,
                CK_OBJECT_HANDLE hWrappingKey,
                CK_OBJECT_HANDLE hKey,
                CK_BYTE_PTR pWrappedKey,
                CK_ULONG_PTR pulWrappedKeyLen)
{
  UNSUPPORTED_FUNCTION(C_WrapKey);
}

CK_RV C_UnwrapKey(CK_SESSION_HANDLE hSession,
                  CK_MECHANISM_PTR pMechanism,
                  CK_OBJECT_HANDLE hUnwrappingKey,
                  CK_BYTE_PTR pWrappedKey,
                  CK_ULONG ulWrappedKeyLen,
                  CK_ATTRIBUTE_PTR pTemplate,
                  CK_ULONG ulAttributeCount,
                  CK_OBJECT_HANDLE_PTR phKey)
{
  UNSUPPORTED_FUNCTION(C_UnwrapKey);
}

CK_RV C_DeriveKey(CK_SESSION_HANDLE hSession,
                  CK_MECHANISM_PTR pMechanism,
                  CK_OBJECT_HANDLE hBaseKey,
                  CK_ATTRIBUTE_PTR pTemplate,
                  CK_ULONG ulAttributeCount,
                  CK_OBJECT_HANDLE_PTR phKey)
{
  UNSUPPORTED_FUNCTION(C_DeriveKey);
}

CK_RV C_WaitForSlotEvent(CK_FLAGS flags,
                         CK_SLOT_ID_PTR pSlot,
                         CK_VOID_PTR pRserved)
{
  UNSUPPORTED_FUNCTION(C_WaitForSlotEvent);
}

/*
 * "Any programmer who fails to comply with the standard naming, formatting,
 *  or commenting conventions should be shot.  If it so happens that it is
 *  inconvenient to shoot him, then he is to be politely requested to recode
 *  his program in adherence to the above standard."
 *                      -- Michael Spier, Digital Equipment Corporation
 *
 * Local variables:
 * indent-tabs-mode: nil
 * End:
 */
