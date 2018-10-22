/*
 * test-rpc_pkey.c
 * ---------------
 * Test code for RPC interface to Cryptech public key operations.
 *
 * Authors: Rob Austein, Paul Selkirk
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
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <hal.h>

#include "test-rsa.h"
#include "test-ecdsa.h"

#define lose(...) do { printf(__VA_ARGS__); goto fail; } while (0)

static inline const char *ecdsa_curve_to_string(const hal_curve_name_t curve)
{
  switch (curve) {
  case HAL_CURVE_P256:  return "P-256";
  case HAL_CURVE_P384:  return "P-384";
  case HAL_CURVE_P521:  return "P-521";
  default:              return "?????";
  }
}

static int test_attributes(const hal_pkey_handle_t pkey,
                           const hal_uuid_t * const name,
                           const hal_key_flags_t flags)
{
  static const size_t sizes[] = { 32, 100, 260, 1000, 2000, 0 };
  static const char format[] = "Test attribute %lu";

  hal_error_t err;

  for (const size_t *size = sizes; *size; size++) {
    uint8_t buf_1[*size], buf_2[*size];
    memset(buf_1, 0x55, sizeof(buf_1));
    snprintf((char *) buf_1, sizeof(buf_1), format, (unsigned long) *size);
    hal_pkey_attribute_t attr_set = { .type = *size, .length = sizeof(buf_1), .value = buf_1 };
    hal_pkey_attribute_t attr_get = { .type = *size };
    hal_pkey_attribute_t attr_del = { .type = *size, .length = HAL_PKEY_ATTRIBUTE_NIL };

    if ((err = hal_rpc_pkey_set_attributes(pkey, &attr_set, 1)) != HAL_OK)
      lose("Could not set attribute %lu: %s\n",
           (unsigned long) *size, hal_error_string(err));

    if ((err = hal_rpc_pkey_get_attributes(pkey, &attr_get, 1, buf_2, sizeof(buf_2))) != HAL_OK)
      lose("Could not get attribute %lu: %s\n",
           (unsigned long) *size, hal_error_string(err));

    if (attr_get.length != *size)
      lose("Unexpected size returned for attribute %lu: %lu\n",
           (unsigned long) *size, (unsigned long) attr_get.length);

    if ((err = hal_rpc_pkey_set_attributes(pkey, &attr_del, 1)) != HAL_OK)
      lose("Could not delete attribute %lu: %s\n",
           (unsigned long) *size, hal_error_string(err));

    if ((err = hal_rpc_pkey_set_attributes(pkey, &attr_set, 1)) != HAL_OK)
      lose("Could not (re)set attribute %lu: %s\n",
           (unsigned long) *size, hal_error_string(err));
  }

  {
    const hal_client_handle_t client = {HAL_HANDLE_NONE};
    const hal_session_handle_t session = {HAL_HANDLE_NONE};
    hal_uuid_t result[10], previous_uuid = {{0}};
    unsigned result_len, state;

    state = 0;
    if ((err = hal_rpc_pkey_match(client, session, HAL_KEY_TYPE_NONE, HAL_CURVE_NONE, 0, 0, NULL, 0,
                                  &state, result, &result_len, sizeof(result)/sizeof(*result),
                                  &previous_uuid)) != HAL_OK)
      lose("Unrestricted match() failed: %s\n", hal_error_string(err));

    if (result_len == 0)
      lose("Unrestricted match found no results\n");

    state = 0;
    for (const size_t *size = sizes; *size; size++) {
      uint8_t buf[*size];
      memset(buf, 0x55, sizeof(buf));
      snprintf((char *) buf, sizeof(buf), format, (unsigned long) *size);
      hal_pkey_attribute_t attribute[1] = {{ *size, sizeof(buf), buf }};

      if ((err = hal_rpc_pkey_match(client, session, HAL_KEY_TYPE_NONE, HAL_CURVE_NONE, 0, 0,
                                    attribute, sizeof(attribute)/sizeof(*attribute),
                                    &state, result, &result_len, sizeof(result)/sizeof(*result),
                                    &previous_uuid)) != HAL_OK)
        lose("Restricted match() for attribute %lu failed: %s\n",
             (unsigned long) *size, hal_error_string(err));

      if (result_len == 0)
        lose("Restricted match for attribute %lu found no results\n", (unsigned long) *size);
    }

#warning More hal_rpc_pkey_match() testing here.

  }

  return 1;

 fail:
  return 0;
}

static int test_rsa_testvec(const rsa_tc_t * const tc, hal_key_flags_t flags)
{
  const hal_client_handle_t client = {HAL_HANDLE_NONE};
  const hal_session_handle_t session = {HAL_HANDLE_NONE};
  hal_pkey_handle_t private_key = {HAL_HANDLE_NONE};
  hal_pkey_handle_t public_key = {HAL_HANDLE_NONE};
  hal_error_t err;
  size_t len;

  assert(tc != NULL);

  {
    flags |= HAL_KEY_FLAG_USAGE_DIGITALSIGNATURE;

    printf("Starting %lu-bit RSA test vector tests, flags 0x%lx\n",
           (unsigned long) tc->size, (unsigned long) flags);

    uint8_t tc_keybuf[hal_rsa_key_t_size];
    hal_rsa_key_t *tc_key = NULL;

    if ((err = hal_rsa_key_load_private(&tc_key,
                                        tc_keybuf, sizeof(tc_keybuf),
                                        tc->n.val,  tc->n.len,
                                        tc->e.val,  tc->e.len,
                                        tc->d.val,  tc->d.len,
                                        tc->p.val,  tc->p.len,
                                        tc->q.val,  tc->q.len,
                                        tc->u.val,  tc->u.len,
                                        tc->dP.val, tc->dP.len,
                                        tc->dQ.val, tc->dQ.len)) != HAL_OK)
      lose("Could not load RSA private key from test vector: %s\n", hal_error_string(err));

    hal_uuid_t private_name, public_name;

    uint8_t private_der[hal_rsa_private_key_to_der_len(tc_key)];
    uint8_t public_der[hal_rsa_public_key_to_der_len(tc_key)];

    if ((err = hal_rsa_private_key_to_der(tc_key, private_der, &len, sizeof(private_der))) != HAL_OK)
      lose("Could not DER encode private key from test vector: %s\n", hal_error_string(err));

    assert(len == sizeof(private_der));

    if ((err = hal_rpc_pkey_load(client, session, &private_key, &private_name,
                                 private_der, sizeof(private_der), flags)) != HAL_OK)
      lose("Could not load private key into RPC: %s\n", hal_error_string(err));

    if ((err = hal_rsa_public_key_to_der(tc_key, public_der, &len, sizeof(public_der))) != HAL_OK)
      lose("Could not DER encode public key from test vector: %s\n", hal_error_string(err));

    assert(len == sizeof(public_der));

    if ((err = hal_rpc_pkey_load(client, session, &public_key, &public_name,
                                 public_der, sizeof(public_der), flags)) != HAL_OK)
      lose("Could not load public key into RPC: %s\n", hal_error_string(err));

    uint8_t sig[tc->s.len];

    /*
     * Raw RSA test cases include PKCS #1.5 padding, we need to drill down to the DigestInfo.
     */
    assert(tc->m.len > 4 && tc->m.val[0] == 0x00 && tc->m.val[1] == 0x01 && tc->m.val[2] == 0xff);
    const uint8_t *digestinfo = memchr(tc->m.val + 2, 0x00, tc->m.len - 2);
    assert(digestinfo != NULL);
    const size_t digestinfo_len = tc->m.val + tc->m.len - ++digestinfo;

    if ((err = hal_rpc_pkey_sign(private_key, hal_hash_handle_none,
                                 digestinfo, digestinfo_len, sig, &len, sizeof(sig))) != HAL_OK)
      lose("Could not sign: %s\n", hal_error_string(err));

    if (tc->s.len != len || memcmp(sig, tc->s.val, tc->s.len) != 0)
      lose("MISMATCH\n");

    if ((err = hal_rpc_pkey_verify(public_key, hal_hash_handle_none,
                                   digestinfo, digestinfo_len, tc->s.val, tc->s.len)) != HAL_OK)
      lose("Could not verify: %s\n", hal_error_string(err));

    if (!test_attributes(private_key, &private_name, flags) || !test_attributes(public_key, &public_name, flags))
      goto fail;

    if ((err = hal_rpc_pkey_delete(private_key)) != HAL_OK)
      lose("Could not delete private key: %s\n", hal_error_string(err));

    if ((err = hal_rpc_pkey_delete(public_key)) != HAL_OK)
      lose("Could not delete public key: %s\n", hal_error_string(err));

    printf("OK\n");
    return 1;
  }

 fail:
  if (private_key.handle != HAL_HANDLE_NONE &&
      (err = hal_rpc_pkey_delete(private_key)) != HAL_OK)
    printf("Warning: could not delete private key: %s\n", hal_error_string(err));

  if (public_key.handle != HAL_HANDLE_NONE &&
      (err = hal_rpc_pkey_delete(public_key)) != HAL_OK)
    printf("Warning: could not delete public key: %s\n", hal_error_string(err));

  return 0;
}

