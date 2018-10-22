//======================================================================
//
// modexpa7_systolic_pe_generic.v
// -----------------------------------------------------------------------------
// Generic low-level systolic array processing element.
//
// Authors: Pavel Shatov
//
// Copyright (c) 2017, NORDUnet A/S All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// - Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
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
//======================================================================

module modexpa7_systolic_pe_generic
	(
		input					clk,
		input		[31: 0]	a,
		input		[31: 0]	b,
		input		[31: 0]	t,
		input		[31: 0]	c_in,
		output	[31: 0]	p,
		output	[31: 0]	c_out
	);

		//
		// Customizable Latency
		//
	parameter LATENCY = 4;
		
		//
		// Delay Line
		//
	reg	[63: 0]	abct[1:LATENCY];
	
		//
		// Outputs
		//
	assign p			= abct[LATENCY][31: 0];
	assign c_out	= abct[LATENCY][63:32];

		//
		// Sub-products
		//
	wire	[63: 0]	ab = {{32{1'b0}}, a}    * {{32{1'b0}}, b};
	wire	[63: 0]	ct = {{32{1'b0}}, c_in} + {{32{1'b0}}, t};

		//
		// Delay
		//
	integer i;
	always @(posedge clk)
		//
		for (i=1; i<=LATENCY; i=i+1)
			abct[i] <= (i == 1) ? ab + ct : abct[i-1];

endmodule

//======================================================================
// End of file
//======================================================================
