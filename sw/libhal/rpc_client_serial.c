/*
 * rpc_client_serial.c
 * -------------------
 * Remote procedure call transport over serial line with SLIP framing.
 *
 * Copyright (c) 2016, NORDUnet A/S All rights reserved.
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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#include "hal.h"
#include "hal_internal.h"
#include "slip_internal.h"

hal_error_t hal_rpc_client_transport_init(void)
{
    const char *device = getenv(HAL_CLIENT_SERIAL_DEVICE_ENVVAR);
    const char *speed_ = getenv(HAL_CLIENT_SERIAL_SPEED_ENVVAR);
    uint32_t    speed  = HAL_CLIENT_SERIAL_DEFAULT_SPEED;

    if (device == NULL)
        device = HAL_CLIENT_SERIAL_DEFAULT_DEVICE;

    if (speed_ != NULL)
        speed = (uint32_t) strtoul(speed_, NULL, 10);

    return hal_serial_init(device, speed);
}

hal_error_t hal_rpc_client_transport_close(void)
{
    return hal_serial_close();
}

hal_error_t hal_rpc_send(const uint8_t * const buf, const size_t len)
{
    return hal_slip_send(buf, len);
}

hal_error_t hal_rpc_recv(uint8_t * const buf, size_t * const len)
{
    size_t maxlen = *len;
    *len = 0;
    hal_error_t err = hal_slip_recv(buf, len, maxlen);
    return err;
}