static int test_ecdsa_testvec(const ecdsa_tc_t * const tc, hal_key_flags_t flags)
{
  const hal_client_handle_t client = {HAL_HANDLE_NONE};
  const hal_session_handle_t session = {HAL_HANDLE_NONE};
  hal_pkey_handle_t private_key = {HAL_HANDLE_NONE};
  hal_pkey_handle_t public_key = {HAL_HANDLE_NONE};
  hal_error_t err;
  size_t len;

  assert(tc != NULL);

  {
    flags |= HAL_KEY_FLAG_USAGE_DIGITALSIGNATURE;

    printf("Starting ECDSA %s test vector tests, flags 0x%lx\n",
           ecdsa_curve_to_string(tc->curve), (unsigned long) flags);

    uint8_t tc_keybuf[hal_ecdsa_key_t_size];
    hal_ecdsa_key_t *tc_key = NULL;

    if ((err = hal_ecdsa_key_load_private(&tc_key, tc_keybuf, sizeof(tc_keybuf), tc->curve,
                                          tc->Qx, tc->Qx_len, tc->Qy, tc->Qy_len,
                                          tc->d,  tc->d_len)) != HAL_OK)
      lose("Could not load ECDSA private key from test vector: %s\n", hal_error_string(err));

    hal_uuid_t private_name, public_name;

    uint8_t private_der[hal_ecdsa_private_key_to_der_len(tc_key)];
    uint8_t public_der[hal_ecdsa_public_key_to_der_len(tc_key)];

    if ((err = hal_ecdsa_private_key_to_der(tc_key, private_der, &len, sizeof(private_der))) != HAL_OK)
      lose("Could not DER encode private key from test vector: %s\n", hal_error_string(err));

    assert(len == sizeof(private_der));

    if ((err = hal_rpc_pkey_load(client, session, &private_key, &private_name,
                                 private_der, sizeof(private_der), flags)) != HAL_OK)
      lose("Could not load private key into RPC: %s\n", hal_error_string(err));

    if ((err = hal_ecdsa_public_key_to_der(tc_key, public_der, &len, sizeof(public_der))) != HAL_OK)
      lose("Could not DER encode public key from test vector: %s\n", hal_error_string(err));

    assert(len == sizeof(public_der));

    if ((err = hal_rpc_pkey_load(client, session, &public_key, &public_name,
                                 public_der, sizeof(public_der), flags)) != HAL_OK)
      lose("Could not load public key into RPC: %s\n", hal_error_string(err));

    if ((err = hal_rpc_pkey_verify(public_key, hal_hash_handle_none,
                                   tc->H, tc->H_len, tc->sig, tc->sig_len)) != HAL_OK)
      lose("Could not verify signature from test vector: %s\n", hal_error_string(err));

    uint8_t sig[tc->sig_len + 4];

    if ((err = hal_rpc_pkey_sign(private_key, hal_hash_handle_none,
                                 tc->H, tc->H_len, sig, &len, sizeof(sig))) != HAL_OK)
      lose("Could not sign: %s\n", hal_error_string(err));

    if ((err = hal_rpc_pkey_verify(public_key, hal_hash_handle_none,
                                   tc->H, tc->H_len, sig, len)) != HAL_OK)
      lose("Could not verify own signature: %s\n", hal_error_string(err));

    if (!test_attributes(private_key, &private_name, flags) || !test_attributes(public_key, &public_name, flags))
      goto fail;

    if ((err = hal_rpc_pkey_delete(private_key)) != HAL_OK)
      lose("Could not delete private key: %s\n", hal_error_string(err));

    if ((err = hal_rpc_pkey_delete(public_key)) != HAL_OK)
      lose("Could not delete public key: %s\n", hal_error_string(err));

    printf("OK\n");
    return 1;
  }

 fail:
  if (private_key.handle != HAL_HANDLE_NONE &&
      (err = hal_rpc_pkey_delete(private_key)) != HAL_OK)
    printf("Warning: could not delete private key: %s\n", hal_error_string(err));

  if (public_key.handle != HAL_HANDLE_NONE &&
      (err = hal_rpc_pkey_delete(public_key)) != HAL_OK)
    printf("Warning: could not delete public key: %s\n", hal_error_string(err));

  return 0;
}

