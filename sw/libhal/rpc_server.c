/*
 * rpc_server.c
 * ------------
 * Remote procedure call server-side private API implementation.
 *
 * Copyright (c) 2016-2018, NORDUnet A/S All rights reserved.
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

#include "hal.h"
#include "hal_internal.h"
#include "xdr_internal.h"
#include "hashsig.h"

/*
 * RPC calls.
 */

#define check(op) do { hal_error_t _err = (op); if (_err != HAL_OK) return _err; } while (0)

#define pad(n) (((n) + 3) & ~3)

#define nargs(n) ((n) * 4)

static hal_error_t get_version(const uint8_t **iptr, const uint8_t * const ilimit,
                               uint8_t **optr, const uint8_t * const olimit)
{
    uint32_t version;

    check(hal_rpc_get_version(&version));

    return hal_xdr_encode_int(optr, olimit, version);
}

static hal_error_t get_random(const uint8_t **iptr, const uint8_t * const ilimit,
                              uint8_t **optr, const uint8_t * const olimit)
{
    uint32_t length;
    hal_error_t err;

    /* skip over unused client argument */
    *iptr += nargs(1);

    check(hal_xdr_decode_int(iptr, ilimit, &length));
    /* sanity check length */
    if (length == 0 || length > (uint32_t)(olimit - *optr - nargs(1)))
        return HAL_ERROR_RPC_PACKET_OVERFLOW;

    /* get the data directly into the output buffer */
    if ((err = hal_rpc_get_random(*optr + nargs(1), (size_t)length)) == HAL_OK) {
        check(hal_xdr_encode_int(optr, olimit, length));
        *optr += pad(length);
    }

    return err;
}

static hal_error_t set_pin(const uint8_t **iptr, const uint8_t * const ilimit,
                           uint8_t **optr, const uint8_t * const olimit)
{
    hal_client_handle_t client;
    uint32_t user;
    const uint8_t *pin;
    size_t pin_len;

    check(hal_xdr_decode_int(iptr, ilimit, &client.handle));
    check(hal_xdr_decode_int(iptr, ilimit, &user));
    check(hal_xdr_decode_variable_opaque_ptr(iptr, ilimit, &pin, &pin_len));

    return hal_rpc_set_pin(client, user, (const char * const)pin, pin_len);
}

static hal_error_t login(const uint8_t **iptr, const uint8_t * const ilimit,
                         uint8_t **optr, const uint8_t * const olimit)
{
    hal_client_handle_t client;
    uint32_t user;
    const uint8_t *pin;
    size_t pin_len;

    check(hal_xdr_decode_int(iptr, ilimit, &client.handle));
    check(hal_xdr_decode_int(iptr, ilimit, &user));
    check(hal_xdr_decode_variable_opaque_ptr(iptr, ilimit, &pin, &pin_len));

    return hal_rpc_login(client, user, (const char * const)pin, pin_len);
}

static hal_error_t logout(const uint8_t **iptr, const uint8_t * const ilimit,
                          uint8_t **optr, const uint8_t * const olimit)
{
    hal_client_handle_t client;

    check(hal_xdr_decode_int(iptr, ilimit, &client.handle));

    return hal_rpc_logout(client);
}

static hal_error_t logout_all(const uint8_t **iptr, const uint8_t * const ilimit,
                              uint8_t **optr, const uint8_t * const olimit)
{
    return hal_rpc_logout_all();
}

static hal_error_t is_logged_in(const uint8_t **iptr, const uint8_t * const ilimit,
                                uint8_t **optr, const uint8_t * const olimit)
{
    hal_client_handle_t client;
    uint32_t user;

    check(hal_xdr_decode_int(iptr, ilimit, &client.handle));
    check(hal_xdr_decode_int(iptr, ilimit, &user));

    return hal_rpc_is_logged_in(client, user);
}

static hal_error_t hash_get_digest_len(const uint8_t **iptr, const uint8_t * const ilimit,
                                       uint8_t **optr, const uint8_t * const olimit)
{
    uint32_t alg;
    size_t length;

    /* skip over unused client argument */
    *iptr += nargs(1);

    check(hal_xdr_decode_int(iptr, ilimit, &alg));

    check(hal_rpc_hash_get_digest_length(alg, &length));

    return hal_xdr_encode_int(optr, olimit, length);
}

