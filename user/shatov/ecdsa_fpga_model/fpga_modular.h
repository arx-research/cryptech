//------------------------------------------------------------------------------
//
// fpga_modular.h
// ---------------------------
// Modular arithmetic routines
//
// Authors: Pavel Shatov
//
// Copyright (c) 2015-2016, NORDUnet A/S
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// - Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// - Neither the name of the NORDUnet nor the names of its contributors may be
//   used to endorse or promote products derived from this software without
//   specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Globals
//------------------------------------------------------------------------------
extern FPGA_BUFFER ecdsa_q;
extern FPGA_BUFFER ecdsa_zero;
extern FPGA_BUFFER ecdsa_one;
extern FPGA_BUFFER ecdsa_delta;


//------------------------------------------------------------------------------
// Prototypes
//------------------------------------------------------------------------------
void	fpga_modular_init						();

void	fpga_modular_add						(FPGA_BUFFER *a, FPGA_BUFFER *b, FPGA_BUFFER *s);
void	fpga_modular_sub						(FPGA_BUFFER *a, FPGA_BUFFER *b, FPGA_BUFFER *d);
void	fpga_modular_mul						(FPGA_BUFFER *a, FPGA_BUFFER *b, FPGA_BUFFER *p);
void	fpga_modular_inv						(FPGA_BUFFER *a, FPGA_BUFFER *a1);

void	fpga_modular_mul_helper_multiply		(FPGA_BUFFER *a, FPGA_BUFFER *b, FPGA_WORD_EXTENDED *si);
void	fpga_modular_mul_helper_accumulate		(FPGA_WORD_EXTENDED *si, FPGA_WORD *c);
void	fpga_modular_mul_helper_reduce_p256		(FPGA_WORD *c, FPGA_BUFFER *p);
void	fpga_modular_mul_helper_reduce_p384		(FPGA_WORD *c, FPGA_BUFFER *p);

void	fpga_modular_inv_helper_shl				(FPGA_WORD *x, FPGA_WORD *y);
void	fpga_modular_inv_helper_shr				(FPGA_WORD *x, FPGA_WORD *y);
void	fpga_modular_inv_helper_add				(FPGA_WORD *x, FPGA_WORD *y, FPGA_WORD *s);
void	fpga_modular_inv_helper_sub				(FPGA_WORD *x, FPGA_WORD *y, FPGA_WORD *d);
void	fpga_modular_inv_helper_cpy				(FPGA_WORD *dst, FPGA_WORD *src);
void	fpga_modular_inv_helper_cmp				(FPGA_WORD *a, FPGA_WORD *b, int *c);


//------------------------------------------------------------------------------
// Reduction Routine Selection
//------------------------------------------------------------------------------

#if USE_CURVE == 1
#define fpga_modular_mul_helper_reduce fpga_modular_mul_helper_reduce_p256
#elif USE_CURVE == 2
#define fpga_modular_mul_helper_reduce fpga_modular_mul_helper_reduce_p384
#else
#error USE_CURVE must be either 1 or 2!
#endif


//------------------------------------------------------------------------------
// End-of-File
//------------------------------------------------------------------------------
