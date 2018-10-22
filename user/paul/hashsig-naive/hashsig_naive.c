/*
 * hashsig.c
 * ---------
 * Naive implementation of draft-mcgrew-hash-sigs-10.txt
 *
 * Copyright (c) 2018, NORDUnet A/S All rights reserved.
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

/*
 * This is a naive implementation of McGrew et al's Hash-Based Signature
 * scheme, which hews as closely as possible to the text of the draft,
 * without any regard for performance (or security - keys are stored
 * unencrypted in the local file system).
 *
 * For simplicity, this implemention restricts all LMS keys in the HSS scheme
 * to use the same LMS type and LM-OTS type.
 */

#include <arpa/inet.h>		/* htonl and friends */

#include <openssl/sha.h>
#include <openssl/rand.h>

#include "hal.h"
#include "hashsig.h"
#include "xdr_internal.h"

typedef struct { uint8_t bytes[32]; } bytestring32;
typedef struct { uint8_t bytes[16]; } bytestring16;

#define check(op) do { hal_error_t _err = (op); if (_err != HAL_OK) return _err; } while (0)
#define checkssl(op) do { if ((op) == 0) { return HAL_ERROR_IMPOSSIBLE; } } while (0)

/* ---------------------------------------------------------------- */

/*
 * XDR extensions
 */

static inline hal_error_t hal_xdr_encode_bytestring32(uint8_t **outbuf, const uint8_t * const limit, const bytestring32 * const value)
{
    return hal_xdr_encode_fixed_opaque(outbuf, limit, (const uint8_t *)value, sizeof(bytestring32));
}

static inline hal_error_t hal_xdr_decode_bytestring32(const uint8_t ** const inbuf, const uint8_t * const limit, bytestring32 *value)
{
    return hal_xdr_decode_fixed_opaque(inbuf, limit, (uint8_t *)value, sizeof(bytestring32));
}

static inline hal_error_t hal_xdr_decode_bytestring32_ptr(const uint8_t ** const inbuf, const uint8_t * const limit, bytestring32 **value)
{
    return hal_xdr_decode_fixed_opaque_ptr(inbuf, limit, (const uint8_t ** const)value, sizeof(bytestring32));
}

static inline hal_error_t hal_xdr_encode_bytestring16(uint8_t **outbuf, const uint8_t * const limit, const bytestring16 *value)
{
    return hal_xdr_encode_fixed_opaque(outbuf, limit, (const uint8_t *)value, sizeof(bytestring16));
}

static inline hal_error_t hal_xdr_decode_bytestring16(const uint8_t ** const inbuf, const uint8_t * const limit, bytestring16 *value)
{
    return hal_xdr_decode_fixed_opaque(inbuf, limit, (uint8_t *)value, sizeof(bytestring16));
}

static inline hal_error_t hal_xdr_decode_bytestring16_ptr(const uint8_t ** const inbuf, const uint8_t * const limit, bytestring16 **value)
{
    return hal_xdr_decode_fixed_opaque_ptr(inbuf, limit, (const uint8_t ** const)value, sizeof(bytestring16));
}

/* ---------------------------------------------------------------- */

// 3.1.  Data Types

#define u32str(X) htonl(X)
#define u16str(X) htons(X)
#define u8str(X) (X & 0xff)

// 3.1.3.  Strings of w-bit elements

static uint8_t coef(const uint8_t * const S, const size_t i, size_t w)
{
    switch (w) {
    case 1:
        return (S[i/8] >> (7 - (i % 8))) & 0x01;
    case 2:
        return (S[i/4] >> (6 - (2 * (i % 4)))) & 0x03;
    case 4:
        return (S[i/2] >> (4 - (4 * (i % 2)))) & 0x0f;
    case 8:
        return S[i];
    default:
        return 0;
    }
}

// 3.2.  Security string

#define D_PBLC 0x8080
#define D_MESG 0x8181
#define D_LEAF 0x8282
#define D_INTR 0x8383

/* ---------------------------------------------------------------- */

// 4.  LM-OTS One-Time Signatures

// 4.2.  Parameter Sets

static inline size_t lmots_type_to_n(const lmots_algorithm_t lmots_type)
{
    switch (lmots_type) {
    case lmots_sha256_n32_w1:
    case lmots_sha256_n32_w2:
    case lmots_sha256_n32_w4:
    case lmots_sha256_n32_w8: return 32;
    default: return 0;
    }
}

static inline size_t lmots_type_to_w(const lmots_algorithm_t lmots_type)
{
    switch (lmots_type) {
    case lmots_sha256_n32_w1: return 1;
    case lmots_sha256_n32_w2: return 2;
    case lmots_sha256_n32_w4: return 4;
    case lmots_sha256_n32_w8: return 8;
    default: return 0;
    }
}

static inline size_t lmots_type_to_p(const lmots_algorithm_t lmots_type)
{
    switch (lmots_type) {
    case lmots_sha256_n32_w1: return 265;
    case lmots_sha256_n32_w2: return 133;
    case lmots_sha256_n32_w4: return  67;
    case lmots_sha256_n32_w8: return  34;
    default: return 0;
    }
}

static inline size_t lmots_type_to_ls(const lmots_algorithm_t lmots_type)
{
    switch (lmots_type) {
    case lmots_sha256_n32_w1: return 7;
    case lmots_sha256_n32_w2: return 6;
    case lmots_sha256_n32_w4: return 4;
    case lmots_sha256_n32_w8: return 0;
    default: return 0;
    }
}

// 4.3.  Private Key

static inline size_t lmots_private_key_len(const lmots_algorithm_t lmots_type)
{
    /* u32str(type) || I || u32str(q) || x[0] || x[1] || ... || x[p-1] */
    return 2 * sizeof(uint32_t) + sizeof(bytestring16) + lmots_type_to_p(lmots_type) * lmots_type_to_n(lmots_type);
}

static hal_error_t lmots_generate_private_key(const lmots_algorithm_t lmots_type,
                                              const bytestring16 * const I, const size_t q,
                                              uint8_t *key, size_t *key_len, const size_t key_max)
{
    if (I == NULL || key == NULL|| key_max < lmots_private_key_len(lmots_type))
        return HAL_ERROR_BAD_ARGUMENTS;

//   Algorithm 0: Generating a Private Key

//  1. retrieve the value p from the type, and the value I the 16 byte
//     identifier of the LMS public/private keypair that this LM-OTS private
//     key will be used with

//  2. set type to the typecode of the algorithm

//  3. set n and p according to the typecode and Table 1

    size_t n = lmots_type_to_n(lmots_type);
    size_t p = lmots_type_to_p(lmots_type);

//  4. compute the array x as follows:
//     for ( i = 0; i < p; i = i + 1 ) {
//       set x[i] to a uniformly random n-byte string
//     }

    bytestring32 x[p];
    for (size_t i = 0; i < p; ++i)
        checkssl(RAND_bytes((unsigned char *)&x[i], n));

//  5. return u32str(type) || I || u32str(q) || x[0] || x[1] || ... || x[p-1]

    uint8_t *keyptr = key;
    const uint8_t * const keylim = key + key_max;
    check(hal_xdr_encode_int(&keyptr, keylim, lmots_type));
    check(hal_xdr_encode_bytestring16(&keyptr, keylim, I));
    check(hal_xdr_encode_int(&keyptr, keylim, q));
    for (size_t i = 0; i < p; ++i)
        check(hal_xdr_encode_bytestring32(&keyptr, keylim, &x[i]));

    if (key_len != NULL)
        *key_len = keyptr - key;

    return HAL_OK;
}

// 4.4.  Public Key

static inline size_t lmots_public_key_len(const lmots_algorithm_t lmots_type)
{
    /* u32str(type) || I || u32str(q) || K */
    return ((2 * sizeof(uint32_t)) + sizeof(bytestring16) + lmots_type_to_n(lmots_type));
}