static hal_error_t hash_get_digest_algorithm_id(const uint8_t **iptr, const uint8_t * const ilimit,
                                                uint8_t **optr, const uint8_t * const olimit)
{
    uint32_t alg;
    size_t len;
    uint32_t len_max;
    hal_error_t err;

    /* skip over unused client argument */
    *iptr += nargs(1);

    check(hal_xdr_decode_int(iptr, ilimit, &alg));
    check(hal_xdr_decode_int(iptr, ilimit, &len_max));
    /* sanity check len_max */
    if (len_max > (uint32_t)(olimit - *optr - nargs(1)))
        return HAL_ERROR_RPC_PACKET_OVERFLOW;

    /* get the data directly into the output buffer */
    if ((err = hal_rpc_hash_get_digest_algorithm_id(alg, *optr + nargs(1), &len, (size_t)len_max)) == HAL_OK) {
        check(hal_xdr_encode_int(optr, olimit, len));
        *optr += pad(len);
    }

    return err;
}

static hal_error_t hash_get_algorithm(const uint8_t **iptr, const uint8_t * const ilimit,
                                      uint8_t **optr, const uint8_t * const olimit)
{
    hal_hash_handle_t hash;
    hal_digest_algorithm_t alg;

    /* skip over unused client argument */
    *iptr += nargs(1);

    check(hal_xdr_decode_int(iptr, ilimit, &hash.handle));

    check(hal_rpc_hash_get_algorithm(hash, &alg));

    return hal_xdr_encode_int(optr, olimit, alg);
}

static hal_error_t hash_initialize(const uint8_t **iptr, const uint8_t * const ilimit,
                                   uint8_t **optr, const uint8_t * const olimit)
{
    hal_client_handle_t client;
    hal_session_handle_t session;
    hal_hash_handle_t hash;
    uint32_t alg;
    const uint8_t *key;
    size_t key_len;

    check(hal_xdr_decode_int(iptr, ilimit, &client.handle));
    check(hal_xdr_decode_int(iptr, ilimit, &session.handle));
    check(hal_xdr_decode_int(iptr, ilimit, &alg));
    check(hal_xdr_decode_variable_opaque_ptr(iptr, ilimit, &key, &key_len));

    check(hal_rpc_hash_initialize(client, session, &hash, (hal_digest_algorithm_t)alg, key, (size_t)key_len));

    return hal_xdr_encode_int(optr, olimit, hash.handle);
}

static hal_error_t hash_update(const uint8_t **iptr, const uint8_t * const ilimit,
                               uint8_t **optr, const uint8_t * const olimit)
{
    hal_hash_handle_t hash;
    const uint8_t *data;
    size_t length;

    /* skip over unused client argument */
    *iptr += nargs(1);

    check(hal_xdr_decode_int(iptr, ilimit, &hash.handle));
    check(hal_xdr_decode_variable_opaque_ptr(iptr, ilimit, &data, &length));

    return hal_rpc_hash_update(hash, data, (size_t)length);
}

static hal_error_t hash_finalize(const uint8_t **iptr, const uint8_t * const ilimit,
                                 uint8_t **optr, const uint8_t * const olimit)
{
    hal_hash_handle_t hash;
    uint32_t length;
    hal_error_t err;

    /* skip over unused client argument */
    *iptr += nargs(1);

    check(hal_xdr_decode_int(iptr, ilimit, &hash.handle));
    check(hal_xdr_decode_int(iptr, ilimit, &length));
    /* sanity check length */
    if (length > (uint32_t)(olimit - *optr - nargs(1)))
        return HAL_ERROR_RPC_PACKET_OVERFLOW;

    /* get the data directly into the output buffer */
    if ((err = hal_rpc_hash_finalize(hash, *optr + nargs(1), (size_t)length)) == HAL_OK) {
        check(hal_xdr_encode_int(optr, olimit, length));
        *optr += pad(length);
    }

    return err;
}

static hal_error_t pkey_load(const uint8_t **iptr, const uint8_t * const ilimit,
                             uint8_t **optr, const uint8_t * const olimit)
{
    hal_client_handle_t client;
    hal_session_handle_t session;
    hal_pkey_handle_t pkey;
    hal_uuid_t name;
    const uint8_t *der;
    size_t der_len;
    hal_key_flags_t flags;
    uint8_t *optr_orig = *optr;
    hal_error_t err;

    check(hal_xdr_decode_int(iptr, ilimit, &client.handle));
    check(hal_xdr_decode_int(iptr, ilimit, &session.handle));
    check(hal_xdr_decode_variable_opaque_ptr(iptr, ilimit, &der, &der_len));
    check(hal_xdr_decode_int(iptr, ilimit, &flags));

    check(hal_rpc_pkey_load(client, session, &pkey, &name, der, der_len, flags));

    if ((err = hal_xdr_encode_int(optr, olimit, pkey.handle)) != HAL_OK ||
        (err = hal_xdr_encode_variable_opaque(optr, olimit, name.uuid, sizeof(name.uuid))) != HAL_OK)
        *optr = optr_orig;

    return err;
}

