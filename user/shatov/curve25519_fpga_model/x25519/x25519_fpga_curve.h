//------------------------------------------------------------------------------
//
// x25519_fpga_curve.h
// -----------------------------------------------
// Elliptic curve arithmetic procedures for X25519
//
// Authors: Pavel Shatov
//
// Copyright (c) 2015-2018 NORDUnet A/S
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

/* x-coordinate of the base point */
#define X25519_G_X_INIT		{0x00000000, 0x00000000, 0x00000000, 0x00000000, \
							 0x00000000, 0x00000000, 0x00000000, 0x00000009}

/* coefficient (A + 2) / 4 */
#define X25519_A24_INIT		{0x00000000, 0x00000000, 0x00000000, 0x00000000, \
							 0x00000000, 0x00000000, 0x00000000, 0x0001DB42}

//------------------------------------------------------------------------------
// Globals
//------------------------------------------------------------------------------
extern FPGA_BUFFER X25519_G_X;	// the base point
extern FPGA_BUFFER X25519_A24;	// coefficient (A + 2) / 4


//------------------------------------------------------------------------------
// Implementation switch
//------------------------------------------------------------------------------
#ifdef USE_MICROCODE
#define fpga_curve_x25519_scalar_multiply fpga_curve_x25519_scalar_multiply_microcode
#else
#define fpga_curve_x25519_scalar_multiply fpga_curve_x25519_scalar_multiply_abstract
#endif


//------------------------------------------------------------------------------
// Prototypes
//------------------------------------------------------------------------------
void	fpga_curve_x25519_init						();

void	fpga_curve_x25519_scalar_multiply_abstract	(const FPGA_BUFFER *P_X, const FPGA_BUFFER *K, FPGA_BUFFER *Q_X);
void	fpga_curve_x25519_scalar_multiply_microcode	(const FPGA_BUFFER *P_X, const FPGA_BUFFER *K, FPGA_BUFFER *Q_X);

void	fpga_curve_x25519_ladder_step				(const FPGA_BUFFER *P_X,
													 const FPGA_BUFFER *R0_X_in,  const FPGA_BUFFER *R0_Z_in,
													 const FPGA_BUFFER *R1_X_in,  const FPGA_BUFFER *R1_Z_in,
													 FPGA_BUFFER *R0_X_out, FPGA_BUFFER *R0_Z_out,
													 FPGA_BUFFER *R1_X_out, FPGA_BUFFER *R1_Z_out);

void	fpga_curve_x25519_to_affine		(const FPGA_BUFFER *P_X,
										 const FPGA_BUFFER *P_Z,
										 FPGA_BUFFER *Q_X);


//------------------------------------------------------------------------------
// End-of-File
//------------------------------------------------------------------------------
