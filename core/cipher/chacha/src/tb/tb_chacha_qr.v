//======================================================================
//
// tb_chacha_qr.v
// --------------
// Testbench for the Chacha stream cipher quarerround (QR) module.
//
//
// Author: Joachim Strombergson
// Copyright (c) 2018, NORDUnet A/S All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
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

module tb_chacha_qr();

  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  parameter CLK_HALF_PERIOD = 2;
  parameter CLK_PERIOD = 2 * CLK_HALF_PERIOD;


  //----------------------------------------------------------------
  // Register and Wire declarations.
  //----------------------------------------------------------------
  reg [31 : 0] cycle_ctr;
  reg [31 : 0] error_ctr;
  reg [31 : 0] tc_ctr;

  reg tb_clk;
  reg tb_reset_n;

  reg [31 : 0] a;
  reg [31 : 0] b;
  reg [31 : 0] c;
  reg [31 : 0] d;

  wire [31 : 0] a_prim;
  wire [31 : 0] b_prim;
  wire [31 : 0] c_prim;
  wire [31 : 0] d_prim;

  reg            display_cycle_ctr;
  reg            display_ctrl_and_ctrs;
  reg            display_qround;
  reg            display_state;


  //----------------------------------------------------------------
  // chacha_core device under test.
  //----------------------------------------------------------------
  chacha_qr dut(
                .clk(tb_clk),
                .reset_n(tb_reset_n),

                .a(a),
                .b(b),
                .c(c),
                .d(d),

                .a_prim(a_prim),
                .b_prim(b_prim),
                .c_prim(c_prim),
                .d_prim(d_prim)
               );


  //----------------------------------------------------------------
  // clk_gen
  //
  // Clock generator process.
  //----------------------------------------------------------------
  always
    begin : clk_gen
      #CLK_HALF_PERIOD tb_clk = !tb_clk;
    end // clk_gen


  //--------------------------------------------------------------------
  // dut_monitor
  //
  // Monitor that displays different types of information
  // every cycle depending on what flags test cases enable.
  //
  // The monitor includes a cycle counter for the testbench.
  //--------------------------------------------------------------------
  always @ (posedge tb_clk)
    begin : dut_monitor
      cycle_ctr = cycle_ctr + 1;

      $display("cycle = %08x:", cycle_ctr);
      $display("a      = 0x%08x, b      = 0x%08x, c      = 0x%08x, d      = 0x%08x",
               a, b, c, d);
      $display("a_prim = 0x%08x, b_prim = 0x%08x, c_prim = 0x%08x, d_prim = 0x%08x",
               a_prim, b_prim, c_prim, d_prim);
      $display("a0     = 0x%08x, a1     = 0x%08x",
               dut.qr.a0, dut.qr.a1);
      $display("b0     = 0x%08x, b1     = 0x%08x, b2     = 0x%08x, b3     = 0x%08x",
               dut.qr.b0, dut.qr.b1, dut.qr.b2, dut.qr.b3);
      $display("c0     = 0x%08x, c1     = 0x%08x",
               dut.qr.c0, dut.qr.c1);
      $display("d0     = 0x%08x, d1     = 0x%08x, d2     = 0x%08x, d3     = 0x%08x",
               dut.qr.d0, dut.qr.d1, dut.qr.d2, dut.qr.d3);
      $display("a0_reg = 0x%08x", dut.a0_reg);
//      $display("a1_reg = 0x%08x", dut.a1_reg);
//      $display("c0_reg = 0x%08x", dut.c0_reg);
      $display("");
    end // dut_monitor


  //----------------------------------------------------------------
  // cycle_reset()
  //
  // Cycles the reset signal on the dut.
  //----------------------------------------------------------------
  task cycle_reset;
    begin
      tb_reset_n = 0;
      #(CLK_PERIOD);

      @(negedge tb_clk)

      tb_reset_n = 1;
      #(CLK_PERIOD);
    end
  endtask // cycle_reset


  //----------------------------------------------------------------
  // init_sim()
  //
  // Set the input to the DUT to defined values.
  //----------------------------------------------------------------
  task init_sim;
    begin
      cycle_ctr         = 0;
      tb_clk            = 0;
      tb_reset_n        = 0;
      error_ctr         = 0;
    end
  endtask // init_dut


  //----------------------------------------------------------------
  // qr_tests()
  //
  // Run some simple test on the qr logic.
  // Note: Not self testing. No expected value used.
  //----------------------------------------------------------------
  task qr_tests;
    begin
      $display("*** Test of Quarterround:");
      $display("");

      $display("First test:");
      a = 32'h11223344;
      b = 32'h11223344;
      c = 32'h11223344;
      d = 32'h11223344;
      #(CLK_PERIOD * 10);
      $display("");

      if (a_prim == 32'he7a34e04 && b_prim == 32'h9a971009 &&
          c_prim == 32'hd66bc95c && d_prim == 32'h6f7d62b2)
        $display("Ok: First test case correct.");
      else
        $display("Error: Expected a_prim = e7a34e04, b_prim = 9a971009, c_prim = d66bc95c, d_prim = 6f7d62b2");
      $display("");


      $display("Second test:");
      a = 32'h55555555;
      b = 32'h55555555;
      c = 32'h55555555;
      d = 32'h55555555;
      #(CLK_PERIOD * 10);

      if (a_prim == 32'haaaabaaa && b_prim == 32'h4d5d54d5 &&
          c_prim == 32'haa9aaaa9 && d_prim == 32'h55455555)
        $display("Ok: Second test case correct.");
      else
        $display("Error: Expected a_prim = aaaabaaa, b_prim = 4d5d54d5, c_prim = aa9aaaa9, d_prim = 55455555");

    end
  endtask // qr_tests


  //----------------------------------------------------------------
  // chacha_qr_test
  //
  // The main test functionality.
  //----------------------------------------------------------------
  initial
    begin : chacha_qr_test
      $display("   -- Testbench for chacha qr started --");
      $display("");

      init_sim();
      #(CLK_PERIOD * 10);
      cycle_reset();
      #(CLK_PERIOD * 10);

      qr_tests();

      // Finish in style.
      $display("   -- Testbench for chacha qr completed --");
      $finish;
    end // chacha_core_test

endmodule // tb_chacha_qr

//======================================================================
// EOF tb_chacha_qr.v
//======================================================================
