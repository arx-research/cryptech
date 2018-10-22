//------------------------------------------------------------------------------
//
// modexpa7_multiplier32_artix7.v
// -----------------------------------------------------------------------------
// Hardware (Artix-7 DSP48E1) 32-bit multiplier.
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

module modexpa7_multiplier32_artix7
	(
		input					clk,
		input		[31: 0]	a,
		input		[31: 0]	b,
		output	[63: 0]	p
	);

		/* split a, b into smaller words */
	wire	[16: 0]	a_lo = a[16: 0];
	wire	[16: 0]	b_lo = b[16: 0];
	wire	[14: 0]	a_hi = a[31:17];
	wire	[14: 0]	b_hi = b[31:17];

		/* smaller sub-products */
	wire	[47: 0]	dsp1_p;
	wire	[47: 0]	dsp2_p;
	wire	[47: 0]	dsp4_p;

		/* direct output mapping */
	assign p[63:34] = dsp4_p[29: 0];
	
		/* delayed output mapping */
	genvar fd;
	generate for (fd=0; fd<17; fd=fd+1)
		begin : gen_FD
			FD # (.INIT( 1'b0)) FD_inst1 (.C(clk), .D(dsp1_p[fd]), .Q(p[fd +  0]));
			FD # (.INIT( 1'b0)) FD_inst3 (.C(clk), .D(dsp2_p[fd]), .Q(p[fd + 17]));
		end
	endgenerate

		/* product chains */
	wire	[47: 0]	dsp1_p_chain;
	wire	[47: 0]	dsp3_p_chain;
	wire	[47: 0]	dsp2_p_chain;
	
		/* operand chains */
	wire	[29: 0]	a_lo_chain;
	wire	[29: 0]	a_hi_chain;  
  
		//
		// a_lo * b_lo
		//
	modexpa7_dsp48e1_wrapper_ext #
	(
		.AREG			(1'b1),
		.PREG			(1'b0),
		.A_INPUT		("DIRECT")
	)
	dsp1
	(
		.clk		(clk),
		.opmode	(7'b0110101),
		.a			({13'd0, a_lo}),
		.b			({1'b0, b_lo}),
		.p			(dsp1_p),
		.acin		(30'd0),
		.pcin		(48'd0),
		.acout	(a_lo_chain),
		.pcout	(dsp1_p_chain)
	);
	
		//
		// a_hi * b_lo
		//
	modexpa7_dsp48e1_wrapper_ext #
	(
		.AREG			(1'b1),
		.PREG			(1'b0),
		.A_INPUT		("DIRECT")
	)
	dsp2
	(
		.clk		(clk),
		.opmode	(7'b0010101),
		.a			({15'd0, a_hi}),
		.b			({1'd0, b_lo}),
		.p			(dsp2_p),
		.acin		(30'd0),
		.pcin		(dsp3_p_chain),
		.acout	(a_hi_chain),
		.pcout	(dsp2_p_chain)
	);
	
		//
		// a_lo * b_hi
		//
	modexpa7_dsp48e1_wrapper_ext #
	(
		.AREG			(1'b0),
		.PREG			(1'b0),
		.A_INPUT		("CASCADE")
	)
	dsp3
	(
		.clk		(clk),
		.opmode	(7'b1010101),
		.a			(30'd0),
		.b			({3'd0, b_hi}),
		.p			(),
		.acin		(a_lo_chain),
		.pcin		(dsp1_p_chain),
		.acout	(),
		.pcout	(dsp3_p_chain)
	);	
	
		//
		// a_hi * b_hi
		//
	modexpa7_dsp48e1_wrapper_ext #
	(
		.AREG			(1'b0),
		.PREG			(1'b1),
		.A_INPUT		("CASCADE")
	)
	dsp4
	(
		.clk		(clk),
		.opmode	(7'b1010101),
		.a			(30'd0),
		.b			({3'd0, b_hi}),
		.p			(dsp4_p),
		.acin		(a_hi_chain),
		.pcin		(dsp2_p_chain),
		.acout	(),
		.pcout	()
	);

endmodule

//------------------------------------------------------------------------------
// End-of-File
//------------------------------------------------------------------------------
