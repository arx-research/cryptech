//------------------------------------------------------------------------------
//
// tb_curve_adder_384.v
// -----------------------------------------------------------------------------
// Testbench for 384-bit curve point adder.
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

module tb_curve_adder_384;


   //
   // Test Vectors
   //
   localparam	[383:0]	PX_1	= 384'h7d51764067faaac686ee307807af5544a93e13c540cac538c7853a590102fa5fe6cdcc7791e44b76ef21c57e21df37e5;
   localparam	[383:0]	PY_1	= 384'h046ed01f219649209a4cfd43572d7bdd1f10f74d5be895a6c8da8e8edfc2601aaf7651e497b8688cf02ba5e1c7e77773;
   localparam	[383:0]	PZ_1	= 384'h27c452f26af1ece2924446b574969fd102556bd28712ff4b3eb1044c5ec23e59f958793e10a510f0aef98c3b724df1d5;

   localparam	[383:0]	RX_1	= 384'h238faca9e58d1fe1b59a24ba94ec6d9836fc360cb84e103715f1761554ff6a09b6d605a34f63ae0f995f59c162e7ff24;
   localparam	[383:0]	RY_1	= 384'h63b7c7672a310f779a2047315523788d69b823d4e97e26c3b45201b0345a95de977024e97e8648215a637727d0f17747;
   localparam	[383:0]	RZ_1	= 384'h394b5d916df6f9120c7771c750df3bc910998eb0a08daac1ca6e15ef70780fd48bf794d06f9cbe9568a2dbf4362dda86;


   localparam	[383:0]	PX_2	= 384'h8418363a2fe99e888abba4df9e4a0e55452b9e968454ffadc96b4fb8072174109755c564a7be3c2f860652315d635f56;
   localparam	[383:0]	PY_2	= 384'h2bef570ce39347040330df4ac581fcc7d9dd9deb286bd80f05257d90d6560f1b5381009a3a6f0acc1ea30a5e7cc7d8b8;
   localparam	[383:0]	PZ_2	= 384'h86a8b840bd52d4dc58e9e4323fd4b40ea9b262a8cb45f7e95f5407b7e5eacc16fe4ce70125f20b76b37900af12cbf909;

   localparam	[383:0]	RX_2	= 384'h7de41f9c2d48cd65d2cd1288c4fcdcd5bfe37575b23b8784e6091917a1c92c264a1105ce3c4ab88ca53947f35610f671;
   localparam	[383:0]	RY_2	= 384'h419decfcef28d06a824595bbcb86ff56aebe48a33ceb80f0a256b90aee8214d0d879454842457d49e00a2330cc0ccb57;
   localparam	[383:0]	RZ_2	= 384'h63843f81a3427e4dea2e3e150c16d26a4aa2af4d8037a1012e0490babdfa55606e5cea5e183cc147f2c1b377748ae58b;


   localparam	[383:0]	PX_3	= 384'hd95cd4004f9417b3bea71ac087da4849841c4fbb41b500736773f41f241414024d0025e1a4337e5b24b1fe2ba2e51a83;
   localparam	[383:0]	PY_3	= 384'h53650c78f3347a07248d6f6452dd80f5e373c7eddc810c6123c912b55da1a297e147e834547d1af4938fc139e71958f7;
   localparam	[383:0]	PZ_3	= 384'hfbcfa387106d61d1e1e5f660206189f13e0861b3eb2e880b200cae1878b8b47bd5b727b2825a5f395025bf42a571093b;

   localparam	[383:0]	RX_3	= 384'hf94e9fb67d6bac22276bb4fb4f965419f1907078236dd8520ff416e8525933055795c496d344a2e1cce480557dde88f4;
   localparam	[383:0]	RY_3	= 384'h3feb5bc25c380ead1a149ab2f5e3ac4cfeaa5d91bd66dda0f29d6e02520ea8583bb3ab744fb826c18981f7bac03a0886;
   localparam	[383:0]	RZ_3	= 384'hbc364ce11866cdeb1abdbc34b633e54fbb000090134600cb6dbff71f69e3d24843e40af30c07444530b53acb89eef49b;


   localparam	[383:0]	PX_4	= 384'hxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx;
   localparam	[383:0]	PY_4	= 384'hxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx;
   localparam	[383:0]	PZ_4	= 384'h000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000;

   localparam	[383:0]	RX_4	= 384'haa87ca22be8b05378eb1c71ef320ad746e1d3b628ba79b9859f741e082542a385502f25dbf55296c3a545e3872760ab7; // G.x
   localparam	[383:0]	RY_4	= 384'h3617de4a96262c6f5d9e98bf9292dc29f8f41dbd289a147ce9da3113b5f0b8c00a60b1ce1d7e819d7a431d7c90ea0e5f; // G.y
   localparam	[383:0]	RZ_4	= 384'h000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001;


   localparam	[383:0]	PX_5	= 384'haa87ca22be8b05378eb1c71ef320ad746e1d3b628ba79b9859f741e082542a385502f25dbf55296c3a545e3872760ab7; // G.x
   localparam	[383:0]	PY_5	= 384'h3617de4a96262c6f5d9e98bf9292dc29f8f41dbd289a147ce9da3113b5f0b8c00a60b1ce1d7e819d7a431d7c90ea0e5f; // G.y
   localparam	[383:0]	PZ_5	= 384'h000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001;

   localparam	[383:0]	RX_5	= 384'haaf06bba82e9f590e29c71c219bea51723c5893ae8b0c8cf4c117c3efb57ab8d55fa1b428155ad278b5743911b13ea8a; // H.x
   localparam	[383:0]	RY_5	= 384'hc9e821b569d9d390a26167406d6d23d6070be242d765eb831625ceec4a0f473ef59f4e30e2817e6285bce2846f15f19d; // H.y
   localparam	[383:0]	RZ_5	= 384'h000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001;


   localparam	[383:0]	PX_6	= 384'haa87ca22be8b05378eb1c71ef320ad746e1d3b628ba79b9859f741e082542a385502f25dbf55296c3a545e3872760ab7; // G.x
   localparam	[383:0]	PY_6	= 384'h3617de4a96262c6f5d9e98bf9292dc29f8f41dbd289a147ce9da3113b5f0b8c00a60b1ce1d7e819d7a431d7c90ea0e5f; // G.y
   localparam	[383:0]	PZ_6	= 384'h000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001;

   localparam	[383:0]	RX_6	= 384'h000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001;	//
   localparam	[383:0]	RY_6	= 384'h000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001;	// O
   localparam	[383:0]	RZ_6	= 384'h000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000;	//


   localparam	[383:0]	Q	= 384'hfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffeffffffff0000000000000000ffffffff;


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
   wire [ 5: 0] 		 add_uop_addr;
   wire [19: 0] 		 add_uop;

   uop_add_rom add_rom
     (
      .clk		(clk),
      .addr		(add_uop_addr),
      .data		(add_uop)
      );

   //
   // UUT
   //
   curve_dbl_add_384 uut
     (
      .clk		(clk),
      .rst_n	(rst_n),

      .ena		(ena),
      .rdy		(rdy),

      .uop_addr	(add_uop_addr),
      .uop			(add_uop),

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
   reg 				 ok = 1;
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
      test_curve_adder(PX_1,     PY_1, PZ_1, RX_1, RY_1, RZ_1);
      test_curve_adder(PX_2,     PY_2, PZ_2, RX_2, RY_2, RZ_2);
      test_curve_adder(PX_3,     PY_3, PZ_3, RX_3, RY_3, RZ_3);
      test_curve_adder(PX_4,     PY_4, PZ_4, RX_4, RY_4, RZ_4);
      test_curve_adder(PX_5,     PY_5, PZ_5, RX_5, RY_5, RZ_5);
      test_curve_adder(PX_6, Q - PY_6, PZ_6, RX_6, RY_6, RZ_6);

      /* print result */
      if (ok)	$display("tb_curve_adder_384: SUCCESS");
      else	$display("tb_curve_adder_384: FAILURE");
      //
      $finish;
      //
   end


   //
   // Test Task
   //
   reg		t_ok;

   integer	w;

   task test_curve_adder;

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
	 $display("test_curve_adder(): %s", t_ok ? "OK" : "ERROR");

	 /* update global flag */
	 ok = ok && t_ok;

      end

   endtask

endmodule


//------------------------------------------------------------------------------
// End-of-File
//------------------------------------------------------------------------------
