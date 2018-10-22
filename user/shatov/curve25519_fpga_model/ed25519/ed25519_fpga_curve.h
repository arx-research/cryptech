//------------------------------------------------------------------------------
//
// ed25519_fpga_curve.h
// ------------------------------------------------
// Elliptic curve arithmetic procedures for Ed25519
//
// Authors: Pavel Shatov
//
// Copyright (c) 2018 NORDUnet A/S
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
// Ed25519 Parameters
//------------------------------------------------------------------------------

/* x-coordinate of the base point */
#define ED25519_G_X_INIT	{0x216936d3, 0xcd6e53fe, 0xc0a4e231, 0xfdd6dc5c, \
							 0x692cc760, 0x9525a7b2, 0xc9562d60, 0x8f25d51a}

/* y-coordinate of the base point */
#define ED25519_G_Y_INIT	{0x66666666, 0x66666666, 0x66666666, 0x66666666, \
							 0x66666666, 0x66666666, 0x66666666, 0x66666658}

/* z-coordinate of the base point */
#define ED25519_G_Z_INIT	{0x00000000, 0x00000000, 0x00000000, 0x00000000, \
							 0x00000000, 0x00000000, 0x00000000, 0x00000001}

/* t-coordinate of the base point */
#define ED25519_G_T_INIT	{0x67875f0f, 0xd78b7665, 0x66ea4e8e, 0x64abe37d, \
							 0x20f09f80, 0x775152f5, 0x6dde8ab3, 0xa5b7dda3}


//------------------------------------------------------------------------------
// Globals
//------------------------------------------------------------------------------
extern FPGA_BUFFER ED25519_G_X;	// the base point
extern FPGA_BUFFER ED25519_G_Y;	//
extern FPGA_BUFFER ED25519_G_Z;	//
extern FPGA_BUFFER ED25519_G_T;	//



//------------------------------------------------------------------------------
// Implementation switch
//------------------------------------------------------------------------------
#ifdef USE_MICROCODE
#define fpga_curve_ed25519_base_scalar_multiply fpga_curve_ed25519_base_scalar_multiply_microcode
#else
#define fpga_curve_ed25519_base_scalar_multiply fpga_curve_ed25519_base_scalar_multiply_abstract
#endif


//------------------------------------------------------------------------------
// Prototypes
//------------------------------------------------------------------------------
void	fpga_curve_ed25519_init								();

void	fpga_curve_ed25519_base_scalar_multiply_abstract	(const FPGA_BUFFER *K, FPGA_BUFFER *Q_Y);
void	fpga_curve_ed25519_base_scalar_multiply_microcode	(const FPGA_BUFFER *K, FPGA_BUFFER *Q_Y);

void	fpga_curve_ed25519_double	(const FPGA_BUFFER *P_X,
									 const FPGA_BUFFER *P_Y,
									 const FPGA_BUFFER *P_Z,
									 const FPGA_BUFFER *P_T,
									 FPGA_BUFFER *Q_X,
									 FPGA_BUFFER *Q_Y,
									 FPGA_BUFFER *Q_Z,
									 FPGA_BUFFER *Q_T);

void	fpga_curve_ed25519_add		(const FPGA_BUFFER *P_X,
									 const FPGA_BUFFER *P_Y,
									 const FPGA_BUFFER *P_Z,
									 const FPGA_BUFFER *P_T,
									 const FPGA_BUFFER *Q_X,
									 const FPGA_BUFFER *Q_Y,
									 const FPGA_BUFFER *Q_Z,
									 const FPGA_BUFFER *Q_T,
									 FPGA_BUFFER *R_X,
									 FPGA_BUFFER *R_Y,
									 FPGA_BUFFER *R_Z,
									 FPGA_BUFFER *R_T);

void	fpga_curve_ed25519_to_affine	(const FPGA_BUFFER *P_X, const FPGA_BUFFER *P_Y,
										 const FPGA_BUFFER *P_Z,
										 FPGA_BUFFER *Q_X, FPGA_BUFFER *Q_Y);


//------------------------------------------------------------------------------
// End-of-File
//------------------------------------------------------------------------------