static hal_error_t pkey_open(const uint8_t **iptr, const uint8_t * const ilimit,
                             uint8_t **optr, const uint8_t * const olimit)
{
    hal_client_handle_t client;
    hal_session_handle_t session;
    hal_pkey_handle_t pkey;
    const uint8_t *name_ptr;
    size_t name_len;

    check(hal_xdr_decode_int(iptr, ilimit, &client.handle));
    check(hal_xdr_decode_int(iptr, ilimit, &session.handle));
    check(hal_xdr_decode_variable_opaque_ptr(iptr, ilimit, &name_ptr, &name_len));

    if (name_len != sizeof(hal_uuid_t))
        return HAL_ERROR_KEY_NAME_TOO_LONG;

    check(hal_rpc_pkey_open(client, session, &pkey, (const hal_uuid_t *) name_ptr));

    return hal_xdr_encode_int(optr, olimit, pkey.handle);
}

static hal_error_t pkey_generate_rsa(const uint8_t **iptr, const uint8_t * const ilimit,
                                     uint8_t **optr, const uint8_t * const olimit)
{
    hal_client_handle_t client;
    hal_session_handle_t session;
    hal_pkey_handle_t pkey;
    hal_uuid_t name;
    uint32_t key_len;
    const uint8_t *exp;
    size_t exp_len;
    hal_key_flags_t flags;
    uint8_t *optr_orig = *optr;
    hal_error_t err;

    check(hal_xdr_decode_int(iptr, ilimit, &client.handle));
    check(hal_xdr_decode_int(iptr, ilimit, &session.handle));
    check(hal_xdr_decode_int(iptr, ilimit, &key_len));
    check(hal_xdr_decode_variable_opaque_ptr(iptr, ilimit, &exp, &exp_len));
    check(hal_xdr_decode_int(iptr, ilimit, &flags));

    check(hal_rpc_pkey_generate_rsa(client, session, &pkey, &name, key_len, exp, exp_len, flags));

    if ((err = hal_xdr_encode_int(optr, olimit, pkey.handle)) != HAL_OK ||
        (err = hal_xdr_encode_variable_opaque(optr, olimit, name.uuid, sizeof(name.uuid))) != HAL_OK)
        *optr = optr_orig;

    return err;
}

static hal_error_t pkey_generate_ec(const uint8_t **iptr, const uint8_t * const ilimit,
                                    uint8_t **optr, const uint8_t * const olimit)
{
    hal_client_handle_t client;
    hal_session_handle_t session;
    hal_pkey_handle_t pkey;
    hal_uuid_t name;
    uint32_t curve;
    hal_key_flags_t flags;
    uint8_t *optr_orig = *optr;
    hal_error_t err;

    check(hal_xdr_decode_int(iptr, ilimit, &client.handle));
    check(hal_xdr_decode_int(iptr, ilimit, &session.handle));
    check(hal_xdr_decode_int(iptr, ilimit, &curve));
    check(hal_xdr_decode_int(iptr, ilimit, &flags));

    check(hal_rpc_pkey_generate_ec(client, session, &pkey, &name, curve, flags));

    if ((err = hal_xdr_encode_int(optr, olimit, pkey.handle)) != HAL_OK ||
        (err = hal_xdr_encode_variable_opaque(optr, olimit, name.uuid, sizeof(name.uuid))) != HAL_OK)
        *optr = optr_orig;

    return err;
}

