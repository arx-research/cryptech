/*
 * asn1.c
 * ------
 * Minimal ASN.1 implementation in support of Cryptech libhal.
 *
 * The functions in this module are not intended to be part of the
 * public API.  Rather, these are utility functions used by more than
 * one module within the library, which would otherwise have to be
 * duplicated.  The main reason for keeping these private is to avoid
 * having the public API depend on any details of the underlying
 * bignum implementation (currently libtfm, but that might change).
 *
 * As of this writing, the ASN.1 support we need is quite minimal, so,
 * rather than attempting to clean all the unecessary cruft out of a
 * general purpose ASN.1 implementation, we hand code the very small
 * number of data types we need.  At some point this will probably
 * become impractical, at which point we might want to look into using
 * something like the asn1c compiler.
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

#include <stdint.h>

#include "hal.h"
#include "hal_internal.h"
#include "asn1_internal.h"

#define INIT_FP_INT     {{{0}}}

/*
 * Algorithm OIDs used in SPKI and PKCS #8.
 */

/*
 * From RFC 5480 New ASN.1 Modules for the Public Key Infrastructure Using X.509 (PKIX)
 *
 *     rsaEncryption OBJECT IDENTIFIER ::= {
 *         iso(1) member-body(2) US(840) rsadsi(113549) pkcs(1)
 *         pkcs-1(1) 1 }
 */
const uint8_t hal_asn1_oid_rsaEncryption[] = { 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x01 };
const size_t  hal_asn1_oid_rsaEncryption_len = sizeof(hal_asn1_oid_rsaEncryption);

/*
 * From RFC 5480 Elliptic Curve Cryptography Subject Public Key Information
 *
 *     id-ecPublicKey OBJECT IDENTIFIER ::= {
 *       iso(1) member-body(2) us(840) ansi-X9-62(10045) keyType(2) 1 }
 */
const uint8_t hal_asn1_oid_ecPublicKey[] = { 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01 };
const size_t  hal_asn1_oid_ecPublicKey_len = sizeof(hal_asn1_oid_ecPublicKey);

/*
 * From RFC 5649 Advanced Encryption Standard (AES) Key Wrap with Padding Algorithm
 *
 *      aes OBJECT IDENTIFIER ::= { joint-iso-itu-t(2) country(16)
 *                us(840) organization(1) gov(101) csor(3)
 *                nistAlgorithm(4) 1 }
 *
 *      id-aes128-wrap-pad OBJECT IDENTIFIER ::= { aes 8 }
 *
 *      id-aes256-wrap-pad OBJECT IDENTIFIER ::= { aes 48 }
 */
#if KEK_LENGTH == (bitsToBytes(128))
const uint8_t hal_asn1_oid_aesKeyWrap[] = { 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x01, 0x08 };
const size_t hal_asn1_oid_aesKeyWrap_len = sizeof(hal_asn1_oid_aesKeyWrap);
#endif

#if KEK_LENGTH == (bitsToBytes(256))
const uint8_t hal_asn1_oid_aesKeyWrap[] = { 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x01, 0x30 };
const size_t hal_asn1_oid_aesKeyWrap_len = sizeof(hal_asn1_oid_aesKeyWrap);
#endif

/*
 * From draft-housley-cms-mts-hash-sig Use of the Hash-based Merkle Tree Signature (MTS) Algorithm in the Cryptographic Message Syntax (CMS)
 *
 *      id-alg-mts-hashsig  OBJECT IDENTIFIER ::= { iso(1) member-body(2)
 *            us(840) rsadsi(113549) pkcs(1) pkcs9(9) smime(16) alg(3) 17 }
 */
const uint8_t hal_asn1_oid_mts_hashsig[] = { 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x09, 0x10, 0x03, 0x11 };
const size_t hal_asn1_oid_mts_hashsig_len = sizeof(hal_asn1_oid_mts_hashsig);

/*
 * Encode tag and length fields of an ASN.1 object.
 *
 * Sets *der_len to the size of of the ASN.1 header (tag and length
 * fields); caller supplied length of value field, so presumably
 * already knows it.
 *
 * If der is NULL, just return the size of the header that would be
 * encoded and returns HAL_OK.
 *
 * If der isn't NULL, returns HAL_ERROR_RESULT_TOO_LONG unless full
 * header plus value will fit; this is a bit weird, but is useful when
 * using this to construct encoders for complte ASN.1 objects.
 */

