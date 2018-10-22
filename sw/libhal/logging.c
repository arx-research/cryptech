/*
 * logging.c
 * ---------
 * Default (stdio-based) logging code for libhal.
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
#include <stdarg.h>

#include "hal.h"
#include "hal_internal.h"

/*
 * Default stdio-based logging code.  Specific environments may supply
 * something more appropriate (eg, on the Alpha, we want to log to the
 * management port).
 *
 * See lock.c for comments on GNU weak functions.
 */

#ifndef ENABLE_WEAK_FUNCTIONS
#define ENABLE_WEAK_FUNCTIONS 0
#endif

#if ENABLE_WEAK_FUNCTIONS
#define WEAK_FUNCTION __attribute__((weak))
#else
#define WEAK_FUNCTION
#endif

static hal_log_level_t current_level;

WEAK_FUNCTION void hal_log_set_level(const hal_log_level_t level)
{
  current_level = level;
}

WEAK_FUNCTION void hal_log(const hal_log_level_t level, const char *format, ...)
{
  if (level < current_level)
    return;

  va_list ap;
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
  fprintf(stderr, "\n");
}

/*
 * Local variables:
 * indent-tabs-mode: nil
 * End:
 */