static hal_error_t lmots_generate_public_key(const uint8_t * const key, const size_t key_len,
                                             uint8_t *pub, size_t *pub_len, const size_t pub_max)
{
    if (key == NULL || pub == NULL)
        return HAL_ERROR_BAD_ARGUMENTS;

//   Algorithm 1: Generating a One Time Signature Public Key From a
//   Private Key
//
//   1. set type to the typecode of the algorithm

    const uint8_t *keyptr = key;
    const uint8_t * const keylim = key + key_len;

    lmots_algorithm_t lmots_type;
    check(hal_xdr_decode_int(&keyptr, keylim, &lmots_type));

    /* private key format
     * u32str(type) || I || u32str(q) || x[0] || x[1] || ... || x[p-1]
     */

//   2. set the integers n, p, and w according to the typecode and Table 1

    /* XXX n is not used here */
    size_t p = lmots_type_to_p(lmots_type);
    size_t w = lmots_type_to_w(lmots_type);

//   3. determine x, I and q from the private key

    bytestring16 I;
    check(hal_xdr_decode_bytestring16(&keyptr, keylim, &I));

    uint32_t q;
    check(hal_xdr_decode_int(&keyptr, keylim, &q));

    bytestring32 x[p];
    for (size_t i = 0; i < p; ++i)
        check(hal_xdr_decode_bytestring32(&keyptr, keylim, &x[i]));

//   4. compute the string K as follows:
//      for ( i = 0; i < p; i = i + 1 ) {
//        tmp = x[i]
//        for ( j = 0; j < 2^w - 1; j = j + 1 ) {
//           tmp = H(I || u32str(q) || u16str(i) || u8str(j) || tmp)
//        }
//        y[i] = tmp
//      }
//      K = H(I || u32str(q) || u16str(D_PBLC) || y[0] || ... || y[p-1])

    SHA256_CTX ctx;
    bytestring32 y[p];
    for (size_t i = 0; i < p; ++i) {
        bytestring32 tmp;
        memcpy(&tmp, &x[i], sizeof(tmp));
        for (size_t j = 0; j < (1U << w) - 1; ++j) {
            checkssl(SHA256_Init(&ctx));
            checkssl(SHA256_Update(&ctx, &I, sizeof(I)));
            uint32_t l = u32str(q); checkssl(SHA256_Update(&ctx, &l, sizeof(l)));
            uint16_t s = u16str(i); checkssl(SHA256_Update(&ctx, &s, sizeof(s)));
            uint8_t b = u8str(j); checkssl(SHA256_Update(&ctx, &b, sizeof(b)));
            checkssl(SHA256_Update(&ctx, &tmp, sizeof(tmp)));
            checkssl(SHA256_Final((unsigned char *)&tmp, &ctx));
        }
        memcpy(&y[i], &tmp, sizeof(tmp));
    }
    bytestring32 K;
    checkssl(SHA256_Init(&ctx));
    checkssl(SHA256_Update(&ctx, &I, sizeof(I)));
    uint32_t l = u32str(q); checkssl(SHA256_Update(&ctx, &l, sizeof(l)));
    uint16_t s = u16str(D_PBLC); checkssl(SHA256_Update(&ctx, &s, sizeof(s)));
    for (size_t i = 0; i < p; ++i)
        checkssl(SHA256_Update(&ctx, (uint8_t *)&y[i], sizeof(y[i])));
    checkssl(SHA256_Final((unsigned char *)&K, &ctx));

//   5. return u32str(type) || I || u32str(q) || K

    uint8_t *pubptr = pub;
    const uint8_t * const publim = pub + pub_max;
    check(hal_xdr_encode_int(&pubptr, publim, lmots_type));
    check(hal_xdr_encode_bytestring16(&pubptr, publim, &I));
    check(hal_xdr_encode_int(&pubptr, publim, q));
    check(hal_xdr_encode_bytestring32(&pubptr, publim, &K));

    if (pub_len != NULL)
        *pub_len = pubptr - pub;

    return HAL_OK;
}

// 4.5.  Checksum

static uint16_t Cksm(const uint8_t * const S, const lmots_algorithm_t lmots_type)
{
//   Algorithm 2: Checksum Calculation
//     sum = 0
//     for ( i = 0; i < (n*8/w); i = i + 1 ) {
//       sum = sum + (2^w - 1) - coef(S, i, w)
//     }
//     return (sum << ls)

    size_t n = lmots_type_to_n(lmots_type);
    size_t w = lmots_type_to_w(lmots_type);
    size_t ls = lmots_type_to_ls(lmots_type);

    uint16_t sum = 0;
    for (size_t i = 0; i < (n * 8 / w); ++i)
        sum += ((1 << w) - 1) - coef(S, i, w);
    return (sum << ls);
}

// 4.6.  Signature Generation

static inline size_t lmots_signature_len(const lmots_algorithm_t lmots_type)
{
    /* u32str(type) || C || y[0] || ... || y[p-1] */
    return (sizeof(uint32_t) + ((lmots_type_to_p(lmots_type) + 1) * lmots_type_to_n(lmots_type)));
}

static hal_error_t lmots_sign(const uint8_t * const key, const size_t key_len,
                              const uint8_t * const msg, const size_t msg_len,
                              uint8_t *sig, size_t *sig_len, const size_t sig_max)
{
    if (key == NULL ||  msg == NULL || sig == NULL)
        return HAL_ERROR_BAD_ARGUMENTS;

//   Algorithm 3: Generating a One Time Signature From a Private Key and a
//   Message

    const uint8_t *keyptr = key;
    const uint8_t * const keylim = key + key_len;

    /* private key format
     * u32str(type) || I || u32str(q) || x[0] || x[1] || ... || x[p-1]
     */

//     1. set type to the typecode of the algorithm

    lmots_algorithm_t lmots_type;
    check(hal_xdr_decode_int(&keyptr, keylim, &lmots_type));

//     2. set n, p, and w according to the typecode and Table 1

    size_t n = lmots_type_to_n(lmots_type);
    size_t p = lmots_type_to_p(lmots_type);
    size_t w = lmots_type_to_w(lmots_type);

//     3. determine x, I and q from the private key

    bytestring16 I;
    check(hal_xdr_decode_bytestring16(&keyptr, keylim, &I));

    uint32_t q;
    check(hal_xdr_decode_int(&keyptr, keylim, &q));

    bytestring32 x[p];
    for (size_t i = 0; i < p; ++i)
        check(hal_xdr_decode_bytestring32(&keyptr, keylim, &x[i]));

//     4. set C to a uniformly random n-byte string

    bytestring32 C;
    checkssl(RAND_bytes((unsigned char *)&C, n));

//     5. compute the array y as follows:
//        Q = H(I || u32str(q) || u16str(D_MESG) || C || message)
//        for ( i = 0; i < p; i = i + 1 ) {
//          a = coef(Q || Cksm(Q), i, w)
//          tmp = x[i]
//          for ( j = 0; j < a; j = j + 1 ) {
//             tmp = H(I || u32str(q) || u16str(i) || u8str(j) || tmp)
//          }
//          y[i] = tmp
//        }

    SHA256_CTX ctx;
    uint8_t Q[n + 2];           /* hash || 16-bit checksum */

    checkssl(SHA256_Init(&ctx));
    checkssl(SHA256_Update(&ctx, &I, sizeof(I)));
    uint32_t l = u32str(q); checkssl(SHA256_Update(&ctx, &l, sizeof(l)));
    uint16_t s = u16str(D_MESG); checkssl(SHA256_Update(&ctx, &s, sizeof(s)));
    checkssl(SHA256_Update(&ctx, &C, sizeof(C)));
    checkssl(SHA256_Update(&ctx, msg, msg_len));
    checkssl(SHA256_Final((unsigned char *)Q, &ctx));

    /* append checksum */
    *(uint16_t *)&Q[n] = u16str(Cksm((uint8_t *)Q, lmots_type));

    bytestring32 y[p];
    for (size_t i = 0; i < p; ++i) {
        size_t a = coef(Q, i, w);
        bytestring32 tmp;
        memcpy(&tmp, &x[i], sizeof(tmp));
        for (size_t j = 0; j < a; ++j) {
            checkssl(SHA256_Init(&ctx));
            checkssl(SHA256_Update(&ctx, &I, sizeof(I)));
            uint32_t l = u32str(q); checkssl(SHA256_Update(&ctx, &l, sizeof(l)));
            uint16_t s = u16str(i); checkssl(SHA256_Update(&ctx, &s, sizeof(s)));
            uint8_t b = u8str(j); checkssl(SHA256_Update(&ctx, &b, sizeof(b)));
            checkssl(SHA256_Update(&ctx, &tmp, sizeof(tmp)));
            checkssl(SHA256_Final((unsigned char *)&tmp, &ctx));
        }
        memcpy(&y[i], &tmp, sizeof(tmp));
    }

//      6. return u32str(type) || C || y[0] || ... || y[p-1]

    uint8_t *sigptr = sig;
    const uint8_t * const siglim = sig + sig_max;
    check(hal_xdr_encode_int(&sigptr, siglim, lmots_type));
    check(hal_xdr_encode_bytestring32(&sigptr, siglim, &C));
    for (size_t i = 0; i < p; ++i)
        check(hal_xdr_encode_bytestring32(&sigptr, siglim, &y[i]));

    if (sig_len != NULL)
        *sig_len = sigptr - sig;

    return HAL_OK;
}

// 4.7.  Signature Verification

static hal_error_t lmots_public_key_candidate(const lmots_algorithm_t pubtype,
                                              const bytestring16 * const I, const size_t q,
                                              const uint8_t * const msg, const size_t msg_len,
                                              const uint8_t * const sig, const size_t sig_len,
                                              bytestring32 * Kc);

#if 0 /* unused */
static hal_error_t lmots_verify(const uint8_t * const key, const size_t key_len,
                                const uint8_t * const msg, const size_t msg_len,
                                const uint8_t * const sig, const size_t sig_len)
{
    if (key == NULL || key_len < lmots_private_key_len(lmots_type) ||
        msg == NULL || sig == NULL || sig_len < lmots_signature_len(lmots_type))
        return HAL_ERROR_BAD_ARGUMENTS;

//   Algorithm 4a: Verifying a Signature and Message Using a Public Key
//
//    1. if the public key is not at least four bytes long, return INVALID

    if (key_len < 4)
        return HAL_ERROR_INVALID_SIGNATURE;

//    2. parse pubtype, I, q, and K from the public key as follows:
//       a. pubtype = strTou32(first 4 bytes of public key)

    const uint8_t *keyptr = key;
    const uint8_t * const keylim = key + key_len;

    lmots_algorithm_t lmots_type;
    check(hal_xdr_decode_int(&keyptr, keylim, &lmots_type));

    /* XXX missing from draft */
    size_t n = lmots_type_to_n(lmots_type);

//       b. if the public key is not exactly 24 + n bytes long,
//          return INVALID

    if (key_len != 24 + n)
        return HAL_ERROR_INVALID_SIGNATURE;

//       c. I = next 16 bytes of public key

    bytestring16 I;
    check(hal_xdr_decode_bytestring16(&keyptr, keylim, &I));

//       d. q = strTou32(next 4 bytes of public key)

    uint32_t q;
    check(hal_xdr_decode_int(&keyptr, keylim, &q));

//       e. K = next n bytes of public key

    bytestring32 K;
    check(hal_xdr_decode_bytestring32(&keyptr, keylim, &K));

//    3. compute the public key candidate Kc from the signature,
//       message, and the identifiers I and q obtained from the
//       public key, using Algorithm 4b.  If Algorithm 4b returns
//       INVALID, then return INVALID.

    bytestring32 Kc;
    check(lmots_public_key_candidate(lmots_type, &I, q, msg, msg_len, sig, sig_len, &Kc));

//    4. if Kc is equal to K, return VALID; otherwise, return INVALID

    return (memcmp(&Kc, &K, sizeof(Kc)) ? HAL_ERROR_INVALID_SIGNATURE : HAL_OK);
}
#endif