static hal_error_t pkey_generate_hashsig(const uint8_t **iptr, const uint8_t * const ilimit,
                                         uint8_t **optr, const uint8_t * const olimit)
{
    hal_client_handle_t client;
    hal_session_handle_t session;
    hal_pkey_handle_t pkey;
    hal_uuid_t name;
    uint32_t hss_levels;
    uint32_t lms_type;
    uint32_t lmots_type;
    hal_key_flags_t flags;
    uint8_t *optr_orig = *optr;
    hal_error_t err;

    check(hal_xdr_decode_int(iptr, ilimit, &client.handle));
    check(hal_xdr_decode_int(iptr, ilimit, &session.handle));
    check(hal_xdr_decode_int(iptr, ilimit, &hss_levels));
    check(hal_xdr_decode_int(iptr, ilimit, &lms_type));
    check(hal_xdr_decode_int(iptr, ilimit, &lmots_type));
    check(hal_xdr_decode_int(iptr, ilimit, &flags));

    check(hal_rpc_pkey_generate_hashsig(client, session, &pkey, &name, hss_levels, lms_type, lmots_type, flags));

    if ((err = hal_xdr_encode_int(optr, olimit, pkey.handle)) != HAL_OK ||
        (err = hal_xdr_encode_variable_opaque(optr, olimit, name.uuid, sizeof(name.uuid))) != HAL_OK)
        *optr = optr_orig;

    return err;
}

static hal_error_t pkey_close(const uint8_t **iptr, const uint8_t * const ilimit,
                              uint8_t **optr, const uint8_t * const olimit)
{
    hal_pkey_handle_t pkey;

    /* skip over unused client argument */
    *iptr += nargs(1);

    check(hal_xdr_decode_int(iptr, ilimit, &pkey.handle));

    return hal_rpc_pkey_close(pkey);
}

static hal_error_t pkey_delete(const uint8_t **iptr, const uint8_t * const ilimit,
                               uint8_t **optr, const uint8_t * const olimit)
{
    hal_pkey_handle_t pkey;

    /* skip over unused client argument */
    *iptr += nargs(1);

    check(hal_xdr_decode_int(iptr, ilimit, &pkey.handle));

    return hal_rpc_pkey_delete(pkey);
}

static hal_error_t pkey_get_key_type(const uint8_t **iptr, const uint8_t * const ilimit,
                                     uint8_t **optr, const uint8_t * const olimit)
{
    hal_pkey_handle_t pkey;
    hal_key_type_t type;

    /* skip over unused client argument */
    *iptr += nargs(1);

    check(hal_xdr_decode_int(iptr, ilimit, &pkey.handle));

    check(hal_rpc_pkey_get_key_type(pkey, &type));

    return hal_xdr_encode_int(optr, olimit, type);
}

static hal_error_t pkey_get_key_curve(const uint8_t **iptr, const uint8_t * const ilimit,
                                     uint8_t **optr, const uint8_t * const olimit)
{
    hal_client_handle_t client;
    hal_pkey_handle_t pkey;
    hal_curve_name_t curve;

    check(hal_xdr_decode_int(iptr, ilimit, &client.handle));
    check(hal_xdr_decode_int(iptr, ilimit, &pkey.handle));

    check(hal_rpc_pkey_get_key_curve(pkey, &curve));

    return hal_xdr_encode_int(optr, olimit, curve);
}

static hal_error_t pkey_get_key_flags(const uint8_t **iptr, const uint8_t * const ilimit,
                                      uint8_t **optr, const uint8_t * const olimit)
{
    hal_pkey_handle_t pkey;
    hal_key_flags_t flags;

    /* skip over unused client argument */
    *iptr += nargs(1);

    check(hal_xdr_decode_int(iptr, ilimit, &pkey.handle));

    check(hal_rpc_pkey_get_key_flags(pkey, &flags));

    return hal_xdr_encode_int(optr, olimit, flags);
}

static hal_error_t pkey_get_public_key_len(const uint8_t **iptr, const uint8_t * const ilimit,
                                           uint8_t **optr, const uint8_t * const olimit)
{
    hal_pkey_handle_t pkey;
    size_t len;

    /* skip over unused client argument */
    *iptr += nargs(1);

    check(hal_xdr_decode_int(iptr, ilimit, &pkey.handle));

    len = hal_rpc_pkey_get_public_key_len(pkey);

    return hal_xdr_encode_int(optr, olimit, len);
}

static hal_error_t pkey_get_public_key(const uint8_t **iptr, const uint8_t * const ilimit,
                                       uint8_t **optr, const uint8_t * const olimit)
{
    hal_pkey_handle_t pkey;
    size_t len;
    uint32_t len_max;
    hal_error_t err;

    /* skip over unused client argument */
    *iptr += nargs(1);

    check(hal_xdr_decode_int(iptr, ilimit, &pkey.handle));
    check(hal_xdr_decode_int(iptr, ilimit, &len_max));
    /* sanity check len_max */
    if (len_max > (uint32_t)(olimit - *optr - nargs(1)))
        return HAL_ERROR_RPC_PACKET_OVERFLOW;

    /* get the data directly into the output buffer */
    if ((err = hal_rpc_pkey_get_public_key(pkey, *optr + nargs(1), &len, len_max)) == HAL_OK) {
        check(hal_xdr_encode_int(optr, olimit, len));
        *optr += pad(len);
    }

    return err;
}

