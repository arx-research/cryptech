//------------------------------------------------------------------------------
//
// modexpa7_systolic_pe_artix7.v
// -----------------------------------------------------------------------------
// Hardware (Artix-7 DSP48E1) low-level systolic array processing element.
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

module modexpa7_systolic_pe_artix7
	(
		input					clk,
		input		[31: 0]	a,
		input		[31: 0]	b,
		input		[31: 0]	t,
		input		[31: 0]	c_in,
		output	[31: 0]	p,
		output	[31: 0]	c_out
	);
	
	reg	[31: 0]	t_dly;
	reg	[31: 0]	c_in_dly;
	
	always @(posedge clk) t_dly <= t;
	always @(posedge clk) c_in_dly <= c_in;
	
	wire	[31: 0]	t_c_in_s;
	wire				t_c_in_c_out;
	
	reg				t_c_in_c_out_dly;
	
	always @(posedge clk) t_c_in_c_out_dly <= t_c_in_c_out;
	
	modexpa7_adder32_artix7 add_t_c_in
	(
		.clk		(clk),
		.ce		(1'b1),
		.a			(t_dly),
		.b			(c_in_dly),
		.c_in		(1'b0),
		.s			(t_c_in_s),
		.c_out	(t_c_in_c_out)
	);

	wire	[63: 0]	a_b;
	
	wire	[31: 0]	a_b_lsb = a_b[31: 0];
	wire	[31: 0]	a_b_msb = a_b[63:32];
	
	reg	[31: 0]	a_b_msb_dly;
	
	always @(posedge clk) a_b_msb_dly <= a_b_msb;
	
	modexpa7_multiplier32_artix7 mul_a_b
	(
		.clk	(clk),
		.a		(a),
		.b		(b),
		.p		(a_b)
	);
	
	wire	[31: 0]	add_p_s;
	wire				add_p_c_out;
	
	reg	[31: 0]	add_p_s_dly;
	
	always @(posedge clk) add_p_s_dly <= add_p_s;
	
	assign p = add_p_s_dly;
	
	modexpa7_adder32_artix7 add_p
	(
		.clk		(clk),
		.ce		(1'b1),
		.a			(a_b_lsb),
		.b			(t_c_in_s),
		.c_in		(1'b0),
		.s			(add_p_s),
		.c_out	(add_p_c_out)
	);

	modexpa7_adder32_artix7 add_c_out
	(
		.clk		(clk),
		.ce		(1'b1),
		.a			(a_b_msb_dly),
		.b			({{31{1'b0}}, t_c_in_c_out_dly}),
		.c_in		(add_p_c_out),
		.s			(c_out),
		.c_out	()
	);

endmodule

//------------------------------------------------------------------------------
// End-of-File
//------------------------------------------------------------------------------