static int test_rsa_generate(const rsa_tc_t * const tc, hal_key_flags_t flags)
{
  const hal_client_handle_t client = {HAL_HANDLE_NONE};
  const hal_session_handle_t session = {HAL_HANDLE_NONE};
  hal_pkey_handle_t private_key = {HAL_HANDLE_NONE};
  hal_pkey_handle_t public_key = {HAL_HANDLE_NONE};
  hal_error_t err;
  size_t len;

  assert(tc != NULL);

  {
    flags |= HAL_KEY_FLAG_USAGE_DIGITALSIGNATURE;

    printf("Starting %lu-bit RSA key generation tests, flags 0x%lx\n",
           (unsigned long) tc->size, (unsigned long) flags);

    hal_uuid_t private_name, public_name;

    if ((err = hal_rpc_pkey_generate_rsa(client, session, &private_key, &private_name,
                                         tc->size, tc->e.val, tc->e.len, flags)) != HAL_OK)
      lose("Could not generate RSA private key: %s\n", hal_error_string(err));

    uint8_t public_der[hal_rpc_pkey_get_public_key_len(private_key)];

    if ((err = hal_rpc_pkey_get_public_key(private_key, public_der, &len, sizeof(public_der))) != HAL_OK)
      lose("Could not DER encode RPC RSA public key from RPC RSA private key: %s\n", hal_error_string(err));

    assert(len == sizeof(public_der));

    if ((err = hal_rpc_pkey_load(client, session, &public_key, &public_name,
                                 public_der, sizeof(public_der), flags)) != HAL_OK)
      lose("Could not load public key into RPC: %s\n", hal_error_string(err));

    uint8_t sig[tc->s.len];

    /*
     * Raw RSA test cases include PKCS #1.5 padding, we need to drill down to the DigestInfo.
     */
    assert(tc->m.len > 4 && tc->m.val[0] == 0x00 && tc->m.val[1] == 0x01 && tc->m.val[2] == 0xff);
    const uint8_t *digestinfo = memchr(tc->m.val + 2, 0x00, tc->m.len - 2);
    assert(digestinfo != NULL);
    const size_t digestinfo_len = tc->m.val + tc->m.len - ++digestinfo;

    if ((err = hal_rpc_pkey_sign(private_key, hal_hash_handle_none,
                                 digestinfo, digestinfo_len, sig, &len, sizeof(sig))) != HAL_OK)
      lose("Could not sign: %s\n", hal_error_string(err));

    if ((err = hal_rpc_pkey_verify(public_key, hal_hash_handle_none,
                                   digestinfo, digestinfo_len, sig, len)) != HAL_OK)
      lose("Could not verify: %s\n", hal_error_string(err));

    if (!test_attributes(private_key, &private_name, flags) || !test_attributes(public_key, &public_name, flags))
      goto fail;

    if ((err = hal_rpc_pkey_delete(private_key)) != HAL_OK)
      lose("Could not delete private key: %s\n", hal_error_string(err));

    if ((err = hal_rpc_pkey_delete(public_key)) != HAL_OK)
      lose("Could not delete public key: %s\n", hal_error_string(err));

    printf("OK\n");
    return 1;
  }

 fail:
  if (private_key.handle != HAL_HANDLE_NONE &&
      (err = hal_rpc_pkey_delete(private_key)) != HAL_OK)
    printf("Warning: could not delete private key: %s\n", hal_error_string(err));

  if (public_key.handle != HAL_HANDLE_NONE &&
      (err = hal_rpc_pkey_delete(public_key)) != HAL_OK)
    printf("Warning: could not delete public key: %s\n", hal_error_string(err));

  return 0;
}

