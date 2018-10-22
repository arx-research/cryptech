//======================================================================
//
// tb_vndecorrelator.v
// -------------------
// Testbench for the von Neumann decorrelator.
//
//
// Author: Joachim Strombergson
// Copyright (c) 2014, NORDUnet A/S All rights reserved.
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

//------------------------------------------------------------------
// Simulator directives.
//------------------------------------------------------------------
`timescale 1ns/100ps


//------------------------------------------------------------------
// Test module.
//------------------------------------------------------------------
module tb_vndecorrelator();


  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  parameter DEBUG           = 1;

  parameter CLK_HALF_PERIOD = 1;
  parameter CLK_PERIOD      = 2 * CLK_HALF_PERIOD;


  //----------------------------------------------------------------
  // Register and Wire declarations.
  //----------------------------------------------------------------
  reg [31 : 0] cycle_ctr;
  reg [31 : 0] error_ctr;
  reg [31 : 0] tc_ctr;

  reg  tb_clk;
  reg  tb_reset_n;
  reg  tb_data_in;
  reg  tb_syn_in;
  wire tb_data_out;
  wire tb_syn_out;


  //----------------------------------------------------------------
  // Device Under Test.
  //----------------------------------------------------------------
  vndecorrelator dut(
                     .clk(tb_clk),
                     .reset_n(tb_reset_n),

                     .data_in(tb_data_in),
                     .syn_in(tb_syn_in),

                     .data_out(tb_data_out),
                     .syn_out(tb_syn_out)
                    );


  //----------------------------------------------------------------
  // clk_gen
  //
  // Always running clock generator process.
  //----------------------------------------------------------------
  always
    begin : clk_gen
      #CLK_HALF_PERIOD;
      tb_clk = !tb_clk;
    end // clk_gen


  //--------------------------------------------------------------------
  // dut_monitor
  //
  // Monitor displaying information every cycle.
  // Includes the cycle counter.
  //--------------------------------------------------------------------
  always @ (posedge tb_clk)
    begin : dut_monitor
      cycle_ctr = cycle_ctr + 1;

      if (DEBUG)
        begin
          $display("cycle = %016x:", cycle_ctr);
          $display("reset_n  = 0x%01x", dut.reset_n);
          $display("data_in  = 0x%01x, syn_in  = 0x%01x", dut.data_in, dut.syn_in);
          $display("data_out = 0x%01x, syn_out = 0x%01x", dut.data_out, dut.syn_out);
          $display("ctrl     = 0x%01x", dut.vndecorr_ctrl_reg);
          $display("");
        end
    end // dut_monitor


  //----------------------------------------------------------------
  // reset_dut()
  //
  // Toggle reset to put the DUT into a well known state.
  //----------------------------------------------------------------
  task reset_dut();
    begin
      $display("*** Toggle reset.");
      tb_reset_n = 0;
      #(2 * CLK_PERIOD);
      tb_reset_n = 1;
    end
  endtask // reset_dut


  //----------------------------------------------------------------
  // init_sim()
  //
  // Initialize all counters and testbed functionality as well
  // as setting the DUT inputs to defined values.
  //----------------------------------------------------------------
  task init_sim();
    begin
      cycle_ctr = 0;
      error_ctr = 0;
      tc_ctr    = 0;

      tb_clk     = 0;
      tb_reset_n = 1;
      tb_data_in = 0;
      tb_syn_in  = 0;
    end
  endtask // init_sim


  //----------------------------------------------------------------
  // decorrelation_test
  //
  // The main test functionality.
  //----------------------------------------------------------------
  initial
    begin : decorrelation_test
      $display("   -= Testbench for the vpn Neumann decorrelator =-");
      $display("    ===============================================");
      $display("");

      init_sim();
      reset_dut();

      // TC1: 1, 0 directly after eachother. Should emit 0.
      #(10 *CLK_PERIOD);
      $display("TC1: 1 directly followed by 0. Should emit 0.");
      tb_data_in = 1;
      tb_syn_in  = 1;
      #(CLK_PERIOD);
      tb_data_in = 0;
      tb_syn_in  = 1;
      #(CLK_PERIOD);
      tb_syn_in  = 0;

      // TC2: 0, 1 directly after eachother. Should generate 1.
      #(10 *CLK_PERIOD);
      $display("TC2: 0 directly followed by 1. Should emit 1.");
      tb_data_in = 0;
      tb_syn_in  = 1;
      #(CLK_PERIOD);
      tb_data_in = 1;
      tb_syn_in  = 1;
      #(CLK_PERIOD);
      tb_syn_in  = 0;

      // TC3: 0, 0 directly after eachother. Should emit nothing.
      #(10 *CLK_PERIOD);
      $display("TC3: 0 directly followed by 0. Should emit nothing.");
      tb_data_in = 0;
      tb_syn_in  = 1;
      #(CLK_PERIOD);
      tb_data_in = 0;
      tb_syn_in  = 1;
      #(CLK_PERIOD);
      tb_syn_in  = 0;

      // TC4: 1, 1 directly after eachother. Should enmit nothing.
      #(10 *CLK_PERIOD);
      $display("TC4: 1 directly followed by 1. Should emit nothing.");
      tb_data_in = 1;
      tb_syn_in  = 1;
      #(CLK_PERIOD);
      tb_data_in = 1;
      tb_syn_in  = 1;
      #(CLK_PERIOD);
      tb_syn_in  = 0;

      // TC5: 1, 0 with 10 cycles in between. Should emit 0.
      #(10 *CLK_PERIOD);
      $display("TC5: 1 and later 0. Should emit 0.");
      tb_data_in = 1;
      tb_syn_in  = 1;
      #(CLK_PERIOD);
      tb_syn_in  = 0;
      #(10 *CLK_PERIOD);
      tb_data_in = 0;
      tb_syn_in  = 1;
      #(CLK_PERIOD);
      tb_syn_in  = 0;

      // TC6: 0, 1 with 10 cycles in between. Should emit 1.
      #(10 *CLK_PERIOD);
      $display("TC6: 0 and later 1. Should emit 1.");
      tb_data_in = 0;
      tb_syn_in  = 1;
      #(CLK_PERIOD);
      tb_syn_in  = 0;
      #(10 *CLK_PERIOD);
      tb_data_in = 1;
      tb_syn_in  = 1;
      #(CLK_PERIOD);
      tb_syn_in  = 0;

      // TC7: 0, 0 with 10 cycles in between. Should emit nothing.
      #(10 *CLK_PERIOD);
      $display("TC7: 0 and later 0. Should emit nothing.");
      tb_data_in = 0;
      tb_syn_in  = 1;
      #(CLK_PERIOD);
      tb_syn_in  = 0;
      #(10 *CLK_PERIOD);
      tb_data_in = 0;
      tb_syn_in  = 1;
      #(CLK_PERIOD);
      tb_syn_in  = 0;

      // TC8: 1, 1 with 10 cycles in between. Should emit nothing.
      #(10 *CLK_PERIOD);
      $display("TC8: 1 and later 1. Should emit nothing.");
      tb_data_in = 1;
      tb_syn_in  = 1;
      #(CLK_PERIOD);
      tb_syn_in  = 0;
      #(10 *CLK_PERIOD);
      tb_data_in = 1;
      tb_syn_in  = 1;
      #(CLK_PERIOD);
      tb_syn_in  = 0;

      $display("");
      $display("*** von Neumann decorrelation simulation done. ***");
      $finish;
    end // decorrelation_test
endmodule // tb_vndecorrelator

//======================================================================
// EOF tb_vndecorrelator.v
//======================================================================
