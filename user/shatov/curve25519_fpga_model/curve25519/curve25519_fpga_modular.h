//------------------------------------------------------------------------------
//
// curve25519_fpga_modular.h
// ------------------------------------------
// Modular arithmetic routines for Curve25519
//
// Authors: Pavel Shatov
//
// Copyright (c) 2015-2016, 2018 NORDUnet A/S
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
// Curve25519 Parameters
//------------------------------------------------------------------------------

/* modulus (2^255 - 19) */
#define CURVE25519_1P_INIT		{0x7FFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, \
								 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFED}

/* double modulus (2^256 - 38) */
#define CURVE25519_2P_INIT		{0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, \
								 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFDA}

//------------------------------------------------------------------------------
// Globals
//------------------------------------------------------------------------------
extern FPGA_BUFFER CURVE25519_1P;
extern FPGA_BUFFER CURVE25519_2P;


//------------------------------------------------------------------------------
// Prototypes
//------------------------------------------------------------------------------
void	fpga_modular_init					();

void	fpga_modular_add					(const FPGA_BUFFER *A, const FPGA_BUFFER *B, FPGA_BUFFER *S, const FPGA_BUFFER *N);
void	fpga_modular_sub					(const FPGA_BUFFER *A, const FPGA_BUFFER *B, FPGA_BUFFER *D, const FPGA_BUFFER *N);

void	fpga_modular_mul					(const FPGA_BUFFER *A, const FPGA_BUFFER *B, FPGA_BUFFER *P, const FPGA_BUFFER *N);

void	fpga_modular_inv_abstract			(const FPGA_BUFFER *A, FPGA_BUFFER *A1, const FPGA_BUFFER *N);

void	fpga_modular_mul_helper_multiply	(const FPGA_BUFFER *A, const FPGA_BUFFER *B, FPGA_WORD_EXTENDED *SI);
void	fpga_modular_mul_helper_accumulate	(const FPGA_WORD_EXTENDED *SI, FPGA_WORD *C);
void	fpga_modular_mul_helper_reduce		(const FPGA_WORD *C, FPGA_BUFFER *P, const FPGA_BUFFER *N);


//------------------------------------------------------------------------------
// End-of-File
//------------------------------------------------------------------------------