hal_error_t hal_asn1_encode_header(const uint8_t tag,
				   const size_t value_len,
				   uint8_t *der, size_t *der_len, const size_t der_max)
{
  size_t header_len = 2;	/* Shortest encoding is one octet each for tag and length */

  if (value_len >= 128)		/* Add octets for longer length encoding as needed */
    for (size_t n = value_len; n > 0; n >>= 8)
      ++header_len;

  if (der_len != NULL)
    *der_len = header_len;

  if (der == NULL)		/* If caller just wanted the length, we're done */
    return HAL_OK;

  /*
   * Make sure there's enough room for header + value, then encode.
   */

  if (value_len + header_len > der_max)
    return HAL_ERROR_RESULT_TOO_LONG;

  *der++ = tag;

  if (value_len < 128) {
    *der = (uint8_t) value_len;
  }

  else {
    *der = 0x80 | (uint8_t) (header_len -= 2);
    for (size_t n = value_len; n > 0 && header_len > 0; n >>= 8)
      der[header_len--] = (uint8_t) (n & 0xFF);
  }

  return HAL_OK;
}

/*
 * Encode an unsigned ASN.1 INTEGER from a libtfm bignum.  If der is
 * NULL, just return the length of what we would have encoded.
 */

hal_error_t hal_asn1_encode_integer(const fp_int * const bn,
				    uint8_t *der, size_t *der_len, const size_t der_max)
{
  if (bn == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  /*
   * We only handle unsigned INTEGERs, so we need to pad data with a
   * leading zero if the most significant bit is set, to avoid
   * flipping the ASN.1 sign bit.  Conveniently, this also handles the
   * difference between libtfm's and ASN.1's encoding of zero.
   */

  if (fp_cmp_d(unconst_fp_int(bn), 0) == FP_LT)
    return HAL_ERROR_BAD_ARGUMENTS;

  const int leading_zero = fp_iszero(bn) || (fp_count_bits(unconst_fp_int(bn)) & 7) == 0;
  const size_t vlen = fp_unsigned_bin_size(unconst_fp_int(bn)) + leading_zero;
  hal_error_t err;
  size_t hlen;

  err = hal_asn1_encode_header(ASN1_INTEGER, vlen, der, &hlen, der_max);

  if (der_len != NULL)
    *der_len = hlen + vlen;

  if (der == NULL || err != HAL_OK)
    return err;

  hal_assert(hlen + vlen <= der_max);

  der += hlen;
  if (leading_zero)
    *der++ = 0x00;
  fp_to_unsigned_bin(unconst_fp_int(bn), der);

  return HAL_OK;
}

/*
 * Encode an unsigned ASN.1 INTEGER from a uint32_t.  If der is
 * NULL, just return the length of what we would have encoded.
 */

hal_error_t hal_asn1_encode_uint32(const uint32_t n,
                                   uint8_t *der, size_t *der_len, const size_t der_max)
{
  /*
   * We only handle unsigned INTEGERs, so we need to pad data with a
   * leading zero if the most significant bit is set, to avoid
   * flipping the ASN.1 sign bit.
   */

  size_t vlen;
  hal_error_t err;
  size_t hlen;

  /* DER says to use the minimum number of octets */
  if (n < 0x80)            vlen = 1;
  else if (n < 0x8000)     vlen = 2;
  else if (n < 0x800000)   vlen = 3;
  else if (n < 0x80000000) vlen = 4;
  else                     vlen = 5;

  err = hal_asn1_encode_header(ASN1_INTEGER, vlen, der, &hlen, der_max);

  if (der_len != NULL)
    *der_len = hlen + vlen;

  if (der == NULL || err != HAL_OK)
    return err;

  hal_assert(hlen + vlen <= der_max);

  der += hlen;

  uint32_t m = n;
  for (size_t i = vlen; i > 0; --i) {
    der[i - 1] = m & 0xff;
    m >>= 8;
  }

  return HAL_OK;
}

/*
 * Encode an ASN.1 OCTET STRING.  If der is NULL, just return the length
 * of what we would have encoded.
 */

hal_error_t hal_asn1_encode_octet_string(const uint8_t * const data,    const size_t data_len,
                                         uint8_t *der, size_t *der_len, const size_t der_max)
{
  if (data_len == 0 || (der != NULL && data == NULL))
    return HAL_ERROR_BAD_ARGUMENTS;

  size_t hlen;
  hal_error_t err;

  if ((err = hal_asn1_encode_header(ASN1_OCTET_STRING, data_len, NULL, &hlen, 0)) != HAL_OK)
    return err;
  
  if (der_len != NULL)
    *der_len = hlen + data_len;

  if (der == NULL)
    return HAL_OK;

  hal_assert(hlen + data_len <= der_max);

  /*
   * Handle data early, in case it was staged into our output buffer.
   */
  memmove(der + hlen, data, data_len);

  if ((err = hal_asn1_encode_header(ASN1_OCTET_STRING, data_len, der, &hlen, der_max)) != HAL_OK)
    return err;

  return HAL_OK;
}

/*
 * Encode a public key into a X.509 SubjectPublicKeyInfo (RFC 5280).
 */

hal_error_t hal_asn1_encode_spki(const uint8_t * const alg_oid,   const size_t alg_oid_len,
                                 const uint8_t * const curve_oid, const size_t curve_oid_len,
                                 const uint8_t * const pubkey,    const size_t pubkey_len,
                                 uint8_t *der, size_t *der_len, const size_t der_max)
{
  if (alg_oid == NULL || alg_oid_len == 0 || pubkey_len == 0 ||
      (der != NULL && pubkey == NULL) || (curve_oid == NULL && curve_oid_len != 0))
    return HAL_ERROR_BAD_ARGUMENTS;

  const uint8_t curve_oid_tag = curve_oid == NULL ? ASN1_NULL : ASN1_OBJECT_IDENTIFIER;

  hal_error_t err;

  size_t hlen, hlen_spki, hlen_algid, hlen_alg, hlen_curve, hlen_bit;

  if ((err = hal_asn1_encode_header(ASN1_OBJECT_IDENTIFIER, alg_oid_len,    NULL, &hlen_alg,   0)) != HAL_OK ||
      (err = hal_asn1_encode_header(curve_oid_tag,          curve_oid_len,  NULL, &hlen_curve, 0)) != HAL_OK ||
      (err = hal_asn1_encode_header(ASN1_BIT_STRING,        1 + pubkey_len, NULL, &hlen_bit,   0)) != HAL_OK)
    return err;

  const size_t algid_len =
    hlen_alg + alg_oid_len + hlen_curve + curve_oid_len;

  if ((err = hal_asn1_encode_header(ASN1_SEQUENCE,          algid_len,     NULL, &hlen_algid, 0)) != HAL_OK)
    return err;

  const size_t vlen =
    hlen_algid + hlen_alg + alg_oid_len + hlen_curve + curve_oid_len + hlen_bit + 1 + pubkey_len;

  if ((err = hal_asn1_encode_header(ASN1_SEQUENCE,          vlen,          NULL, &hlen_spki,  0)) != HAL_OK)
    return err;

  /*
   * Handle pubkey early, in case it was staged into our output buffer.
   */
  if (der != NULL && hlen_spki + vlen <= der_max)
    memmove(der + hlen_spki + vlen - pubkey_len, pubkey, pubkey_len);

  err = hal_asn1_encode_header(ASN1_SEQUENCE, vlen, der, &hlen, der_max);

  if (der_len != NULL)
    *der_len = hlen + vlen;

  if (der == NULL || err != HAL_OK)
    return err;

  uint8_t *d = der + hlen;
  memset(d, 0, vlen - pubkey_len);

  if ((err = hal_asn1_encode_header(ASN1_SEQUENCE, algid_len, d, &hlen, der + der_max - d)) != HAL_OK)
    return err;
  d += hlen;

  if ((err = hal_asn1_encode_header(ASN1_OBJECT_IDENTIFIER, alg_oid_len, d, &hlen, der + der_max - d)) != HAL_OK)
    return err;
  d += hlen;
  memcpy(d, alg_oid, alg_oid_len);
  d += alg_oid_len;

  if ((err = hal_asn1_encode_header(curve_oid_tag, curve_oid_len, d, &hlen, der + der_max - d)) != HAL_OK)
    return err;
  d += hlen;
  if (curve_oid != NULL)
    memcpy(d, curve_oid, curve_oid_len);
  d += curve_oid_len;

  if ((err = hal_asn1_encode_header(ASN1_BIT_STRING, 1 + pubkey_len, d, &hlen, der + der_max - d)) != HAL_OK)
    return err;
  d += hlen;
  *d++ = 0x00;
  d += pubkey_len;              /* pubkey handled early, above. */

  hal_assert(d == der + hlen_spki + vlen);
  hal_assert(d <= der + der_max);

  return HAL_OK;
}

/*
 * Encode a PKCS #8 PrivateKeyInfo (RFC 5208).
 */

hal_error_t hal_asn1_encode_pkcs8_privatekeyinfo(const uint8_t * const alg_oid,   const size_t alg_oid_len,
                                                 const uint8_t * const curve_oid, const size_t curve_oid_len,
                                                 const uint8_t * const privkey,   const size_t privkey_len,
                                                 uint8_t *der, size_t *der_len, const size_t der_max)
{
  if (alg_oid == NULL || alg_oid_len == 0 || privkey_len == 0 ||
      (der != NULL && privkey == NULL) || (curve_oid == NULL && curve_oid_len != 0))
    return HAL_ERROR_BAD_ARGUMENTS;

  const uint8_t curve_oid_tag = curve_oid == NULL ? ASN1_NULL : ASN1_OBJECT_IDENTIFIER;

  fp_int version[1] = INIT_FP_INT;

  hal_error_t err;

  size_t version_len, hlen, hlen_algid, hlen_alg, hlen_curve, hlen_oct;

  if ((err = hal_asn1_encode_integer(version,                               NULL, &version_len, 0)) != HAL_OK ||
      (err = hal_asn1_encode_header(ASN1_OBJECT_IDENTIFIER, alg_oid_len,    NULL, &hlen_alg,    0)) != HAL_OK ||
      (err = hal_asn1_encode_header(curve_oid_tag,          curve_oid_len,  NULL, &hlen_curve,  0)) != HAL_OK ||
      (err = hal_asn1_encode_header(ASN1_OCTET_STRING,      privkey_len,    NULL, &hlen_oct,    0)) != HAL_OK)
    return err;

  const size_t algid_len =
    hlen_alg + alg_oid_len + hlen_curve + curve_oid_len;

  if ((err = hal_asn1_encode_header(ASN1_SEQUENCE,          algid_len,     NULL, &hlen_algid,   0)) != HAL_OK)
    return err;

  const size_t vlen =
    version_len + hlen_algid + hlen_alg + alg_oid_len + hlen_curve + curve_oid_len + hlen_oct + privkey_len;

  if ((err = hal_asn1_encode_header(ASN1_SEQUENCE,          vlen,          NULL, &hlen,         0)) != HAL_OK)
    return err;

  if (der_len != NULL)
    *der_len = hlen + vlen;

  if (der == NULL)
    return HAL_OK;

  uint8_t * const der_end = der + hlen + vlen;

  /*
   * Handle privkey early, in case it was staged into our output buffer.
   */
  if (der_end <= der + der_max)
    memmove(der_end - privkey_len, privkey, privkey_len);

  uint8_t *d = der;
  memset(d, 0, hlen + vlen - privkey_len);

  if ((err = hal_asn1_encode_header(ASN1_SEQUENCE, vlen, d, &hlen, der + der_max - d)) != HAL_OK)
    return err;
  d += hlen;

  if ((err = hal_asn1_encode_integer(version, d, NULL, der + der_max - d)) != HAL_OK)
    return err;
  d += version_len;

  if ((err = hal_asn1_encode_header(ASN1_SEQUENCE, algid_len, d, &hlen, der + der_max - d)) != HAL_OK)
    return err;
  d += hlen;

  if ((err = hal_asn1_encode_header(ASN1_OBJECT_IDENTIFIER, alg_oid_len, d, &hlen, der + der_max - d)) != HAL_OK)
    return err;
  d += hlen;
  memcpy(d, alg_oid, alg_oid_len);
  d += alg_oid_len;

  if ((err = hal_asn1_encode_header(curve_oid_tag, curve_oid_len, d, &hlen, der + der_max - d)) != HAL_OK)
    return err;
  d += hlen;
  if (curve_oid != NULL)
    memcpy(d, curve_oid, curve_oid_len);
  d += curve_oid_len;

  if ((err = hal_asn1_encode_header(ASN1_OCTET_STRING, privkey_len, d, &hlen, der + der_max - d)) != HAL_OK)
    return err;
  d += hlen;
  d += privkey_len;             /* privkey handled early, above. */

  hal_assert(d == der_end);
  hal_assert(d <= der + der_max);

  return HAL_OK;
}

/*
 * Encode a PKCS #8 EncryptedPrivateKeyInfo (RFC 5208).
 */

hal_error_t hal_asn1_encode_pkcs8_encryptedprivatekeyinfo(const uint8_t * const alg_oid, const size_t alg_oid_len,
                                                 const uint8_t * const data,    const size_t data_len,
                                                 uint8_t *der, size_t *der_len, const size_t der_max)
{
  if (alg_oid == NULL || alg_oid_len == 0 || data_len == 0 || (der != NULL && data == NULL))
    return HAL_ERROR_BAD_ARGUMENTS;

  hal_error_t err;

  size_t hlen, hlen_pkcs8, hlen_algid, hlen_alg, hlen_oct;

  if ((err = hal_asn1_encode_header(ASN1_OBJECT_IDENTIFIER, alg_oid_len,    NULL, &hlen_alg,    0)) != HAL_OK ||
      (err = hal_asn1_encode_header(ASN1_OCTET_STRING,      data_len,       NULL, &hlen_oct,    0)) != HAL_OK)
    return err;

  const size_t algid_len = hlen_alg + alg_oid_len;

  if ((err = hal_asn1_encode_header(ASN1_SEQUENCE,          algid_len,     NULL, &hlen_algid,   0)) != HAL_OK)
    return err;

  const size_t vlen = hlen_algid + hlen_alg + alg_oid_len + hlen_oct + data_len;

  if ((err = hal_asn1_encode_header(ASN1_SEQUENCE,          vlen,          NULL, &hlen_pkcs8,   0)) != HAL_OK)
    return err;

  /*
   * Handle data early, in case it was staged into our output buffer.
   */
  if (der != NULL && hlen_pkcs8 + vlen <= der_max)
    memmove(der + hlen_pkcs8 + vlen - data_len, data, data_len);

  err = hal_asn1_encode_header(ASN1_SEQUENCE, vlen, der, &hlen, der_max);

  if (der_len != NULL)
    *der_len = hlen + vlen;

  if (der == NULL || err != HAL_OK)
    return err;

  uint8_t *d = der + hlen;
  memset(d, 0, vlen - data_len);

  if ((err = hal_asn1_encode_header(ASN1_SEQUENCE, algid_len, d, &hlen, der + der_max - d)) != HAL_OK)
    return err;
  d += hlen;

  if ((err = hal_asn1_encode_header(ASN1_OBJECT_IDENTIFIER, alg_oid_len, d, &hlen, der + der_max - d)) != HAL_OK)
    return err;
  d += hlen;
  memcpy(d, alg_oid, alg_oid_len);
  d += alg_oid_len;

  if ((err = hal_asn1_encode_header(ASN1_OCTET_STRING, data_len, d, &hlen, der + der_max - d)) != HAL_OK)
    return err;
  d += hlen;

  d += data_len;                /* data handled early, above. */

  hal_assert(d == der + hlen_pkcs8 + vlen);
  hal_assert(d <= der + der_max);

  return HAL_OK;
}


/*
 * Parse tag and length of an ASN.1 object.  Tag must match value
 * specified by the caller.  On success, sets hlen and vlen to lengths
 * of header and value, respectively.
 */

hal_error_t hal_asn1_decode_header(const uint8_t tag,
				   const uint8_t * const der, size_t der_max,
				   size_t *hlen, size_t *vlen)
{
  hal_assert(der != NULL && hlen != NULL && vlen != NULL);

  if (der_max < 2 || der[0] != tag)
    return HAL_ERROR_ASN1_PARSE_FAILED;

  if ((der[1] & 0x80) == 0) {
    *hlen = 2;
    *vlen = der[1];
  }

  else {
    *hlen = 2 + (der[1] & 0x7F);
    *vlen = 0;

    if (*hlen > der_max)
      return HAL_ERROR_ASN1_PARSE_FAILED;

    for (size_t i = 2; i < *hlen; i++)
      *vlen = (*vlen << 8) + der[i];
  }

  if (*hlen + *vlen > der_max)
    return HAL_ERROR_ASN1_PARSE_FAILED;

  return HAL_OK;
}

/*
 * Decode an ASN.1 INTEGER into a libtfm bignum.  Since we only
 * support (or need to support, or expect to see) unsigned integers,
 * we return failure if the sign bit is set in the ASN.1 INTEGER.
 */

hal_error_t hal_asn1_decode_integer(fp_int *bn,
				    const uint8_t * const der, size_t *der_len, const size_t der_max)
{
  if (bn == NULL || der == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  hal_error_t err;
  size_t hlen, vlen;

  if ((err = hal_asn1_decode_header(ASN1_INTEGER, der, der_max, &hlen, &vlen)) != HAL_OK)
    return err;

  if (der_len != NULL)
    *der_len = hlen + vlen;

  if (vlen < 1 || (der[hlen] & 0x80) != 0x00)
    return HAL_ERROR_ASN1_PARSE_FAILED;

  fp_init(bn);
  fp_read_unsigned_bin(bn, (uint8_t *) der + hlen, vlen);
  return HAL_OK;
}

/*
 * Decode an ASN.1 INTEGER into a uint32_t.  Since we only
 * support (or need to support, or expect to see) unsigned integers,
 * we return failure if the sign bit is set in the ASN.1 INTEGER.
 */

hal_error_t hal_asn1_decode_uint32(uint32_t *np,
                                   const uint8_t * const der, size_t *der_len, const size_t der_max)
{
  if (np == NULL || der == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  hal_error_t err;
  size_t hlen, vlen;

  if ((err = hal_asn1_decode_header(ASN1_INTEGER, der, der_max, &hlen, &vlen)) != HAL_OK)
    return err;

  if (der_len != NULL)
    *der_len = hlen + vlen;

  if (vlen < 1 || vlen > 5 || (der[hlen] & 0x80) != 0x00 || (vlen == 5 && der[hlen] != 0))
    return HAL_ERROR_ASN1_PARSE_FAILED;

  uint32_t n = 0;
  for (size_t i = 0; i < vlen; ++i) {
    n <<= 8;		// slightly inefficient for the first octet
    n += der[hlen + i];
  }
  *np = n;

  return HAL_OK;
}

/*
 * Decode an ASN.1 OCTET STRING.
 */

hal_error_t hal_asn1_decode_octet_string(uint8_t *data, const size_t data_len,
                                         const uint8_t * const der, size_t *der_len, const size_t der_max)
{
  if (der == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  size_t hlen, vlen;
  hal_error_t err;

  if ((err = hal_asn1_decode_header(ASN1_OCTET_STRING, der, der_max, &hlen, &vlen)) != HAL_OK)
    return err;

  if (der_len != NULL)
    *der_len = hlen + vlen;

  if (data != NULL) {
    if (data_len != vlen)
      return HAL_ERROR_ASN1_PARSE_FAILED;
    memmove(data, der + hlen, vlen);
  }

  return HAL_OK;
}

/*
 * Decode a public key from a X.509 SubjectPublicKeyInfo (RFC 5280).
 */

hal_error_t hal_asn1_decode_spki(const uint8_t **alg_oid,   size_t *alg_oid_len,
                                 const uint8_t **curve_oid, size_t *curve_oid_len,
                                 const uint8_t **pubkey,    size_t *pubkey_len,
                                 const uint8_t *const der,  const size_t der_len)
{
  if (der == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  const uint8_t * const der_end = der + der_len;
  const uint8_t *d = der;

  size_t hlen, vlen;
  hal_error_t err;

  if ((err = hal_asn1_decode_header(ASN1_SEQUENCE, d, der_end - d, &hlen, &vlen)) != HAL_OK)
    return err;
  d += hlen;

  if (hlen + vlen != der_len)
    return HAL_ERROR_ASN1_PARSE_FAILED;

  if ((err = hal_asn1_decode_header(ASN1_SEQUENCE, d, der_end - d, &hlen, &vlen)) != HAL_OK)
    return err;
  d += hlen;

  const uint8_t * const algid_end = d + vlen;

  if ((err = hal_asn1_decode_header(ASN1_OBJECT_IDENTIFIER, d, algid_end - d, &hlen, &vlen)) != HAL_OK)
    return err;
  d += hlen;
  if (vlen > (size_t)(algid_end - d))
    return HAL_ERROR_ASN1_PARSE_FAILED;
  if (alg_oid != NULL)
    *alg_oid = d;
  if (alg_oid_len != NULL)
    *alg_oid_len = vlen;
  d += vlen;

  if (curve_oid != NULL)
    *curve_oid = NULL;
  if (curve_oid_len != NULL)
    *curve_oid_len = 0;

  if (d < algid_end) {
    switch (*d) {

    case ASN1_OBJECT_IDENTIFIER:
      if ((err = hal_asn1_decode_header(ASN1_OBJECT_IDENTIFIER, d, algid_end - d, &hlen, &vlen)) != HAL_OK)
        return err;
      d += hlen;
      if (vlen > (size_t)(algid_end - d))
        return HAL_ERROR_ASN1_PARSE_FAILED;
      if (curve_oid != NULL)
        *curve_oid = d;
      if (curve_oid_len != NULL)
        *curve_oid_len = vlen;
      d += vlen;
      break;

    case ASN1_NULL:
      if ((err = hal_asn1_decode_header(ASN1_NULL, d, algid_end - d, &hlen, &vlen)) != HAL_OK)
        return err;
      d += hlen;
      if (vlen == 0)
        break;

    default:
      return HAL_ERROR_ASN1_PARSE_FAILED;
    }
  }

  if (d != algid_end)
    return HAL_ERROR_ASN1_PARSE_FAILED;

  if ((err = hal_asn1_decode_header(ASN1_BIT_STRING, d, der_end - d, &hlen, &vlen)) != HAL_OK)
    return err;
  d += hlen;
  if (vlen >= (size_t)(algid_end - d) || vlen == 0 || *d != 0x00)
    return HAL_ERROR_ASN1_PARSE_FAILED;
  ++d; --vlen;
  if (pubkey != NULL)
    *pubkey = d;
  if (pubkey_len != NULL)
    *pubkey_len = vlen;
  d += vlen;

  if (d != der_end)
    return HAL_ERROR_ASN1_PARSE_FAILED;

  return HAL_OK;
}

/*
 * Decode a private key from a PKCS #8 PrivateKeyInfo (RFC 5208).
 */

hal_error_t hal_asn1_decode_pkcs8_privatekeyinfo(const uint8_t **alg_oid,   size_t *alg_oid_len,
                                                 const uint8_t **curve_oid, size_t *curve_oid_len,
                                                 const uint8_t **privkey,   size_t *privkey_len,
                                                 const uint8_t *const der,  const size_t der_len)
{
  if (der == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  const uint8_t * const der_end = der + der_len;
  const uint8_t *d = der;

  fp_int version[1] = INIT_FP_INT;
  size_t hlen, vlen;
  hal_error_t err;

  if ((err = hal_asn1_decode_header(ASN1_SEQUENCE, d, der_end - d, &hlen, &vlen)) != HAL_OK)
    return err;
  d += hlen;

  if (hlen + vlen != der_len)
    return HAL_ERROR_ASN1_PARSE_FAILED;

  if ((err = hal_asn1_decode_integer(version, d, &hlen, der_end - d)) != HAL_OK)
    return err;
  if (!fp_iszero(version))
    return HAL_ERROR_ASN1_PARSE_FAILED;
  d += hlen;

  if ((err = hal_asn1_decode_header(ASN1_SEQUENCE, d, der_end - d, &hlen, &vlen)) != HAL_OK)
    return err;
  d += hlen;

  const uint8_t * const algid_end = d + vlen;

  if ((err = hal_asn1_decode_header(ASN1_OBJECT_IDENTIFIER, d, algid_end - d, &hlen, &vlen)) != HAL_OK)
    return err;
  d += hlen;
  if (vlen > (size_t)(algid_end - d))
    return HAL_ERROR_ASN1_PARSE_FAILED;
  if (alg_oid != NULL)
    *alg_oid = d;
  if (alg_oid_len != NULL)
    *alg_oid_len = vlen;
  d += vlen;

  if (curve_oid != NULL)
    *curve_oid = NULL;
  if (curve_oid_len != NULL)
    *curve_oid_len = 0;

  if (d < algid_end) {
    switch (*d) {

    case ASN1_OBJECT_IDENTIFIER:
      if ((err = hal_asn1_decode_header(ASN1_OBJECT_IDENTIFIER, d, algid_end - d, &hlen, &vlen)) != HAL_OK)
        return err;
      d += hlen;
      if (vlen > (size_t)(algid_end - d))
        return HAL_ERROR_ASN1_PARSE_FAILED;
      if (curve_oid != NULL)
        *curve_oid = d;
      if (curve_oid_len != NULL)
        *curve_oid_len = vlen;
      d += vlen;
      break;

    case ASN1_NULL:
      if ((err = hal_asn1_decode_header(ASN1_NULL, d, algid_end - d, &hlen, &vlen)) != HAL_OK)
        return err;
      d += hlen;
      if (vlen == 0)
        break;

    default:
      return HAL_ERROR_ASN1_PARSE_FAILED;
    }
  }

  if (d != algid_end)
    return HAL_ERROR_ASN1_PARSE_FAILED;

  if ((err = hal_asn1_decode_header(ASN1_OCTET_STRING, d, der_end - d, &hlen, &vlen)) != HAL_OK)
    return err;
  d += hlen;
  if (vlen >= (size_t)(algid_end - d))
    return HAL_ERROR_ASN1_PARSE_FAILED;
  if (privkey != NULL)
    *privkey = d;
  if (privkey_len != NULL)
    *privkey_len = vlen;
  d += vlen;

  if (d != der_end)
    return HAL_ERROR_ASN1_PARSE_FAILED;

  return HAL_OK;
}

/*
 * Decode a private key from a PKCS #8 EncryptedPrivateKeyInfo (RFC 5208).
 */

hal_error_t hal_asn1_decode_pkcs8_encryptedprivatekeyinfo(const uint8_t **alg_oid,  size_t *alg_oid_len,
                                                          const uint8_t **data,     size_t *data_len,
                                                          const uint8_t *const der, const size_t der_len)
{
  if (alg_oid == NULL || alg_oid_len == NULL || data == NULL || data_len == NULL || der == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  const uint8_t * const der_end = der + der_len;
  const uint8_t *d = der;

  size_t hlen, vlen;
  hal_error_t err;

  if ((err = hal_asn1_decode_header(ASN1_SEQUENCE, d, der_end - d, &hlen, &vlen)) != HAL_OK)
    return err;
  d += hlen;

  if (hlen + vlen != der_len)
    return HAL_ERROR_ASN1_PARSE_FAILED;

  if ((err = hal_asn1_decode_header(ASN1_SEQUENCE, d, der_end - d, &hlen, &vlen)) != HAL_OK)
    return err;
  d += hlen;

  const uint8_t * const algid_end = d + vlen;

  if ((err = hal_asn1_decode_header(ASN1_OBJECT_IDENTIFIER, d, algid_end - d, &hlen, &vlen)) != HAL_OK)
    return err;
  d += hlen;
  if (vlen > (size_t)(algid_end - d))
    return HAL_ERROR_ASN1_PARSE_FAILED;
  if (alg_oid != NULL)
    *alg_oid = d;
  if (alg_oid_len != NULL)
    *alg_oid_len = vlen;
  d += vlen;

  if (d < algid_end) {
    if ((err = hal_asn1_decode_header(ASN1_NULL, d, algid_end - d, &hlen, &vlen)) != HAL_OK)
      return err;
    d += hlen;
    if (vlen != 0)
      return HAL_ERROR_ASN1_PARSE_FAILED;
  }

  if (d != algid_end)
    return HAL_ERROR_ASN1_PARSE_FAILED;

  if ((err = hal_asn1_decode_header(ASN1_OCTET_STRING, d, der_end - d, &hlen, &vlen)) != HAL_OK)
    return err;
  d += hlen;
  if (vlen >= (size_t)(algid_end - d))
    return HAL_ERROR_ASN1_PARSE_FAILED;
  if (data != NULL)
    *data = d;
  if (data_len != NULL)
    *data_len = vlen;
  d += vlen;

  if (d != der_end)
    return HAL_ERROR_ASN1_PARSE_FAILED;

  return HAL_OK;
}

/*
 * Attempt to guess what kind of key we're looking at.
 */

hal_error_t hal_asn1_guess_key_type(hal_key_type_t *type,
                                    hal_curve_name_t *curve,
                                    const uint8_t *const der,  const size_t der_len)
{
  if (type == NULL || curve == NULL || der == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  const uint8_t *alg_oid, *curve_oid;
  size_t alg_oid_len, curve_oid_len;
  hal_error_t err;
  int public = 0;

  err = hal_asn1_decode_pkcs8_privatekeyinfo(&alg_oid, &alg_oid_len, &curve_oid, &curve_oid_len, NULL, 0, der, der_len);

  if (err == HAL_ERROR_ASN1_PARSE_FAILED &&
      (err = hal_asn1_decode_spki(&alg_oid, &alg_oid_len, &curve_oid, &curve_oid_len, NULL, 0, der, der_len)) == HAL_OK)
    public = 1;

  if (err != HAL_OK)
    return err;

  if (alg_oid_len == hal_asn1_oid_rsaEncryption_len && memcmp(alg_oid, hal_asn1_oid_rsaEncryption, alg_oid_len) == 0) {
    *type = public ? HAL_KEY_TYPE_RSA_PUBLIC : HAL_KEY_TYPE_RSA_PRIVATE;
    *curve = HAL_CURVE_NONE;
    return HAL_OK;
  }

  if (alg_oid_len == hal_asn1_oid_ecPublicKey_len && memcmp(alg_oid, hal_asn1_oid_ecPublicKey, alg_oid_len) == 0) {
    *type = public ? HAL_KEY_TYPE_EC_PUBLIC : HAL_KEY_TYPE_EC_PRIVATE;
    if ((err = hal_ecdsa_oid_to_curve(curve, curve_oid, curve_oid_len)) != HAL_OK)
      *curve = HAL_CURVE_NONE;
    return err;
  }

  if (alg_oid_len == hal_asn1_oid_mts_hashsig_len && memcmp(alg_oid, hal_asn1_oid_mts_hashsig, alg_oid_len) == 0) {
    *type = public ? HAL_KEY_TYPE_HASHSIG_PUBLIC : HAL_KEY_TYPE_HASHSIG_PRIVATE;
    *curve = HAL_CURVE_NONE;
    return HAL_OK;
  }

  *type = HAL_KEY_TYPE_NONE;
  *curve = HAL_CURVE_NONE;
  return HAL_ERROR_UNSUPPORTED_KEY;
}

/*
 * Local variables:
 * indent-tabs-mode: nil
 * End:
 */