static hal_error_t lmots_public_key_candidate(const lmots_algorithm_t lmots_type,
                                              const bytestring16 * const I, const size_t q,
                                              const uint8_t * const msg, const size_t msg_len,
                                              const uint8_t * const sig, const size_t sig_len,
                                              bytestring32 * const Kc)
{
    if (I == NULL || msg == NULL || sig == NULL || Kc == NULL)
        return HAL_ERROR_BAD_ARGUMENTS;

//   Algorithm 4b: Computing a Public Key Candidate Kc from a Signature,
//   Message, Signature Typecode Type, and identifiers I, q
//
//  1. if the signature is not at least four bytes long, return INVALID

    if (sig_len < 4)
        return HAL_ERROR_INVALID_SIGNATURE;

//  2. parse sigtype, C, and y from the signature as follows:
//     a. sigtype = strTou32(first 4 bytes of signature)

    const uint8_t *sigptr = sig;
    const uint8_t * const siglim = sig + sig_len;

    lmots_algorithm_t sigtype;
    check(hal_xdr_decode_int(&sigptr, siglim, &sigtype));

//     b. if sigtype is not equal to pubtype, return INVALID

    if (sigtype != lmots_type)
        return HAL_ERROR_INVALID_SIGNATURE;

//     c. set n and p according to the pubtype and Table 1;  if the
//     signature is not exactly 4 + n * (p+1) bytes long, return INVALID

    size_t n = lmots_type_to_n(lmots_type);
    size_t p = lmots_type_to_p(lmots_type);

    if (sig_len != 4 + n * (p + 1))
        return HAL_ERROR_INVALID_SIGNATURE;

//     d. C = next n bytes of signature

    bytestring32 C;
    check(hal_xdr_decode_bytestring32(&sigptr, siglim, &C));

//     e.  y[0] = next n bytes of signature
//         y[1] = next n bytes of signature
//         ...
//       y[p-1] = next n bytes of signature

    bytestring32 y[p];
    for (size_t i = 0; i < p; ++i)
        check(hal_xdr_decode_bytestring32(&sigptr, siglim, &y[i]));

//  3. compute the string Kc as follows
//     Q = H(I || u32str(q) || u16str(D_MESG) || C || message)
//     for ( i = 0; i < p; i = i + 1 ) {
//       a = coef(Q || Cksm(Q), i, w)
//       tmp = y[i]
//       for ( j = a; j < 2^w - 1; j = j + 1 ) {
//          tmp = H(I || u32str(q) || u16str(i) || u8str(j) || tmp)
//       }
//       z[i] = tmp
//     }
//     Kc = H(I || u32str(q) || u16str(D_PBLC) || z[0] || z[1] || ... || z[p-1])

    SHA256_CTX ctx;
    uint8_t Q[n + 2];           /* hash || 16-bit checksum */

    checkssl(SHA256_Init(&ctx));
    checkssl(SHA256_Update(&ctx, I, sizeof(*I)));
    uint32_t l = u32str(q); checkssl(SHA256_Update(&ctx, &l, sizeof(l)));
    uint16_t s = u16str(D_MESG); checkssl(SHA256_Update(&ctx, &s, sizeof(s)));
    checkssl(SHA256_Update(&ctx, &C, sizeof(C)));
    checkssl(SHA256_Update(&ctx, msg, msg_len));
    checkssl(SHA256_Final((unsigned char *)Q, &ctx));

    /* append checksum */
    *(uint16_t *)&Q[n] = u16str(Cksm((uint8_t *)Q, lmots_type));

    size_t w = lmots_type_to_w(lmots_type);
    bytestring32 z[p];

    for (size_t i = 0; i < p; ++i) {
        uint8_t a = coef(Q, i, w);
        bytestring32 tmp;
        memcpy(&tmp, &y[i], sizeof(tmp));
        for (size_t j = (size_t)a; j < (1U << w) - 1; ++j) {
            checkssl(SHA256_Init(&ctx));
            checkssl(SHA256_Update(&ctx, I, sizeof(*I)));
            l = u32str(q); checkssl(SHA256_Update(&ctx, &l, sizeof(l)));
            s = u16str(i); checkssl(SHA256_Update(&ctx, &s, sizeof(s)));
            uint8_t b = u8str(j); checkssl(SHA256_Update(&ctx, &b, sizeof(b)));
            checkssl(SHA256_Update(&ctx, &tmp, sizeof(tmp)));
            checkssl(SHA256_Final((unsigned char *)&tmp, &ctx));
        }
        memcpy(&z[i], &tmp, sizeof(tmp));
    }

    checkssl(SHA256_Init(&ctx));
    checkssl(SHA256_Update(&ctx, I, sizeof(*I)));
    l = u32str(q); checkssl(SHA256_Update(&ctx, &l, sizeof(l)));
    s = u16str(D_PBLC); checkssl(SHA256_Update(&ctx, &s, sizeof(s)));
    for (size_t i = 0; i < p; ++i)
        checkssl(SHA256_Update(&ctx, &z[i], sizeof(z[i])));
    checkssl(SHA256_Final((unsigned char *)Kc, &ctx));

//  4. return Kc

    return HAL_OK;
}

/* ---------------------------------------------------------------- */

// 5.  Leighton Micali Signatures

// 5.1.  Parameters

static inline size_t lms_type_to_h(const lms_algorithm_t lms_type)
{
    switch (lms_type) {
    case lms_sha256_n32_h5:  return  5;
    case lms_sha256_n32_h10: return 10;
    case lms_sha256_n32_h15: return 15;
    case lms_sha256_n32_h20: return 20;
    case lms_sha256_n32_h25: return 25;
    default: return 0;
    }
}

static inline size_t lms_type_to_m(const lms_algorithm_t lms_type)
{
    switch (lms_type) {
    case lms_sha256_n32_h5:
    case lms_sha256_n32_h10:
    case lms_sha256_n32_h15:
    case lms_sha256_n32_h20:
    case lms_sha256_n32_h25: return 32;
    default: return 0;
    }
}

// 5.2.  LMS Private Key

static inline size_t lms_private_key_len(const lms_algorithm_t lms_type, const lmots_algorithm_t lmots_type)
{
    /* u32str(type) || u32str(q) || OTS_PRIV[0] || OTS_PRIV[1] || ... || OTS_PRIV[2^h - 1] */
    return ((2 * sizeof(uint32_t)) + ((1 << lms_type_to_h(lms_type)) * lmots_private_key_len(lmots_type)));
}

static hal_error_t lms_generate_private_key(const lms_algorithm_t lms_type,
                                            const lmots_algorithm_t lmots_type,
                                            uint8_t * const key, size_t * const key_len, const size_t key_max)
{
    if (key == NULL || key_max < lms_private_key_len(lms_type, lmots_type))
        return HAL_ERROR_BAD_ARGUMENTS;

//   Algorithm 5: Computing an LMS Private Key.

//      1. determine h and m from the typecode and Table 2.

    size_t h = lms_type_to_h(lms_type);
    /* XXX m is not used here */

    /* XXX missing from draft: generate I for this keypair */
    bytestring16 I;
    checkssl(RAND_bytes((unsigned char *)&I, sizeof(I)));

//      2. compute the array OTS_PRIV[] as follows:
//         for ( q = 0; q < 2^h; q = q + 1) {
//            OTS_PRIV[q] = LM-OTS private key with identifiers I, q
//         }

    /* initialize the key buffer with type and q=0, so we can generate the
     * lmots private keys directly in the key buffer
     */
    uint8_t *keyptr = key;
    uint8_t *keylim = key + key_max;
    check(hal_xdr_encode_int(&keyptr, keylim, lms_type));
    check(hal_xdr_encode_int(&keyptr, keylim, 0));

    for (size_t q = 0; q < (1U << h); ++q) {
        size_t ots_len;
        check(lmots_generate_private_key(lmots_type, &I, q, keyptr, &ots_len, keylim - keyptr));
        keyptr += ots_len;
    }

//      3. q = 0

    /* XXX missing from draft: return the key in output format */
    /* 4. return u32str(type) || u32str(q) || OTS_PRIV[0] || OTS_PRIV[1] || ... || OTS_PRIV[2^h - 1] */

    if (key_len != NULL)
        *key_len = keyptr - key;

    return HAL_OK;
}

// 5.3.  LMS Public Key

static inline size_t lms_public_key_len(const lms_algorithm_t lms_type)
{
    /* u32str(type) || u32str(otstype) || I || T[1] */
    return ((2 * sizeof(uint32_t)) + 16 + lms_type_to_m(lms_type));
}