static int test_ecdsa_generate(const ecdsa_tc_t * const tc, hal_key_flags_t flags)
{
  const hal_client_handle_t client = {HAL_HANDLE_NONE};
  const hal_session_handle_t session = {HAL_HANDLE_NONE};
  hal_pkey_handle_t private_key = {HAL_HANDLE_NONE};
  hal_pkey_handle_t public_key = {HAL_HANDLE_NONE};
  hal_error_t err;
  size_t len;

  assert(tc != NULL);

  {
    flags |= HAL_KEY_FLAG_USAGE_DIGITALSIGNATURE;

    printf("Starting ECDSA %s key generation tests, flags 0x%lx\n",
           ecdsa_curve_to_string(tc->curve), (unsigned long) flags);

    hal_uuid_t private_name, public_name;

    if ((err = hal_rpc_pkey_generate_ec(client, session, &private_key, &private_name, tc->curve, flags)) != HAL_OK)
      lose("Could not generate EC key pair: %s\n", hal_error_string(err));

    uint8_t public_der[hal_rpc_pkey_get_public_key_len(private_key)];

    if ((err = hal_rpc_pkey_get_public_key(private_key, public_der, &len, sizeof(public_der))) != HAL_OK)
      lose("Could not DER encode public key from test vector: %s\n", hal_error_string(err));

    assert(len == sizeof(public_der));

    if ((err = hal_rpc_pkey_load(client, session, &public_key, &public_name,
                                 public_der, sizeof(public_der), flags)) != HAL_OK)
      lose("Could not load public key into RPC: %s\n", hal_error_string(err));

    uint8_t sig[tc->sig_len + 4];

    if ((err = hal_rpc_pkey_sign(private_key, hal_hash_handle_none,
                                 tc->H, tc->H_len, sig, &len, sizeof(sig))) != HAL_OK)
      lose("Could not sign: %s\n", hal_error_string(err));

    if ((err = hal_rpc_pkey_verify(public_key, hal_hash_handle_none,
                                   tc->H, tc->H_len, sig, len)) != HAL_OK)
      lose("Could not verify own signature: %s\n", hal_error_string(err));

    if (!test_attributes(private_key, &private_name, flags) || !test_attributes(public_key, &public_name, flags))
      goto fail;

    if ((err = hal_rpc_pkey_delete(private_key)) != HAL_OK)
      lose("Could not delete private key: %s\n", hal_error_string(err));

    if ((err = hal_rpc_pkey_delete(public_key)) != HAL_OK)
      lose("Could not delete public key: %s\n", hal_error_string(err));

    printf("OK\n");
    return 1;
  }

 fail:
  if (private_key.handle != HAL_HANDLE_NONE &&
      (err = hal_rpc_pkey_delete(private_key)) != HAL_OK)
    printf("Warning: could not delete private key: %s\n", hal_error_string(err));

  if (public_key.handle != HAL_HANDLE_NONE &&
      (err = hal_rpc_pkey_delete(public_key)) != HAL_OK)
    printf("Warning: could not delete public key: %s\n", hal_error_string(err));

  return 0;
}

