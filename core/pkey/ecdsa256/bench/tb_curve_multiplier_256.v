//------------------------------------------------------------------------------
//
// tb_curve_multiplier_256.v
// -----------------------------------------------------------------------------
// Testbench for 256-bit curve point scalar multiplier.
//
// Authors: Pavel Shatov
//
// Copyright (c) 2016, NORDUnet A/S
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
`timescale 1ns / 1ps
//------------------------------------------------------------------------------

module tb_curve_multiplier_256;


		//
		// Test Vectors
		//
		 
		/* Q = d * G */
	localparam	[255:0]	K_1	= 256'h70a12c2db16845ed56ff68cfc21a472b3f04d7d6851bf6349f2d7d5b3452b38a;
	localparam	[255:0]	PX_1	= 256'h8101ece47464a6ead70cf69a6e2bd3d88691a3262d22cba4f7635eaff26680a8;
	localparam	[255:0]	PY_1	= 256'hd8a12ba61d599235f67d9cb4d58f1783d3ca43e78f0a5abaa624079936c0c3a9;

		/* R = k * G */
	localparam	[255:0]	K_2	= 256'h580ec00d856434334cef3f71ecaed4965b12ae37fa47055b1965c7b134ee45d0;
	localparam	[255:0]	PX_2	= 256'h7214bc9647160bbd39ff2f80533f5dc6ddd70ddf86bb815661e805d5d4e6f27c;
	localparam	[255:0]	PY_2	= 256'h8b81e3e977597110c7cf2633435b2294b72642987defd3d4007e1cfc5df84541;

		/* O = n * G */
	localparam	[255:0]	K_3	= 256'hffffffff00000000ffffffffffffffffbce6faada7179e84f3b9cac2fc632551;
	localparam	[255:0]	PX_3	= 256'h0000000000000000000000000000000000000000000000000000000000000000;
	localparam	[255:0]	PY_3	= 256'h0000000000000000000000000000000000000000000000000000000000000000;

		/* H = 2 * G */
	localparam	[255:0]	K_4	= 256'h0000000000000000000000000000000000000000000000000000000000000002;
	localparam	[255:0]	PX_4	= 256'h7cf27b188d034f7e8a52380304b51ac3c08969e277f21b35a60b48fc47669978;
	localparam	[255:0]	PY_4	= 256'h07775510db8ed040293d9ac69f7430dbba7dade63ce982299e04b79d227873d1;
		
		/* G = (n + 1) * G */
	localparam	[255:0]	K_5	= 256'hffffffff00000000ffffffffffffffffbce6faada7179e84f3b9cac2fc632551 + 'd1;
	localparam	[255:0]	PX_5	= 256'h6b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296;
	localparam	[255:0]	PY_5	= 256'h4fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ececbb6406837bf51f5;

		/* H = (n + 2) * G */
	localparam	[255:0]	K_6	= 256'hffffffff00000000ffffffffffffffffbce6faada7179e84f3b9cac2fc632551 + 'd2;
	localparam	[255:0]	PX_6	= 256'h7cf27b188d034f7e8a52380304b51ac3c08969e277f21b35a60b48fc47669978;
	localparam	[255:0]	PY_6	= 256'h07775510db8ed040293d9ac69f7430dbba7dade63ce982299e04b79d227873d1;
				
		
		//
		// Core Parameters
		//
	localparam	WORD_COUNTER_WIDTH	=  3;
	localparam	OPERAND_NUM_WORDS		=  8;
	

		//
		// Clock (100 MHz)
		//
	reg clk = 1'b0;
	always #5 clk = ~clk;

	
		//
		// Inputs, Outputs
		//
	reg	rst_n;
	reg	ena;
	wire	rdy;

	
		//
		// Buffers (K, PX, PY)
		//
	wire	[WORD_COUNTER_WIDTH-1:0]	core_k_addr;
	wire	[WORD_COUNTER_WIDTH-1:0]	core_px_addr;
	wire	[WORD_COUNTER_WIDTH-1:0]	core_py_addr;
	
	wire										core_px_wren;
	wire										core_py_wren;
	
	wire	[                32-1:0]	core_k_data;
	wire	[                32-1:0]	core_px_data;
	wire	[                32-1:0]	core_py_data;
	
	reg	[WORD_COUNTER_WIDTH-1:0]	tb_k_addr;
	reg	[WORD_COUNTER_WIDTH-1:0]	tb_pxy_addr;
	
	reg										tb_k_wren;
	
	reg	[                  31:0]	tb_k_data;
	wire	[                  31:0]	tb_px_data;
	wire	[                  31:0]	tb_py_data;
	
	bram_1rw_1ro_readfirst # (.MEM_WIDTH(32), .MEM_ADDR_BITS(WORD_COUNTER_WIDTH))
	bram_k
	(	.clk(clk),
		.a_addr(tb_k_addr), .a_wr(tb_k_wren), .a_in(tb_k_data), .a_out(),
		.b_addr(core_k_addr), .b_out(core_k_data)
	);
	
	bram_1rw_1ro_readfirst # (.MEM_WIDTH(32), .MEM_ADDR_BITS(WORD_COUNTER_WIDTH))
	bram_px
	(	.clk(clk),
		.a_addr(core_px_addr), .a_wr(core_px_wren), .a_in(core_px_data), .a_out(),
		.b_addr(tb_pxy_addr), .b_out(tb_px_data)
	);
	
	bram_1rw_1ro_readfirst # (.MEM_WIDTH(32), .MEM_ADDR_BITS(WORD_COUNTER_WIDTH))
	bram_py
	(	.clk(clk),
		.a_addr(core_py_addr), .a_wr(core_py_wren), .a_in(core_py_data), .a_out(),
		.b_addr(tb_pxy_addr), .b_out(tb_py_data)
	);
	
	
		//
		// UUT
		//
	curve_mul_256 uut
	(
		.clk			(clk),
		.rst_n		(rst_n),
		
		.ena			(ena),
		.rdy			(rdy),
		
		.k_addr		(core_k_addr),
		.rx_addr		(core_px_addr),
		.ry_addr		(core_py_addr),
		
		.rx_wren		(core_px_wren),
		.ry_wren		(core_py_wren),
		
		.k_din		(core_k_data),
		
		.rx_dout		(core_px_data),
		.ry_dout		(core_py_data)
	);
		
		
		//
		// Testbench Routine
		//
	reg ok = 1;
	initial begin
		
			/* initialize control inputs */
		rst_n		= 0;
		ena		= 0;
		
			/* wait for some time */
		#200;
		
			/* de-assert reset */
		rst_n		= 1;
		
			/* wait for some time */
		#100;		
		
			/* run tests */
		//test_curve_multiplier(K_1, PX_1, PY_1);
		//test_curve_multiplier(K_2, PX_2, PY_2);
		//test_curve_multiplier(K_3, PX_3, PY_3);
		//test_curve_multiplier(K_4, PX_4, PY_4);
		//test_curve_multiplier(K_5, PX_5, PY_5);
		test_curve_multiplier(K_6, PX_6, PY_6);
		
			/* print result */
		if (ok)	$display("tb_curve_multiplier_256: SUCCESS");
		else		$display("tb_curve_multiplier_256: FAILURE");
		//
		//$finish;
		//
	end
	
	
		//
		// Test Task
		//	
	reg		p_ok;
	
	integer	w;

	task test_curve_multiplier;
	
		input	[255:0]	k;
		input	[255:0]	px;
		input	[255:0]	py;
		
		reg	[255:0]	k_shreg;
		reg	[255:0]	px_shreg;
		reg	[255:0]	py_shreg;
		
		begin
		
				/* start filling memories */
			tb_k_wren = 1;
			
				/* initialize shift registers */
			k_shreg = k;
			
				/* write all the words */
			for (w=0; w<OPERAND_NUM_WORDS; w=w+1) begin
				
					/* set addresses */
				tb_k_addr = w[WORD_COUNTER_WIDTH-1:0];
				
					/* set data words */
				tb_k_data	= k_shreg[31:0];
				
					/* shift inputs */
				k_shreg = {{32{1'bX}}, k_shreg[255:32]};
				
					/* wait for 1 clock tick */
				#10;
				
			end
			
				/* wipe addresses */
			tb_k_addr = {WORD_COUNTER_WIDTH{1'bX}};
			
				/* wipe data words */
			tb_k_data = {32{1'bX}};
			
				/* stop filling memories */
			tb_k_wren = 0;
			
				/* start operation */
			ena = 1;
			
				/* clear flag */
			#10 ena = 0;
			
				/* wait for operation to complete */
			while (!rdy) #10;
			
				/* read result */
			for (w=0; w<OPERAND_NUM_WORDS; w=w+1) begin
				
					/* set address */
				tb_pxy_addr = w[WORD_COUNTER_WIDTH-1:0];
				
					/* wait for 1 clock tick */
				#10;
				
					/* store data word */
				px_shreg = {tb_px_data, px_shreg[255:32]};
				py_shreg = {tb_py_data, py_shreg[255:32]};

			end
			
				/* compare */
			p_ok =	(px_shreg == px) &&
						(py_shreg == py);

				/* display results */
			$display("test_curve_multiplier(): %s", p_ok ? "OK" : "ERROR");
			
				/* update global flag */
			ok = ok && p_ok;
		
		end
		
	endtask
	
endmodule


//------------------------------------------------------------------------------
// End-of-File
//------------------------------------------------------------------------------
