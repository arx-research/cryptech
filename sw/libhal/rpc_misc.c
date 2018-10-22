/*
 * rpc_misc.c
 * ----------
 * RPC interface to TRNG and PIN functions
 *
 * Authors: Rob Austein
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

#include "hal.h"
#include "hal_internal.h"

static hal_error_t get_version(uint32_t *version)
{
  *version = RPC_VERSION;
  return HAL_OK;
}

static hal_error_t get_random(void *buffer, const size_t length)
{
  if (buffer == NULL || length == 0)
    return HAL_ERROR_IMPOSSIBLE;

  return hal_get_random(NULL, buffer, length);
}

/*
 * PINs, salt, and iteration count live in the keystore.
 *
 * We also need a client table in conventional memory (here, probably)
 * to record login status.
 *
 * The USER and SO PINs correspond to PKCS #11.
 *
 * The WHEEL PIN is the one that's allowed to change the SO PIN.
 *
 * It's a bit unclear how we should manage changes to the WHEEL PIN.
 * Implementing a factory default would be easy enough (just
 * pre-compute and compile in a const hal_ks_pin_t), question is
 * whether doing so provides anything useful.  Certainly adds no real
 * security, question is whether it would help prevent accidently
 * bricking the HSM right out of the shrink wrap.
 *
 * More interesting question is whether we should ever allow the WHEEL
 * PIN to be changed a second time without toasting the keystore.
 */

typedef struct {
  hal_client_handle_t handle;
  hal_user_t logged_in;
} client_slot_t;

#ifndef HAL_PIN_MINIMUM_ITERATIONS
#define HAL_PIN_MINIMUM_ITERATIONS 1000
#endif

#ifndef HAL_PIN_DEFAULT_ITERATIONS
#define HAL_PIN_DEFAULT_ITERATIONS 2000
#endif

static uint32_t hal_pin_default_iterations = HAL_PIN_DEFAULT_ITERATIONS;

/*
 * Seconds to delay when given a bad PIN.
 */

#ifndef HAL_PIN_DELAY_ON_FAILURE
#define HAL_PIN_DELAY_ON_FAILURE 5
#endif

#ifndef HAL_STATIC_CLIENT_STATE_BLOCKS
#define HAL_STATIC_CLIENT_STATE_BLOCKS  10
#endif

#if HAL_STATIC_CLIENT_STATE_BLOCKS > 0
static client_slot_t client_handle[HAL_STATIC_CLIENT_STATE_BLOCKS];
#endif

/*
 * Client handles are supplied by the application, we don't get to
 * pick them, we just store them and associate a login state with
 * them.  HAL_USER_NONE indicates an empty slot in the table.
 */

static inline hal_error_t alloc_slot(const hal_client_handle_t client,
                                     const hal_user_t user)
{
  client_slot_t *slot = NULL;
  hal_critical_section_start();

#if HAL_STATIC_CLIENT_STATE_BLOCKS > 0

  for (size_t i = 0; slot == NULL && i < sizeof(client_handle)/sizeof(*client_handle); i++)
    if (client_handle[i].logged_in != HAL_USER_NONE &&
        client_handle[i].handle.handle == client.handle)
      slot = &client_handle[i];

  for (size_t i = 0; slot == NULL && i < sizeof(client_handle)/sizeof(*client_handle); i++)
    if (client_handle[i].logged_in == HAL_USER_NONE)
      slot = &client_handle[i];

#endif

  if (slot != NULL) {
    slot->handle = client;
    slot->logged_in = user;
  }

  hal_critical_section_end();
  return slot == NULL ? HAL_ERROR_NO_CLIENT_SLOTS_AVAILABLE : HAL_OK;
}

static inline hal_error_t clear_slot(client_slot_t *slot)
{
  if (slot == NULL)
    return HAL_OK;

  hal_error_t err;

  if ((err = hal_pkey_logout(slot->handle)) != HAL_OK)
    return err;

  hal_critical_section_start();

  memset(slot, 0, sizeof(*slot));

  hal_critical_section_end();

  return HAL_OK;
}

static inline client_slot_t *find_handle(const hal_client_handle_t handle)
{
  client_slot_t *slot = NULL;
  hal_critical_section_start();

#if HAL_STATIC_CLIENT_STATE_BLOCKS > 0
  for (size_t i = 0; slot == NULL && i < sizeof(client_handle)/sizeof(*client_handle); i++)
    if (client_handle[i].logged_in != HAL_USER_NONE && client_handle[i].handle.handle == handle.handle)
      slot = &client_handle[i];
#endif

  hal_critical_section_end();
  return slot;
}

