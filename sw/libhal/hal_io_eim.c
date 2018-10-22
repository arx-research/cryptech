/*
 * hal_io_eim.c
 * ------------
 * This module contains common code to talk to the FPGA over the EIM bus.
 *
 * Author: Paul Selkirk
 * Copyright (c) 2014-2016, NORDUnet A/S All rights reserved.
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

#include "novena-eim.h"
#include "hal.h"
#include "hal_internal.h"

static int debug = 0;
static int inited = 0;

static inline hal_error_t init(void)
{
  if (inited)
    return HAL_OK;

  if (eim_setup() != 0)
    return HAL_ERROR_IO_SETUP_FAILED;

  inited = 1;
  return HAL_OK;
}

/* translate cryptech register number to EIM address
 */
static inline hal_addr_t eim_offset(hal_addr_t offset)
{
  return EIM_BASE_ADDR + (offset << 2);
}

void hal_io_set_debug(int onoff)
{
  debug = onoff;
}

static void dump(char *label, const uint8_t *buf, size_t len)
{
  if (debug) {
    size_t i;
    printf("%s [", label);
    for (i = 0; i < len; ++i)
      printf(" %02x", buf[i]);
    printf(" ]\n");
  }
}

hal_error_t hal_io_write(const hal_core_t *core, hal_addr_t offset, const uint8_t *buf, size_t len)
{
  hal_error_t err;

  if (core == NULL)
    return HAL_ERROR_CORE_NOT_FOUND;

  if (len % 4 != 0)
    return HAL_ERROR_IO_BAD_COUNT;

  if ((err = init()) != HAL_OK)
    return err;

  dump("write ", buf, len);

  offset = eim_offset(offset + hal_core_base(core));
  for (; len > 0; offset += 4, buf += 4, len -= 4) {
    uint32_t val;
    val = htonl(*(uint32_t *)buf);
    eim_write_32(offset, &val);
  }

  return HAL_OK;
}

hal_error_t hal_io_read(const hal_core_t *core, hal_addr_t offset, uint8_t *buf, size_t len)
{
  uint8_t *rbuf = buf;
  int rlen = len;
  hal_error_t err;

  if (core == NULL)
    return HAL_ERROR_CORE_NOT_FOUND;

  if (len % 4 != 0)
    return HAL_ERROR_IO_BAD_COUNT;

  if ((err = init()) != HAL_OK)
    return err;

  offset = eim_offset(offset + hal_core_base(core));
  for (; rlen > 0; offset += 4, rbuf += 4, rlen -= 4) {
    uint32_t val;
    eim_read_32(offset, &val);
    *(uint32_t *)rbuf = ntohl(val);
  }

  dump("read  ", buf, len);

  return HAL_OK;
}

/*
 * Local variables:
 * indent-tabs-mode: nil
 * End:
 */
