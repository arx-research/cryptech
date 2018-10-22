/*
 * hal_io.c
 * --------
 * This module contains common code to talk to the FPGA over the bus du jour.
 *
 * Author: Paul Selkirk, Rob Austein
 * Copyright (c) 2014-2017, NORDUnet A/S All rights reserved.
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

#ifndef HAL_IO_TIMEOUT
#define HAL_IO_TIMEOUT  100000000
#endif

static inline hal_error_t test_status(const hal_core_t *core,
                                      const uint8_t status,
                                      int *done)
{
  if (done == NULL)
    return HAL_ERROR_IMPOSSIBLE;

  if (*done || core == NULL)
    return HAL_OK;

  uint8_t buf[4];

  const hal_error_t err = hal_io_read(core, ADDR_STATUS, buf, sizeof(buf));

  if (err == HAL_OK)
    *done = (buf[3] & status) != 0;

  return err;
}

hal_error_t hal_io_wait2(const hal_core_t *core1,
                         const hal_core_t *core2,
                         const uint8_t status,
                         int *count)
{
  int done1 = 0, done2 = 0;
  hal_error_t err;

  if (core1 == NULL)
    return HAL_ERROR_BAD_ARGUMENTS;

  if (core2 == NULL)
    done2 = 1;

  if (count && *count == -1)
    *count = HAL_IO_TIMEOUT;

  for (int i = 1; ; ++i) {

    if (count && (*count > 0) && (i >= *count))
      return HAL_ERROR_IO_TIMEOUT;

    hal_task_yield();

    if ((err = test_status(core1, status, &done1)) != HAL_OK ||
        (err = test_status(core2, status, &done2)) != HAL_OK)
      return err;

    if (done1 && done2) {
      if (count)
        *count = i;
      return HAL_OK;
    }
  }
}

hal_error_t hal_io_wait(const hal_core_t *core,
                        const uint8_t status,
                        int *count)
{
  return hal_io_wait2(core, NULL, status, count);
}

/*
 * Local variables:
 * indent-tabs-mode: nil
 * c-basic-offset: 2
 * End:
 */