static hal_error_t pkey_sign(const uint8_t **iptr, const uint8_t * const ilimit,
                             uint8_t **optr, const uint8_t * const olimit)
{
    hal_client_handle_t client;
    hal_pkey_handle_t pkey;
    hal_hash_handle_t hash;
    const uint8_t *input;
    size_t input_len;
    uint32_t sig_max;
    size_t sig_len;
    hal_error_t err;

    check(hal_xdr_decode_int(iptr, ilimit, &client.handle));
    check(hal_xdr_decode_int(iptr, ilimit, &pkey.handle));
    check(hal_xdr_decode_int(iptr, ilimit, &hash.handle));
    check(hal_xdr_decode_variable_opaque_ptr(iptr, ilimit, &input, &input_len));
    check(hal_xdr_decode_int(iptr, ilimit, &sig_max));
    /* sanity check sig_max */
    if (sig_max > (uint32_t)(olimit - *optr - nargs(1)))
        return HAL_ERROR_RPC_PACKET_OVERFLOW;

    /* get the data directly into the output buffer */
    err = hal_rpc_pkey_sign(pkey, hash, input, input_len, *optr + nargs(1), &sig_len, sig_max);
    if (err == HAL_OK) {
        check(hal_xdr_encode_int(optr, olimit, sig_len));
        *optr += pad(sig_len);
    }

    return err;
}

static hal_error_t pkey_verify(const uint8_t **iptr, const uint8_t * const ilimit,
                               uint8_t **optr, const uint8_t * const olimit)
{
    hal_client_handle_t client;
    hal_pkey_handle_t pkey;
    hal_hash_handle_t hash;
    const uint8_t *input;
    size_t input_len;
    const uint8_t *sig;
    size_t sig_len;

    check(hal_xdr_decode_int(iptr, ilimit, &client.handle));
    check(hal_xdr_decode_int(iptr, ilimit, &pkey.handle));
    check(hal_xdr_decode_int(iptr, ilimit, &hash.handle));
    check(hal_xdr_decode_variable_opaque_ptr(iptr, ilimit, &input, &input_len));
    check(hal_xdr_decode_variable_opaque_ptr(iptr, ilimit, &sig, &sig_len));

    return hal_rpc_pkey_verify(pkey, hash, input, input_len, sig, sig_len);
}

static hal_error_t pkey_match(const uint8_t **iptr, const uint8_t * const ilimit,
                              uint8_t **optr, const uint8_t * const olimit)
{
    hal_client_handle_t client;
    hal_session_handle_t session;
    uint32_t type, curve, attributes_len, state, result_max;
    size_t previous_uuid_len;
    const uint8_t *previous_uuid_ptr;
    hal_key_flags_t mask, flags;
    uint8_t *optr_orig = *optr;
    hal_error_t err;

    check(hal_xdr_decode_int(iptr, ilimit, &client.handle));
    check(hal_xdr_decode_int(iptr, ilimit, &session.handle));
    check(hal_xdr_decode_int(iptr, ilimit, &type));
    check(hal_xdr_decode_int(iptr, ilimit, &curve));
    check(hal_xdr_decode_int(iptr, ilimit, &mask));
    check(hal_xdr_decode_int(iptr, ilimit, &flags));
    check(hal_xdr_decode_int(iptr, ilimit, &attributes_len));

    hal_pkey_attribute_t attributes[attributes_len > 0 ? attributes_len : 1];

    for (size_t i = 0; i < attributes_len; i++) {
        hal_pkey_attribute_t *a = &attributes[i];
        const uint8_t *value;
        size_t value_len;
        check(hal_xdr_decode_int(iptr, ilimit, &a->type));
        check(hal_xdr_decode_variable_opaque_ptr(iptr, ilimit, &value, &value_len));
        a->value  = value;
        a->length = value_len;
    }

    check(hal_xdr_decode_int(iptr, ilimit, &state));
    check(hal_xdr_decode_int(iptr, ilimit, &result_max));
    check(hal_xdr_decode_variable_opaque_ptr(iptr, ilimit, &previous_uuid_ptr, &previous_uuid_len));

    if (previous_uuid_len != sizeof(hal_uuid_t))
        return HAL_ERROR_KEY_NAME_TOO_LONG;

    const hal_uuid_t * const previous_uuid = (const void *) previous_uuid_ptr;

    hal_uuid_t result[result_max];
    unsigned result_len, ustate = state;

    check(hal_rpc_pkey_match(client, session, type, curve, mask, flags,
                             attributes, attributes_len, &ustate, result,
                             &result_len, result_max, previous_uuid));

    if ((err = hal_xdr_encode_int(optr, olimit, ustate)) == HAL_OK)
        err = hal_xdr_encode_int(optr, olimit, result_len);

    for (size_t i = 0; err == HAL_OK && i < result_len; ++i)
        err = hal_xdr_encode_variable_opaque(optr, olimit, result[i].uuid,
                                             sizeof(result[i].uuid));

    if (err != HAL_OK)
        *optr = optr_orig;

    return err;
}

