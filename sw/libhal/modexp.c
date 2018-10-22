/*
 * modexp.c
 * ----------
 * Wrapper around Cryptech ModExp core.
 *
 * This doesn't do full RSA, that's another module.  This module's job
 * is just the I/O to get bits in and out of the ModExp core, including
 * compensating for a few known bugs that haven't been resolved yet.
 *
 * If at some point the interface to the ModExp core becomes simple
 * enough that this module is no longer needed, it will go away.
 *
 * Authors: Rob Austein
 * Copyright (c) 2015-2017, NORDUnet A/S
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

#include "hal.h"
#include "hal_internal.h"

/*
 * Whether we want debug output.
 */

static int debug = 0;

void hal_modexp_set_debug(const int onoff)
{
  debug = onoff;
}

/*
 * Get value of an ordinary register.
 */

static inline hal_error_t get_register(const hal_core_t *core,
                                       const hal_addr_t addr,
                                       uint32_t *value)
{
  hal_error_t err;
  uint8_t w[4];

  if (value == NULL)
    return HAL_ERROR_IMPOSSIBLE;

  if ((err = hal_io_read(core, addr, w, sizeof(w))) != HAL_OK)
    return err;

  *value = (w[0] << 0) | (w[1] << 8) | (w[2] << 16) | (w[3] << 24);

  return HAL_OK;
}

/*
 * Set value of an ordinary register.
 */

static inline hal_error_t set_register(const hal_core_t *core,
                                       const hal_addr_t addr,
                                       const uint32_t value)
{
  const uint8_t w[4] = {
    ((value >> 24) & 0xFF),
    ((value >> 16) & 0xFF),
    ((value >>  8) & 0xFF),
    ((value >>  0) & 0xFF)
  };

  return hal_io_write(core, addr, w, sizeof(w));
}

/*
 * Get value of a data buffer.  We reverse the order of 32-bit words
 * in the buffer during the transfer to match what the modexpa7 core
 * expects.
 */

static inline hal_error_t get_buffer(const hal_core_t *core,
                                     const hal_addr_t data_addr,
                                     uint8_t *value,
                                     const size_t length)
{
  hal_error_t err;
  size_t i;

  if (value == NULL || length % 4 != 0)
    return HAL_ERROR_IMPOSSIBLE;

  for (i = 0; i < length; i += 4)
    if ((err = hal_io_read(core, data_addr + i/4, &value[length - 4 - i], 4)) != HAL_OK)
      return err;

  return HAL_OK;
}

/*
 * Set value of a data buffer.  We reverse the order of 32-bit words
 * in the buffer during the transfer to match what the modexpa7 core
 * expects.
 *
 * Do we need to zero the portion of the buffer we're not using
 * explictly (that is, the portion between `length` and the value of
 * the core's MODEXPA7_ADDR_BUFFER_BITS register)?  We've gotten away
 * without doing this so far, but the core doesn't take an explicit
 * length parameter for the message itself, instead it assumes that
 * the message is either as long as or twice as long as the exponent,
 * depending on the setting of the CRT mode bit.  Maybe initializing
 * the core clears the excess bits so there's no issue?  Dunno.  Have
 * never seen a problem with this yet, just dont' know why not.
 */

static inline hal_error_t set_buffer(const hal_core_t *core,
                                     const hal_addr_t data_addr,
                                     const uint8_t * const value,
                                     const size_t length)
{
  hal_error_t err;
  size_t i;

  if (value == NULL || length % 4 != 0)
    return HAL_ERROR_IMPOSSIBLE;

  for (i = 0; i < length; i += 4)
    if ((err = hal_io_write(core, data_addr + i/4, &value[length - 4 - i], 4)) != HAL_OK)
      return err;

  return HAL_OK;
}

