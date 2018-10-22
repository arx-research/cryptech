//------------------------------------------------------------------------------
//
// tb_curve_doubler_384.v
// -----------------------------------------------------------------------------
// Testbench for 384-bit curve point doubler.
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

module tb_curve_doubler_384;


   //
   // Test Vectors
   //
   localparam	[383:0]	PX_1	= 384'haa87ca22be8b05378eb1c71ef320ad746e1d3b628ba79b9859f741e082542a385502f25dbf55296c3a545e3872760ab7;
   localparam	[383:0]	PY_1	= 384'h3617de4a96262c6f5d9e98bf9292dc29f8f41dbd289a147ce9da3113b5f0b8c00a60b1ce1d7e819d7a431d7c90ea0e5f;
   localparam	[383:0]	PZ_1	= 384'h000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001;

   localparam	[383:0]	RX_1	= 384'he50dbe0981ef5f4f52eebd29e34e6d18d279318fe6b5d5616c54c93ea906c671b223d61ab7ba23cd59ab4f6ec5e40b94;
   localparam	[383:0]	RY_1	= 384'h5b939b5a52ac7ebed90b8c1a809fb3a5c068b421b9e0a16208c2e53b1fe41e8373333e360ad2205e9dd63d29f1e0757a;
   localparam	[383:0]	RZ_1	= 384'h6c2fbc952c4c58debb3d317f2525b853f1e83b7a513428f9d3b462276be1718014c1639c3afd033af4863af921d41cbe;


   localparam	[383:0]	PX_2	= 384'heea14dc6c53e682b16c979fcbf2b39c0f5c43efa5b412f3e9bec8251a5ff9e243f46c6ba91a1604abdd5028b56e60334;
   localparam	[383:0]	PY_2	= 384'h7ff6e6e9ed8da4aea5baa06d3a9583b6a3f206e935566659cd2025202a9e2a62e0c44e603f3a4304bfa974470f53f646;
   localparam	[383:0]	PZ_2	= 384'hcb16156bf82550274e39900f2ed4359794160a166257ae2c71c6503c129b6ce92f31277387aa1c538e7702e3658a883d;

   localparam	[383:0]	RX_2	= 384'h9455050d6d00285ae2cd25c95372b30d321fb31899f7b0e6f3b4dd557d3465edbc500cc403b076c7534a07a48d87824d;
   localparam	[383:0]	RY_2	= 384'hd0cc7a7eb6fbe9cd962efa82f89b701fbd1f8a3579066feda96c9124ee3ef2764d923a2c0039e06865442dbf3ed460ae;
   localparam	[383:0]	RZ_2	= 384'h473d297a1e5ed8b7afd51a27d5d933ab4555414ef75e1104c1fb56ba9b110f5d0d63a79e8e12edc4432c37663cc828f2;


   localparam	[383:0]	PX_3	= 384'h4af343df7a804f6b345562a471c4cea419f5e87086eaba95b7a7fa43aeb24a357d22047eea55c529fcdfb44ea80d3aab;
   localparam	[383:0]	PY_3	= 384'hfa3e2ceb637c90fde564039da3e256456a8d0995c2f69846bf3e22f2f0760a2df00f2df2f79ce2484ed5b26124d733f1;
   localparam	[383:0]	PZ_3	= 384'hdf144e3659137591d09ba17e7045f74c477e4f27e0f6c602f11306def2abeae08aee53fa40e3ca3b3ca52f9eaae47720;

   localparam	[383:0]	RX_3	= 384'h9e2aad0ad19c585e03af779d1205a3a98bf7d367c0c829f741161983dc0d24d222d5d9c2ae39399ff40d4974df22ce4d;
   localparam	[383:0]	RY_3	= 384'he0e40da86ae320396c813bb891f31f3d59be8c4957c602b56b5aa976e47d128127625b1ce7470fa33ed9e57eca5a268d;
   localparam	[383:0]	RZ_3	= 384'h836e9d9117ba665abbee7063599a8631cd42f685a8bc29b05895c2928c19f640a065cfd2cc5b0d607ea52e388be513cf;


   localparam	[383:0]	PX_4	= 384'hxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx;
   localparam	[383:0]	PY_4	= 384'hxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx;
   localparam	[383:0]	PZ_4	= 384'h000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000;

   localparam	[383:0]	RX_4	= 384'h000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001;
   localparam	[383:0]	RY_4	= 384'h000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001;
   localparam	[383:0]	RZ_4	= 384'h000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000;


   localparam	[383:0]	Q	= 384'hfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffeffffffff0000000000000000ffffffff;



   //
   // TODO: Test special cases!
   //


   //
   // Core Parameters
   //
   localparam	WORD_COUNTER_WIDTH	=  4;
   localparam	OPERAND_NUM_WORDS	= 12;


   //
   // Clock (100 MHz)
   //
   reg clk = 1'b0;
   always #5 clk = ~clk;


   //
   // Inputs, Outputs
   //
   reg rst_n;
   reg ena;
   wire rdy;


   //
   // Buffers (PX, PY, PZ, RX, RY, RZ, Q)
   //
   wire [WORD_COUNTER_WIDTH-1:0] core_px_addr;
   wire [WORD_COUNTER_WIDTH-1:0] core_py_addr;
   wire [WORD_COUNTER_WIDTH-1:0] core_pz_addr;

   wire [WORD_COUNTER_WIDTH-1:0] core_rx_addr;
   wire [WORD_COUNTER_WIDTH-1:0] core_ry_addr;
   wire [WORD_COUNTER_WIDTH-1:0] core_rz_addr;

   wire [WORD_COUNTER_WIDTH-1:0] core_q_addr;

   wire 			 core_rx_wren;
   wire 			 core_ry_wren;
   wire 			 core_rz_wren;

   wire [                32-1:0] core_px_data;
   wire [                32-1:0] core_py_data;
   wire [                32-1:0] core_pz_data;

   wire [                32-1:0] core_rx_data_wr;
   wire [                32-1:0] core_ry_data_wr;
   wire [                32-1:0] core_rz_data_wr;

   wire [                32-1:0] core_rx_data_rd;
   wire [                32-1:0] core_ry_data_rd;
   wire [                32-1:0] core_rz_data_rd;

   wire [                32-1:0] core_q_data;

   reg [WORD_COUNTER_WIDTH-1:0]  tb_xyzq_addr;
   reg 				 tb_xyzq_wren;

   reg [                  31:0]  tb_px_data;
   reg [                  31:0]  tb_py_data;
   reg [                  31:0]  tb_pz_data;
   wire [                  31:0] tb_rx_data;
   wire [                  31:0] tb_ry_data;
   wire [                  31:0] tb_rz_data;
   reg [                  31:0]  tb_q_data;

   bram_1rw_1ro_readfirst # (.MEM_WIDTH(32), .MEM_ADDR_BITS(WORD_COUNTER_WIDTH))
   bram_px
     (	.clk(clk),
	.a_addr(tb_xyzq_addr), .a_wr(tb_xyzq_wren), .a_in(tb_px_data), .a_out(),
	.b_addr(core_px_addr), .b_out(core_px_data)
	);

   bram_1rw_1ro_readfirst # (.MEM_WIDTH(32), .MEM_ADDR_BITS(WORD_COUNTER_WIDTH))
   bram_py
     (	.clk(clk),
	.a_addr(tb_xyzq_addr), .a_wr(tb_xyzq_wren), .a_in(tb_py_data), .a_out(),
	.b_addr(core_py_addr), .b_out(core_py_data)
	);

   bram_1rw_1ro_readfirst # (.MEM_WIDTH(32), .MEM_ADDR_BITS(WORD_COUNTER_WIDTH))
   bram_pz
     (	.clk(clk),
	.a_addr(tb_xyzq_addr), .a_wr(tb_xyzq_wren), .a_in(tb_pz_data), .a_out(),
	.b_addr(core_pz_addr), .b_out(core_pz_data)
	);

   bram_1rw_1ro_readfirst # (.MEM_WIDTH(32), .MEM_ADDR_BITS(WORD_COUNTER_WIDTH))
   bram_q
     (	.clk(clk),
	.a_addr(tb_xyzq_addr), .a_wr(tb_xyzq_wren), .a_in(tb_q_data), .a_out(),
	.b_addr(core_q_addr), .b_out(core_q_data)
	);

   bram_1rw_1ro_readfirst # (.MEM_WIDTH(32), .MEM_ADDR_BITS(WORD_COUNTER_WIDTH))
   bram_rx
     (	.clk(clk),
	.a_addr(core_rx_addr), .a_wr(core_rx_wren), .a_in(core_rx_data_wr), .a_out(core_rx_data_rd),
	.b_addr(tb_xyzq_addr), .b_out(tb_rx_data)
	);

   bram_1rw_1ro_readfirst # (.MEM_WIDTH(32), .MEM_ADDR_BITS(WORD_COUNTER_WIDTH))
   bram_ry
     (	.clk(clk),
	.a_addr(core_ry_addr), .a_wr(core_ry_wren), .a_in(core_ry_data_wr), .a_out(core_ry_data_rd),
	.b_addr(tb_xyzq_addr), .b_out(tb_ry_data)
	);

   bram_1rw_1ro_readfirst # (.MEM_WIDTH(32), .MEM_ADDR_BITS(WORD_COUNTER_WIDTH))
   bram_rz
     (	.clk(clk),
	.a_addr(core_rz_addr), .a_wr(core_rz_wren), .a_in(core_rz_data_wr), .a_out(core_rz_data_rd),
	.b_addr(tb_xyzq_addr), .b_out(tb_rz_data)
	);


   //
   // Opcode
   //
   wire [ 5: 0]		 dbl_uop_addr;
   wire [19: 0]		 dbl_uop;

   uop_dbl_rom dbl_rom
     (
      .clk		(clk),
      .addr		(dbl_uop_addr),
      .data		(dbl_uop)
      );


   //
   // UUT
   //
   curve_dbl_add_384 uut
     (
      .clk	(clk),
      .rst_n	(rst_n),

      .ena	(ena),
      .rdy	(rdy),

      .uop_addr	(dbl_uop_addr),
      .uop			(dbl_uop),

      .px_addr	(core_px_addr),
      .py_addr	(core_py_addr),
      .pz_addr	(core_pz_addr),
      .rx_addr	(core_rx_addr),
      .ry_addr	(core_ry_addr),
      .rz_addr	(core_rz_addr),
      .q_addr	(core_q_addr),

      .rx_wren	(core_rx_wren),
      .ry_wren	(core_ry_wren),
      .rz_wren	(core_rz_wren),

      .px_din	(core_px_data),
      .py_din	(core_py_data),
      .pz_din	(core_pz_data),
      .rx_din	(core_rx_data_rd),
      .ry_din	(core_ry_data_rd),
      .rz_din	(core_rz_data_rd),
      .rx_dout	(core_rx_data_wr),
      .ry_dout	(core_ry_data_wr),
      .rz_dout	(core_rz_data_wr),
      .q_din	(core_q_data)
      );


   //
   // Testbench Routine
   //
   reg 			 ok = 1;
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
      test_curve_doubler(PX_1, PY_1, PZ_1, RX_1, RY_1, RZ_1);
      test_curve_doubler(PX_2, PY_2, PZ_2, RX_2, RY_2, RZ_2);
      test_curve_doubler(PX_3, PY_3, PZ_3, RX_3, RY_3, RZ_3);
      test_curve_doubler(PX_4, PY_4, PZ_4, RX_4, RY_4, RZ_4);

      /* print result */
      if (ok)	$display("tb_curve_doubler_384: SUCCESS");
      else	$display("tb_curve_doubler_384: FAILURE");
      //
      //		$finish;
      //
   end


   //
   // Test Task
   //
   reg		t_ok;

   integer	w;

   task test_curve_doubler;

      input	[383:0]	px;
      input [383:0] 	py;
      input [383:0] 	pz;

      input [383:0] 	rx;
      input [383:0] 	ry;
      input [383:0] 	rz;

      reg [383:0] 	px_shreg;
      reg [383:0] 	py_shreg;
      reg [383:0] 	pz_shreg;

      reg [383:0] 	rx_shreg;
      reg [383:0] 	ry_shreg;
      reg [383:0] 	rz_shreg;

      reg [383:0] 	q_shreg;

      begin

	 /* start filling memories */
	 tb_xyzq_wren = 1;

	 /* initialize shift registers */
	 px_shreg = px;
	 py_shreg = py;
	 pz_shreg = pz;
	 q_shreg  = Q;

	 /* write all the words */
	 for (w=0; w<OPERAND_NUM_WORDS; w=w+1) begin

	    /* set addresses */
	    tb_xyzq_addr = w[WORD_COUNTER_WIDTH-1:0];

	    /* set data words */
	    tb_px_data	= px_shreg[31:0];
	    tb_py_data	= py_shreg[31:0];
	    tb_pz_data	= pz_shreg[31:0];
	    tb_q_data	= q_shreg[31:0];

	    /* shift inputs */
	    px_shreg = {{32{1'bX}}, px_shreg[383:32]};
	    py_shreg = {{32{1'bX}}, py_shreg[383:32]};
	    pz_shreg = {{32{1'bX}}, pz_shreg[383:32]};
	    q_shreg  = {{32{1'bX}}, q_shreg[383:32]};

	    /* wait for 1 clock tick */
	    #10;

	 end

	 /* wipe addresses */
	 tb_xyzq_addr = {WORD_COUNTER_WIDTH{1'bX}};

	 /* wipe data words */
	 tb_px_data = {32{1'bX}};
	 tb_py_data = {32{1'bX}};
	 tb_pz_data = {32{1'bX}};
	 tb_q_data  = {32{1'bX}};

	 /* stop filling memories */
	 tb_xyzq_wren = 0;

	 /* start operation */
	 ena = 1;

	 /* clear flag */
	 #10 ena = 0;

	 /* wait for operation to complete */
	 while (!rdy) #10;

	 /* read result */
	 for (w=0; w<OPERAND_NUM_WORDS; w=w+1) begin

	    /* set address */
	    tb_xyzq_addr = w[WORD_COUNTER_WIDTH-1:0];

	    /* wait for 1 clock tick */
	    #10;

	    /* store data word */
	    rx_shreg = {tb_rx_data, rx_shreg[383:32]};
	    ry_shreg = {tb_ry_data, ry_shreg[383:32]};
	    rz_shreg = {tb_rz_data, rz_shreg[383:32]};

	 end

	 /* compare */
	 t_ok =	(rx_shreg == rx) &&
	       (ry_shreg == ry) &&
	       (rz_shreg == rz);

	 /* display results */
	 $display("test_curve_doubler(): %s", t_ok ? "OK" : "ERROR");

	 /* update global flag */
	 ok = ok && t_ok;

      end

   endtask


endmodule

//------------------------------------------------------------------------------
// End-of-File
//------------------------------------------------------------------------------