static hal_error_t pkey_set_attributes(const uint8_t **iptr, const uint8_t * const ilimit,
                                       uint8_t **optr, const uint8_t * const olimit)
{
    hal_client_handle_t client;
    hal_pkey_handle_t pkey;
    uint32_t attributes_len;

    check(hal_xdr_decode_int(iptr, ilimit, &client.handle));
    check(hal_xdr_decode_int(iptr, ilimit, &pkey.handle));
    check(hal_xdr_decode_int(iptr, ilimit, &attributes_len));

    hal_pkey_attribute_t attributes[attributes_len > 0 ? attributes_len : 1];

    for (size_t i = 0; i < attributes_len; i++) {
        hal_pkey_attribute_t *a = &attributes[i];
        check(hal_xdr_decode_int(iptr, ilimit, &a->type));
        const uint8_t *iptr_prior_to_decoding_length = *iptr;
        check(hal_xdr_decode_int(iptr, ilimit, &a->length));
        if (a->length == HAL_PKEY_ATTRIBUTE_NIL) {
            a->value = NULL;
        }
        else {
            *iptr = iptr_prior_to_decoding_length;
            const uint8_t *value;
            size_t len;
            check(hal_xdr_decode_variable_opaque_ptr(iptr, ilimit, &value, &len));
            a->value = value;
            a->length = len;
        }
    }

    return hal_rpc_pkey_set_attributes(pkey, attributes, attributes_len);
}

static hal_error_t pkey_get_attributes(const uint8_t **iptr, const uint8_t * const ilimit,
                                       uint8_t **optr, const uint8_t * const olimit)
{
    hal_client_handle_t client;
    hal_pkey_handle_t pkey;
    uint32_t attributes_len, u32;
    uint8_t *optr_orig = *optr;
    hal_error_t err;

    check(hal_xdr_decode_int(iptr, ilimit, &client.handle));
    check(hal_xdr_decode_int(iptr, ilimit, &pkey.handle));
    check(hal_xdr_decode_int(iptr, ilimit, &attributes_len));

    hal_pkey_attribute_t attributes[attributes_len > 0 ? attributes_len : 1];

    for (size_t i = 0; i < attributes_len; i++)
        check(hal_xdr_decode_int(iptr, ilimit, &attributes[i].type));

    check(hal_xdr_decode_int(iptr, ilimit, &u32));

    const size_t attributes_buffer_len = u32;

    if (nargs(1 + 2 * attributes_len) + attributes_buffer_len > (uint32_t)(olimit - *optr))
        return HAL_ERROR_RPC_PACKET_OVERFLOW;

    uint8_t attributes_buffer[attributes_buffer_len > 0 ? attributes_buffer_len : 1];

    check(hal_rpc_pkey_get_attributes(pkey, attributes, attributes_len,
                                      attributes_buffer, attributes_buffer_len));

    if ((err = hal_xdr_encode_int(optr, olimit, attributes_len)) == HAL_OK) {
        for (size_t i = 0; err == HAL_OK && i < attributes_len; i++) {
            if ((err = hal_xdr_encode_int(optr, olimit, attributes[i].type)) == HAL_OK) {
                if (attributes_buffer_len == 0)
                    err = hal_xdr_encode_int(optr, olimit, attributes[i].length);
                else
                    err = hal_xdr_encode_variable_opaque(optr, olimit, attributes[i].value, attributes[i].length);
            }
        }
    }

    if (err != HAL_OK)
        *optr = optr_orig;

    return err;
}

