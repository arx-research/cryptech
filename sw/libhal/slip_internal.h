/*
 * slip_internal.h
 * ---------------
 * Send/recv data over a serial connection with SLIP framing
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

#ifndef _HAL_SLIP_INTERNAL_H
#define _HAL_SLIP_INTERNAL_H

#include "hal_internal.h"

/* Defined in slip.c - send/recv serial data with SLIP framing.
 */
extern hal_error_t hal_slip_send_char(const uint8_t c);
extern hal_error_t hal_slip_send(const uint8_t * const buf, const size_t len);
extern hal_error_t hal_slip_process_char(uint8_t c, uint8_t * const buf, size_t * const len, const size_t maxlen, int * const complete);
extern hal_error_t hal_slip_recv_char(uint8_t * const buf, size_t * const len, const size_t maxlen, int * const complete);
extern hal_error_t hal_slip_recv(uint8_t * const buf, size_t * const len, const size_t maxlen);

/* Defined in rpc_serial.c - send/recv one byte over a serial connection.
 */
extern hal_error_t hal_serial_send_char(const uint8_t c);
extern hal_error_t hal_serial_recv_char(uint8_t * const c);

#ifndef STM32F4XX
extern hal_error_t hal_serial_init(const char * const device, const uint32_t speed);
extern hal_error_t hal_serial_close(void);
extern int hal_serial_get_fd(void);
#endif

#endif /* _HAL_SLIP_INTERNAL_H */