static hal_error_t lms_generate_T(const lms_algorithm_t lms_type,
                                  const lmots_algorithm_t lmots_type,
                                  const bytestring16 * const I,
                                  const uint8_t * const key,
                                  bytestring32 * const T)
{
    size_t h = lms_type_to_h(lms_type);
    size_t lmots_prv_len = lmots_private_key_len(lmots_type);
    size_t lmots_pub_len = lmots_public_key_len(lmots_type);
    const uint8_t *keyptr = key;
    SHA256_CTX ctx;

//  T[r]=/ H(I||u32str(r)||u16str(D_LEAF)||OTS_PUB_HASH[r-2^h])   if r >= 2^h,
//       \ H(I||u32str(r)||u16str(D_INTR)||T[2*r]||T[2*r+1]) otherwise.

    /* first generate the leaf-node T values */
    for (size_t q = 0; q < (1U << h); ++q) {
        /* generate OTS_PUB_HASH[q] */
        size_t r = (1 << h) + q;
        uint8_t pub[lmots_pub_len];
        check(lmots_generate_public_key(keyptr, lmots_prv_len, pub, NULL, lmots_pub_len));
        keyptr += lmots_prv_len;

        checkssl(SHA256_Init(&ctx));
        checkssl(SHA256_Update(&ctx, I, sizeof(*I)));
        uint32_t l = u32str(r); checkssl(SHA256_Update(&ctx, &l, sizeof(l)));
        uint16_t s = u16str(D_LEAF); checkssl(SHA256_Update(&ctx, &s, sizeof(s)));
        checkssl(SHA256_Update(&ctx, &pub[24], sizeof(bytestring32)));
        checkssl(SHA256_Final((unsigned char *)&T[r], &ctx));
    }

    /* then generate the rest of T[], from bottom to top */
    for (size_t r = (1 << h) - 1; r > 0; --r) {
        checkssl(SHA256_Init(&ctx));
        checkssl(SHA256_Update(&ctx, I, sizeof(*I)));
        uint32_t l = u32str(r); checkssl(SHA256_Update(&ctx, &l, sizeof(l)));
        uint16_t s = u16str(D_INTR); checkssl(SHA256_Update(&ctx, &s, sizeof(s)));
        checkssl(SHA256_Update(&ctx, &T[2*r], sizeof(T[r])));
        checkssl(SHA256_Update(&ctx, &T[2*r+1], sizeof(T[r])));
        checkssl(SHA256_Final((unsigned char *)&T[r], &ctx));
    }

    return HAL_OK;
}

static hal_error_t lms_generate_public_key(const uint8_t * const key, const size_t key_len,
                                           uint8_t *pub, size_t *pub_len, const size_t pub_max)
{
    if (key == NULL ||  pub == NULL)
        return HAL_ERROR_BAD_ARGUMENTS;

    const uint8_t *keyptr = key;
    const uint8_t * const keylim = key + key_len;

    /* private key format
     * u32str(type) || u32str(q) || OTS_PRIV[0] || OTS_PRIV[1] || ... || OTS_PRIV[2^h - 1]
     */

    lms_algorithm_t lms_type;
    check(hal_xdr_decode_int(&keyptr, keylim, &lms_type));
    size_t h = lms_type_to_h(lms_type);

    /* skip over q, which will be 0 */
    keyptr += 4;

    /* peek into the first lmots private key for type and I */
    /* u32str(type) || I || u32str(q) || x[0] || x[1] || ... || x[p-1] */
    const uint8_t *lookahead = keyptr;
    lmots_algorithm_t lmots_type;
    bytestring16 I;
    check(hal_xdr_decode_int(&lookahead, keylim, &lmots_type));
    check(hal_xdr_decode_bytestring16(&lookahead, keylim, &I));

//   The LMS public key is the string u32str(type) || u32str(otstype) || I || T[1].

    uint8_t *pubptr = pub;
    const uint8_t * const publim = pub + pub_max;
    check(hal_xdr_encode_int(&pubptr, publim, lms_type));
    check(hal_xdr_encode_int(&pubptr, publim, lmots_type));
    check(hal_xdr_encode_bytestring16(&pubptr, publim, &I));
    bytestring32 T[1 << (h + 1)];
    check(lms_generate_T(lms_type, lmots_type, &I, keyptr, T));
    check(hal_xdr_encode_bytestring32(&pubptr, publim, &T[1]));

    if (pub_len != NULL)
        *pub_len = pubptr - pub;

    return HAL_OK;
}

// 5.4.  LMS Signature

static inline size_t lms_signature_len(const lms_algorithm_t lms_type, const lmots_algorithm_t lmots_type)
{
    /* u32str(q) || ots_signature || u32str(type) || path[0] || path[1] || ... || path[h-1] */
    return ((2 * sizeof(uint32_t)) + lmots_signature_len(lmots_type) + (lms_type_to_h(lms_type) * lms_type_to_m(lms_type)));
}

static hal_error_t lms_sign(const uint8_t * const key, const size_t key_len,
                            const uint8_t * const msg, const size_t msg_len,
                            uint8_t *sig, size_t *sig_len, const size_t sig_max)
{
//   An LMS signature consists of
//
//      the number q of the leaf associated with the LM-OTS signature, as
//      a four-byte unsigned integer in network byte order,
//
//      an LM-OTS signature, and
//
//      a typecode indicating the particular LMS algorithm,
//
//      an array of h m-byte values that is associated with the path
//      through the tree from the leaf associated with the LM-OTS
//      signature to the root.
//
//   Symbolically, the signature can be represented as u32str(q) ||
//   ots_signature || u32str(type) || path[0] || path[1] || ... ||
//   path[h-1].

    const uint8_t *keyptr = key;
    const uint8_t * const keylim = key + key_len;

    /* private key format
     * u32str(type) || u32str(q) || OTS_PRIV[0] || OTS_PRIV[1] || ... || OTS_PRIV[2^h - 1]
     */

    lms_algorithm_t lms_type;
    check(hal_xdr_decode_int(&keyptr, keylim, &lms_type));
    size_t h = lms_type_to_h(lms_type);

    uint32_t q;
    check(hal_xdr_decode_int(&keyptr, keylim, &q));
    if (q >= (1U << h))
        return HAL_ERROR_HASHSIG_KEY_EXHAUSTED;

    /* peek into the first lmots private key for lmots_type and I */
    const uint8_t *lookahead = keyptr;
    lmots_algorithm_t lmots_type;
    check(hal_xdr_decode_int(&lookahead, keylim, &lmots_type));
    bytestring16 I;
    check(hal_xdr_decode_bytestring16(&lookahead, keylim, &I));

    if (sig_max < lms_signature_len(lms_type, lmots_type))
        return HAL_ERROR_BAD_ARGUMENTS;

    uint8_t *sigptr = sig;
    const uint8_t * const siglim = sig + sig_max;
    check(hal_xdr_encode_int(&sigptr, siglim, q));

    /* locate the lmots signing key OTS_PRIV[q] */
    const size_t lmots_key_len = lmots_private_key_len(lmots_type);
    const uint8_t * const lmots_key = keyptr + q * lmots_key_len;

    /* generate the lmots signature */
    size_t lmots_sig_len;
    check(lmots_sign(lmots_key, lmots_key_len, msg, msg_len, sigptr, &lmots_sig_len, siglim - sigptr));
    sigptr += lmots_sig_len;

    check(hal_xdr_encode_int(&sigptr, siglim, lms_type));

    /* generate the path array */
    bytestring32 T[1 << (h + 1)];
    check(lms_generate_T(lms_type, lmots_type, &I, keyptr, T));
    for (size_t r = (1 << h) + q; r > 1; r /= 2)
        check(hal_xdr_encode_bytestring32(&sigptr, siglim, ((r & 1) ? &T[r-1] : &T[r+1])));

    if (sig_len != NULL)
        *sig_len = sigptr - sig;

    /* update q before returning the signature */
    keyptr = key + 4;
    check(hal_xdr_encode_int((uint8_t ** const)&keyptr, keylim, ++q));

    return HAL_OK;
}

// 5.5.  LMS Signature Verification

static hal_error_t lms_public_key_candidate(const lms_algorithm_t pubtype,
                                            const lmots_algorithm_t pubotstype,
                                            const bytestring16 * const I,
                                            const uint8_t * const msg, const size_t msg_len,
                                            const uint8_t * const sig, const size_t sig_len,
                                            bytestring32 * Tc);

static hal_error_t lms_verify(const uint8_t * const key, const size_t key_len,
                              const uint8_t * const msg, const size_t msg_len,
                              const uint8_t * const sig, const size_t sig_len)
{
    if (key == NULL || msg == NULL || sig == NULL)
        return HAL_ERROR_BAD_ARGUMENTS;

//   Algorithm 6: LMS Signature Verification

//     1. if the public key is not at least eight bytes long, return
//        INVALID

    if (key_len < 8)
        return HAL_ERROR_INVALID_SIGNATURE;

//     2. parse pubtype, I, and T[1] from the public key as follows:
//
//        a. pubtype = strTou32(first 4 bytes of public key)

    const uint8_t *keyptr = key;
    const uint8_t * const keylim = key + key_len;

    lms_algorithm_t pubtype;
    check(hal_xdr_decode_int(&keyptr, keylim, &pubtype));

//       b. ots_typecode = strTou32(next 4 bytes of public key)

    lmots_algorithm_t pubotstype;
    check(hal_xdr_decode_int(&keyptr, keylim, &pubotstype));

//        c. set m according to pubtype, based on Table 2

    size_t m = lms_type_to_m(pubtype);

//        d. if the public key is not exactly 24 + m bytes
//           long, return INVALID

    if (key_len != 24 + m)
        return HAL_ERROR_INVALID_SIGNATURE;

//        e. I = next 16 bytes of the public key

    bytestring16 I;
    check(hal_xdr_decode_bytestring16(&keyptr, keylim, &I));

//        f. T[1] = next m bytes of the public key

    bytestring32 T1;
    check(hal_xdr_decode_bytestring32(&keyptr, keylim, &T1));

//     3. compute the candidate LMS root value Tc from the signature,
//        message, identifier and pubtype using Algorithm 6b.

    bytestring32 Tc;
    check(lms_public_key_candidate(pubtype, pubotstype, &I, msg, msg_len, sig, sig_len, &Tc));

//     4. if Tc is equal to T[1], return VALID; otherwise, return INVALID

    return (memcmp(&Tc, &T1, sizeof(Tc)) ? HAL_ERROR_INVALID_SIGNATURE : HAL_OK);
}