static hal_error_t pkey_export(const uint8_t **iptr, const uint8_t * const ilimit,
                               uint8_t **optr, const uint8_t * const olimit)
{
    hal_client_handle_t client;
    hal_pkey_handle_t pkey;
    hal_pkey_handle_t kekek;
    size_t   pkcs8_len, kek_len;
    uint32_t pkcs8_max, kek_max;
    uint8_t *optr_orig = *optr;
    hal_error_t err;

    check(hal_xdr_decode_int(iptr, ilimit, &client.handle));
    check(hal_xdr_decode_int(iptr, ilimit, &pkey.handle));
    check(hal_xdr_decode_int(iptr, ilimit, &kekek.handle));
    check(hal_xdr_decode_int(iptr, ilimit, &pkcs8_max));
    check(hal_xdr_decode_int(iptr, ilimit, &kek_max));

    uint8_t pkcs8[pkcs8_max], kek[kek_max];

    check(hal_rpc_pkey_export(pkey, kekek, pkcs8, &pkcs8_len, sizeof(pkcs8), kek, &kek_len, sizeof(kek)));

    if ((err = hal_xdr_encode_variable_opaque(optr, olimit, pkcs8, pkcs8_len)) != HAL_OK ||
        (err = hal_xdr_encode_variable_opaque(optr, olimit, kek, kek_len)) != HAL_OK)
        *optr = optr_orig;

    return err;
}

static hal_error_t pkey_import(const uint8_t **iptr, const uint8_t * const ilimit,
                               uint8_t **optr, const uint8_t * const olimit)
{
    hal_client_handle_t client;
    hal_session_handle_t session;
    hal_pkey_handle_t pkey;
    hal_pkey_handle_t kekek;
    hal_uuid_t name;
    const uint8_t *pkcs8, *kek;
    size_t pkcs8_len, kek_len;
    uint8_t *optr_orig = *optr;
    hal_key_flags_t flags;
    hal_error_t err;

    check(hal_xdr_decode_int(iptr, ilimit, &client.handle));
    check(hal_xdr_decode_int(iptr, ilimit, &session.handle));
    check(hal_xdr_decode_int(iptr, ilimit, &kekek.handle));
    check(hal_xdr_decode_variable_opaque_ptr(iptr, ilimit, &pkcs8, &pkcs8_len));
    check(hal_xdr_decode_variable_opaque_ptr(iptr, ilimit, &kek, &kek_len));
    check(hal_xdr_decode_int(iptr, ilimit, &flags));

    check(hal_rpc_pkey_import(client, session, &pkey, &name, kekek, pkcs8, pkcs8_len, kek, kek_len, flags));

    if ((err = hal_xdr_encode_int(optr, olimit, pkey.handle)) != HAL_OK ||
        (err = hal_xdr_encode_variable_opaque(optr, olimit, name.uuid, sizeof(name.uuid))) != HAL_OK)
        *optr = optr_orig;

    return err;
}


