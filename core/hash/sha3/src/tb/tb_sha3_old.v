//======================================================================
//
// tb_sha3.v
// -----------
// Testbench for the SHA-3 core.
//
//
// Author: Joachim Strombergson, Pernd Paysan
// Copyright (c) 2015, NORDUnet A/S
// All rights reserved.
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

module tb_sha3();

  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  parameter DEBUG = 0;

  parameter CLK_HALF_PERIOD = 2;
  parameter CLK_PERIOD = 2 * CLK_HALF_PERIOD;


  //----------------------------------------------------------------
  // Register and Wire declarations.
  //----------------------------------------------------------------
  reg [31 : 0]  cycle_ctr;
  reg [31 : 0]  error_ctr;
  reg [31 : 0]  tc_ctr;

  reg           tb_clk;
  reg           tb_reset_n;
  reg           tb_we;
  reg [7 : 0]   tb_address;
  reg [31 : 0]  tb_write_data;
  wire [31 : 0] tb_read_data;
  wire          tb_ready;


  //----------------------------------------------------------------
  // Device Under Test.
  //----------------------------------------------------------------
  sha3 dut(
           .clk(tb_clk),
           .nreset(tb_reset_n),
           .w(tb_we),
           .addr(tb_address[6 : 0]),
           .din(tb_write_data),
           .dout(tb_read_data),
           .ready(tb_ready)
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


  //----------------------------------------------------------------
  // sys_monitor
  //
  // Generates a cycle counter and displays information about
  // the dut as needed.
  //----------------------------------------------------------------
  always
    begin : sys_monitor
      #(2 * CLK_HALF_PERIOD);
      cycle_ctr = cycle_ctr + 1;
    end


  //----------------------------------------------------------------
  // dump_state()
  //
  // Dump the internal state of the DUT.
  //----------------------------------------------------------------
  task dump_state;
    begin
      $display("State of the DUT");
      $display("----------------");
      $display("st[00] = 0x%016x, st[01] = 0x%016x", dut.st[00], dut.st[01]);
      $display("st[02] = 0x%016x, st[03] = 0x%016x", dut.st[02], dut.st[03]);
      $display("st[04] = 0x%016x, st[05] = 0x%016x", dut.st[04], dut.st[05]);
      $display("st[06] = 0x%016x, st[07] = 0x%016x", dut.st[06], dut.st[07]);
      $display("st[08] = 0x%016x, st[09] = 0x%016x", dut.st[08], dut.st[09]);
      $display("st[10] = 0x%016x, st[11] = 0x%016x", dut.st[10], dut.st[11]);
      $display("st[12] = 0x%016x, st[13] = 0x%016x", dut.st[12], dut.st[13]);
      $display("st[14] = 0x%016x, st[15] = 0x%016x", dut.st[14], dut.st[15]);
      $display("st[16] = 0x%016x, st[17] = 0x%016x", dut.st[16], dut.st[17]);
      $display("st[18] = 0x%016x, st[19] = 0x%016x", dut.st[18], dut.st[19]);
      $display("st[20] = 0x%016x, st[21] = 0x%016x", dut.st[20], dut.st[21]);
      $display("st[22] = 0x%016x, st[23] = 0x%016x", dut.st[22], dut.st[23]);
      $display("st[24] = 0x%016x", dut.st[24]);
      $display("");

      $display("stn[00] = 0x%016x, stn[01] = 0x%016x", dut.stn[00], dut.stn[01]);
      $display("stn[02] = 0x%016x, stn[03] = 0x%016x", dut.stn[02], dut.stn[03]);
      $display("stn[04] = 0x%016x, stn[05] = 0x%016x", dut.stn[04], dut.stn[05]);
      $display("stn[06] = 0x%016x, stn[07] = 0x%016x", dut.stn[06], dut.stn[07]);
      $display("stn[08] = 0x%016x, stn[09] = 0x%016x", dut.stn[08], dut.stn[09]);
      $display("stn[10] = 0x%016x, stn[11] = 0x%016x", dut.stn[10], dut.stn[11]);
      $display("stn[12] = 0x%016x, stn[13] = 0x%016x", dut.stn[12], dut.stn[13]);
      $display("stn[14] = 0x%016x, stn[15] = 0x%016x", dut.stn[14], dut.stn[15]);
      $display("stn[16] = 0x%016x, stn[17] = 0x%016x", dut.stn[16], dut.stn[17]);
      $display("stn[18] = 0x%016x, stn[19] = 0x%016x", dut.stn[18], dut.stn[19]);
      $display("stn[20] = 0x%016x, stn[21] = 0x%016x", dut.stn[20], dut.stn[21]);
      $display("stn[22] = 0x%016x, stn[23] = 0x%016x", dut.stn[22], dut.stn[23]);
      $display("stn[24] = 0x%016x", dut.stn[24]);
      $display("");

      $display("bc[00] = 0x%016x, bc[01] = 0x%016x", dut.bc[00], dut.bc[01]);
      $display("bc[02] = 0x%016x, bc[03] = 0x%016x", dut.bc[02], dut.bc[03]);
      $display("bc[04] = 0x%016x", dut.stn[04]);
      $display("");

      $display("t = 0x%016x", dut.t);
      $display("");

      $display("round = 0x%04x, roundlimit = 0x%04x", dut.round, dut.roundlimit);
      $display("");
    end
  endtask // dump_state


  //----------------------------------------------------------------
  // reset_dut()
  //
  // Toggles reset to force the DUT into a well defined state.
  //----------------------------------------------------------------
  task reset_dut;
    begin
      $display("*** Toggle reset.");
      tb_reset_n = 0;
      #(4 * CLK_HALF_PERIOD);
      tb_reset_n = 1;
    end
  endtask // reset_dut


  //----------------------------------------------------------------
  // init_sim()
  //
  // Initialize all counters and testbed functionality as well
  // as setting the DUT inputs to defined values.
  //----------------------------------------------------------------
  task init_sim;
    begin
      cycle_ctr     = 32'h0;

      tb_clk        = 0;
      tb_reset_n    = 0;
      tb_we         = 0;
      tb_address    = 6'h0;
      tb_write_data = 32'h0;
    end
  endtask // init_dut


  //----------------------------------------------------------------
  // display_test_result()
  //
  // Display the accumulated test results.
  //----------------------------------------------------------------
  task display_test_result;
    begin
      if (error_ctr == 0)
        begin
          $display("*** All %02d test cases completed successfully.", tc_ctr);
        end
      else
        begin
          $display("*** %02d test cases completed.", tc_ctr);
          $display("*** %02d errors detected during testing.", error_ctr);
        end
    end
  endtask // display_test_result


  //----------------------------------------------------------------
  // simple_test()
  //
  // A first simple test that verifies what happens with an all
  // zero data input.
  //----------------------------------------------------------------
  task simple_test;
    reg [7 : 0] i;
    begin
      dump_state();

      for (i = 0 ; i < 64 ; i = i + 1)
        begin
          tb_write_data = {i, i, i, i};
          tb_address = i[6 : 0];
          tb_we = 1;
          #(CLK_PERIOD);
          tb_we = 0;
        end

      dump_state();
    end
  endtask // simple_test


  //----------------------------------------------------------------
  // sha3_test
  // The main test functionality.
  //----------------------------------------------------------------
  initial
    begin : sha3_test
      $display("   -- Testbench for sha3 started --");

      init_sim();
      reset_dut();
      simple_test();

      $display("   -- Testbench for sha3 done. --");
      $finish;
    end // sha3_test
endmodule // tb_sha3

//======================================================================
// EOF tb_sha3.v
//======================================================================
