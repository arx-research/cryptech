//======================================================================
//
// tb_systolic_multiplier.v
// -----------------------------------------------------------------------------
// Testbench for systolic Montgomery multiplier.
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

module tb_systolic_multiplier;

	
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
		// Model Settings
		//
	localparam NUM_ROUNDS = 1000;
	
	
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
	
	reg	[ 3: 0]	n_num_words;


		//
		// Outputs
		//
	wire	rdy;


		//
		// Integers
		//
	integer w;
	
	
		//
		// BRAM Interfaces
		//
	wire	[ 3: 0]	core_a_addr;
	wire	[ 3: 0]	core_b_addr;
	wire	[ 3: 0]	core_n_addr;
	wire	[ 3: 0]	core_n_coeff_addr;
	wire	[ 3: 0]	core_r_addr;
	
	wire	[31: 0]	core_a_data;
	wire	[31: 0]	core_b_data;
	wire	[31: 0]	core_n_data;
	wire	[31: 0]	core_n_coeff_data;
	wire	[31: 0]	core_r_data;

	wire				core_r_wren;

	reg	[ 3: 0]	tb_abn_addr;
	reg	[ 3: 0]	tb_r_addr;

	reg	[31:0]	tb_a_data;
	reg	[31:0]	tb_b_data;
	reg	[31:0]	tb_n_data;
	reg	[31:0]	tb_n_coeff_data;
	wire	[31:0]	tb_r_data;
	
	reg				tb_abn_wren;
	

		//
		// BRAMs
		//
	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(4))
	bram_a (.clk(clk),
		.a_addr(tb_abn_addr), .a_wr(tb_abn_wren), .a_in(tb_a_data), .a_out(),
		.b_addr(core_a_addr), .b_out(core_a_data));

	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(4))
	bram_b (.clk(clk),
		.a_addr(tb_abn_addr), .a_wr(tb_abn_wren), .a_in(tb_b_data), .a_out(),
		.b_addr(core_b_addr), .b_out(core_b_data));

	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(4))
	bram_n (.clk(clk),
		.a_addr(tb_abn_addr), .a_wr(tb_abn_wren), .a_in(tb_n_data), .a_out(),
		.b_addr(core_n_addr), .b_out(core_n_data));

	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(4))
	bram_n_coeff (.clk(clk),
		.a_addr(tb_abn_addr), .a_wr(tb_abn_wren), .a_in(tb_n_coeff_data), .a_out(),
		.b_addr(core_n_coeff_addr), .b_out(core_n_coeff_data));

	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(4))
	bram_r (.clk(clk),
		.a_addr(core_r_addr), .a_wr(core_r_wren), .a_in(core_r_data), .a_out(),
		.b_addr(tb_r_addr), .b_out(tb_r_data));
		

		//
		// UUT
		//
	modexpa7_systolic_multiplier #
	(
		.OPERAND_ADDR_WIDTH		(4),	// 32 * (2**4) = 512-bit operands
		.SYSTOLIC_ARRAY_POWER	(2)	// 2 ** 2 = 4-tap array
	)
	uut
	(
		.clk						(clk), 
		.rst_n					(rst_n), 
		
		.ena						(ena), 
		.rdy						(rdy), 
		
		.reduce_only			(1'b0),
		
		.a_bram_addr			(core_a_addr), 
		.b_bram_addr			(core_b_addr), 
		.n_bram_addr			(core_n_addr), 
		.n_coeff_bram_addr	(core_n_coeff_addr), 
		.r_bram_addr			(core_r_addr), 

		.a_bram_out				(core_a_data), 
		.b_bram_out				(core_b_data), 
		.n_bram_out				(core_n_data), 
		.n_coeff_bram_out		(core_n_coeff_data), 
		
		.r_bram_in				(core_r_data), 
		.r_bram_wr				(core_r_wren), 
		
		.n_num_words			(n_num_words)
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
		
		test_systolic_multiplier_384(M_384, N_384, N_COEFF_384, FACTOR_384, COEFF_384);
		test_systolic_multiplier_512(M_512, N_512, N_COEFF_512, FACTOR_512, COEFF_512);
		
	end
      
		
		//
		// Test Tasks
		//
	task test_systolic_multiplier_384;
	
		input	[383:0] m;
		input	[383:0] n;
		input	[383:0] n_coeff;
		input	[383:0] factor;
		input [383:0] coeff;
		
		reg	[767:0] m_factor_full;
		reg	[383:0] m_factor_modulo;
		
		reg	[383:0] a;
		reg	[383:0] b;
		reg	[383:0] r;
	
		reg	[767:0] ab_full;
		reg	[383:0] ab_modulo;
				
		integer			round;
		integer			num_passed;
		integer			num_failed;
	
		begin
			
			m_factor_full = m * factor;					// m * factor
			m_factor_modulo = m_factor_full % n;		// m * factor % n
																	
			m_factor_full = m_factor_modulo * coeff;	// m * factor * coeff
			m_factor_modulo = m_factor_full % n;		// m * factor * coeff % n
			
			a = m_factor_modulo;								// start with a = m_factor...
			b = m_factor_modulo;								// ... and b = m_factor

			n_num_words = 4'd11;								// set number of words
	
			num_passed = 0;									// nothing tested so far
			num_failed = 0;									//
		
			for (round=0; round<NUM_ROUNDS; round=round+1) begin
			
					// obtain reference value of product
				ab_full  			= a * b;						// calculate product
				ab_modulo			= ab_full % n;				// reduce
	
				ab_full				= ab_modulo * coeff;		// take extra coefficient into account
				ab_modulo			= ab_full % n;				// reduce again

				write_memories_384(a, b, n, n_coeff);		// fill memories
			
				ena = 1;												// start operation
				#10;													//
				ena = 0;												// clear flag
			
				while (!rdy) #10;									// wait for operation to complete

				read_memory_384(r);								// get result from memory
								
				$display("test_systolic_multiplier_384(): round #%0d of %0d", round+1, NUM_ROUNDS);
				$display("    calculated: %x", r);
				$display("    expected:   %x", ab_modulo);
								
					// check calculated value
				if (r === ab_modulo) begin
					$display("        OK");
					num_passed = num_passed + 1;
				end else begin
					$display("        ERROR");
					num_failed = num_failed + 1;
				end

				b = ab_modulo;										// prepare for next round

			end		
		
				// final step, display results
			if (num_passed == NUM_ROUNDS)
				$display("SUCCESS: All tests passed.");
			else
				$display("FAILURE: %0d test(s) not passed.", num_failed);
		
		end
		
	endtask


		//
		// Test Tasks
		//
	task test_systolic_multiplier_512;
	
		input	[ 511:0] m;
		input	[ 511:0] n;
		input	[ 511:0] n_coeff;
		input	[ 511:0] factor;
		input [ 511:0] coeff;
		
		reg	[1023:0] m_factor_full;
		reg	[ 511:0] m_factor_modulo;
		
		reg	[ 511:0] a;
		reg	[ 511:0] b;
		reg	[ 511:0] r;
	
		reg	[1023:0] ab_full;
		reg	[ 511:0] ab_modulo;
				
		integer			round;
		integer			num_passed;
		integer			num_failed;
	
		begin
			
			m_factor_full = m * factor;					// m * factor
			m_factor_modulo = m_factor_full % n;		// m * factor % n
																	
			m_factor_full = m_factor_modulo * coeff;	// m * factor * coeff
			m_factor_modulo = m_factor_full % n;		// m * factor * coeff % n
			
			a = m_factor_modulo;								// start with a = m_factor...
			b = m_factor_modulo;								// ... and b = m_factor

			n_num_words = 4'd15;								// set number of words
	
			num_passed = 0;									// nothing tested so far
			num_failed = 0;									//
		
			for (round=0; round<NUM_ROUNDS; round=round+1) begin
			
					// obtain reference value of product
				ab_full  			= a * b;						// calculate product
				ab_modulo			= ab_full % n;				// reduce
	
				ab_full				= ab_modulo * coeff;		// take extra coefficient into account
				ab_modulo			= ab_full % n;				// reduce again

				write_memories_512(a, b, n, n_coeff);		// fill memories
			
				ena = 1;												// start operation
				#10;													//
				ena = 0;												// clear flag
			
				while (!rdy) #10;									// wait for operation to complete

				read_memory_512(r);								// get result from memory
								
				$display("test_systolic_multiplier_512(): round #%0d of %0d", round+1, NUM_ROUNDS);
				$display("    calculated: %x", r);
				$display("    expected:   %x", ab_modulo);
								
					// check calculated value
				if (r === ab_modulo) begin
					$display("        OK");
					num_passed = num_passed + 1;
				end else begin
					$display("        ERROR");
					num_failed = num_failed + 1;
				end

				b = ab_modulo;										// prepare for next round

			end		
		
				// final step, display results
			if (num_passed == NUM_ROUNDS)
				$display("SUCCESS: All tests passed.");
			else
				$display("FAILURE: %0d test(s) not passed.", num_failed);
		
		end
		
	endtask
	
	
		//
		// BRAM Writer
		//
	task write_memories_384;

		input	[383:0] a;
		input	[383:0] b;
		input	[383:0] n;
		input	[383:0] n_coeff;
		
		reg	[383:0] a_shreg;
		reg	[383:0] b_shreg;
		reg	[383:0] n_shreg;
		reg	[383:0] n_coeff_shreg;
		
		begin
			
			tb_abn_wren	= 1;														// start filling memories
			
			a_shreg       = a;													// initialize shift registers
			b_shreg       = b;													//
			n_shreg       = n;													//
			n_coeff_shreg = n_coeff;											//
			
			for (w=0; w<NUM_WORDS_384; w=w+1) begin						// write all words
				
				tb_abn_addr	= w[3:0];											// set addresses
				
				tb_a_data       = a_shreg[31:0];								// set data words
				tb_b_data       = b_shreg[31:0];								//
				tb_n_data       = n_shreg[31:0];								//
				tb_n_coeff_data = n_coeff_shreg[31:0];						//
				
				a_shreg       = {{32{1'bX}}, a_shreg[383:32]};			// shift inputs
				b_shreg       = {{32{1'bX}}, b_shreg[383:32]};			//
				n_shreg       = {{32{1'bX}}, n_shreg[383:32]};			//
				n_coeff_shreg = {{32{1'bX}}, n_coeff_shreg[383:32]};	//
				
				#10;																	// wait for 1 clock tick
				
			end
			
			tb_abn_addr	= {4{1'bX}};											// wipe addresses
			
			tb_a_data       = {32{1'bX}};										// wipe data words
			tb_b_data       = {32{1'bX}};										//
			tb_n_data       = {32{1'bX}};										//
			tb_n_coeff_data = {32{1'bX}};										//
			
			tb_abn_wren = 0;														// stop filling memories
		
		end
		
	endtask
		
		
		//
		// BRAM Writer
		//
	task write_memories_512;

		input	[511:0] a;
		input	[511:0] b;
		input	[511:0] n;
		input	[511:0] n_coeff;
		
		reg	[511:0] a_shreg;
		reg	[511:0] b_shreg;
		reg	[511:0] n_shreg;
		reg	[511:0] n_coeff_shreg;
		
		begin
			
			tb_abn_wren	= 1;														// start filling memories
			
			a_shreg       = a;													// initialize shift registers
			b_shreg       = b;													//
			n_shreg       = n;													//
			n_coeff_shreg = n_coeff;											//
			
			for (w=0; w<NUM_WORDS_512; w=w+1) begin						// write all words
				
				tb_abn_addr	= w[3:0];											// set addresses
				
				tb_a_data       = a_shreg[31:0];								// set data words
				tb_b_data       = b_shreg[31:0];								//
				tb_n_data       = n_shreg[31:0];								//
				tb_n_coeff_data = n_coeff_shreg[31:0];						//
				
				a_shreg       = {{32{1'bX}}, a_shreg[511:32]};			// shift inputs
				b_shreg       = {{32{1'bX}}, b_shreg[511:32]};			//
				n_shreg       = {{32{1'bX}}, n_shreg[511:32]};			//
				n_coeff_shreg = {{32{1'bX}}, n_coeff_shreg[511:32]};	//
				
				#10;																	// wait for 1 clock tick
				
			end
			
			tb_abn_addr	= {4{1'bX}};											// wipe addresses
			
			tb_a_data       = {32{1'bX}};										// wipe data words
			tb_b_data       = {32{1'bX}};										//
			tb_n_data       = {32{1'bX}};										//
			tb_n_coeff_data = {32{1'bX}};										//
			
			tb_abn_wren = 0;														// stop filling memories
		
		end
		
	endtask
	

		//
		// BRAM Reader
		//
	task read_memory_384;

		output	[383:0] r;
		reg		[383:0] r_shreg;
		
		begin
			
			for (w=0; w<NUM_WORDS_384; w=w+1) begin		// read result
				
				tb_r_addr = w[3:0];								// set address
				#10;													// wait for 1 clock tick
				r_shreg = {tb_r_data, r_shreg[383:32]};	// store data word

			end				
		
			tb_r_addr = {4{1'bX}};								// wipe address
			r = r_shreg;											// return

		end		
		
	endtask


		//
		// BRAM Reader
		//
	task read_memory_512;

		output	[511:0] r;
		reg		[511:0] r_shreg;
		
		begin
			
			for (w=0; w<NUM_WORDS_512; w=w+1) begin		// read result
				
				tb_r_addr = w[3:0];								// set address
				#10;													// wait for 1 clock tick
				r_shreg = {tb_r_data, r_shreg[511:32]};	// store data word

			end				
		
			tb_r_addr = {4{1'bX}};								// wipe address
			r = r_shreg;											// return

		end		
		
	endtask

endmodule


//======================================================================
// End of file
//======================================================================
