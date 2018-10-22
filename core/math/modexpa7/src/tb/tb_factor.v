//======================================================================
//
// tb_factor.v
// -----------------------------------------------------------------------------
// Testbench for Montgomery factor calculation block.
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

module tb_factor;

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
	
	reg	[ 3: 0]	n_num_words;

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
	wire	[ 3: 0]	core_n_addr;
	wire	[ 3: 0]	core_f_addr;
	
	wire	[31: 0]	core_n_data;
	wire	[31: 0]	core_f_data_in;

	wire				core_f_wren;

	reg	[ 3: 0]	tb_n_addr;
	reg	[ 3: 0]	tb_f_addr;

	reg	[31:0]	tb_n_data;
	wire	[31:0]	tb_f_data;
	
	reg				tb_n_wren;
	
		//
		// BRAMs
		//
	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(4))
	bram_n (.clk(clk),
		.a_addr(tb_n_addr), .a_wr(tb_n_wren), .a_in(tb_n_data), .a_out(),
		.b_addr(core_n_addr), .b_out(core_n_data));

	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(4))
	bram_f (.clk(clk),
		.a_addr(core_f_addr), .a_wr(core_f_wren), .a_in(core_f_data_in), .a_out(),
		.b_addr(tb_f_addr), .b_out(tb_f_data));

		//
		// UUT
		//
	modexpa7_factor #
	(
		.OPERAND_ADDR_WIDTH		(4)	// 32 * (2**4) = 512-bit operands
	)
	uut
	(
		.clk				(clk), 
		.rst_n			(rst_n), 
		
		.ena				(ena), 
		.rdy				(rdy), 
		
		.n_bram_addr	(core_n_addr), 
		.f_bram_addr	(core_f_addr), 

		.n_bram_out		(core_n_data), 
		
		.f_bram_in		(core_f_data_in), 
		.f_bram_wr		(core_f_wren), 
		
		.n_num_words	(n_num_words)
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
		
		test_factor_384(N_384);
		test_factor_512(N_512);
		
	end
      
		
		//
		// Test Tasks
		//
		
	task test_factor_384;
		//
		input	[383:0] n;
		reg	[383:0] f;
		reg	[383:0] factor;
		integer i;
		//
		begin
			//			
			calc_factor_384(n, f);	// calculate factor on-the-fly
			
				// make sure, that the value matches the one saved in the include file
			if (f !== FACTOR_384) begin
				$display("ERROR: Calculated factor value differs from the one in the test vector!");
				$finish;
			end
			
			
			n_num_words = 4'd11;								// set number of words
			write_memory_384(n);								// fill memory
			
			ena = 1;												// start operation
			#10;													//
			ena = 0;												// clear flag
			
			while (!rdy) #10;									// wait for operation to complete
			read_memory_384(factor);						// get result from memory
						
			$display("    calculated: %x", factor);	// display result
			$display("    expected:   %x", f);			//
							
				// check calculated value
			if (f === factor) begin
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

	task test_factor_512;
		//
		input	[511:0] n;
		reg	[511:0] f;
		reg	[511:0] factor;
		integer i;
		//
		begin
			//			
			calc_factor_512(n, f);	// calculate factor on-the-fly
			
				// make sure, that the value matches the one saved in the include file
			if (f !== FACTOR_512) begin
				$display("ERROR: Calculated factor value differs from the one in the test vector!");
				$finish;
			end
			
			
			n_num_words = 4'd15;								// set number of words
			write_memory_512(n);								// fill memory
			
			ena = 1;												// start operation
			#10;													//
			ena = 0;												// clear flag
			
			while (!rdy) #10;									// wait for operation to complete
			read_memory_512(factor);						// get result from memory
						
			$display("    calculated: %x", factor);	// display result
			$display("    expected:   %x", f);			//
							
				// check calculated value
			if (f === factor) begin
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
		input	[383:0] n;
		reg	[383:0] n_shreg;
		//
		begin
			//
			tb_n_wren	= 1;	// start filling memories
			n_shreg		= n;	// preload shift register
			//
			for (w=0; w<NUM_WORDS_384; w=w+1) begin				// write all words
				tb_n_addr	= w[3:0];									// set address
				tb_n_data	= n_shreg[31:0];							// set data
				n_shreg		= {{32{1'bX}}, n_shreg[383:32]};		// update shift register
				#10;															// wait for 1 clock tick
			end
			//
			tb_n_addr	= {4{1'bX}};	// wipe addresses
			tb_n_data	= {32{1'bX}};	// wipe data
			tb_n_wren	= 0;				// stop filling memory
			//
		end
		//
	endtask
			
			
	//
	// write_memory_512
	//
	task write_memory_512;
		//
		input	[511:0] n;
		reg	[511:0] n_shreg;
		//
		begin
			//
			tb_n_wren	= 1;	// start filling memories
			n_shreg		= n;	// preload shift register
			//
			for (w=0; w<NUM_WORDS_512; w=w+1) begin				// write all words
				tb_n_addr	= w[3:0];									// set address
				tb_n_data	= n_shreg[31:0];							// set data
				n_shreg		= {{32{1'bX}}, n_shreg[511:32]};		// update shift register
				#10;															// wait for 1 clock tick
			end
			//
			tb_n_addr	= {4{1'bX}};	// wipe addresses
			tb_n_data	= {32{1'bX}};	// wipe data
			tb_n_wren	= 0;				// stop filling memory
			//
		end
		//
	endtask


	//
	// read_memory_384
	//
	task read_memory_384;
		//
		output	[383:0] f;
		reg		[383:0] f_shreg;
		//
		begin
			//
			for (w=0; w<NUM_WORDS_384; w=w+1) begin		// read result word-by-word
				tb_f_addr	= w[3:0];							// set address
				#10;													// wait for 1 clock tick
				f_shreg = {tb_f_data, f_shreg[383:32]};	// store data word
			end
			//
			tb_f_addr = {4{1'bX}};								// wipe address
			f = f_shreg;											// return
			//
		end
		//
	endtask


	//
	// read_memory_512
	//
	task read_memory_512;
		//
		output	[511:0] f;
		reg		[511:0] f_shreg;
		//
		begin
			//
			for (w=0; w<NUM_WORDS_512; w=w+1) begin		// read result word-by-word
				tb_f_addr	= w[3:0];							// set address
				#10;													// wait for 1 clock tick
				f_shreg = {tb_f_data, f_shreg[511:32]};	// store data word
			end
			//
			tb_f_addr = {4{1'bX}};								// wipe address
			f = f_shreg;											// return
			//
		end
		//
	endtask


	//
	// calc_factor_384
	//
	task calc_factor_384;
		//
		input		[383:0] n;
		output	[383:0] factor;
		reg		[383:0] f;
		reg		[384:0] f1;
		reg		[384:0] f2;
		integer i;
		//
		begin
			//
			f = 384'd1;
			//
			for (i=0; i<2*384; i=i+1) begin
				f1 = {f, 1'b0};
				f2 = f1 - {1'b0, n};
				f = (f1 >= {1'b0, n}) ? f2 : f1;
			end
			//
			factor = f;
			//
		end
		//
	endtask


	//
	// calc_factor_512
	//
	task calc_factor_512;
		//
		input		[511:0] n;
		output	[511:0] factor;
		reg		[511:0] f;
		reg		[512:0] f1;
		reg		[512:0] f2;
		integer i;
		//
		begin
			//
			f = 512'd1;
			//
			for (i=0; i<2*512; i=i+1) begin
				f1 = {f, 1'b0};
				f2 = f1 - {1'b0, n};
				f = (f1 >= {1'b0, n}) ? f2 : f1;
			end
			//
			factor = f;
			//
		end
		//
	endtask


endmodule

//======================================================================
// End of file
//======================================================================
