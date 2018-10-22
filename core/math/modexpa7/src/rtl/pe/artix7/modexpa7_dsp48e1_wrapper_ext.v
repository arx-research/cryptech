//------------------------------------------------------------------------------
//
// modexpa7_dsp48e1_wrapper_ext.v
// -----------------------------------------------------------------------------
// Extended hardware (Artix-7 DSP48E1) tile wrapper.
//
// Authors: Pavel Shatov
//
// Copyright (c) 2016-2017, NORDUnet A/S
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

module modexpa7_dsp48e1_wrapper_ext #
	(
		parameter	AREG		= 1'b0,
		parameter	PREG		= 1'b0,
		
		parameter	A_INPUT	= "DIRECT"
	)
	(
		input					clk,
		input		[ 6: 0]	opmode,
		input		[29: 0]	a,
		input		[17: 0]	b,
		output	[47: 0]	p,
		input		[29: 0]	acin,
		input		[47: 0]	pcin,
		output	[29: 0]	acout,
		output	[47: 0]	pcout
	);
	
		//
		// Tile instantiation
		//
	DSP48E1 #
	(
		.AREG						(AREG),
		.BREG						(1'b1),
		.CREG						(0),
		.DREG						(0),
		.MREG						(0),
		.PREG						(PREG),
		.ADREG					(0),
		
		.ACASCREG				(AREG),
		.BCASCREG				(1'b1),
		.ALUMODEREG				(0),
		.INMODEREG				(0),
		.OPMODEREG				(0),
		.CARRYINREG				(0),
		.CARRYINSELREG			(0),

		.A_INPUT					(A_INPUT),
		.B_INPUT					("DIRECT"),
		
		.USE_DPORT				("FALSE"),
		.USE_MULT				("MULTIPLY"),
		.USE_SIMD				("ONE48"),

		.USE_PATTERN_DETECT	("NO_PATDET"),
		.SEL_PATTERN			("PATTERN"),
		.SEL_MASK				("MASK"),
		.PATTERN					(48'h000000000000),
		.MASK						(48'h3fffffffffff),
		.AUTORESET_PATDET		("NO_RESET")
	)
	DSP48E1_inst
	(
		.CLK					(clk),

		.RSTA					(1'b0),
		.RSTB					(1'b0),
		.RSTC					(1'b0),
		.RSTD					(1'b0),
		.RSTM					(1'b0),
		.RSTP					(1'b0),

		.RSTCTRL				(1'b0),
		.RSTINMODE			(1'b0),
		.RSTALUMODE			(1'b0),
		.RSTALLCARRYIN		(1'b0),

		.CEA1					(1'b0),
		.CEA2					(AREG),
		.CEB1					(1'b0),
		.CEB2					(1'b1),
		.CEC					(1'b0),
		.CED					(1'b0),
		.CEM					(1'b0),
		.CEP					(PREG),
		.CEAD					(1'b0),
		.CEALUMODE			(1'b0),
		.CEINMODE			(1'b0),

		.CECTRL				(1'b0),
		.CECARRYIN			(1'b0),

		.A						(a),
		.B						(b),
		.C						({48{1'b0}}),
		.D						({25{1'b1}}),
		.P						(p),

		.CARRYIN				(1'b0),
		.CARRYOUT			(),
		.CARRYINSEL			(3'b000),

		.CARRYCASCIN		(1'b0),
		.CARRYCASCOUT		(),

		.PATTERNDETECT		(),
		.PATTERNBDETECT	(),

		.OPMODE				(opmode),
		.ALUMODE				(4'b0000),
		.INMODE				(5'b00000),

		.MULTSIGNIN			(1'b0),
		.MULTSIGNOUT		(),

		.UNDERFLOW			(),
		.OVERFLOW			(),

		.ACIN					(acin),
		.BCIN					(18'd0),
		.PCIN					(pcin),

		.ACOUT				(acout),
		.BCOUT				(),
		.PCOUT				(pcout)
  );

endmodule