static hal_error_t lms_public_key_candidate(const lms_algorithm_t pubtype,
                                            const lmots_algorithm_t pubotstype, // XXX missing from draft
                                            const bytestring16 * const I,
                                            const uint8_t * const msg, const size_t msg_len,
                                            const uint8_t * const sig, const size_t sig_len,
                                            bytestring32 * Tc)
{
//   Algorithm 6b: Computing an LMS Public Key Candidate from a Signature,
//   Message, Identifier, and algorithm typecode

//  1. if the signature is not at least eight bytes long, return INVALID

    if (sig_len < 8)
        return HAL_ERROR_INVALID_SIGNATURE;

//  2. parse sigtype, q, ots_signature, and path from the signature as
//     follows:
    const uint8_t *sigptr = sig;
    const uint8_t * const siglim = sig + sig_len;

//    a. q = strTou32(first 4 bytes of signature)

    uint32_t q;
    check(hal_xdr_decode_int(&sigptr, siglim, &q));

//    b. otssigtype = strTou32(next 4 bytes of signature)

    lmots_algorithm_t otssigtype;
    check(hal_xdr_decode_int_peek(&sigptr, siglim, &otssigtype));

//    c. if otssigtype is not the OTS typecode from the public key, return INVALID

    if (otssigtype != pubotstype)
        return HAL_ERROR_INVALID_SIGNATURE;

//    d. set n, p according to otssigtype and Table 1; if the
//    signature is not at least 12 + n * (p + 1) bytes long, return INVALID

    size_t n = lmots_type_to_n(otssigtype);
    size_t p = lmots_type_to_p(otssigtype);
    if (sig_len < 12 + n * (p + 1))
        return HAL_ERROR_INVALID_SIGNATURE;

//    e. ots_signature = bytes 8 through 8 + n * (p + 1) - 1 of signature

    /* XXX This is the remainder of ots_signature after otssigtype.
     * The full ots_signature would be bytes 4 through 8 + n * (p + 1) - 1.
     */

    const uint8_t * const ots_signature = sigptr;
    sigptr += lmots_signature_len(otssigtype);

//    f. sigtype = strTou32(4 bytes of signature at location 8 + n * (p + 1))

    lms_algorithm_t sigtype;
    check(hal_xdr_decode_int(&sigptr, siglim, &sigtype));

//    f. if sigtype is not the LM typecode from the public key, return INVALID

    if (sigtype != pubtype)
        return HAL_ERROR_INVALID_SIGNATURE;

//    g. set m, h according to sigtype and Table 2

    size_t m = lms_type_to_m(sigtype);
    size_t h = lms_type_to_h(sigtype);

//    h. if q >= 2^h or the signature is not exactly 12 + n * (p + 1) + m * h bytes long, return INVALID

    if (q >= (1U << h) || sig_len != 12 + n * (p + 1) + m * h)
        return HAL_ERROR_INVALID_SIGNATURE;

//    i. set path as follows:
//          path[0] = next m bytes of signature
//          path[1] = next m bytes of signature
//          ...
//          path[h-1] = next m bytes of signature

    bytestring32 path[h];
    for (size_t i = 0; i < h; ++i)
        check(hal_xdr_decode_bytestring32(&sigptr, siglim, &path[i]));

//  3. Kc = candidate public key computed by applying Algorithm 4b
//     to the signature ots_signature, the message, and the
//     identifiers I, q

    bytestring32 Kc;
    check(lmots_public_key_candidate(pubotstype, I, q, msg, msg_len, ots_signature, lmots_signature_len(otssigtype), &Kc));

//  4. compute the candidate LMS root value Tc as follows:
//     node_num = 2^h + q
    size_t r = (1 << h) + q;

//     tmp = H(I || u32str(node_num) || u16str(D_LEAF) || Kc)
    bytestring32 tmp;
    SHA256_CTX ctx;
    checkssl(SHA256_Init(&ctx));
    checkssl(SHA256_Update(&ctx, I, sizeof(*I)));
    uint32_t l = u32str(r); checkssl(SHA256_Update(&ctx, &l, sizeof(l)));
    uint16_t s = u16str(D_LEAF); checkssl(SHA256_Update(&ctx, &s, sizeof(s)));
    checkssl(SHA256_Update(&ctx, &Kc, sizeof(Kc)));
    checkssl(SHA256_Final((unsigned char *)&tmp, &ctx));

//     i = 0
//     while (node_num > 1) {
//       if (node_num is odd):
//         tmp = H(I||u32str(node_num/2)||u16str(D_INTR)||path[i]||tmp)
//       else:
//         tmp = H(I||u32str(node_num/2)||u16str(D_INTR)||tmp||path[i])
//       node_num = node_num/2
//       i = i + 1
//     }
    for (size_t i = 0; r > 1; r /= 2, ++i) {
        checkssl(SHA256_Init(&ctx));
        checkssl(SHA256_Update(&ctx, I, sizeof(*I)));
        uint32_t l = u32str(r/2); checkssl(SHA256_Update(&ctx, &l, sizeof(l)));
        uint16_t s = u16str(D_INTR); checkssl(SHA256_Update(&ctx, &s, sizeof(s)));
        if (r & 1) {
            checkssl(SHA256_Update(&ctx, &path[i], m));
            checkssl(SHA256_Update(&ctx, &tmp, sizeof(tmp)));
        }
        else {
            checkssl(SHA256_Update(&ctx, &tmp, sizeof(tmp)));
            checkssl(SHA256_Update(&ctx, &path[i], m));
        }
        checkssl(SHA256_Final((unsigned char *)&tmp, &ctx));
    }

//     Tc = tmp
    memcpy(Tc, &tmp, sizeof(*Tc));

    return HAL_OK;
}

/* ---------------------------------------------------------------- */

//6.  Hierarchical signatures

// 6.1.  Key Generation

static inline size_t hss_private_key_len(const size_t L, const lms_algorithm_t lms_type, const lmots_algorithm_t lmots_type)
{
    /* u32str(L) || lms_priv[0] || lms_priv[1] || ... || lms_priv[L-1] || sig[0] || pub[1] || ... || sig[L-2] || pub[L-1] */
    return (4 + (L * lms_private_key_len(lms_type, lmots_type)) +
            ((L - 1) * (lms_signature_len(lms_type, lmots_type) + lms_public_key_len(lms_type))));
}

static hal_error_t hss_generate_private_key(const size_t L,
                                            const lms_algorithm_t lms_type,
                                            const lmots_algorithm_t lmots_type,
                                            uint8_t * const key, size_t * const key_len, const size_t key_max)
{
    if (L == 0 || L > 8 ||
        lms_type < lms_sha256_n32_h5 || lms_type > lms_sha256_n32_h25 ||
        lmots_type < lmots_sha256_n32_w1 || lmots_type > lmots_sha256_n32_w8 ||
        key == NULL || key_max < hss_private_key_len(L, lms_type, lmots_type))
        return HAL_ERROR_BAD_ARGUMENTS;

    uint8_t *keyptr = key;
    const uint8_t * const keylim = key + key_max;

//   The HSS private key consists of prv[0], ... , prv[L-1].
    /* XXX This omits L */

    check(hal_xdr_encode_int(&keyptr, keylim, L));

    size_t lms_prv_len = lms_private_key_len(lms_type, lmots_type);
    size_t lms_pub_len = lms_public_key_len(lms_type);
    size_t lms_sig_len = lms_signature_len(lms_type, lmots_type);

    for (size_t i = 0; i < L; ++i, keyptr += lms_prv_len) {
        check(lms_generate_private_key(lms_type, lmots_type, keyptr, NULL, lms_prv_len));
        if (i > 0) {
            /* Generate the public key, and sign it with the previous private
             * key. Stash the signed public keys at the end of the private
             * key file, so we can copy them wholesale into the signature.
             */
            uint8_t *sigptr = key + 4 + (L * lms_prv_len) + ((i - 1) * (lms_sig_len + lms_pub_len));
            uint8_t *pubptr = sigptr + lms_sig_len;
            check(lms_generate_public_key(keyptr, lms_prv_len, pubptr, NULL, lms_pub_len));
            check(lms_sign(keyptr - lms_prv_len, lms_prv_len, pubptr, lms_pub_len, sigptr, NULL, lms_sig_len));
        }
    }
    keyptr += (L - 1) * (lms_sig_len + lms_pub_len);

    if (key_len != NULL)
        *key_len = keyptr - key;

    return HAL_OK;
}

static inline size_t hss_public_key_len(const lms_algorithm_t lms_type)
{
    /* L || pub[0] */
    return (sizeof(uint32_t) + lms_public_key_len(lms_type));
}

