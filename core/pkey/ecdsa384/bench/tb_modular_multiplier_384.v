//------------------------------------------------------------------------------
//
// tb_modular_multiplier_384.v
// -----------------------------------------------------------------------------
// Testbench for modular multi-word multiplier.
//
// Authors: Pavel Shatov
//
// Copyright (c) 2015-2016, NORDUnet A/S
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

module tb_modular_multiplier_384;


   //
   // Test Vectors
   //
   localparam	[383:0]	N	= 384'hfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffeffffffff0000000000000000ffffffff;

   localparam	[383:0]	X_1	= 384'haa87ca22be8b05378eb1c71ef320ad746e1d3b628ba79b9859f741e082542a385502f25dbf55296c3a545e3872760ab7;
   localparam	[383:0]	Y_1	= 384'h3617de4a96262c6f5d9e98bf9292dc29f8f41dbd289a147ce9da3113b5f0b8c00a60b1ce1d7e819d7a431d7c90ea0e5f;
   localparam	[383:0]	P_1	= 384'h332e559389c970313cb29c4b55af5783821971a99c250daf84dc5d3cc441cb0a482e90de9d3ccd96b3c8c48b2ad3f025;

   localparam	[383:0]	X_2	= 384'haaf06bba82e9f590e29c71c219bea51723c5893ae8b0c8cf4c117c3efb57ab8d55fa1b428155ad278b5743911b13ea8a;
   localparam	[383:0]	Y_2	= 384'hc9e821b569d9d390a26167406d6d23d6070be242d765eb831625ceec4a0f473ef59f4e30e2817e6285bce2846f15f19d;
   localparam	[383:0]	P_2	= 384'haa1a9db70fba0a4c034777cdcd93e8bd6e9afa1171d43bdea0a16c32da20e7ebccb2fac9676f9d67a31e6f4f69e876e5;

   localparam	[383:0]	X_3	= 384'h1fbac8eebd0cbf35640b39efe0808dd774debff20a2a329e91713baf7d7f3c3e81546d883730bee7e48678f857b02ca0;
   localparam	[383:0]	Y_3	= 384'heb213103bd68ce343365a8a4c3d4555fa385f5330203bdd76ffad1f3affb95751c132007e1b240353cb0a4cf1693bdf9;
   localparam	[383:0]	P_3	= 384'h80f70000040a44b05f3752b7d5338f87e409b868f032911bda888451c13097039d66d9e7b0e3e799b9dd613d2524b7af;

   localparam	[383:0]	X_4	= 384'ha0c27ec893092dea1e1bd2ccfed3cf945c8134ed0c9f81311a0f4a05942db8dbed8dd59f267471d5462aa14fe72de856;
   localparam	[383:0]	Y_4	= 384'h855649409815bb91424eaca5fd76c97375d575d1422ec53d343bd33b847fdf0c11569685b528ab25493015428d7cf72b;
   localparam	[383:0]	P_4	= 384'h548e8456d5b3c36557a59914af514739a92908e59ddde731b8746891ad26199de955789e7cc34bfe966e3471c2684969;


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
   // Buffers (X, Y, N, P)
   //
   wire [WORD_COUNTER_WIDTH-1:0] core_x_addr;
   wire [WORD_COUNTER_WIDTH-1:0] core_y_addr;
   wire [WORD_COUNTER_WIDTH-1:0] core_n_addr;
   wire [WORD_COUNTER_WIDTH-1:0] core_p_addr;

   wire 			 core_p_wren;

   wire [                  31:0] core_x_data;
   wire [                  31:0] core_y_data;
   wire [                  31:0] core_n_data;
   wire [                  31:0] core_p_data;

   reg [WORD_COUNTER_WIDTH-1:0]  tb_xyn_addr;
   reg [WORD_COUNTER_WIDTH-1:0]  tb_p_addr;

   reg 				 tb_xyn_wren;

   reg [                  31:0]  tb_x_data;
   reg [                  31:0]  tb_y_data;
   reg [                  31:0]  tb_n_data;
   wire [                  31:0] tb_p_data;

   bram_1rw_1ro_readfirst #
     (
      .MEM_WIDTH			(32),
      .MEM_ADDR_BITS		(WORD_COUNTER_WIDTH)
      )
   bram_x
     (
      .clk		(clk),

      .a_addr	(tb_xyn_addr),
      .a_wr		(tb_xyn_wren),
      .a_in		(tb_x_data),
      .a_out	(),

      .b_addr	(core_x_addr),
      .b_out	(core_x_data)
      );

   bram_1rw_1ro_readfirst #
     (
      .MEM_WIDTH			(32),
      .MEM_ADDR_BITS		(WORD_COUNTER_WIDTH)
      )
   bram_y
     (
      .clk		(clk),

      .a_addr	(tb_xyn_addr),
      .a_wr		(tb_xyn_wren),
      .a_in		(tb_y_data),
      .a_out	(),

      .b_addr	(core_y_addr),
      .b_out	(core_y_data)
      );

   bram_1rw_1ro_readfirst #
     (
      .MEM_WIDTH			(32),
      .MEM_ADDR_BITS		(WORD_COUNTER_WIDTH)
      )
   bram_n
     (
      .clk		(clk),

      .a_addr	(tb_xyn_addr),
      .a_wr		(tb_xyn_wren),
      .a_in		(tb_n_data),
      .a_out	(),

      .b_addr	(core_n_addr),
      .b_out	(core_n_data)
      );

   bram_1rw_1ro_readfirst #
     (
      .MEM_WIDTH			(32),
      .MEM_ADDR_BITS		(WORD_COUNTER_WIDTH)
      )
   bram_s
     (
      .clk	(clk),

      .a_addr	(core_p_addr),
      .a_wr	(core_p_wren),
      .a_in	(core_p_data),
      .a_out	(),

      .b_addr	(tb_p_addr),
      .b_out	(tb_p_data)
      );


   //
   // UUT
   //
   modular_multiplier_384 uut
     (
      .clk	(clk),
      .rst_n	(rst_n),

      .ena	(ena),
      .rdy	(rdy),

      .a_addr	(core_x_addr),
      .b_addr	(core_y_addr),
      .n_addr	(core_n_addr),
      .p_addr	(core_p_addr),
      .p_wren	(core_p_wren),

      .a_din	(core_x_data),
      .b_din	(core_y_data),
      .n_din	(core_n_data),
      .p_dout	(core_p_data)
      );


   //
   // Testbench Routine
   //
   reg			ok = 1;
   initial begin

      /* initialize control inputs */
      rst_n		= 0;
      ena		= 0;

      tb_xyn_wren	= 0;

      /* wait for some time */
      #200;

      /* de-assert reset */
      rst_n		= 1;

      /* wait for some time */
      #100;

      /* run tests */
      test_modular_multiplier_384(X_1, Y_1, N, P_1);
      test_modular_multiplier_384(X_2, Y_2, N, P_2);
      test_modular_multiplier_384(X_3, Y_3, N, P_3);
      test_modular_multiplier_384(X_4, Y_4, N, P_4);

      /* print result */
      if (ok)	$display("tb_modular_multiplier_384: SUCCESS");
      else	$display("tb_modular_multiplier_384: FAILURE");
      //
      //$finish;
      //
   end


   //
   // Test Task
   //
   reg	[383:0]	p;
   reg 		p_ok;

   integer 	w;

   reg [767:0] 	pp_full;
   reg [383:0] 	pp_ref;

   task test_modular_multiplier_384;

      input	[383:0] x;
      input [383:0] 	y;
      input [383:0] 	n;
      input [383:0] 	pp;

      reg [383:0] 	x_shreg;
      reg [383:0] 	y_shreg;
      reg [383:0] 	n_shreg;
      reg [383:0] 	p_shreg;

      begin

	 /* start filling memories */
	 tb_xyn_wren	= 1;

	 /* initialize shift registers */
	 x_shreg = x;
	 y_shreg = y;
	 n_shreg = n;

	 /* write all the words */
	 for (w=0; w<OPERAND_NUM_WORDS; w=w+1) begin

	    /* set addresses */
	    tb_xyn_addr	= w[WORD_COUNTER_WIDTH-1:0];

	    /* set data words */
	    tb_x_data	= x_shreg[31:0];
	    tb_y_data	= y_shreg[31:0];
	    tb_n_data	= n_shreg[31:0];

	    /* shift inputs */
	    x_shreg = {{32{1'bX}}, x_shreg[383:32]};
	    y_shreg = {{32{1'bX}}, y_shreg[383:32]};
	    n_shreg = {{32{1'bX}}, n_shreg[383:32]};

	    /* wait for 1 clock tick */
	    #10;

	 end

	 /* wipe addresses */
	 tb_xyn_addr	= {WORD_COUNTER_WIDTH{1'bX}};

	 /* wipe data words */
	 tb_x_data	= {32{1'bX}};
	 tb_y_data	= {32{1'bX}};
	 tb_n_data	= {32{1'bX}};

	 /* stop filling memories */
	 tb_xyn_wren	= 0;

	 /* calculate reference value */
	 pp_full = {{384{1'b0}}, x} * {{384{1'b0}}, y};
	 pp_ref = pp_full % {{384{1'b0}}, n};

	 /* compare reference value against hard-coded one */
	 if (pp_ref != pp) begin
	    $display("ERROR: pp_ref != pp");
	    $finish;
	 end

	 /* start operation */
	 ena = 1;

	 /* clear flag */
	 #10 ena = 0;

	 /* wait for operation to complete */
	 while (!rdy) #10;

	 /* read result */
	 for (w=0; w<OPERAND_NUM_WORDS; w=w+1) begin

	    /* set address */
	    tb_p_addr	= w[WORD_COUNTER_WIDTH-1:0];

	    /* wait for 1 clock tick */
	    #10;

	    /* store data word */
	    p_shreg = {tb_p_data, p_shreg[383:32]};

	 end

	 /* compare */
	 p_ok = (p_shreg == pp);

	 /* display results */
	 $display("test_modular_multiplier_384(): %s", p_ok ? "OK" : "ERROR");

	 /* update flag */
	 ok = ok && p_ok;

      end

   endtask




endmodule

//------------------------------------------------------------------------------
// End-of-File
//------------------------------------------------------------------------------
