//------------------------------------------------------------------------------
//
// fpga_curve.h
// ------------------------------------
// Elliptic curve arithmetic procedures
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
extern FPGA_BUFFER ecdsa_g_x, ecdsa_g_y;
extern FPGA_BUFFER ecdsa_h_x, ecdsa_h_y;
extern FPGA_BUFFER ecdsa_q_x, ecdsa_q_y;
extern FPGA_BUFFER ecdsa_r_x, ecdsa_r_y;


//------------------------------------------------------------------------------
// Prototypes
//------------------------------------------------------------------------------
void	fpga_curve_init					();
void	fpga_curve_scalar_multiply		(FPGA_BUFFER *k, FPGA_BUFFER *qx, FPGA_BUFFER *qy);
void	fpga_curve_add_jacobian			(FPGA_BUFFER *px, FPGA_BUFFER *py, FPGA_BUFFER *pz, FPGA_BUFFER *rx, FPGA_BUFFER *ry, FPGA_BUFFER *rz);
void	fpga_curve_double_jacobian		(FPGA_BUFFER *px, FPGA_BUFFER *py, FPGA_BUFFER *pz, FPGA_BUFFER *rx, FPGA_BUFFER *ry, FPGA_BUFFER *rz);
void	fpga_curve_point_to_affine		(FPGA_BUFFER *px, FPGA_BUFFER *py, FPGA_BUFFER *pz, FPGA_BUFFER *qx, FPGA_BUFFER *qy);


//------------------------------------------------------------------------------
// End-of-File
//------------------------------------------------------------------------------
