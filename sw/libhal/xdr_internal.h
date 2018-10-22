/*
 * xdr_internal.h
 * --------------
 * Serialization/deserialization routines, using XDR (RFC 4506) encoding.
 * These functions are not part of the public libhal API.
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

#ifndef _XDR_INTERNAL_H
#define _XDR_INTERNAL_H

hal_error_t hal_xdr_encode_int(uint8_t ** const outbuf,
                               const uint8_t * const limit,
                               const uint32_t value);

hal_error_t hal_xdr_decode_int(const uint8_t ** const inbuf,
                               const uint8_t * const limit,
                               uint32_t * const value);

hal_error_t hal_xdr_decode_int_peek(const uint8_t ** const inbuf,
                                    const uint8_t * const limit,
                                    uint32_t * const value);

hal_error_t hal_xdr_encode_fixed_opaque(uint8_t ** const outbuf,
                                        const uint8_t * const limit,
                                        const uint8_t * const value, const size_t len);

hal_error_t hal_xdr_decode_fixed_opaque(const uint8_t ** const inbuf,
                                        const uint8_t * const limit,
                                        uint8_t * const value, const size_t len);

hal_error_t hal_xdr_decode_fixed_opaque_ptr(const uint8_t ** const inbuf,
                                            const uint8_t * const limit,
                                           const  uint8_t ** const vptr, const size_t len);

hal_error_t hal_xdr_encode_variable_opaque(uint8_t ** const outbuf,
                                           const uint8_t * const limit,
                                           const uint8_t * const value,
                                           const size_t len);

hal_error_t hal_xdr_decode_variable_opaque(const uint8_t ** const inbuf,
                                           const uint8_t * const limit,
                                           uint8_t * const value,
                                           size_t * const len);

hal_error_t hal_xdr_decode_variable_opaque_ptr(const uint8_t ** const inbuf,
                                               const uint8_t * const limit,
                                               const uint8_t ** const vptr,
                                               size_t * const len);

#endif /* _XDR_INTERNAL_H*/