/*
 * Stuff moved out of modexp so we can run two cores in parallel more
 * easily.  We have to return to the jacket routine every time we kick
 * a core into doing something, since only the jacket routines know
 * how many cores we're running for any particular calculation.
 *
 * In theory we could do something clever where we don't wait for both
 * cores to finish precalc before starting either of them on the main
 * computation, but that way probably lies madness.
 */

static inline hal_error_t check_args(hal_modexp_arg_t *a)
{
  /*
   * All data pointers must be set, exponent may not be longer than
   * modulus, message may not be longer than twice the modulus (CRT
   * mode), result buffer must not be shorter than modulus, and all
   * input lengths must be a multiple of four bytes (the core is all
   * about 32-bit words).
   */

  if (a         == NULL ||
      a->msg    == NULL || a->msg_len    > MODEXPA7_OPERAND_BYTES || a->msg_len    >  a->mod_len * 2 ||
      a->exp    == NULL || a->exp_len    > MODEXPA7_OPERAND_BYTES || a->exp_len    >  a->mod_len     ||
      a->mod    == NULL || a->mod_len    > MODEXPA7_OPERAND_BYTES ||
      a->result == NULL || a->result_len > MODEXPA7_OPERAND_BYTES || a->result_len <  a->mod_len     ||
      a->coeff  == NULL || a->coeff_len  > MODEXPA7_OPERAND_BYTES ||
      a->mont   == NULL || a->mont_len   > MODEXPA7_OPERAND_BYTES ||
      ((a->msg_len | a->exp_len | a->mod_len) & 3) != 0)
    return HAL_ERROR_BAD_ARGUMENTS;

  return HAL_OK;
}

static inline hal_error_t setup_precalc(const int precalc, hal_modexp_arg_t *a)
{
  hal_error_t err;

  /*
   * Check that operand size is compatabible with the core.
   */

  uint32_t operand_max = 0;

  if ((err = get_register(a->core, MODEXPA7_ADDR_BUFFER_BITS, &operand_max)) != HAL_OK)
    return err;

  operand_max /= 8;

  if (a->msg_len   > operand_max ||
      a->exp_len   > operand_max ||
      a->mod_len   > operand_max ||
      a->coeff_len > operand_max ||
      a->mont_len  > operand_max)
    return HAL_ERROR_BAD_ARGUMENTS;

  /*
   * Set the modulus, then initiate calculation of modulus-dependent
   * speedup factors if necessary, by edge-triggering the "init" bit,
   * then return to caller so it can wait for precalc.
   */

  if ((err = set_register(a->core, MODEXPA7_ADDR_MODULUS_BITS, a->mod_len * 8)) != HAL_OK  ||
      (err = set_buffer(a->core, MODEXPA7_ADDR_MODULUS, a->mod, a->mod_len))    != HAL_OK  ||
      (precalc && (err = hal_io_zero(a->core))                                  != HAL_OK) ||
      (precalc && (err = hal_io_init(a->core))                                  != HAL_OK))
    return err;

  return HAL_OK;
}