static hal_error_t login(const hal_client_handle_t client,
                         const hal_user_t user,
                         const char * const pin, const size_t pin_len)
{
  if (pin == NULL || pin_len == 0 || (user != HAL_USER_NORMAL && user != HAL_USER_SO && user != HAL_USER_WHEEL))
    return HAL_ERROR_IMPOSSIBLE;

  const hal_ks_pin_t *p;
  hal_error_t err;

  if ((err = hal_get_pin(user, &p)) != HAL_OK)
    return err;

  uint8_t buf[sizeof(p->pin)];
  const uint32_t iterations = p->iterations == 0 ? hal_pin_default_iterations : p->iterations;

  if ((err = hal_pbkdf2(NULL, hal_hash_sha256, (const uint8_t *) pin, pin_len,
                        p->salt, sizeof(p->salt), buf, sizeof(buf), iterations)) != HAL_OK)
    return err;

  unsigned diff = 0;
  for (size_t i = 0; i < sizeof(buf); i++)
    diff |= buf[i] ^ p->pin[i];

  if (diff != 0) {
    hal_sleep(HAL_PIN_DELAY_ON_FAILURE);
    return HAL_ERROR_PIN_INCORRECT;
  }

  return alloc_slot(client, user);
}

static hal_error_t is_logged_in(const hal_client_handle_t client,
                                const hal_user_t user)
{
  if (user != HAL_USER_NORMAL && user != HAL_USER_SO && user != HAL_USER_WHEEL)
    return HAL_ERROR_IMPOSSIBLE;

  client_slot_t *slot = find_handle(client);

  if (slot == NULL || slot->logged_in != user)
    return HAL_ERROR_FORBIDDEN;

  return HAL_OK;
}

static hal_error_t logout(const hal_client_handle_t client)
{
  return clear_slot(find_handle(client));
}

static hal_error_t logout_all(void)
{
#if HAL_STATIC_CLIENT_STATE_BLOCKS > 0

  client_slot_t *slot;
  hal_error_t err;
  size_t i = 0;

  do {

    hal_critical_section_start();

    for (slot = NULL; slot == NULL && i < sizeof(client_handle)/sizeof(*client_handle); i++)
      if (client_handle[i].logged_in != HAL_USER_NONE)
        slot = &client_handle[i];

    hal_critical_section_end();

    if ((err = clear_slot(slot)) != HAL_OK)
      return err;

  } while (slot != NULL);

#endif

  return HAL_OK;
}

static hal_error_t set_pin(const hal_client_handle_t client,
                           const hal_user_t user,
                           const char * const newpin, const size_t newpin_len)
{
  if (newpin == NULL || newpin_len < hal_rpc_min_pin_length || newpin_len > hal_rpc_max_pin_length)
    return HAL_ERROR_IMPOSSIBLE;

  if ((user != HAL_USER_NORMAL || is_logged_in(client, HAL_USER_SO) != HAL_OK) &&
      is_logged_in(client, HAL_USER_WHEEL) != HAL_OK)
    return HAL_ERROR_FORBIDDEN;

  const hal_ks_pin_t *pp;
  hal_error_t err;

  if ((err = hal_get_pin(user, &pp)) != HAL_OK)
    return err;

  hal_ks_pin_t p = *pp;

  p.iterations = hal_pin_default_iterations;

  if ((err = hal_get_random(NULL, p.salt, sizeof(p.salt)))      != HAL_OK ||
      (err = hal_pbkdf2(NULL, hal_hash_sha256,
                        (const uint8_t *) newpin, newpin_len,
                        p.salt, sizeof(p.salt),
                        p.pin,  sizeof(p.pin), p.iterations))   != HAL_OK ||
      (err = hal_set_pin(user, &p))                             != HAL_OK)
    return err;

  return HAL_OK;
}

hal_error_t hal_set_pin_default_iterations(const hal_client_handle_t client,
                                           const uint32_t iterations)
{
  if ((is_logged_in(client, HAL_USER_WHEEL) != HAL_OK) &&
      (is_logged_in(client, HAL_USER_SO) != HAL_OK))
    return HAL_ERROR_FORBIDDEN;

  /* should probably store this in flash somewhere */
  hal_pin_default_iterations = (iterations == 0) ? HAL_PIN_DEFAULT_ITERATIONS : iterations;
  return HAL_OK;
}

const hal_rpc_misc_dispatch_t hal_rpc_local_misc_dispatch = {
  .set_pin      = set_pin,
  .login        = login,
  .logout       = logout,
  .logout_all   = logout_all,
  .is_logged_in = is_logged_in,
  .get_random   = get_random,
  .get_version  = get_version
};

/*
 * Local variables:
 * indent-tabs-mode: nil
 * End:
 */
