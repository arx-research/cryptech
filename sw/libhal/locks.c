/*
 * locks.c
 * -------
 * Dummy lock code for libhal.
 *
 * Copyright (c) 2017, NORDUnet A/S All rights reserved.
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
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "hal.h"
#include "hal_internal.h"

/*
 * There are three slightly peculiar things about this module.
 *
 * 1) We want to include optional support for GNU weak functions,
 *    because they're convenient, but we don't want to require support
 *    for them.  So we wrap this in a compilation time conditional
 *    which defaults to something compatible with C99, but allow this
 *    to be overriden via an external definition.
 *
 * 2) The functions in this module are all no-ops, just here so that
 *    things will link correctly on platforms that don't define them.
 *    Real definitions for these functions have to come from the port
 *    to a specific environment, eg, from sw/stm32/projects/hsm.c.
 *
 * 3) Because we want to expose as little as possible of the
 *    underlying mechanisms, some of the functions here are closures
 *    encapsulating objects things which would otherwise be arguments.
 *    So, for example, we have functions to lock and unlock the HSM
 *    keystore, rather than general lock and unlock functions which
 *    they HSM keystore lock as an argument.  Since the versions in
 *    this file are the no-ops, the lock itself goes away here.
 */

#ifndef ENABLE_WEAK_FUNCTIONS
#define ENABLE_WEAK_FUNCTIONS 0
#endif

#if ENABLE_WEAK_FUNCTIONS
#define WEAK_FUNCTION __attribute__((weak))
#else
#define WEAK_FUNCTION
#endif

/*
 * Critical sections -- disable preemption BRIEFLY.
 */

WEAK_FUNCTION void hal_critical_section_start(void) { return; }
WEAK_FUNCTION void hal_critical_section_end(void)   { return; }

/*
 * Keystore lock -- lock call blocks indefinitely.
 */

WEAK_FUNCTION void hal_ks_lock(void)   { return; }
WEAK_FUNCTION void hal_ks_unlock(void) { return; }

/*
 * RSA blinding cache lock -- lock call blocks indefinitely.
 */

WEAK_FUNCTION void hal_rsa_bf_lock(void)   { return; }
WEAK_FUNCTION void hal_rsa_bf_unlock(void) { return; }

/*
 * Non-preemptive task yield.
 */

WEAK_FUNCTION void hal_task_yield(void)
{
  return;
}

WEAK_FUNCTION void hal_task_yield_maybe(void)
{
  return;
}

/*
 * Local variables:
 * indent-tabs-mode: nil
 * End:
 */
