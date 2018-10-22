//------------------------------------------------------------------------------
//
// ecdh_fpga_model.h
// --------------------------------------------
// Base point scalar multiplier model for ECDSA
//
// Authors: Pavel Shatov
//
// Copyright (c) 2017-2018, NORDUnet A/S
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
// Headers
//------------------------------------------------------------------------------
#include "test_vectors/ecdh_test_vectors.h"


//------------------------------------------------------------------------------
//
// Curve Selection
//
// USE_CURVE == 1  -> P-256
// USE_CURVE == 2  -> P-384
//
//------------------------------------------------------------------------------
#ifndef USE_CURVE
#define USE_CURVE	2
#endif


//------------------------------------------------------------------------------
// Model Parameters
//------------------------------------------------------------------------------
#if USE_CURVE == 1
#define OPERAND_WIDTH	(256)	// largest supported operand width in bits
#elif USE_CURVE == 2
#define OPERAND_WIDTH	(384)	// largest supported operand width in bits
#else
#error USE_CURVE must be either 1 or 2!
#endif


//------------------------------------------------------------------------------
// P-256 Parameters and Test Vectors
//------------------------------------------------------------------------------

/* Field Size */
#define P_256_Q			{0xffffffff, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0xffffffff, 0xffffffff}

/* Generic Numbers */
#define P_256_ZERO		{0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}
#define P_256_ONE		{0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001}

/* Division Factor  */
#define P_256_DELTA		{0x7fffffff, 0x80000000, 0x80000000, 0x00000000, 0x00000000, 0x80000000, 0x00000000, 0x00000000}

/* Base Point */
#define P_256_G_X		{0x6b17d1f2, 0xe12c4247, 0xf8bce6e5, 0x63a440f2, 0x77037d81, 0x2deb33a0, 0xf4a13945, 0xd898c296}
#define P_256_G_Y		{0x4fe342e2, 0xfe1a7f9b, 0x8ee7eb4a, 0x7c0f9e16, 0x2bce3357, 0x6b315ece, 0xcbb64068, 0x37bf51f5}

/* Base Point Order */
#define P_256_N			{0xffffffff, 0x00000000, 0xffffffff, 0xffffffff, 0xbce6faad, 0xa7179e84, 0xf3b9cac2, 0xfc632551}


//------------------------------------------------------------------------------
// P-384 Parameters and Test Vectors
//------------------------------------------------------------------------------

/* Field Size */
#define P_384_Q			{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xfffffffe, 0xffffffff, 0x00000000, 0x00000000, 0xffffffff}

/* Generic Numbers */
#define P_384_ZERO		{0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000}
#define P_384_ONE		{0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001}

/* Division Factor  */
#define P_384_DELTA		{0x7fffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x7fffffff, 0x80000000, 0x00000000, 0x80000000}

/* Base Point */
#define P_384_G_X		{0xaa87ca22, 0xbe8b0537, 0x8eb1c71e, 0xf320ad74, 0x6e1d3b62, 0x8ba79b98, 0x59f741e0, 0x82542a38, 0x5502f25d, 0xbf55296c, 0x3a545e38, 0x72760ab7}
#define P_384_G_Y		{0x3617de4a, 0x96262c6f, 0x5d9e98bf, 0x9292dc29, 0xf8f41dbd, 0x289a147c, 0xe9da3113, 0xb5f0b8c0, 0x0a60b1ce, 0x1d7e819d, 0x7a431d7c, 0x90ea0e5f}

/* Base Point Order */
#define P_384_N			{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xc7634d81, 0xf4372ddf, 0x581a0db2, 0x48b0a77a, 0xecec196a, 0xccc52973}


//------------------------------------------------------------------------------
// Parameter and Test Vector Selection
//------------------------------------------------------------------------------
#if USE_CURVE == 1

#define ECDSA_Q			P_256_Q

#define ECDSA_ZERO		P_256_ZERO
#define ECDSA_ONE		P_256_ONE

#define ECDSA_DELTA		P_256_DELTA

#define ECDSA_G_X		P_256_G_X
#define ECDSA_G_Y		P_256_G_Y

#define ECDH_DA			P_256_DA
#define ECDH_DB			P_256_DB

#define ECDH_QA_X		P_256_QA_X
#define ECDH_QA_Y		P_256_QA_Y

#define ECDH_QB_X		P_256_QB_X
#define ECDH_QB_Y		P_256_QB_Y

#define ECDH_S_X		P_256_S_X
#define ECDH_S_Y		P_256_S_Y

#define ECDSA_N			P_256_N

#elif USE_CURVE == 2

#define ECDSA_Q			P_384_Q

#define ECDSA_ZERO		P_384_ZERO
#define ECDSA_ONE		P_384_ONE

#define ECDSA_DELTA		P_384_DELTA

#define ECDSA_G_X		P_384_G_X
#define ECDSA_G_Y		P_384_G_Y

#define ECDH_DA			P_384_DA
#define ECDH_DB			P_384_DB

#define ECDH_QA_X		P_384_QA_X
#define ECDH_QA_Y		P_384_QA_Y

#define ECDH_QB_X		P_384_QB_X
#define ECDH_QB_Y		P_384_QB_Y

#define ECDH_S_X		P_384_S_X
#define ECDH_S_Y		P_384_S_Y

#define ECDSA_N			P_384_N

#else

#error USE_CURVE must be either 1 or 2!

#endif


//------------------------------------------------------------------------------
// End-of-File
//------------------------------------------------------------------------------
