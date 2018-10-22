//
// modexp_fpga_model_montgomery.h
// -------------------------------------------------------------
// Montgomery modular multiplication and exponentiation routines
//
// Authors: Pavel Shatov
// Copyright (c) 2017, NORDUnet A/S
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// - Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
//
// - Neither the name of the NORDUnet nor the names of its contributors may
//   be used to endorse or promote products derived from this software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//


//----------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------
void montgomery_multiply(		const FPGA_WORD *A,
								const FPGA_WORD *B,
								const FPGA_WORD *N,
								const FPGA_WORD *N_COEFF,
								      FPGA_WORD *R,
								      size_t     len,
									  bool       reduce_only);

void montgomery_exponentiate(	const FPGA_WORD *A,
								const FPGA_WORD *B,
								const FPGA_WORD *N,
								const FPGA_WORD *N_COEFF,
								      FPGA_WORD *R,
									  size_t     len);

void montgomery_calc_factor(	const FPGA_WORD *N,
								      FPGA_WORD *FACTOR,
									  size_t     len);

void montgomery_calc_n_coeff(	const FPGA_WORD *N,
								      FPGA_WORD *N_COEFF,
									  size_t     len);


//----------------------------------------------------------------
// End of file
//----------------------------------------------------------------
