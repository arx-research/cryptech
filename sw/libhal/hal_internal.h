/*
 * hal_internal.h
 * --------------
 * Internal API declarations for libhal.
 *
 * Authors: Rob Austein, Paul Selkirk
 * Copyright (c) 2015, NORDUnet A/S All rights reserved.
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

#ifndef _HAL_INTERNAL_H_
#define _HAL_INTERNAL_H_

#include <string.h>

#include "hal.h"
#include "verilog_constants.h"

/*
 * Everything in this file is part of the internal API, that is,
 * subject to change without notice.  Nothing outside of libhal itself
 * should be looking at this file.
 */

/*
 * Assertions, using our logger rather than printf() and assuming a
 * hal_error_t return value.
 */

#define hal_assert(_whatever_)                                          \
  do {                                                                  \
    if (!(_whatever_)) {                                                \
      hal_log(HAL_LOG_ERROR, "Assertion failed: %s", #_whatever_);      \
      return HAL_ERROR_ASSERTION_FAILED;                                \
    }                                                                   \
  } while (0)

/*
 * htonl and htons are not available in arm-none-eabi headers or libc.
 */
#ifndef STM32F4XX
#include <arpa/inet.h>
#else
#ifdef __ARMEL__                /* little endian */
inline uint32_t htonl(uint32_t w)
{
    return
        ((w & 0x000000ff) << 24) +
        ((w & 0x0000ff00) << 8) +
        ((w & 0x00ff0000) >> 8) +
        ((w & 0xff000000) >> 24);
}
inline uint16_t htons(uint16_t w)
{
    return
        ((w & 0x00ff) << 8) +
        ((w & 0xff00) >> 8);
}
#else                           /* big endian */
#define htonl(x) (x)
#define htons(x) (x)
#endif
#define ntohl htonl
#define ntohs htons
#endif

/*
 * Low-level I/O convenience functions, moved here from hal.h
 * because they use symbols defined in verilog_constants.h.
 */

static inline hal_error_t hal_io_zero(const hal_core_t *core)
{
  const uint8_t buf[4] = { 0, 0, 0, 0 };
  return hal_io_write(core, ADDR_CTRL, buf, sizeof(buf));
}

static inline hal_error_t hal_io_init(const hal_core_t *core)
{
  const uint8_t buf[4] = { 0, 0, 0, CTRL_INIT };
  return hal_io_write(core, ADDR_CTRL, buf, sizeof(buf));
}

static inline hal_error_t hal_io_next(const hal_core_t *core)
{
  const uint8_t buf[4] = { 0, 0, 0, CTRL_NEXT };
  return hal_io_write(core, ADDR_CTRL, buf, sizeof(buf));
}

static inline hal_error_t hal_io_wait_ready(const hal_core_t *core)
{
  int limit = -1;
  return hal_io_wait(core, STATUS_READY, &limit);
}

static inline hal_error_t hal_io_wait_valid(const hal_core_t *core)
{
  int limit = -1;
  return hal_io_wait(core, STATUS_VALID, &limit);
}

static inline hal_error_t hal_io_wait_ready2(const hal_core_t *core1, const hal_core_t *core2)
{
  int limit = -1;
  return hal_io_wait2(core1, core2, STATUS_READY, &limit);
}

static inline hal_error_t hal_io_wait_valid2(const hal_core_t *core1, const hal_core_t *core2)
{
  int limit = -1;
  return hal_io_wait2(core1, core2, STATUS_VALID, &limit);
}

/*
 * Static memory allocation on start-up.  Don't use this except where
 * really necessary.  Intent is just to allow allocation of things like
 * the large-ish ks_index arrays used by ks_flash.c from a memory source
 * external to the executable image file (eg, from the secondary SDRAM
 * chip on the Cryptech Alpha board).
 *
 * We shouldn't need this except on the HSM, so for now we don't bother
 * with implementing a version of this based on malloc() or sbrk().
 */

extern void *hal_allocate_static_memory(const size_t size);
extern hal_error_t hal_free_static_memory(const void * const ptr);

/*
 * Longest hash block and digest we support at the moment.
 */

#define HAL_MAX_HASH_BLOCK_LENGTH       SHA512_BLOCK_LEN
#define HAL_MAX_HASH_DIGEST_LENGTH      SHA512_DIGEST_LEN

/*
 * Locks and critical sections.
 */

extern void hal_critical_section_start(void);
extern void hal_critical_section_end(void);
extern void hal_ks_lock(void);
extern void hal_ks_unlock(void);
extern void hal_rsa_bf_lock(void);
extern void hal_rsa_bf_unlock(void);
extern void hal_task_yield(void);
extern void hal_task_yield_maybe(void);

/*
 * Thread sleep.  Currently used only for bad-PIN delays.
 */

extern void hal_sleep(const unsigned seconds);

/*
 * Logging.
 */

typedef enum {
  HAL_LOG_DEBUG, HAL_LOG_INFO, HAL_LOG_WARN, HAL_LOG_ERROR, HAL_LOG_SILENT
} hal_log_level_t;

extern void hal_log_set_level(const hal_log_level_t level);
extern void hal_log(const hal_log_level_t level, const char *format, ...);

/*
 * Dispatch structures for RPC implementation.
 *
 * The breakdown of which functions go into which dispatch vectors is
 * based entirely on pesky details like making sure that the right
 * functions get linked in the right cases, and should not be
 * construed as making any particular sense in any larger context.
 *
 * In theory eventually we might want a fully general mechanism to
 * allow us to dispatch arbitrary groups of functions either locally
 * or remotely on a per-user basis.  In practice, we probably want to
 * run everything on the HSM except for hashing and digesting, so just
 * code for that case initially while leaving the design open for a
 * more general mechanism later if warranted.
 *
 * So we have three cases:
 *
 * - We're the HSM, so we do everything locally (ie, we run the RPC
 *   server functions.
 *
 * - We're the host, so we do everything remotely (ie, we do
 *   everything using the client-side RPC calls.
 *
 * - We're the host but are doing hashing locally, so we do a mix.
 *   This is slightly more complicated than it might at first appear,
 *   because we must handle the case of one of the pkey functions
 *   taking a hash context instead of a literal hash value, in which
 *   case we have to extract the hash value from the context and
 *   supply it to the pkey RPC client code as a literal value.
 *
 * ...Except that for PKCS #11 we also have to handle the case of
 * "session keys", ie, keys which are not stored on the HSM.
 * Apparently people really do use these, mostly for public keys, in
 * order to conserve expensive memory on the HSM.  So this is another
 * feature of mixed mode: keys with HAL_KEY_FLAG_PROXIMATE set live on
 * the host, not in the HSM, and the mixed-mode pkey handlers deal
 * with the routing.  In the other two modes we ignore the flag and
 * send everything where we were going to send it anyway.  Restricting
 * the fancy key handling to mixed mode lets us drop this complexity
 * out entirely for applications which have no use for it.
 */

typedef struct {

  hal_error_t (*set_pin)(const hal_client_handle_t client,
                         const hal_user_t user,
                         const char * const newpin, const size_t newpin_len);

  hal_error_t (*login)(const hal_client_handle_t client,
                       const hal_user_t user,
                       const char * const newpin, const size_t newpin_len);

  hal_error_t (*logout)(const hal_client_handle_t client);

  hal_error_t (*logout_all)(void);

  hal_error_t (*is_logged_in)(const hal_client_handle_t client,
                              const hal_user_t user);

  hal_error_t (*get_random)(void *buffer, const size_t length);

  hal_error_t (*get_version)(uint32_t *version);

} hal_rpc_misc_dispatch_t;


typedef struct {

  hal_error_t (*get_digest_length)(const hal_digest_algorithm_t alg, size_t *length);

  hal_error_t (*get_digest_algorithm_id)(const hal_digest_algorithm_t alg,
                                         uint8_t *id, size_t *len, const size_t len_max);

  hal_error_t (*get_algorithm)(const hal_hash_handle_t hash, hal_digest_algorithm_t *alg);

  hal_error_t (*initialize)(const hal_client_handle_t client,
                            const hal_session_handle_t session,
                            hal_hash_handle_t *hash,
                            const hal_digest_algorithm_t alg,
                            const uint8_t * const key, const size_t key_length);

  hal_error_t (*update)(const hal_hash_handle_t hash,
                        const uint8_t * data, const size_t length);

  hal_error_t (*finalize)(const hal_hash_handle_t hash,
                          uint8_t *digest, const size_t length);
} hal_rpc_hash_dispatch_t;


typedef struct {

  hal_error_t  (*load)(const hal_client_handle_t client,
                       const hal_session_handle_t session,
                       hal_pkey_handle_t *pkey,
                       hal_uuid_t *name,
                       const uint8_t * const der, const size_t der_len,
                       const hal_key_flags_t flags);

  hal_error_t  (*open)(const hal_client_handle_t client,
                       const hal_session_handle_t session,
                       hal_pkey_handle_t *pkey,
                       const hal_uuid_t * const name);

  hal_error_t  (*generate_rsa)(const hal_client_handle_t client,
                               const hal_session_handle_t session,
                               hal_pkey_handle_t *pkey,
                               hal_uuid_t *name,
                               const unsigned key_length,
                               const uint8_t * const public_exponent, const size_t public_exponent_len,
                               const hal_key_flags_t flags);

  hal_error_t  (*generate_ec)(const hal_client_handle_t client,
                              const hal_session_handle_t session,
                              hal_pkey_handle_t *pkey,
                              hal_uuid_t *name,
                              const hal_curve_name_t curve,
                              const hal_key_flags_t flags);

  hal_error_t  (*generate_hashsig)(const hal_client_handle_t client,
                                   const hal_session_handle_t session,
                                   hal_pkey_handle_t *pkey,
                                   hal_uuid_t *name,
                                   const size_t hss_levels,
                                   const hal_lms_algorithm_t lms_type,
                                   const hal_lmots_algorithm_t lmots_type,
                                   const hal_key_flags_t flags);

  hal_error_t  (*close)(const hal_pkey_handle_t pkey);

  hal_error_t  (*delete)(const hal_pkey_handle_t pkey);

  hal_error_t  (*get_key_type)(const hal_pkey_handle_t pkey,
                               hal_key_type_t *type);

  hal_error_t  (*get_key_curve)(const hal_pkey_handle_t pkey,
                                hal_curve_name_t *curve);

  hal_error_t  (*get_key_flags)(const hal_pkey_handle_t pkey,
                                hal_key_flags_t *flags);

  size_t (*get_public_key_len)(const hal_pkey_handle_t pkey);

  hal_error_t  (*get_public_key)(const hal_pkey_handle_t pkey,
                                 uint8_t *der, size_t *der_len, const size_t der_max);

  hal_error_t  (*sign)(const hal_pkey_handle_t pkey,
                       const hal_hash_handle_t hash,
                       const uint8_t * const input,  const size_t input_len,
                       uint8_t * signature, size_t *signature_len, const size_t signature_max);

  hal_error_t  (*verify)(const hal_pkey_handle_t pkey,
                         const hal_hash_handle_t hash,
                         const uint8_t * const input, const size_t input_len,
                         const uint8_t * const signature, const size_t signature_len);

  hal_error_t (*match)(const hal_client_handle_t client,
                       const hal_session_handle_t session,
                       const hal_key_type_t type,
                       const hal_curve_name_t curve,
                       const hal_key_flags_t mask,
                       const hal_key_flags_t flags,
                       const hal_pkey_attribute_t *attributes,
                       const unsigned attributes_len,
                       unsigned *state,
                       hal_uuid_t *result,
                       unsigned *result_len,
                       const unsigned result_max,
                       const hal_uuid_t * const previous_uuid);

  hal_error_t (*set_attributes)(const hal_pkey_handle_t pkey,
                                const hal_pkey_attribute_t *attributes,
                                const unsigned attributes_len);

  hal_error_t (*get_attributes)(const hal_pkey_handle_t pkey,
                                hal_pkey_attribute_t *attributes,
                                const unsigned attributes_len,
                                uint8_t *attributes_buffer,
                                const size_t attributes_buffer_len);

  hal_error_t (*export)(const hal_pkey_handle_t pkey_handle,
                        const hal_pkey_handle_t kekek_handle,
                        uint8_t *pkcs8, size_t *pkcs8_len, const size_t pkcs8_max,
                        uint8_t *kek, size_t *kek_len, const size_t kek_max);

  hal_error_t (*import)(const hal_client_handle_t client,
                        const hal_session_handle_t session,
                        hal_pkey_handle_t *pkey,
                        hal_uuid_t *name,
                        const hal_pkey_handle_t kekek_handle,
                        const uint8_t * const pkcs8, const size_t pkcs8_len,
                        const uint8_t * const kek, const size_t kek_len,
                        const hal_key_flags_t flags);

} hal_rpc_pkey_dispatch_t;


extern const hal_rpc_misc_dispatch_t hal_rpc_local_misc_dispatch, hal_rpc_remote_misc_dispatch, *hal_rpc_misc_dispatch;
extern const hal_rpc_hash_dispatch_t hal_rpc_local_hash_dispatch, hal_rpc_remote_hash_dispatch, *hal_rpc_hash_dispatch;
extern const hal_rpc_pkey_dispatch_t hal_rpc_local_pkey_dispatch, hal_rpc_remote_pkey_dispatch, hal_rpc_mixed_pkey_dispatch, *hal_rpc_pkey_dispatch;

/*
 * See code in rpc_pkey.c for how this flag fits into the pkey handle.
 */

#define HAL_PKEY_HANDLE_TOKEN_FLAG  (1 << 31)

/*
 * Mostly used by the local_pkey code, but the mixed_pkey code needs
 * it to pad hashes for RSA PKCS #1.5 signatures.  This may indicate
 * that we need a slightly more general internal API here, but not
 * worth worrying about as long as we can treat RSA as a special case
 * and just pass the plain hash for everything else.
 */

extern hal_error_t hal_rpc_pkcs1_construct_digestinfo(const hal_hash_handle_t handle,
                                                      uint8_t *digest_info, size_t *digest_info_len,
                                                      const size_t digest_info_max);

/*
 * CRC-32 stuff (for flash keystore, etc).  Dunno if we want a Verilog
 * implementation of this, or if it would even be faster than doing it
 * the main CPU taking I/O overhead and so forth into account.
 *
 * These prototypes were generated by pycrc.py, see notes in crc32.c.
 */

typedef uint32_t hal_crc32_t;

static inline hal_crc32_t hal_crc32_init(void)
{
  return 0xffffffff;
}

extern hal_crc32_t hal_crc32_update(hal_crc32_t crc, const void *data, size_t data_len);

static inline hal_crc32_t hal_crc32_finalize(hal_crc32_t crc)
{
  return crc ^ 0xffffffff;
}

/*
 * Sizes for PKCS #8 encoded private keys.  This may not be exact due
 * to ASN.1 INTEGER encoding rules, but should be good enough for
 * buffer sizing.
 *
 * 2048-bit RSA:        1219 bytes
 * 4096-bit RSA:        2373 bytes
 * 8192-bit RSA:        4679 bytes
 * EC P-256:             138 bytes
 * EC P-384:             185 bytes
 * EC P-521:             240 bytes
 *
 * Plus extra space for pre-computed speed-up factors specific to our
 * Verilog implementation, which we store as fixed-length byte strings.
 *
 * Plus we need a bit of AES-keywrap overhead, since we're storing the
 * wrapped form (see hal_aes_keywrap_cyphertext_length()).
 *
 * Length check warning moved to ks.h since size of keystore blocks is
 * internal to the keystore implementation.
 */

#define HAL_KS_WRAPPED_KEYSIZE  ((2373 + 6 * 4096 / 8 + 6 * 4 + 15) & ~7)

/*
 * PINs.
 *
 * The functions here might want renaming, eg, to hal_pin_*().
 */

#ifndef HAL_PIN_SALT_LENGTH
#define HAL_PIN_SALT_LENGTH 16
#endif

typedef struct {
  uint32_t iterations;
  uint8_t pin[HAL_MAX_HASH_DIGEST_LENGTH];
  uint8_t salt[HAL_PIN_SALT_LENGTH];
} hal_ks_pin_t;

extern hal_error_t hal_set_pin_default_iterations(const hal_client_handle_t client,
                                                  const uint32_t iterations);

extern hal_error_t hal_get_pin(const hal_user_t user,
                               const hal_ks_pin_t **pin);

extern hal_error_t hal_set_pin(const hal_user_t user,
                               const hal_ks_pin_t * const pin);

/*
 * Master key memory (MKM) and key-encryption-key (KEK).
 *
 * Providing a mechanism for storing the KEK in flash is a horrible
 * kludge which defeats the entire purpose of having the MKM.  We
 * support it for now because the Alpha hardware does not yet have
 * a working battery backup for the MKM, but it should go away RSN.
 */

#ifndef HAL_MKM_FLASH_BACKUP_KLUDGE
#define HAL_MKM_FLASH_BACKUP_KLUDGE 1
#endif

#ifndef KEK_LENGTH
#define KEK_LENGTH      (bitsToBytes(256))
#endif

extern hal_error_t hal_mkm_get_kek(uint8_t *kek, size_t *kek_len, const size_t kek_max);

extern hal_error_t hal_mkm_volatile_read(uint8_t *buf, const size_t len);
extern hal_error_t hal_mkm_volatile_write(const uint8_t * const buf, const size_t len);
extern hal_error_t hal_mkm_volatile_erase(const size_t len);

#if HAL_MKM_FLASH_BACKUP_KLUDGE

/* #warning MKM flash backup kludge enabled.  Do NOT use this in production! */

extern hal_error_t hal_mkm_flash_read(uint8_t *buf, const size_t len);
extern hal_error_t hal_mkm_flash_read_no_lock(uint8_t *buf, const size_t len);
extern hal_error_t hal_mkm_flash_write(const uint8_t * const buf, const size_t len);
extern hal_error_t hal_mkm_flash_erase(const size_t len);

#endif

/*
 * Clean up pkey stuff that's tied to a particular client on logout.
 */

extern hal_error_t hal_pkey_logout(const hal_client_handle_t client);

/*
 * Keystore API for use by the pkey implementation.
 *
 * In an attempt to emulate what current theory says will eventually
 * be the behavior of the underlying Cryptech Verilog "hardware",
 * these functions automatically apply the AES keywrap transformations.
 *
 * Unclear whether these should also call the ASN.1 encode/decode
 * functions.  For the moment, the answer is no, but we may need to
 * revisit this as the underlying Verilog API evolves.
 *
 * hal_pkey_slot_t is defined here too, so that keystore drivers can
 * piggyback on the pkey database for storage related to keys on which
 * the user currently has an active pkey handle.  Nothing outside the
 * pkey and keystore code should touch this.
 */

typedef struct {
  hal_client_handle_t client;
  hal_session_handle_t session;
  hal_pkey_handle_t pkey;
  hal_key_type_t type;
  hal_curve_name_t curve;
  hal_key_flags_t flags;
  hal_uuid_t name;
  int hint;

  /*
   * This might be where we'd stash one or more (hal_core_t *)
   * pointing to cores which have already been loaded with the key.
   */
} hal_pkey_slot_t;

/*
 * Keystore is an opaque type, we just pass pointers.
 */

typedef struct hal_ks hal_ks_t;

extern hal_ks_t * const hal_ks_token;
extern hal_ks_t * const hal_ks_volatile;

extern hal_error_t hal_ks_init(hal_ks_t *ks,
                               const int alloc);

extern void hal_ks_init_read_only_pins_only(void);

extern hal_error_t hal_ks_store(hal_ks_t *ks,
                                hal_pkey_slot_t *slot,
                                const uint8_t * const der, const size_t der_len);

extern hal_error_t hal_ks_fetch(hal_ks_t *ks,
                                hal_pkey_slot_t *slot,
                                uint8_t *der, size_t *der_len, const size_t der_max);

extern hal_error_t hal_ks_delete(hal_ks_t *ks,
                                 hal_pkey_slot_t *slot);

extern hal_error_t hal_ks_match(hal_ks_t *ks,
                                const hal_client_handle_t client,
                                const hal_session_handle_t session,
                                const hal_key_type_t type,
                                const hal_curve_name_t curve,
                                const hal_key_flags_t mask,
                                const hal_key_flags_t flags,
                                const hal_pkey_attribute_t *attributes,
                                const unsigned attributes_len,
                                hal_uuid_t *result,
                                unsigned *result_len,
                                const unsigned result_max,
                                const hal_uuid_t * const previous_uuid);

extern hal_error_t hal_ks_set_attributes(hal_ks_t *ks,
                                         hal_pkey_slot_t *slot,
                                         const hal_pkey_attribute_t *attributes,
                                         const unsigned attributes_len);

extern hal_error_t hal_ks_get_attributes(hal_ks_t *ks,
                                         hal_pkey_slot_t *slot,
                                         hal_pkey_attribute_t *attributes,
                                         const unsigned attributes_len,
                                         uint8_t *attributes_buffer,
                                         const size_t attributes_buffer_len);

extern hal_error_t hal_ks_logout(hal_ks_t *ks,
                                 const hal_client_handle_t client);

extern hal_error_t hal_ks_rewrite_der(hal_ks_t *ks,
                                      hal_pkey_slot_t *slot,
                                      const uint8_t * const der, const size_t der_len);

/*
 * RPC lowest-level send and receive routines. These are blocking, and
 * transport-specific (sockets, USB).
 */

extern hal_error_t hal_rpc_send(const uint8_t * const buf, const size_t len);
extern hal_error_t hal_rpc_recv(uint8_t * const buf, size_t * const len);

extern hal_error_t hal_rpc_sendto(const uint8_t * const buf, const size_t len, void *opaque);
extern hal_error_t hal_rpc_recvfrom(uint8_t * const buf, size_t * const len, void **opaque);

extern hal_error_t hal_rpc_client_transport_init(void);
extern hal_error_t hal_rpc_client_transport_close(void);

extern hal_error_t hal_rpc_server_transport_init(void);
extern hal_error_t hal_rpc_server_transport_close(void);


/*
 * RPC function numbers
 */

typedef enum {
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
    RPC_FUNC_PKEY_GENERATE_HASHSIG,
} rpc_func_num_t;

#define RPC_VERSION 0x01010100          /* 1.1.1.0 */

/*
 * RPC client locality. These have to be defines rather than an enum,
 * because they're handled by the preprocessor.
 */

#define RPC_CLIENT_LOCAL        0
#define RPC_CLIENT_REMOTE       1
#define RPC_CLIENT_MIXED        2
#define RPC_CLIENT_NONE         3

/*
 * Maximum size of a HAL RPC packet.
 */

#ifndef HAL_RPC_MAX_PKT_SIZE
#define HAL_RPC_MAX_PKT_SIZE    16384
#endif

/*
 * Location of AF_UNIX socket for RPC client mux daemon.
 */

#ifndef HAL_CLIENT_DAEMON_DEFAULT_SOCKET_NAME
#define HAL_CLIENT_DAEMON_DEFAULT_SOCKET_NAME   "/tmp/.cryptech_muxd.rpc"
#endif

/*
 * Default device name and line speed for HAL RPC serial connection to HSM.
 */

#ifndef HAL_CLIENT_SERIAL_DEFAULT_DEVICE
#define HAL_CLIENT_SERIAL_DEFAULT_DEVICE        "/dev/ttyUSB0"
#endif

#ifndef HAL_CLIENT_SERIAL_DEFAULT_SPEED
#define HAL_CLIENT_SERIAL_DEFAULT_SPEED         921600
#endif

/*
 * Names of environment variables for setting the above in RPC clients.
 */

#define HAL_CLIENT_SERIAL_DEVICE_ENVVAR         "CRYPTECH_RPC_CLIENT_SERIAL_DEVICE"
#define HAL_CLIENT_SERIAL_SPEED_ENVVAR          "CRYPTECH_RPC_CLIENT_SERIAL_SPEED"

#endif /* _HAL_INTERNAL_H_ */

/*
 * Local variables:
 * indent-tabs-mode: nil
 * End:
 */
