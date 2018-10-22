//======================================================================
//
// tb_expoentiator.v
// -----------------------------------------------------------------------------
// Testbench for Montgomery modular exponentiation block.
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

`timescale 1ns / 1ps

module tb_exponentiator;

		//
		// Test Vectors
		//
	`include "modexp_fpga_model_vectors.v";
	
		//
		// Parameters
		//
	localparam NUM_WORDS_384 = 384 / 32;
	localparam NUM_WORDS_512 = 512 / 32;
		
		//
		// Clock (100 MHz)
		//
	reg clk = 1'b0;
	always #5 clk = ~clk;
			
		//
		// Inputs
		//
	reg				rst_n;
	reg				ena;
	
	reg				crt;
	
	reg	[ 3: 0]	n_num_words;
	reg	[ 8: 0]	d_num_bits;

		//
		// Outputs
		//
	wire				rdy;

		//
		// Integers
		//
	integer w;
	
		//
		// BRAM Interfaces
		//
	wire	[ 3: 0]	core_m_addr;
	wire	[ 3: 0]	core_d_addr;
	wire	[ 3: 0]	core_f_addr;
	wire	[ 3: 0]	core_n1_addr;
	wire	[ 3: 0]	core_n2_addr;
	wire	[ 3: 0]	core_n_coeff1_addr;
	wire	[ 3: 0]	core_n_coeff2_addr;
	wire	[ 3: 0]	core_r_addr;
	
	wire	[31: 0]	core_m_data;
	wire	[31: 0]	core_d_data;
	wire	[31: 0]	core_f_data;
	wire	[31: 0]	core_n1_data;
	wire	[31: 0]	core_n2_data;
	wire	[31: 0]	core_n_coeff1_data;
	wire	[31: 0]	core_n_coeff2_data;
	wire	[31: 0]	core_r_data_in;

	wire				core_r_wren;

	reg	[ 3: 0]	tb_mdfn_addr;
	reg	[ 3: 0]	tb_r_addr;

	reg	[31:0]	tb_m_data;
	reg	[31:0]	tb_d_data;
	reg	[31:0]	tb_f_data;
	reg	[31:0]	tb_n_data;
	reg	[31:0]	tb_n_coeff_data;
	wire	[31:0]	tb_r_data;
	
	reg				tb_mdfn_wren;
	
		//
		// BRAMs
		//
	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(4))
	bram_m (.clk(clk),
		.a_addr(tb_mdfn_addr), .a_wr(tb_mdfn_wren), .a_in(tb_m_data), .a_out(),
		.b_addr(core_m_addr), .b_out(core_m_data));

	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(4))
	bram_d (.clk(clk),
		.a_addr(tb_mdfn_addr), .a_wr(tb_mdfn_wren), .a_in(tb_d_data), .a_out(),
		.b_addr(core_d_addr), .b_out(core_d_data));

	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(4))
	bram_f (.clk(clk),
		.a_addr(tb_mdfn_addr), .a_wr(tb_mdfn_wren), .a_in(tb_f_data), .a_out(),
		.b_addr(core_f_addr), .b_out(core_f_data));

	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(4))
	bram_n1 (.clk(clk),
		.a_addr(tb_mdfn_addr), .a_wr(tb_mdfn_wren), .a_in(tb_n_data), .a_out(),
		.b_addr(core_n1_addr), .b_out(core_n1_data));

	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(4))
	bram_n2 (.clk(clk),
		.a_addr(tb_mdfn_addr), .a_wr(tb_mdfn_wren), .a_in(tb_n_data), .a_out(),
		.b_addr(core_n2_addr), .b_out(core_n2_data));

	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(4))
	bram_n_coeff1 (.clk(clk),
		.a_addr(tb_mdfn_addr), .a_wr(tb_mdfn_wren), .a_in(tb_n_coeff_data), .a_out(),
		.b_addr(core_n_coeff1_addr), .b_out(core_n_coeff1_data));

	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(4))
	bram_n_coeff2 (.clk(clk),
		.a_addr(tb_mdfn_addr), .a_wr(tb_mdfn_wren), .a_in(tb_n_coeff_data), .a_out(),
		.b_addr(core_n_coeff2_addr), .b_out(core_n_coeff2_data));
		
	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(4))
	bram_r (.clk(clk),
		.a_addr(core_r_addr), .a_wr(core_r_wren), .a_in(core_r_data_in), .a_out(),
		.b_addr(tb_r_addr), .b_out(tb_r_data));

		//
		// UUT
		//
	modexpa7_exponentiator #
	(
		.OPERAND_ADDR_WIDTH		(4),	// 32 * (2**4) = 512-bit operands
		.SYSTOLIC_ARRAY_POWER	(3)	// 2 ** 2 = 4-tap systolic array
	)
	uut
	(
		.clk						(clk), 
		.rst_n					(rst_n), 
		
		.ena						(ena), 
		.rdy						(rdy), 
		
		.crt						(crt),
		
		.m_bram_addr			(core_m_addr),
		.d_bram_addr			(core_d_addr),
		.f_bram_addr			(core_f_addr),
		.n1_bram_addr			(core_n1_addr),
		.n2_bram_addr			(core_n2_addr),
		.n_coeff1_bram_addr	(core_n_coeff1_addr),
		.n_coeff2_bram_addr	(core_n_coeff2_addr),
		.r_bram_addr			(core_r_addr),

		.m_bram_out				(core_m_data), 
		.d_bram_out				(core_d_data), 
		.f_bram_out				(core_f_data), 
		.n1_bram_out			(core_n1_data), 
		.n2_bram_out			(core_n2_data), 
		.n_coeff1_bram_out	(core_n_coeff1_data), 
		.n_coeff2_bram_out	(core_n_coeff1_data), 
		
		.r_bram_in				(core_r_data_in), 
		.r_bram_wr				(core_r_wren), 
		
		.m_num_words			(n_num_words),
		.d_num_bits				(d_num_bits)
	);


		//
		// Script
		//
	initial begin

		rst_n = 1'b0;
		ena = 1'b0;
		
		#200;		
		rst_n = 1'b1;
		#100;

			// test "honest" exponentiation
		test_exponent_384(M_384, D_384, FACTOR_384, N_384, N_COEFF_384, S_384);
		test_exponent_512(M_512, D_512, FACTOR_512, N_512, N_COEFF_512, S_512);

			// test crt mode
		test_exponent_192_crt(M_384, DP_192, FACTOR_P_192, P_192, P_COEFF_192, MP_192);
		test_exponent_192_crt(M_384, DQ_192, FACTOR_Q_192, Q_192, Q_COEFF_192, MQ_192);

		test_exponent_256_crt(M_512, DP_256, FACTOR_P_256, P_256, P_COEFF_256, MP_256);
		test_exponent_256_crt(M_512, DQ_256, FACTOR_Q_256, Q_256, Q_COEFF_256, MQ_256);
		
	end
      
		
		//
		// Test Tasks
		//
	task test_exponent_384;
		//
		input	[383:0] m;
		input	[383:0] d;
		input [383:0] f;
		input	[383:0] n;
		input	[383:0] n_coeff;
		input	[383:0] s;
		reg   [383:0] r;
		//
		integer i;
		//
		begin
			//						
			n_num_words = 4'd11;								// set number of words
			d_num_bits = 9'd383;								// set number of bits
			//
			crt = 0;												// disable crt mode
			//
			write_memory_384(m, d, f, n, n_coeff);		// fill memory
			
			ena = 1;												// start operation
			#10;													//
			ena = 0;												// clear flag
			
			while (!rdy) #10;									// wait for operation to complete
			read_memory_384(r);								// get result from memory
						
			$display("    calculated: %x", r);			// display result
			$display("    expected:   %x", s);			//
							
				// check calculated value
			if (r === s) begin
				$display("        OK");
				$display("SUCCESS: Test passed.");
			end else begin
				$display("        ERROR");
				$display("FAILURE: Test not passed.");
			end
			//
		end
		//
	endtask

	task test_exponent_512;
		//
		input	[511:0] m;
		input	[511:0] d;
		input [511:0] f;
		input	[511:0] n;
		input	[511:0] n_coeff;
		input	[511:0] s;
		reg   [511:0] r;
		//
		integer i;
		//
		begin
			//						
			n_num_words = 4'd15;								// set number of words
			d_num_bits = 9'd511;								// set number of bits
			//
			crt = 0;												// disable crt mode
			//
			write_memory_512(m, d, f, n, n_coeff);		// fill memory
			
			ena = 1;												// start operation
			#10;													//
			ena = 0;												// clear flag
			
			while (!rdy) #10;									// wait for operation to complete
			read_memory_512(r);								// get result from memory
						
			$display("    calculated: %x", r);			// display result
			$display("    expected:   %x", s);			//
							
				// check calculated value
			if (r === s) begin
				$display("        OK");
				$display("SUCCESS: Test passed.");
			end else begin
				$display("        ERROR");
				$display("FAILURE: Test not passed.");
			end
			//
		end
		//
	endtask

	task test_exponent_192_crt;
		//
		input	[383:0] m;
		input	[191:0] d;
		input [191:0] f;
		input	[191:0] n;
		input	[191:0] n_coeff;
		input	[191:0] s;
		reg   [191:0] r;
		//
		integer i;
		//
		begin
			//						
			n_num_words = 4'd5;								// set number of words
			d_num_bits = 9'd191;								// set number of bits
			//
			crt = 1;												// enable crt mode
			//
			write_memory_192(m, d, f, n, n_coeff);		// fill memory
			
			ena = 1;												// start operation
			#10;													//
			ena = 0;												// clear flag
			
			while (!rdy) #10;									// wait for operation to complete
			read_memory_192(r);								// get result from memory
						
			$display("    calculated: %x", r);			// display result
			$display("    expected:   %x", s);			//
							
				// check calculated value
			if (r === s) begin
				$display("        OK");
				$display("SUCCESS: Test passed.");
			end else begin
				$display("        ERROR");
				$display("FAILURE: Test not passed.");
			end
			//
		end
		//
	endtask
	
	task test_exponent_256_crt;
		//
		input	[511:0] m;
		input	[255:0] d;
		input [255:0] f;
		input	[255:0] n;
		input	[255:0] n_coeff;
		input	[255:0] s;
		reg   [255:0] r;
		//
		integer i;
		//
		begin
			//						
			n_num_words = 4'd7;								// set number of words
			d_num_bits = 9'd255;								// set number of bits
			//
			crt = 1;												// enable crt mode
			//
			write_memory_256(m, d, f, n, n_coeff);		// fill memory
			
			ena = 1;												// start operation
			#10;													//
			ena = 0;												// clear flag
			
			while (!rdy) #10;									// wait for operation to complete
			read_memory_256(r);								// get result from memory
						
			$display("    calculated: %x", r);			// display result
			$display("    expected:   %x", s);			//
							
				// check calculated value
			if (r === s) begin
				$display("        OK");
				$display("SUCCESS: Test passed.");
			end else begin
				$display("        ERROR");
				$display("FAILURE: Test not passed.");
			end
			//
		end
		//
	endtask

	//
	// write_memory_384
	//
	task write_memory_384;
		//
		input	[383:0] m;
		input	[383:0] d;
		input	[383:0] f;
		input	[383:0] n;
		input	[383:0] n_coeff;
		reg	[383:0] m_shreg;
		reg	[383:0] f_shreg;
		reg	[383:0] d_shreg;
		reg	[383:0] n_shreg;
		reg	[383:0] n_coeff_shreg;
		//
		begin
			//
			tb_mdfn_wren	= 1;			// start filling memories
			m_shreg			= m;			// preload shift register
			d_shreg			= d;			// preload shift register
			f_shreg			= f;			// preload shift register
			n_shreg			= n;			// preload shift register
			n_coeff_shreg	= n_coeff;	// preload shift register
			//
			for (w=0; w<NUM_WORDS_384; w=w+1) begin							// write all words
				tb_mdfn_addr		= w[3:0];											// set address
				tb_m_data			= m_shreg[31:0];									// set data
				tb_d_data			= d_shreg[31:0];									// set data
				tb_f_data			= f_shreg[31:0];									// set data
				tb_n_data			= n_shreg[31:0];									// set data
				tb_n_coeff_data	= n_coeff_shreg[31:0];							// set data
				m_shreg				= {{32{1'bX}}, m_shreg[383:32]};				// update shift register
				d_shreg				= {{32{1'bX}}, d_shreg[383:32]};				// update shift register
				f_shreg				= {{32{1'bX}}, f_shreg[383:32]};				// update shift register
				n_shreg				= {{32{1'bX}}, n_shreg[383:32]};				// update shift register
				n_coeff_shreg		= {{32{1'bX}}, n_coeff_shreg[383:32]};		// update shift register
				#10;																			// wait for 1 clock tick
			end
			//
			tb_mdfn_addr		= {4{1'bX}};	// wipe addresses
			tb_m_data			= {32{1'bX}};	// wipe data
			tb_d_data			= {32{1'bX}};	// wipe data
			tb_f_data			= {32{1'bX}};	// wipe data
			tb_n_data			= {32{1'bX}};	// wipe data
			tb_n_coeff_data	= {32{1'bX}};	// wipe data
			tb_mdfn_wren	= 0;				// stop filling memory
			//
		end
		//
	endtask
			

	//
	// write_memory_512
	//
	task write_memory_512;
		//
		input	[511:0] m;
		input	[511:0] d;
		input	[511:0] f;
		input	[511:0] n;
		input	[511:0] n_coeff;
		reg	[511:0] m_shreg;
		reg	[511:0] f_shreg;
		reg	[511:0] d_shreg;
		reg	[511:0] n_shreg;
		reg	[511:0] n_coeff_shreg;
		//
		begin
			//
			tb_mdfn_wren	= 1;			// start filling memories
			m_shreg			= m;			// preload shift register
			d_shreg			= d;			// preload shift register
			f_shreg			= f;			// preload shift register
			n_shreg			= n;			// preload shift register
			n_coeff_shreg	= n_coeff;	// preload shift register
			//
			for (w=0; w<NUM_WORDS_512; w=w+1) begin							// write all words
				tb_mdfn_addr		= w[3:0];											// set address
				tb_m_data			= m_shreg[31:0];									// set data
				tb_d_data			= d_shreg[31:0];									// set data
				tb_f_data			= f_shreg[31:0];									// set data
				tb_n_data			= n_shreg[31:0];									// set data
				tb_n_coeff_data	= n_coeff_shreg[31:0];							// set data
				m_shreg				= {{32{1'bX}}, m_shreg[511:32]};				// update shift register
				d_shreg				= {{32{1'bX}}, d_shreg[511:32]};				// update shift register
				f_shreg				= {{32{1'bX}}, f_shreg[511:32]};				// update shift register
				n_shreg				= {{32{1'bX}}, n_shreg[511:32]};				// update shift register
				n_coeff_shreg		= {{32{1'bX}}, n_coeff_shreg[511:32]};		// update shift register
				#10;																			// wait for 1 clock tick
			end
			//
			tb_mdfn_addr		= {4{1'bX}};	// wipe addresses
			tb_m_data			= {32{1'bX}};	// wipe data
			tb_d_data			= {32{1'bX}};	// wipe data
			tb_f_data			= {32{1'bX}};	// wipe data
			tb_n_data			= {32{1'bX}};	// wipe data
			tb_n_coeff_data	= {32{1'bX}};	// wipe data
			tb_mdfn_wren	= 0;				// stop filling memory
			//
		end
		//
	endtask


	//
	// write_memory_192
	//
	task write_memory_192;
		//
		input	[383:0] m;
		input	[191:0] d;
		input	[191:0] f;
		input	[191:0] n;
		input	[191:0] n_coeff;
		reg	[383:0] m_shreg;
		reg	[191:0] f_shreg;
		reg	[191:0] d_shreg;
		reg	[191:0] n_shreg;
		reg	[191:0] n_coeff_shreg;
		//
		begin
			//
			tb_mdfn_wren	= 1;			// start filling memories
			m_shreg			= m;			// preload shift register
			d_shreg			= d;			// preload shift register
			f_shreg			= f;			// preload shift register
			n_shreg			= n;			// preload shift register
			n_coeff_shreg	= n_coeff;	// preload shift register
			//
			for (w=0; w<NUM_WORDS_384; w=w+1) begin							// write all words
				tb_mdfn_addr		= w[3:0];											// set address
				tb_m_data			= m_shreg[31:0];									// set data
				tb_d_data			= d_shreg[31:0];									// set data
				tb_f_data			= f_shreg[31:0];									// set data
				tb_n_data			= n_shreg[31:0];									// set data
				tb_n_coeff_data	= n_coeff_shreg[31:0];							// set data
				m_shreg				= {{32{1'bX}}, m_shreg[383:32]};				// update shift register
				d_shreg				= {{32{1'bX}}, d_shreg[191:32]};				// update shift register
				f_shreg				= {{32{1'bX}}, f_shreg[191:32]};				// update shift register
				n_shreg				= {{32{1'bX}}, n_shreg[191:32]};				// update shift register
				n_coeff_shreg		= {{32{1'bX}}, n_coeff_shreg[191:32]};		// update shift register
				#10;																			// wait for 1 clock tick
			end
			//
			tb_mdfn_addr		= {4{1'bX}};	// wipe addresses
			tb_m_data			= {32{1'bX}};	// wipe data
			tb_d_data			= {32{1'bX}};	// wipe data
			tb_f_data			= {32{1'bX}};	// wipe data
			tb_n_data			= {32{1'bX}};	// wipe data
			tb_n_coeff_data	= {32{1'bX}};	// wipe data
			tb_mdfn_wren	= 0;				// stop filling memory
			//
		end
		//
	endtask


		//
		// write_memory_256
		//
	task write_memory_256;
		//
		input	[511:0] m;
		input	[255:0] d;
		input	[255:0] f;
		input	[255:0] n;
		input	[255:0] n_coeff;
		reg	[511:0] m_shreg;
		reg	[255:0] f_shreg;
		reg	[255:0] d_shreg;
		reg	[255:0] n_shreg;
		reg	[255:0] n_coeff_shreg;
		//
		begin
			//
			tb_mdfn_wren	= 1;			// start filling memories
			m_shreg			= m;			// preload shift register
			d_shreg			= d;			// preload shift register
			f_shreg			= f;			// preload shift register
			n_shreg			= n;			// preload shift register
			n_coeff_shreg	= n_coeff;	// preload shift register
			//
			for (w=0; w<NUM_WORDS_512; w=w+1) begin							// write all words
				tb_mdfn_addr		= w[3:0];											// set address
				tb_m_data			= m_shreg[31:0];									// set data
				tb_d_data			= d_shreg[31:0];									// set data
				tb_f_data			= f_shreg[31:0];									// set data
				tb_n_data			= n_shreg[31:0];									// set data
				tb_n_coeff_data	= n_coeff_shreg[31:0];							// set data
				m_shreg				= {{32{1'bX}}, m_shreg[511:32]};				// update shift register
				d_shreg				= {{32{1'bX}}, d_shreg[255:32]};				// update shift register
				f_shreg				= {{32{1'bX}}, f_shreg[255:32]};				// update shift register
				n_shreg				= {{32{1'bX}}, n_shreg[255:32]};				// update shift register
				n_coeff_shreg		= {{32{1'bX}}, n_coeff_shreg[255:32]};		// update shift register
				#10;																			// wait for 1 clock tick
			end
			//
			tb_mdfn_addr		= {4{1'bX}};	// wipe addresses
			tb_m_data			= {32{1'bX}};	// wipe data
			tb_d_data			= {32{1'bX}};	// wipe data
			tb_f_data			= {32{1'bX}};	// wipe data
			tb_n_data			= {32{1'bX}};	// wipe data
			tb_n_coeff_data	= {32{1'bX}};	// wipe data
			tb_mdfn_wren	= 0;				// stop filling memory
			//
		end
		//
	endtask


	//
	// read_memory_384
	//
	task read_memory_384;
		//
		output	[383:0] r;
		reg		[383:0] r_shreg;
		//
		begin
			//
			for (w=0; w<NUM_WORDS_384; w=w+1) begin		// read result word-by-word
				tb_r_addr	= w[3:0];							// set address
				#10;													// wait for 1 clock tick
				r_shreg = {tb_r_data, r_shreg[383:32]};	// store data word
			end
			//
			tb_r_addr = {4{1'bX}};								// wipe address
			r = r_shreg;											// return
			//
		end
		//
	endtask


	//
	// read_memory_512
	//
	task read_memory_512;
		//
		output	[511:0] r;
		reg		[511:0] r_shreg;
		//
		begin
			//
			for (w=0; w<NUM_WORDS_512; w=w+1) begin		// read result word-by-word
				tb_r_addr	= w[3:0];							// set address
				#10;													// wait for 1 clock tick
				r_shreg = {tb_r_data, r_shreg[511:32]};	// store data word
			end
			//
			tb_r_addr = {4{1'bX}};								// wipe address
			r = r_shreg;											// return
			//
		end
		//
	endtask

	//
	// read_memory_192
	//
	task read_memory_192;
		//
		output	[191:0] r;
		reg		[191:0] r_shreg;
		//
		begin
			//
			for (w=0; w<NUM_WORDS_384/2; w=w+1) begin		// read result word-by-word
				tb_r_addr	= w[3:0];							// set address
				#10;													// wait for 1 clock tick
				r_shreg = {tb_r_data, r_shreg[191:32]};	// store data word
			end
			//
			tb_r_addr = {4{1'bX}};								// wipe address
			r = r_shreg;											// return
			//
		end
		//
	endtask
	
	//
	// read_memory_256
	//
	task read_memory_256;
		//
		output	[255:0] r;
		reg		[255:0] r_shreg;
		//
		begin
			//
			for (w=0; w<NUM_WORDS_512/2; w=w+1) begin		// read result word-by-word
				tb_r_addr	= w[3:0];							// set address
				#10;													// wait for 1 clock tick
				r_shreg = {tb_r_data, r_shreg[255:32]};	// store data word
			end
			//
			tb_r_addr = {4{1'bX}};								// wipe address
			r = r_shreg;											// return
			//
		end
		//
	endtask

endmodule

//======================================================================
// End of file
//======================================================================
