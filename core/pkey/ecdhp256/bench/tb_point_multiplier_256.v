//------------------------------------------------------------------------------
//
// tb_point_multiplier_256.v
// -----------------------------------------------------------------------------
// Testbench for P-256 point scalar multiplier.
//
// Authors: Pavel Shatov
//
// Copyright (c) 2018, NORDUnet A/S
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

module tb_point_multiplier_256;


		//
		// Test Vectors
		//
	`include "../../../user/shatov/ecdh_fpga_model/test_vectors/ecdh_test_vectors.v"
			
		
				
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
		// Buffers (K, PX, PY, QX, QY)
		//
	wire	[WORD_COUNTER_WIDTH-1:0]	core_k_addr;
	wire	[WORD_COUNTER_WIDTH-1:0]	core_px_addr;
	wire	[WORD_COUNTER_WIDTH-1:0]	core_py_addr;
	wire	[WORD_COUNTER_WIDTH-1:0]	core_qx_addr;
	wire	[WORD_COUNTER_WIDTH-1:0]	core_qy_addr;
	
	wire										core_qx_wren;
	wire										core_qy_wren;
	
	wire	[                32-1:0]	core_k_data;
	wire	[                32-1:0]	core_px_data;
	wire	[                32-1:0]	core_py_data;
	wire	[                32-1:0]	core_qx_data;
	wire	[                32-1:0]	core_qy_data;
	
	reg	[WORD_COUNTER_WIDTH-1:0]	tb_k_addr;
	reg	[WORD_COUNTER_WIDTH-1:0]	tb_pxy_addr;
	reg	[WORD_COUNTER_WIDTH-1:0]	tb_qxy_addr;
	
	reg										tb_k_wren;
	reg										tb_pxy_wren;
	
	reg	[                  31:0]	tb_k_data;
	reg	[                  31:0]	tb_px_data;
	reg	[                  31:0]	tb_py_data;
	wire	[                  31:0]	tb_qx_data;
	wire	[                  31:0]	tb_qy_data;
	
	bram_1rw_1ro_readfirst # (.MEM_WIDTH(32), .MEM_ADDR_BITS(WORD_COUNTER_WIDTH))
	bram_k
	(	.clk(clk),
		.a_addr(tb_k_addr), .a_wr(tb_k_wren), .a_in(tb_k_data), .a_out(),
		.b_addr(core_k_addr), .b_out(core_k_data)
	);
	
	bram_1rw_1ro_readfirst # (.MEM_WIDTH(32), .MEM_ADDR_BITS(WORD_COUNTER_WIDTH))
	bram_px
	(	.clk(clk),
		.a_addr(tb_pxy_addr), .a_wr(tb_pxy_wren), .a_in(tb_px_data), .a_out(),
		.b_addr(core_px_addr), .b_out(core_px_data)
	);
	
	bram_1rw_1ro_readfirst # (.MEM_WIDTH(32), .MEM_ADDR_BITS(WORD_COUNTER_WIDTH))
	bram_py
	(	.clk(clk),
		.a_addr(tb_pxy_addr), .a_wr(tb_pxy_wren), .a_in(tb_py_data), .a_out(),
		.b_addr(core_py_addr), .b_out(core_py_data)
	);

	bram_1rw_1ro_readfirst # (.MEM_WIDTH(32), .MEM_ADDR_BITS(WORD_COUNTER_WIDTH))
	bram_qx
	(	.clk(clk),
		.a_addr(core_qx_addr), .a_wr(core_qx_wren), .a_in(core_qx_data), .a_out(),
		.b_addr(tb_qxy_addr), .b_out(tb_qx_data)
	);
	
	bram_1rw_1ro_readfirst # (.MEM_WIDTH(32), .MEM_ADDR_BITS(WORD_COUNTER_WIDTH))
	bram_qy
	(	.clk(clk),
		.a_addr(core_qy_addr), .a_wr(core_qy_wren), .a_in(core_qy_data), .a_out(),
		.b_addr(tb_qxy_addr), .b_out(tb_qy_data)
	);
	
	
		//
		// UUT
		//
	point_mul_256 uut
	(
		.clk			(clk),
		.rst_n		(rst_n),
		
		.ena			(ena),
		.rdy			(rdy),
		
		.k_addr		(core_k_addr),
		.qx_addr		(core_px_addr),
		.qy_addr		(core_py_addr),
		.rx_addr		(core_qx_addr),
		.ry_addr		(core_qy_addr),
		
		.rx_wren		(core_qx_wren),
		.ry_wren		(core_qy_wren),
		
		.k_din		(core_k_data),
		.qx_din		(core_px_data),
		.qy_din		(core_py_data),
		.rx_dout		(core_qx_data),
		.ry_dout		(core_qy_data)
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

		$display(" 1. H = 2 * G...");
		test_point_multiplier(256'd2, P_256_G_X, P_256_G_Y, P_256_H_X, P_256_H_Y);

		$display(" 2. H = (n + 2) * G...");
		test_point_multiplier(P_256_N + 256'd2, P_256_G_X, P_256_G_Y, P_256_H_X, P_256_H_Y);

		$display(" 3. QA = dA * G...");
		test_point_multiplier(P_256_DA, P_256_G_X, P_256_G_Y, P_256_QA_X, P_256_QA_Y);

		$display(" 4. QB = dB * G...");
		test_point_multiplier(P_256_DB, P_256_G_X, P_256_G_Y, P_256_QB_X, P_256_QB_Y);
	
		$display(" 5. S = dB * QA...");
		test_point_multiplier(P_256_DB, P_256_QA_X, P_256_QA_Y, P_256_S_X, P_256_S_Y);

		$display(" 6. S = dA * QB...");
		test_point_multiplier(P_256_DA, P_256_QB_X, P_256_QB_Y, P_256_S_X, P_256_S_Y);
		
		$display(" 7. QA2 = 2 * QA...");
		test_point_multiplier(256'd2, P_256_QA_X, P_256_QA_Y, P_256_QA2_X, P_256_QA2_Y);

		$display(" 8. QA2 = (n + 2) * QA...");
		test_point_multiplier(P_256_N + 256'd2, P_256_QA_X, P_256_QA_Y, P_256_QA2_X, P_256_QA2_Y);

		$display(" 9. QB2 = 2 * QB...");
		test_point_multiplier(256'd2, P_256_QB_X, P_256_QB_Y, P_256_QB2_X, P_256_QB2_Y);

		$display("10. QB2 = (n + 2) * QB...");
		test_point_multiplier(P_256_N + 256'd2, P_256_QB_X, P_256_QB_Y, P_256_QB2_X, P_256_QB2_Y);

		
			/* print result */
		if (ok)	$display("tb_point_multiplier_256: SUCCESS");
		else		$display("tb_point_multiplier_256: FAILURE");
		//
		//$finish;
		//
	end
	
	
		//
		// Test Task
		//	
	reg		q_ok;
	
	integer	w;

	task test_point_multiplier;
	
		input	[255:0]	k;
		input	[255:0]	px;
		input	[255:0]	py;
		input [255:0]	qx;
		input	[255:0]	qy;
		
		reg	[255:0]	k_shreg;
		reg	[255:0]	px_shreg;
		reg	[255:0]	py_shreg;
		reg	[255:0]	qx_shreg;
		reg	[255:0]	qy_shreg;
		
		begin
		
				/* start filling memories */
			tb_k_wren = 1;
			tb_pxy_wren = 1;
			
				/* initialize shift registers */
			k_shreg = k;
			px_shreg = px;
			py_shreg = py;
			
				/* write all the words */
			for (w=0; w<OPERAND_NUM_WORDS; w=w+1) begin
				
					/* set addresses */
				tb_k_addr = w[WORD_COUNTER_WIDTH-1:0];
				tb_pxy_addr = w[WORD_COUNTER_WIDTH-1:0];
				
					/* set data words */
				tb_k_data	= k_shreg[31:0];
				tb_px_data	= px_shreg[31:0];
				tb_py_data	= py_shreg[31:0];
				
					/* shift inputs */
				k_shreg = {{32{1'bX}}, k_shreg[255:32]};
				px_shreg = {{32{1'bX}}, px_shreg[255:32]};
				py_shreg = {{32{1'bX}}, py_shreg[255:32]};
				
					/* wait for 1 clock tick */
				#10;
				
			end
			
				/* wipe addresses */
			tb_k_addr = {WORD_COUNTER_WIDTH{1'bX}};
			tb_pxy_addr = {WORD_COUNTER_WIDTH{1'bX}};
			
				/* wipe data words */
			tb_k_data = {32{1'bX}};
			tb_px_data = {32{1'bX}};
			tb_py_data = {32{1'bX}};
			
				/* stop filling memories */
			tb_k_wren = 0;
			tb_pxy_wren = 0;
			
				/* start operation */
			ena = 1;
			
				/* clear flag */
			#10 ena = 0;
			
				/* wait for operation to complete */
			while (!rdy) #10;
			
				/* read result */
			for (w=0; w<OPERAND_NUM_WORDS; w=w+1) begin
				
					/* set address */
				tb_qxy_addr = w[WORD_COUNTER_WIDTH-1:0];
				
					/* wait for 1 clock tick */
				#10;
				
					/* store data word */
				qx_shreg = {tb_qx_data, qx_shreg[255:32]};
				qy_shreg = {tb_qy_data, qy_shreg[255:32]};

			end
			
				/* compare */
			q_ok =	(qx_shreg == qx) &&
						(qy_shreg == qy);

				/* display results */
			$display("test_point_multiplier(): %s", q_ok ? "OK" : "ERROR");
			
				/* update global flag */
			ok = ok && q_ok;
		
		end
		
	endtask
	
endmodule


//------------------------------------------------------------------------------
// End-of-File
//------------------------------------------------------------------------------