hal_error_t hal_rpc_server_dispatch(const uint8_t * const ibuf, const size_t ilen,
                                    uint8_t * const obuf, size_t * const olen)
{
    const uint8_t * iptr = ibuf;
    const uint8_t * const ilimit = ibuf + ilen;
    uint8_t * optr = obuf + nargs(3);	/* reserve space for opcode, client, and response code */
    const uint8_t * const olimit = obuf + *olen;
    uint32_t rpc_func_num;
    uint32_t client_handle;
    hal_error_t err;
    hal_error_t (*handler)(const uint8_t **iptr, const uint8_t * const ilimit,
                           uint8_t **optr, const uint8_t * const olimit) = NULL;

    check(hal_xdr_decode_int(&iptr, ilimit, &rpc_func_num));
    check(hal_xdr_decode_int_peek(&iptr, ilimit, &client_handle));

    switch (rpc_func_num) {
    case RPC_FUNC_GET_VERSION:
        handler = get_version;
        break;
    case RPC_FUNC_GET_RANDOM:
        handler = get_random;
        break;
    case RPC_FUNC_SET_PIN:
        handler = set_pin;
        break;
    case RPC_FUNC_LOGIN:
        handler = login;
        break;
    case RPC_FUNC_LOGOUT:
        handler = logout;
        break;
    case RPC_FUNC_LOGOUT_ALL:
        handler = logout_all;
        break;
    case RPC_FUNC_IS_LOGGED_IN:
        handler = is_logged_in;
        break;
    case RPC_FUNC_HASH_GET_DIGEST_LEN:
        handler = hash_get_digest_len;
        break;
    case RPC_FUNC_HASH_GET_DIGEST_ALGORITHM_ID:
        handler = hash_get_digest_algorithm_id;
        break;
    case RPC_FUNC_HASH_GET_ALGORITHM:
        handler = hash_get_algorithm;
        break;
    case RPC_FUNC_HASH_INITIALIZE:
        handler = hash_initialize;
        break;
    case RPC_FUNC_HASH_UPDATE:
        handler = hash_update;
        break;
    case RPC_FUNC_HASH_FINALIZE:
        handler = hash_finalize;
        break;
    case RPC_FUNC_PKEY_LOAD:
        handler = pkey_load;
        break;
    case RPC_FUNC_PKEY_OPEN:
        handler = pkey_open;
        break;
    case RPC_FUNC_PKEY_GENERATE_RSA:
        handler = pkey_generate_rsa;
        break;
    case RPC_FUNC_PKEY_GENERATE_EC:
        handler = pkey_generate_ec;
        break;
    case RPC_FUNC_PKEY_GENERATE_HASHSIG:
        handler = pkey_generate_hashsig;
        break;
    case RPC_FUNC_PKEY_CLOSE:
        handler = pkey_close;
        break;
    case RPC_FUNC_PKEY_DELETE:
        handler = pkey_delete;
        break;
    case RPC_FUNC_PKEY_GET_KEY_TYPE:
        handler = pkey_get_key_type;
        break;
    case RPC_FUNC_PKEY_GET_KEY_CURVE:
        handler = pkey_get_key_curve;
        break;
    case RPC_FUNC_PKEY_GET_KEY_FLAGS:
        handler = pkey_get_key_flags;
        break;
    case RPC_FUNC_PKEY_GET_PUBLIC_KEY_LEN:
        handler = pkey_get_public_key_len;
        break;
    case RPC_FUNC_PKEY_GET_PUBLIC_KEY:
        handler = pkey_get_public_key;
        break;
    case RPC_FUNC_PKEY_SIGN:
        handler = pkey_sign;
        break;
    case RPC_FUNC_PKEY_VERIFY:
        handler = pkey_verify;
        break;
    case RPC_FUNC_PKEY_MATCH:
        handler = pkey_match;
        break;
    case RPC_FUNC_PKEY_SET_ATTRIBUTES:
        handler = pkey_set_attributes;
        break;
    case RPC_FUNC_PKEY_GET_ATTRIBUTES:
        handler = pkey_get_attributes;
        break;
    case RPC_FUNC_PKEY_EXPORT:
        handler = pkey_export;
        break;
    case RPC_FUNC_PKEY_IMPORT:
        handler = pkey_import;
        break;
    }

    if (handler)
        err = handler(&iptr, ilimit, &optr, olimit);
    else
        err = HAL_ERROR_RPC_BAD_FUNCTION;

    /* Encode opcode, client ID, and response code at the beginning of the payload */
    *olen = optr - obuf;
    optr = obuf;
    check(hal_xdr_encode_int(&optr, olimit, rpc_func_num));
    check(hal_xdr_encode_int(&optr, olimit, client_handle));
    check(hal_xdr_encode_int(&optr, olimit, err));
    return HAL_OK;
}

/*
 * Dispatch vectors.
 */

#if RPC_CLIENT == RPC_CLIENT_LOCAL
const hal_rpc_misc_dispatch_t *hal_rpc_misc_dispatch = &hal_rpc_local_misc_dispatch;
const hal_rpc_hash_dispatch_t *hal_rpc_hash_dispatch = &hal_rpc_local_hash_dispatch;
const hal_rpc_pkey_dispatch_t *hal_rpc_pkey_dispatch = &hal_rpc_local_pkey_dispatch;
#endif

hal_error_t hal_rpc_server_init(void)
{
    hal_error_t err;

    if ((err = hal_ks_init(hal_ks_volatile, 1)) != HAL_OK ||
        (err = hal_ks_init(hal_ks_token, 1))    != HAL_OK ||
        (err = hal_rpc_server_transport_init()) != HAL_OK)
        return err;

    return HAL_OK;
}

hal_error_t hal_rpc_server_close(void)
{
    hal_error_t err;

    if ((err = hal_rpc_server_transport_close()) != HAL_OK)
        return err;

    return HAL_OK;
}


/*
 * Local variables:
 * indent-tabs-mode: nil
 * End:
 */
