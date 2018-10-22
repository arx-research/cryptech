/*
 * asn1_internal.h
 * ---------------
 * Library internal header file for ASN.1 routines.  These functions
 * are not part of the public libhal API.
 *
 * The only reason for not collapsing this header file into
 * hal_internal.h is to maintain some isolation between the few
 * modules which use libtfm and the rest of the library.
 *
 * More than 20 years after it was written, the best simple
 * introduction to ASN.1 is still Burt Kalski's "A Layman's Guide to a
 * Subset of ASN.1, BER, and DER".  Ask your nearest search engine.
 *
 * Authors: Rob Austein
 * Copyright (c) 2015, NORDUnet A/S
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

#ifndef _HAL_ASN1_INTERNAL_H_
#define _HAL_ASN1_INTERNAL_H_

#include <stdint.h>

#include <tfm.h>

#define ASN1_UNIVERSAL          0x00
#define ASN1_APPLICATION        0x40
#define ASN1_CONTEXT_SPECIFIC   0x80
#define ASN1_PRIVATE            0xC0

#define ASN1_PRIMITIVE          0x00
#define ASN1_CONSTRUCTED        0x20

#define ASN1_TAG_MASK           0x1F

#define ASN1_INTEGER            (ASN1_PRIMITIVE   | 0x02)
#define ASN1_BIT_STRING         (ASN1_PRIMITIVE   | 0x03)
#define ASN1_OCTET_STRING       (ASN1_PRIMITIVE   | 0x04)
#define ASN1_NULL               (ASN1_PRIMITIVE   | 0x05)
#define ASN1_OBJECT_IDENTIFIER  (ASN1_PRIMITIVE   | 0x06)
#define ASN1_SEQUENCE           (ASN1_CONSTRUCTED | 0x10)
#define ASN1_SET                (ASN1_CONSTRUCTED | 0x11)

#define ASN1_EXPLICIT_CONTEXT   (ASN1_CONTEXT_SPECIFIC | ASN1_CONSTRUCTED)
#define ASN1_EXPLICIT_0         (ASN1_EXPLICIT_CONTEXT + 0)
#define ASN1_EXPLICIT_1         (ASN1_EXPLICIT_CONTEXT + 1)

/*
 * Functions to strip const qualifiers from arguments to libtfm calls
 * in a relatively type-safe manner.  These don't really have anything
 * to do with ASN.1 per se, but all the code that needs them reads
 * this header file, so this is the simplest place to put them.
 */

static inline fp_int *unconst_fp_int(const fp_int * const arg)
{
  return (fp_int *) arg;
}

static inline uint8_t *unconst_uint8_t(const uint8_t * const arg)
{
  return (uint8_t *) arg;
}

/*
 * OIDs.
 */

extern const uint8_t hal_asn1_oid_rsaEncryption[];
extern const size_t  hal_asn1_oid_rsaEncryption_len;

extern const uint8_t hal_asn1_oid_ecPublicKey[];
extern const size_t  hal_asn1_oid_ecPublicKey_len;

extern const uint8_t hal_asn1_oid_aesKeyWrap[];
extern const size_t  hal_asn1_oid_aesKeyWrap_len;

extern const uint8_t hal_asn1_oid_mts_hashsig[];
extern const size_t hal_asn1_oid_mts_hashsig_len;

/*
 * Transcoding functions.
 */

extern hal_error_t hal_asn1_encode_header(const uint8_t tag,
                                          const size_t value_len,
                                          uint8_t *der, size_t *der_len, const size_t der_max);

extern hal_error_t hal_asn1_decode_header(const uint8_t tag,
                                          const uint8_t * const der, size_t der_max,
                                          size_t *hlen, size_t *vlen);

extern hal_error_t hal_asn1_encode_integer(const fp_int * const bn,
                                           uint8_t *der, size_t *der_len, const size_t der_max);

extern hal_error_t hal_asn1_encode_uint32(const uint32_t n,
                                          uint8_t *der, size_t *der_len, const size_t der_max);

extern hal_error_t hal_asn1_encode_octet_string(const uint8_t * const data,    const size_t data_len,
                                                uint8_t *der, size_t *der_len, const size_t der_max);

extern hal_error_t hal_asn1_decode_integer(fp_int *bn,
                                           const uint8_t * const der, size_t *der_len, const size_t der_max);

extern hal_error_t hal_asn1_decode_uint32(uint32_t *np,
                                          const uint8_t * const der, size_t *der_len, const size_t der_max);

extern hal_error_t hal_asn1_decode_octet_string(uint8_t *data, const size_t data_len,
                                                const uint8_t * const der, size_t *der_len, const size_t der_max);

extern hal_error_t hal_asn1_encode_spki(const uint8_t * const alg_oid,   const size_t alg_oid_len,
                                        const uint8_t * const curve_oid, const size_t curve_oid_len,
                                        const uint8_t * const pubkey,    const size_t pubkey_len,
                                        uint8_t *der, size_t *der_len, const size_t der_max);

extern hal_error_t hal_asn1_decode_spki(const uint8_t **alg_oid,   size_t *alg_oid_len,
                                        const uint8_t **curve_oid, size_t *curve_oid_len,
                                        const uint8_t **pubkey,    size_t *pubkey_len,
                                        const uint8_t *const der,  const size_t der_len);

extern hal_error_t hal_asn1_encode_pkcs8_privatekeyinfo(const uint8_t * const alg_oid,   const size_t alg_oid_len,
                                                        const uint8_t * const curve_oid, const size_t curve_oid_len,
                                                        const uint8_t * const privkey,   const size_t privkey_len,
                                                        uint8_t *der, size_t *der_len, const size_t der_max);

extern hal_error_t hal_asn1_decode_pkcs8_privatekeyinfo(const uint8_t **alg_oid,   size_t *alg_oid_len,
                                                        const uint8_t **curve_oid, size_t *curve_oid_len,
                                                        const uint8_t **privkey,   size_t *privkey_len,
                                                        const uint8_t *const der,  const size_t der_len);

extern hal_error_t hal_asn1_encode_pkcs8_encryptedprivatekeyinfo(const uint8_t * const alg_oid, const size_t alg_oid_len,
                                                                 const uint8_t * const data,    const size_t data_len,
                                                                 uint8_t *der, size_t *der_len, const size_t der_max);

extern hal_error_t hal_asn1_decode_pkcs8_encryptedprivatekeyinfo(const uint8_t **alg_oid,  size_t *alg_oid_len,
                                                                 const uint8_t **data,     size_t *data_len,
                                                                 const uint8_t *const der, const size_t der_len);

extern hal_error_t hal_asn1_guess_key_type(hal_key_type_t *type, hal_curve_name_t *curve,
                                           const uint8_t *const der,  const size_t der_len);

/*
 * Peek ahead for an OPTIONAL attribute.
 */

static inline int hal_asn1_peek(const uint8_t tag,
                                const uint8_t * const der, size_t der_max)
{
  return der != NULL && der_max > 0 && der[0] == tag;
}

#endif /* _HAL_ASN1_INTERNAL_H_ */

/*
 * Local variables:
 * indent-tabs-mode: nil
 * End:
 */