static hal_error_t hss_generate_public_key(const size_t L,
                                           const lms_algorithm_t lms_type,
                                           const lmots_algorithm_t lmots_type,
                                           const uint8_t * const key, const size_t key_len,
                                           uint8_t *pub, size_t *pub_len, const size_t pub_max)
{
    if (key == NULL || key_len < hss_private_key_len(L, lms_type, lmots_type) ||
        pub == NULL || pub_max < hss_public_key_len(lms_type))
        return HAL_ERROR_BAD_ARGUMENTS;

    const uint8_t *keyptr = key;
    const uint8_t * const keylim = key + key_len;
    uint8_t *pubptr = pub;
    const uint8_t * const publim = pub + pub_max;

//   The public key of the HSS scheme consists of the number of levels L,
//   followed by pub[0], the public key of the top level.

    uint32_t L_key;
    check(hal_xdr_decode_int(&keyptr, keylim, &L_key));
    if ((size_t)L_key != L)
        return HAL_ERROR_BAD_ARGUMENTS;

    check(hal_xdr_encode_int(&pubptr, publim, L));

    /* generate the lms public key from the first lms private key */
    size_t len;
    check(lms_generate_public_key(keyptr, lms_private_key_len(lms_type, lmots_type), pubptr, &len, publim - pubptr));
    pubptr += len;

    if (pub_len != NULL)
        *pub_len = pubptr - pub;

    return HAL_OK;
}

//6.2.  Signature Generation

static inline size_t hss_signature_len(const size_t L, const lms_algorithm_t lms_type, const lmots_algorithm_t lmots_type)
{
    /* u32str(Nspk) || sig[0] || pub[1] || ... || sig[Nspk-1] || pub[Nspk] || sig[Nspk] */
    return (sizeof(uint32_t) +
            (L * lms_signature_len(lms_type, lmots_type)) +
            ((L - 1) * lms_public_key_len(lms_type)));
}

static hal_error_t hss_sign(const uint8_t * const key, const size_t key_len,
                            const uint8_t * const msg, const size_t msg_len,
                            uint8_t *sig, size_t *sig_len, const size_t sig_max)
{
    if (key == NULL || msg == NULL || sig == NULL)
        return HAL_ERROR_BAD_ARGUMENTS;

    const uint8_t *keyptr = key;
    const uint8_t * const keylim = key + key_len;

    /* private key format:
     * u32str(L) || lms_priv[0] || lms_priv[1] || ... || lms_priv[L-1]
     */

    uint32_t L;
    check(hal_xdr_decode_int(&keyptr, keylim, &L));

    /* peek into the first lms private key for lms and lmots types */
    lms_algorithm_t lms_type;
    check(hal_xdr_decode_int(&keyptr, keylim, &lms_type));
    keyptr += 4;	/* skip over q */
    lmots_algorithm_t lmots_type;
    check(hal_xdr_decode_int(&keyptr, keylim, &lmots_type));

    size_t h = lms_type_to_h(lms_type);
    size_t lms_prv_len = lms_private_key_len(lms_type, lmots_type);
    size_t lms_pub_len = lms_public_key_len(lms_type);
    size_t lms_sig_len = lms_signature_len(lms_type, lmots_type);

//   To sign a message using the private key prv, the following steps are
//   performed:
//
//      If prv[L-1] is exhausted, then determine the smallest integer d
//      such that all of the private keys prv[d], prv[d+1], ... , prv[L-1]
//      are exhausted.  If d is equal to zero, then the HSS key pair is
//      exhausted, and it MUST NOT generate any more signatures.
//      Otherwise, the key pairs for levels d through L-1 must be
//      regenerated during the signature generation process, as follows.
//      For i from d to L-1, a new LMS public and private key pair with a
//      new identifier is generated, pub[i] and prv[i] are set to those
//      values, then the public key pub[i] is signed with prv[i-1], and
//      sig[i-1] is set to the resulting value.

    uint32_t q;
    keyptr = key + 4 + ((L - 1) * lms_prv_len) + 4;
    check(hal_xdr_decode_int_peek(&keyptr, keylim, &q));
    if (q >= (1U << h)) {
        size_t d;
        for (d = L - 1; d > 0; --d) {
            keyptr = key + 4 + ((d -1) * lms_prv_len) + 4;
            check(hal_xdr_decode_int_peek(&keyptr, keylim, &q));
            if (q < (1U << h))
                break;
        }
        if (d == 0)
            return HAL_ERROR_HASHSIG_KEY_EXHAUSTED;
        for ( ; d < L; ++d) {
            keyptr = key + 4 + (d * lms_prv_len);
            /* generate the new private key */
            check(lms_generate_private_key(lms_type, lmots_type, (uint8_t * const)keyptr, NULL, lms_prv_len));
            /* generate the new signed public key, after all the private keys */
            uint8_t *sigptr = (uint8_t *)key + 4 + (L * lms_prv_len) + ((d - 1) * (lms_sig_len + lms_pub_len));
            uint8_t *pubptr = sigptr + lms_pub_len;
            check(lms_generate_public_key(keyptr, lms_prv_len, pubptr, NULL, lms_pub_len));
            check(lms_sign(keyptr - lms_prv_len, lms_prv_len, pubptr, lms_pub_len, sigptr, NULL, lms_sig_len));
        }
    }

    uint8_t *sigptr = sig;
    const uint8_t * const siglim = sig + sig_max;
    check(hal_xdr_encode_int(&sigptr, siglim, L - 1));

//      The message is signed with prv[L-1], and the value sig[L-1] is set
//      to that result.
//
//      The value of the HSS signature is set as follows.  We let
//      signed_pub_key denote an array of octet strings, where
//      signed_pub_key[i] = sig[i] || pub[i+1], for i between 0 and Nspk-
//      1, inclusive, where Nspk = L-1 denotes the number of signed public
//      keys.  Then the HSS signature is u32str(Nspk) ||
//      signed_pub_key[0] || ... || signed_pub_key[Nspk-1] || sig[Nspk].

    /* copy the precomputed signed public keys from the end of the private key */
    keyptr = key + 4 + (L * lms_prv_len);
    check(hal_xdr_encode_fixed_opaque(&sigptr, siglim, keyptr, (L - 1) * (lms_sig_len + lms_pub_len)));

    /* sign the message with the last lms private key */
    keyptr -= lms_prv_len;
    check(lms_sign(keyptr, lms_prv_len, msg, msg_len, sigptr, NULL, lms_sig_len));
    sigptr += lms_sig_len;

    if (sig_len != NULL)
        *sig_len = sigptr - sig;

    return HAL_OK;
}

//6.3.  Signature Verification

static hal_error_t hss_verify(const uint8_t * const pub, const size_t pub_len,
                              const uint8_t * const message, const size_t message_len,
                              const uint8_t * const signature, const size_t signature_len)
{
    if (pub == NULL || message == NULL || signature == NULL)
        return HAL_ERROR_BAD_ARGUMENTS;

//   To verify a signature sig and message using the public key pub, the
//   following steps are performed:
//
//      The signature S is parsed into its components as follows:

//      Nspk = strTou32(first four bytes of S)

    const uint8_t * sigptr = signature;
    const uint8_t * const siglim = signature + signature_len;
    uint32_t Nspk;
    check(hal_xdr_decode_int(&sigptr, siglim, &Nspk));

//      if Nspk+1 is not equal to the number of levels L in pub:
//         return INVALID

    const uint8_t * pubptr = pub;
    const uint8_t * const publim = pub + pub_len;
    uint32_t L;
    check(hal_xdr_decode_int(&pubptr, publim, &L));
    if (Nspk + 1 != L)
        return HAL_ERROR_INVALID_SIGNATURE;

//      key = pub
    const uint8_t *key = pubptr; size_t key_len = pub_len - sizeof(uint32_t);
    const uint8_t *sig; size_t sig_len;
    const uint8_t *msg; size_t msg_len;

//      for (i = 0; i < Nspk; i = i + 1) {
    for (size_t i = 0; i < Nspk; ++i) {

        /* peek into the lms public key for lms and lmots types */
        const uint8_t *keyptr = key;
        const uint8_t * const keylim = key + key_len;
        lms_algorithm_t lms_type;
        check(hal_xdr_decode_int(&keyptr, keylim, &lms_type));
        lmots_algorithm_t lmots_type;
        check(hal_xdr_decode_int(&keyptr, keylim, &lmots_type));

//         sig = next LMS signature parsed from S
//         msg = next LMS public key parsed from S
        sig = sigptr; sig_len = lms_signature_len(lms_type, lmots_type); sigptr += sig_len;
        msg = sigptr; msg_len = lms_public_key_len(lms_type); sigptr += msg_len;

        if (sigptr > siglim)
            return HAL_ERROR_INVALID_SIGNATURE;

//         if (lms_verify(msg, key, sig) != VALID):
//             return INVALID
        check(lms_verify(key, key_len, msg, msg_len, sig, sig_len));

//         key = msg
        key = msg; key_len = msg_len;
//      }
    }

    /* peek into the lms public key for lms and lmots types */
    const uint8_t *keyptr = key;
    const uint8_t * const keylim = key + key_len;
    lms_algorithm_t lms_type;
    check(hal_xdr_decode_int(&keyptr, keylim, &lms_type));
    lmots_algorithm_t lmots_type;
    check(hal_xdr_decode_int(&keyptr, keylim, &lmots_type));

//      sig = next LMS signature parsed from S
    sig = sigptr; sig_len = lms_signature_len(lms_type, lmots_type); sigptr += sig_len;

    if (sigptr != siglim)
        return HAL_ERROR_INVALID_SIGNATURE;

//      return lms_verify(message, key, sig)
    check(lms_verify(key, key_len, message, message_len, sig, sig_len));

    return HAL_OK;
}

/* ---------------------------------------------------------------- */

