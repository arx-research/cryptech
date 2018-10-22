//------------------------------------------------------------------------------
//
// tb_curve_multiplier_384.v
// -----------------------------------------------------------------------------
// Testbench for 384-bit curve point scalar multiplier.
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

module tb_curve_multiplier_384;


   //
   // Test Vectors
   //

		/* Q = d * G */
   localparam	[383:0]	K_1	= 384'hc838b85253ef8dc7394fa5808a5183981c7deef5a69ba8f4f2117ffea39cfcd90e95f6cbc854abacab701d50c1f3cf24;
   localparam	[383:0]	PX_1	= 384'h1fbac8eebd0cbf35640b39efe0808dd774debff20a2a329e91713baf7d7f3c3e81546d883730bee7e48678f857b02ca0;
   localparam	[383:0]	PY_1	= 384'heb213103bd68ce343365a8a4c3d4555fa385f5330203bdd76ffad1f3affb95751c132007e1b240353cb0a4cf1693bdf9;

		/* R = k * G */
   localparam	[383:0]	K_2	= 384'hdc6b44036989a196e39d1cdac000812f4bdd8b2db41bb33af51372585ebd1db63f0ce8275aa1fd45e2d2a735f8749359;
   localparam	[383:0]	PX_2	= 384'ha0c27ec893092dea1e1bd2ccfed3cf945c8134ed0c9f81311a0f4a05942db8dbed8dd59f267471d5462aa14fe72de856;
   localparam	[383:0]	PY_2	= 384'h855649409815bb91424eaca5fd76c97375d575d1422ec53d343bd33b847fdf0c11569685b528ab25493015428d7cf72b;

		/* O = n * G */
   localparam	[383:0]	K_3	= 384'hffffffffffffffffffffffffffffffffffffffffffffffffc7634d81f4372ddf581a0db248b0a77aecec196accc52973;
   localparam	[383:0]	PX_3	= 384'h000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000;
   localparam	[383:0]	PY_3	= 384'h000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000;
		
		/* H = 2 * G */
	localparam	[383:0]	K_4	= 384'h000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002;
	localparam	[383:0]	PX_4	= 384'h08d999057ba3d2d969260045c55b97f089025959a6f434d651d207d19fb96e9e4fe0e86ebe0e64f85b96a9c75295df61;
	localparam	[383:0]	PY_4	= 384'h8e80f1fa5b1b3cedb7bfe8dffd6dba74b275d875bc6cc43e904e505f256ab4255ffd43e94d39e22d61501e700a940e80;
		
		/* G = (n + 1) * G */
	localparam	[383:0]	K_5	= 384'hffffffffffffffffffffffffffffffffffffffffffffffffc7634d81f4372ddf581a0db248b0a77aecec196accc52973 + 'd1;
	localparam	[383:0]	PX_5	= 384'haa87ca22be8b05378eb1c71ef320ad746e1d3b628ba79b9859f741e082542a385502f25dbf55296c3a545e3872760ab7;
	localparam	[383:0]	PY_5	= 384'h3617de4a96262c6f5d9e98bf9292dc29f8f41dbd289a147ce9da3113b5f0b8c00a60b1ce1d7e819d7a431d7c90ea0e5f;

		/* H = (n + 2) * G */
	localparam	[383:0]	K_6	= 384'hffffffffffffffffffffffffffffffffffffffffffffffffc7634d81f4372ddf581a0db248b0a77aecec196accc52973 + 'd2;
	localparam	[383:0]	PX_6	= 384'h08d999057ba3d2d969260045c55b97f089025959a6f434d651d207d19fb96e9e4fe0e86ebe0e64f85b96a9c75295df61;
	localparam	[383:0]	PY_6	= 384'h8e80f1fa5b1b3cedb7bfe8dffd6dba74b275d875bc6cc43e904e505f256ab4255ffd43e94d39e22d61501e700a940e80;


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
   // Buffers (K, PX, PY)
   //
   wire [WORD_COUNTER_WIDTH-1:0] core_k_addr;
   wire [WORD_COUNTER_WIDTH-1:0] core_px_addr;
   wire [WORD_COUNTER_WIDTH-1:0] core_py_addr;

   wire 			 core_px_wren;
   wire 			 core_py_wren;

   wire [                32-1:0] core_k_data;
   wire [                32-1:0] core_px_data;
   wire [                32-1:0] core_py_data;

   reg [WORD_COUNTER_WIDTH-1:0]  tb_k_addr;
   reg [WORD_COUNTER_WIDTH-1:0]  tb_pxy_addr;

   reg 				 tb_k_wren;

   reg [                  31:0]  tb_k_data;
   wire [                  31:0] tb_px_data;
   wire [                  31:0] tb_py_data;

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
   curve_mul_384 uut
     (
      .clk		(clk),
      .rst_n		(rst_n),

      .ena		(ena),
      .rdy		(rdy),

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
      //test_curve_multiplier(K_1, PX_1, PY_1);
      //test_curve_multiplier(K_2, PX_2, PY_2);
      //test_curve_multiplier(K_3, PX_3, PY_3);
      //test_curve_multiplier(K_4, PX_4, PY_4);
      //test_curve_multiplier(K_5, PX_5, PY_5);
      test_curve_multiplier(K_6, PX_6, PY_6);

      /* print result */
      if (ok)	$display("tb_curve_multiplier_384: SUCCESS");
      else	$display("tb_curve_multiplier_384: FAILURE");
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

      input [383:0]	k;
      input [383:0] 	px;
      input [383:0] 	py;

      reg [383:0] 	k_shreg;
      reg [383:0] 	px_shreg;
      reg [383:0] 	py_shreg;

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
	    k_shreg = {{32{1'bX}}, k_shreg[383:32]};

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
	    px_shreg = {tb_px_data, px_shreg[383:32]};
	    py_shreg = {tb_py_data, py_shreg[383:32]};

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