int main (int argc, char *argv[])
{
  const hal_client_handle_t client = {HAL_HANDLE_NONE};
  const char *pin = argc > 1 ? argv[1] : "fnord";
  hal_error_t err;
  int ok = 1;

  if ((err = hal_rpc_client_init()) != HAL_OK)
    printf("Warning: Trouble initializing RPC client: %s\n", hal_error_string(err));

  if ((err = hal_rpc_login(client, HAL_USER_NORMAL, pin, strlen(pin))) != HAL_OK)
    printf("Warning: Trouble logging into HSM: %s\n", hal_error_string(err));

  for (int i = 0; i < (sizeof(rsa_tc)/sizeof(*rsa_tc)); i++)
    for (int j = 0; j < 2; j++)
      ok &= test_rsa_testvec(&rsa_tc[i], j * HAL_KEY_FLAG_TOKEN);

  for (int i = 0; i < (sizeof(ecdsa_tc)/sizeof(*ecdsa_tc)); i++)
    for (int j = 0; j < 2; j++)
      ok &= test_ecdsa_testvec(&ecdsa_tc[i], j * HAL_KEY_FLAG_TOKEN);

  for (int i = 0; i < (sizeof(rsa_tc)/sizeof(*rsa_tc)); i++)
    for (int j = 0; j < 2; j++)
      ok &= test_rsa_generate(&rsa_tc[i], j * HAL_KEY_FLAG_TOKEN);

  for (int i = 0; i < (sizeof(ecdsa_tc)/sizeof(*ecdsa_tc)); i++)
    for (int j = 0; j < 2; j++)
      ok &= test_ecdsa_generate(&ecdsa_tc[i], j * HAL_KEY_FLAG_TOKEN);

  if ((err = hal_rpc_logout(client)) != HAL_OK)
    printf("Warning: Trouble logging out of HSM: %s\n", hal_error_string(err));

  if ((err = hal_rpc_client_close()) != HAL_OK)
    printf("Warning: Trouble shutting down RPC client: %s\n", hal_error_string(err));

  return !ok;
}

/*
 * Local variables:
 * indent-tabs-mode: nil
 * End:
 */