/*
 * driver code, roughly like hash-sigs-c/demo.c
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

static void hexdump(const char * const label, const void * const buf, const size_t len)
{
    printf("%-11s ", label);

    const uint8_t *b = buf;
    for (size_t i = 0; i < len; ++i, ++b) {
        printf("%02x", *b);
        if ((i & 0x0f) == 0x0f) {
            printf("\n");
            if (i < len - 1)
                printf("            ");
        }
    }
    if ((len & 0x0f) != 0)
        printf("\n");
}

static hal_error_t dump_lmots_prv(const uint8_t * const buf, size_t * const len, const size_t max)
{
    const uint8_t *bufptr = buf;
    const uint8_t * const buflim = buf + max;

    hexdump("lmots type", bufptr, 4);
    uint32_t lmots_type;
    check(hal_xdr_decode_int(&bufptr, buflim, &lmots_type));
    hexdump("I", bufptr, 16);
    bufptr += 16;
    hexdump("q", bufptr, 4);
    bufptr += 4;
    size_t p = lmots_type_to_p(lmots_type);
    for (size_t i = 0; i < p && bufptr < buflim; ++i) {
        char label[16];
        sprintf(label, "x[%lu]", i);
        hexdump(label, bufptr, 32);
        bufptr += 32;
    }

    if (len != NULL)
        *len = bufptr - buf;

    return HAL_OK;
}

static hal_error_t dump_lmots_sig(const uint8_t * const buf, size_t * const len, const size_t max)
{
    const uint8_t *bufptr = buf;
    const uint8_t * const buflim = buf + max;

    hexdump("lmots type", bufptr, 4);
    uint32_t lmots_type;
    check(hal_xdr_decode_int(&bufptr, buflim, &lmots_type));
    hexdump("C", bufptr, 32);
    bufptr += 32;
    size_t p = lmots_type_to_p(lmots_type);
    for (size_t i = 0; i < p && bufptr < buflim; ++i) {
        char label[16];
        sprintf(label, "y[%lu]", i);
        hexdump(label, bufptr, 32);
        bufptr += 32;
    }

    if (len != NULL)
        *len = bufptr - buf;

    return HAL_OK;
}

static hal_error_t dump_lms_prv(const uint8_t * const buf, size_t * const len, const size_t max)
{
    const uint8_t *bufptr = buf;
    const uint8_t * const buflim = buf + max;

    hexdump("lms type", bufptr, 4);
    uint32_t lms_type;
    check(hal_xdr_decode_int(&bufptr, buflim, &lms_type));
    hexdump("q", bufptr, 4);
    uint32_t q;
    check(hal_xdr_decode_int(&bufptr, buflim, &q));
    size_t h = lms_type_to_h(lms_type);
    for (size_t i = 0; i < (1U << h) && bufptr < buflim; ++i) {
        printf("--------------------------------------------\notsprv[%lu]\n", i);
        size_t lmots_len;
        check(dump_lmots_prv(bufptr, &lmots_len, buflim - bufptr));
        bufptr += lmots_len;
    }

    if (len != NULL)
        *len = bufptr - buf;

    return HAL_OK;
}

static hal_error_t dump_lms_pub(const uint8_t * const buf, size_t * const len, const size_t max)
{
    const uint8_t *bufptr = buf;
    const uint8_t * const buflim = buf + max;

    hexdump("lms type", bufptr, 4);
    uint32_t lms_type;
    check(hal_xdr_decode_int(&bufptr, buflim, &lms_type));
    hexdump("lmots type", bufptr, 4);
    bufptr += 4;
    hexdump("I", bufptr, 16);
    bufptr += 16;
    hexdump("T[1]", bufptr, 32);
    bufptr += 32;

    if (len != NULL)
        *len = bufptr - buf;

    return HAL_OK;
}

static hal_error_t dump_lms_sig(const uint8_t * const buf, size_t * const len, const size_t max)
{
    const uint8_t *bufptr = buf;
    const uint8_t * const buflim = buf + max;

    hexdump("q", bufptr, 4);
    bufptr += 4;
    size_t lmots_len;
    check(dump_lmots_sig(bufptr, &lmots_len, buflim - bufptr));
    bufptr += lmots_len;
    hexdump("lms type", bufptr, 4);
    uint32_t lms_type;
    check(hal_xdr_decode_int(&bufptr, buflim, &lms_type));
    size_t h = lms_type_to_h(lms_type);
    for (size_t i = 0; i < h && bufptr < buflim; ++i) {
        char label[16];
        sprintf(label, "path[%lu]", i);
        hexdump(label, bufptr, 32);
        bufptr += 32;
    }

    if (len != NULL)
        *len = bufptr - buf;

    return HAL_OK;
}

static hal_error_t dump_hss_prv(const uint8_t * const buf, const size_t max)
{
    const uint8_t *bufptr = buf;
    const uint8_t * const buflim = buf + max;
    size_t len;

    hexdump("L", bufptr, 4);
    uint32_t L;
    check(hal_xdr_decode_int(&bufptr, buflim, &L));

    for (size_t i = 0; i < L && bufptr < buflim; ++i) {
        printf("--------------------------------------------\nlmsprv[%lu]\n", i);
        check(dump_lms_prv(bufptr, &len, buflim - bufptr));
        bufptr += len;
    }

    printf("--------------------------------------------\n");
    for (size_t i = 0; i < L - 1 && bufptr < buflim; ++i) {
        printf("--------------------------------------------\nsig[%lu]\n", i);
        check(dump_lms_sig(bufptr, &len, buflim - bufptr));
        bufptr += len;
        printf("--------------------------------------------\npub[%lu]\n", i + 1);
        check(dump_lms_pub(bufptr, &len, buflim - bufptr));
        bufptr += len;
    }

    return HAL_OK;
}

static hal_error_t dump_hss_pub(const uint8_t * const buf, const size_t max)
{
    const uint8_t *bufptr = buf;
    const uint8_t * const buflim = buf + max;

    hexdump("L", bufptr, 4);
    uint32_t L;
    check(hal_xdr_decode_int(&bufptr, buflim, &L));

    printf("--------------------------------------------\npubkey[0]\n");
    size_t lms_len;
    check(dump_lms_pub(bufptr, &lms_len, buflim - bufptr));
    bufptr += lms_len;

    return HAL_OK;
}

static hal_error_t dump_hss_sig(const uint8_t * const buf, const size_t max)
{
    const uint8_t *bufptr = buf;
    const uint8_t * const buflim = buf + max;

    hexdump("Nspk", bufptr, 4);
    uint32_t Nspk;
    check(hal_xdr_decode_int(&bufptr, buflim, &Nspk));

    for (size_t i = 0; i <= Nspk && bufptr < buflim; ++i) {
        printf("--------------------------------------------\nsig[%lu]\n", i);
        size_t lms_len;
        check(dump_lms_sig(bufptr, &lms_len, buflim - bufptr));
        bufptr += lms_len;
        if (bufptr < buflim) {
            printf("--------------------------------------------\npubkey[%lu]\n", i + 1);
            check(dump_lms_pub(bufptr, &lms_len, buflim - bufptr));
            bufptr += lms_len;
        }
    }

    return HAL_OK;
}

int main(int argc, char *argv[])
{
    char usage[] = "\
Usage: %s genkey <keyname> [parameters]\n\
       %s sign <keyname> <files to sign>\n\
       %s verify <keyname> <files to sign>\n";

    int fd;
    struct stat statbuf;
    size_t len;
    hal_error_t err;

    if (argc < 3) {
        fprintf(stderr, usage, argv[0], argv[0], argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "genkey") == 0) {
        /* default parameter set */
        size_t L = 2;
        lms_algorithm_t lms_type = lms_sha256_n32_h5;
        lmots_algorithm_t lmots_type = lmots_sha256_n32_w8;
        if (argc > 3) {
            int val[3];
            for (int i = 0; i < 3; ++i) {
                char *str = strtok(i == 0 ? argv[3] : NULL, "/");
                if (str == NULL) {
                    fprintf(stderr, "genkey parameters are of the form \"L/h/w\", e.g. \"3/10/4\"\n");
                    return 1;
                }
                val[i] = atoi(str);
            }
            L = (size_t)val[0];
            if (L < 1 || L > 8) {
                fprintf(stderr, "unsupported value of L (%d), must be 1-8\n", val[0]);
                return 1;
            }
            switch (val[1]) {
            case 5: lms_type = lms_sha256_n32_h5; break;
            case 10: lms_type = lms_sha256_n32_h10; break;
            case 15: lms_type = lms_sha256_n32_h15; break;
            case 20: lms_type = lms_sha256_n32_h20; break;
            case 25: lms_type = lms_sha256_n32_h25; break;
            default:
                fprintf(stderr, "unsupported value of h (%d), must be 5,10,15,20,25\n", val[1]);
                return 1;
            }
            switch (val[2]) {
            case 1: lmots_type = lmots_sha256_n32_w1; break;
            case 2: lmots_type = lmots_sha256_n32_w2; break;
            case 4: lmots_type = lmots_sha256_n32_w4; break;
            case 8: lmots_type = lmots_sha256_n32_w8; break;
            default:
                fprintf(stderr, "unsupported value of w (%d), must be 1,2,4,8\n", val[2]);
                return 1;
            }
        }
        uint8_t prv[hss_private_key_len(L, lms_type, lmots_type)];
        size_t prv_len;
        if ((err = hss_generate_private_key(L, lms_type, lmots_type, prv, &prv_len, sizeof(prv))) != HAL_OK) {
            fprintf(stderr, "hss_generate_private_key: %s\n", hal_error_string(err));
            return 1;
        }
        if (prv_len != sizeof(prv)) {
            fprintf(stderr, "hss_generate_private_key returned length %lu, expected %lu\n", prv_len, sizeof(prv));
            return 1;
        }
        char fn[strlen(argv[2]) + 5];
        strcpy(fn, argv[2]);
        strcat(fn, ".prv");
        if ((fd = creat(fn, S_IRUSR|S_IWUSR)) == -1) {
            perror("creat prv");
            return 1;
        }
        if ((len = write(fd, prv, sizeof(prv))) != sizeof(prv)) {
            fprintf(stderr, "write prv returned %lu, expected %lu\n", len, sizeof(prv));
            return 1;
        }
        if (close(fd) != 0) {
            perror("close prv");
            return 1;
        }
        uint8_t pub[hss_public_key_len(lms_type)];
        if ((err = hss_generate_public_key(L, lms_type, lmots_type, prv, sizeof(prv), pub, &len, sizeof(pub))) != HAL_OK) {
            fprintf(stderr, "hss_generate_public_key: %s\n", hal_error_string(err));
            return 1;
        }
        if (len != sizeof(pub)) {
            fprintf(stderr, "hss_generate_public_key returned length %lu, expected %lu\n", len, sizeof(pub));
            return 1;
        }
        strcpy(fn, argv[2]);
        strcat(fn, ".pub");
        if ((fd = creat(fn, S_IRUSR|S_IWUSR)) == -1) {
            perror("creat pub");
            return 1;
        }
        if ((len = write(fd, pub, sizeof(pub))) != sizeof(pub)) {
            fprintf(stderr, "write pub returned %lu, expected %lu\n", len, sizeof(pub));
            return 1;
        }
        if (close(fd) != 0) {
            perror("close pub");
            return 1;
        }
        return 0;
    }

    else if (strcmp(argv[1], "sign") == 0) {
        if (argc < 4) {
            fprintf(stderr, usage, argv[0], argv[0], argv[0]);
            return 1;
        }
        char fn[strlen(argv[2]) + 5];
        strcpy(fn, argv[2]);
        strcat(fn, ".prv");
        if (stat(fn, &statbuf) != 0) {
            perror("stat prv");
            return 1;
        }
        if ((fd = open(fn, O_RDONLY)) == -1) {
            perror("open prv");
            return 1;
        }
        uint8_t prv[statbuf.st_size];
        if ((len = read(fd, prv, sizeof(prv))) != sizeof(prv)) {
            fprintf(stderr, "read prv returned %lu, expected %lu\n", len, sizeof(prv));
            return 1;
        }
        if (close(fd) != 0) {
            perror("close prv");
            return 1;
        }
        strcpy(fn, argv[2]);
        strcat(fn, ".pub");
        if (stat(fn, &statbuf) != 0) {
            perror("stat pub");
            return 1;
        }
        if ((fd = open(fn, O_RDONLY)) == -1) {
            perror("open pub");
            return 1;
        }
        uint8_t pub[statbuf.st_size];
        if ((len = read(fd, pub, sizeof(pub))) != sizeof(pub)) {
            fprintf(stderr, "read pub returned %lu, expected %lu\n", len, sizeof(pub));
            return 1;
        }
        if (close(fd) != 0) {
            perror("close pub");
            return 1;
        }
        for (int i = 3; i < argc; ++i) {
            char *fn = argv[i];
            printf("signing %s\n", fn);
            if (stat(fn, &statbuf) != 0) {
                perror("stat msg");
                return 1;
            }
            if ((fd = open(fn, O_RDONLY)) == -1) {
                perror("open msg");
                return 1;
            }
            uint8_t msg[statbuf.st_size];
            if ((len = read(fd, msg, sizeof(msg))) != sizeof(msg)) {
                fprintf(stderr, "read msg returned %lu, expected %lu\n", len, sizeof(msg));
                return 1;
            }
            if (close(fd) != 0) {
                perror("close msg");
                return 1;
            }
            const uint8_t * pubptr = pub;
            const uint8_t * const publim = pub + sizeof(pub);
            uint32_t L;
            check(hal_xdr_decode_int(&pubptr, publim, &L));
            lms_algorithm_t lms_type;
            check(hal_xdr_decode_int(&pubptr, publim, &lms_type));
            lmots_algorithm_t lmots_type;
            check(hal_xdr_decode_int(&pubptr, publim, &lmots_type));
            uint8_t sig[hss_signature_len(L, lms_type, lmots_type)];
            if ((err = hss_sign(prv, sizeof(prv), msg, sizeof(msg), sig, &len, sizeof(sig))) != HAL_OK) {
                fprintf(stderr, "hss_sign: %s\n", hal_error_string(err));
                return 1;
            }
            if (len != sizeof(sig)) {
                fprintf(stderr, "hss_sign returned length %lu, expected %lu\n", len, sizeof(sig));
                return 1;
            }
            char sn[strlen(fn) + 5];
            strcpy(sn, fn);
            strcat(sn, ".sig");
            if ((fd = creat(sn, S_IRUSR|S_IWUSR)) == -1) {
                perror("creat sig");
                return 1;
            }
            if ((len = write(fd, sig, sizeof(sig))) != sizeof(sig)) {
                fprintf(stderr, "write sig returned %lu, expected %lu\n", len, sizeof(sig));
                return 1;
            }
            if (close(fd) != 0) {
                perror("close sig");
                return 1;
            }
        }
        strcpy(fn, argv[2]);
        strcat(fn, ".prv");
        if ((fd = creat(fn, S_IRUSR|S_IWUSR)) == -1) {
            perror("creat prv");
            return 1;
        }
        if ((len = write(fd, prv, sizeof(prv))) != sizeof(prv)) {
            fprintf(stderr, "write prv returned %lu, expected %lu\n", len, sizeof(prv));
            return 1;
        }
        if (close(fd) != 0) {
            perror("close prv");
            return 1;
        }
        return 0;
    }

    else if (strcmp(argv[1], "verify") == 0) {
        if (argc < 4) {
            fprintf(stderr, usage, argv[0], argv[0], argv[0]);
            return 1;
        }
        char fn[strlen(argv[2]) + 5];
        strcpy(fn, argv[2]);
        strcat(fn, ".pub");
        if (stat(fn, &statbuf) != 0) {
            perror("stat pub");
            return 1;
        }
        if ((fd = open(fn, O_RDONLY)) == -1) {
            perror("open pub");
            return 1;
        }
        uint8_t pub[statbuf.st_size];
        if ((len = read(fd, pub, sizeof(pub))) != sizeof(pub)) {
            fprintf(stderr, "read pub returned %lu, expected %lu\n", len, sizeof(pub));
            return 1;
        }
        if (close(fd) != 0) {
            perror("close pub");
            return 1;
        }
        int ret = 0;
        for (int i = 3; i < argc; ++i) {
            char *fn = argv[i];
            printf("verifying %s\n", fn);
            if (stat(fn, &statbuf) != 0) {
                perror("stat msg");
                return 1;
            }
            if ((fd = open(fn, O_RDONLY)) == -1) {
                perror("open msg");
                return 1;
            }
            uint8_t msg[statbuf.st_size];
            if ((len = read(fd, msg, sizeof(msg))) != sizeof(msg)) {
                fprintf(stderr, "read msg returned %lu, expected %lu\n", len, sizeof(msg));
                return 1;
            }
            if (close(fd) != 0) {
                perror("close msg");
                return 1;
            }
            char sn[strlen(fn) + 5];
            strcpy(sn, fn);
            strcat(sn, ".sig");
            if (stat(sn, &statbuf) != 0) {
                perror("stat sig");
                return 1;
            }
            uint8_t sig[statbuf.st_size];
            if ((fd = open(sn, O_RDONLY)) == -1) {
                perror("open sig");
                return 1;
            }
            if ((len = read(fd, sig, sizeof(sig))) != sizeof(sig)) {
                fprintf(stderr, "write sig returned %lu, expected %lu\n", len, sizeof(sig));
                return 1;
            }
            if (close(fd) != 0) {
                perror("close sig");
                return 1;
            }
            if ((err = hss_verify(pub, sizeof(pub), msg, sizeof(msg), sig, sizeof(sig))) != HAL_OK) {
                fprintf(stderr, "hss_verify: %s\n", hal_error_string(err));
                ret++;
            }
            else
                printf("signature verified\n");
        }
        return ret;
    }

    else if (strcmp(argv[1], "read") == 0) {
        if (argc < 3) {
            fprintf(stderr, usage, argv[0], argv[0], argv[0]);
            return 1;
        }
        for (int i = 2; i < argc; ++i) {
            char *fn = argv[i];
            printf("reading %s\n", fn);
            if (stat(fn, &statbuf) != 0) {
                perror("stat");
                return 1;
            }
            if ((fd = open(fn, O_RDONLY)) == -1) {
                perror("open");
                return 1;
            }
            uint8_t buf[statbuf.st_size];
            if ((len = read(fd, buf, sizeof(buf))) != sizeof(buf)) {
                fprintf(stderr, "read returned %lu, expected %lu\n", len, sizeof(buf));
                return 1;
            }
            if (close(fd) != 0) {
                perror("close");
                return 1;
            }
            char *ext = strrchr(fn, '.');
            if (strcmp(ext, ".prv") == 0) {
                if (dump_hss_prv(buf, sizeof(buf)) != HAL_OK)
                    return 1;
            }
            else if (strcmp(ext, ".pub") == 0) {
                if (dump_hss_pub(buf, sizeof(buf)) != HAL_OK)
                    return 1;
            }
            else if (strcmp(ext, ".sig") == 0) {
                if (dump_hss_sig(buf, sizeof(buf)) != HAL_OK)
                    return 1;
            }
            else {
                fprintf(stderr, "unknown file type\n");
                return 1;
            }
        }
        return 0;
    }

    fprintf(stderr, usage, argv[0], argv[0], argv[0]);
    return 1;
}