static inline hal_error_t setup_calc(const int precalc, hal_modexp_arg_t *a)
{
  hal_error_t err;

  /*
   * Select CRT mode if and only if message is longer than exponent.
   */

  const uint32_t mode = a->msg_len > a->mod_len ? MODEXPA7_MODE_CRT : MODEXPA7_MODE_PLAIN;

  /*
   * Copy out precalc results if necessary, then load everything and
   * start the calculation by edge-triggering the "next" bit.  If
   * everything works, return to caller so it can wait for the
   * calculation to complete.
   */

  if ((precalc &&
       (err = get_buffer(a->core, MODEXPA7_ADDR_MODULUS_COEFF_OUT,     a->coeff, a->coeff_len)) != HAL_OK) ||
      (precalc &&
        (err = get_buffer(a->core, MODEXPA7_ADDR_MONTGOMERY_FACTOR_OUT, a->mont,  a->mont_len)) != HAL_OK) ||
      (err = set_buffer(a->core, MODEXPA7_ADDR_MODULUS_COEFF_IN,     a->coeff, a->coeff_len))   != HAL_OK  ||
      (err = set_buffer(a->core, MODEXPA7_ADDR_MONTGOMERY_FACTOR_IN, a->mont,  a->mont_len))    != HAL_OK  ||
      (err = set_register(a->core, MODEXPA7_ADDR_MODE, mode))                                   != HAL_OK  ||
      (err = set_buffer(a->core, MODEXPA7_ADDR_MESSAGE, a->msg, a->msg_len))                    != HAL_OK  ||
      (err = set_buffer(a->core, MODEXPA7_ADDR_EXPONENT, a->exp, a->exp_len))                   != HAL_OK  ||
      (err = set_register(a->core, MODEXPA7_ADDR_EXPONENT_BITS, a->exp_len * 8))                != HAL_OK  ||
      (err = hal_io_zero(a->core))                                                              != HAL_OK  ||
      (err = hal_io_next(a->core)) != HAL_OK)
    return err;

  return HAL_OK;
}

static inline hal_error_t extract_result(hal_modexp_arg_t *a)
{
  /*
   * Extract results from the main calculation and we're done.
   */

  return get_buffer(a->core, MODEXPA7_ADDR_RESULT, a->result, a->mod_len);
}

/*
 * Run one modexp operation.
 */

hal_error_t hal_modexp(const int precalc, hal_modexp_arg_t *a)
{
  hal_error_t err;

  if ((err = check_args(a)) != HAL_OK)
    return err;

  const int free_core = a->core == NULL;

  if ((!free_core ||
       (err = hal_core_alloc(MODEXPA7_NAME, &a->core, NULL)) == HAL_OK) &&
      (err = setup_precalc(precalc, a))                      == HAL_OK  &&
      (!precalc ||
       (err = hal_io_wait_ready(a->core))                    == HAL_OK) &&
      (err = setup_calc(precalc, a))                         == HAL_OK  &&
      (err = hal_io_wait_valid(a->core))                     == HAL_OK  &&
      (err = extract_result(a))                              == HAL_OK)
    err = HAL_OK;

  if (free_core) {
    hal_core_free(a->core);
    a->core = NULL;
  }

  return err;
}

/*
 * Run two modexp operations in parallel.
 */

hal_error_t hal_modexp2(const int precalc, hal_modexp_arg_t *a1, hal_modexp_arg_t *a2)
{
  int free_core = 0;
  hal_error_t err;

  if ((err = check_args(a1)) != HAL_OK ||
      (err = check_args(a2)) != HAL_OK)
    return err;

  if (a1->core == NULL && a2->core == NULL)
    free_core = 1;
  else if (a1->core == NULL || a2->core == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  if ((!free_core ||
       (err = hal_core_alloc2(MODEXPA7_NAME, &a1->core, NULL,
                              MODEXPA7_NAME, &a2->core, NULL)) == HAL_OK) &&
      (err = setup_precalc(precalc, a1))                       == HAL_OK  &&
      (err = setup_precalc(precalc, a2))                       == HAL_OK  &&
      (!precalc ||
       (err = hal_io_wait_ready2(a1->core, a2->core))          == HAL_OK) &&
      (err = setup_calc(precalc, a1))                          == HAL_OK  &&
      (err = setup_calc(precalc, a2))                          == HAL_OK  &&
      (err = hal_io_wait_valid2(a1->core, a2->core))           == HAL_OK  &&
      (err = extract_result(a1))                               == HAL_OK  &&
      (err = extract_result(a2))                               == HAL_OK)
    err = HAL_OK;

  if (free_core) {
    hal_core_free(a1->core);
    hal_core_free(a2->core);
    a1->core = a2->core = NULL;
  }

  return err;
}

/*
 * Local variables:
 * indent-tabs-mode: nil
 * End:
 */
