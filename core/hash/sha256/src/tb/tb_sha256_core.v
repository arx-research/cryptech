//======================================================================
//
// tb_sha256_core.v
// ----------------
// Testbench for the SHA-256 core.
//
//
// Author: Joachim Strombergson
// Copyright (c) 2014, NORDUnet A/S
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

//------------------------------------------------------------------
// Test module.
//------------------------------------------------------------------
module tb_sha256_core();

  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  parameter DEBUG = 0;

  parameter CLK_HALF_PERIOD = 2;
  parameter CLK_PERIOD = 2 * CLK_HALF_PERIOD;


  //----------------------------------------------------------------
  // Register and Wire declarations.
  //----------------------------------------------------------------
  reg [31 : 0] cycle_ctr;
  reg [31 : 0] error_ctr;
  reg [31 : 0] tc_ctr;

  reg            tb_clk;
  reg            tb_reset_n;
  reg            tb_init;
  reg            tb_next;
  reg            tb_mode;
  reg [511 : 0]  tb_block;
  reg  [31 : 0]  tb_state_wr_data;
  reg            tb_state0_we;
  reg            tb_state1_we;
  reg            tb_state2_we;
  reg            tb_state3_we;
  reg            tb_state4_we;
  reg            tb_state5_we;
  reg            tb_state6_we;
  reg            tb_state7_we;
  wire           tb_ready;
  wire [255 : 0] tb_digest;
  wire           tb_digest_valid;


  //----------------------------------------------------------------
  // Device Under Test.
  //----------------------------------------------------------------
  sha256_core dut(
                  .clk(tb_clk),
                  .reset_n(tb_reset_n),

                  .init(tb_init),
                  .next(tb_next),
                  .mode(tb_mode),

                  .block(tb_block),

                  .state_wr_data(tb_state_wr_data),
                  .state0_we(tb_state0_we),
                  .state1_we(tb_state1_we),
                  .state2_we(tb_state2_we),
                  .state3_we(tb_state3_we),
                  .state4_we(tb_state4_we),
                  .state5_we(tb_state5_we),
                  .state6_we(tb_state6_we),
                  .state7_we(tb_state7_we),

                  .ready(tb_ready),

                  .digest(tb_digest),
                  .digest_valid(tb_digest_valid)
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


  //----------------------------------------------------------------
  // sys_monitor()
  //
  // An always running process that creates a cycle counter and
  // conditionally displays information about the DUT.
  //----------------------------------------------------------------
  always
    begin : sys_monitor
      cycle_ctr = cycle_ctr + 1;
      #(2 * CLK_HALF_PERIOD);
      if (DEBUG)
        begin
          dump_dut_state;
        end
    end


  //----------------------------------------------------------------
  // dump_dut_state()
  //
  // Dump the state of the dump when needed.
  //----------------------------------------------------------------
  task dump_dut_state;
    begin
      $display("State of DUT");
      $display("------------");
      $display("Inputs and outputs:");
      $display("init   = 0x%01x, next  = 0x%01x",
               dut.init, dut.next);
      $display("block  = 0x%0128x", dut.block);

      $display("ready  = 0x%01x, valid = 0x%01x",
               dut.ready, dut.digest_valid);
      $display("digest = 0x%064x", dut.digest);
      $display("H0_reg = 0x%08x, H1_reg = 0x%08x, H2_reg = 0x%08x, H3_reg = 0x%08x",
               dut.H0_reg, dut.H1_reg, dut.H2_reg, dut.H3_reg);
      $display("H4_reg = 0x%08x, H5_reg = 0x%08x, H6_reg = 0x%08x, H7_reg = 0x%08x",
               dut.H4_reg, dut.H5_reg, dut.H6_reg, dut.H7_reg);
      $display("");

      $display("Control signals and counter:");
      $display("sha256_ctrl_reg = 0x%02x", dut.sha256_ctrl_reg);
      $display("digest_init     = 0x%01x, digest_update = 0x%01x",
               dut.digest_init, dut.digest_update);
      $display("state_init      = 0x%01x, state_update  = 0x%01x",
               dut.state_init, dut.state_update);
      $display("first_block     = 0x%01x, ready_flag    = 0x%01x, w_init    = 0x%01x",
               dut.first_block, dut.ready_flag, dut.w_init);
      $display("t_ctr_inc       = 0x%01x, t_ctr_rst     = 0x%01x, t_ctr_reg = 0x%02x",
               dut.t_ctr_inc, dut.t_ctr_rst, dut.t_ctr_reg);
      $display("");

      $display("State registers:");
      $display("a_reg = 0x%08x, b_reg = 0x%08x, c_reg = 0x%08x, d_reg = 0x%08x",
               dut.a_reg, dut.b_reg, dut.c_reg, dut.d_reg);
      $display("e_reg = 0x%08x, f_reg = 0x%08x, g_reg = 0x%08x, h_reg = 0x%08x",
               dut.e_reg, dut.f_reg, dut.g_reg, dut.h_reg);
      $display("");
      $display("a_new = 0x%08x, b_new = 0x%08x, c_new = 0x%08x, d_new = 0x%08x",
               dut.a_new, dut.b_new, dut.c_new, dut.d_new);
      $display("e_new = 0x%08x, f_new = 0x%08x, g_new = 0x%08x, h_new = 0x%08x",
               dut.e_new, dut.f_new, dut.g_new, dut.h_new);
      $display("");

      $display("State update values:");
      $display("w  = 0x%08x, k  = 0x%08x", dut.w_data, dut.k_data);
      $display("t1_new = 0x%08x, t1_reg = 0x%08x, t2_new = 0x%08x, t2_reg = 0x%08x",
               dut.t1_new, dut.t1_reg, dut.t2_new, dut.t2_reg);
      $display("");
    end
  endtask // dump_dut_state


  //----------------------------------------------------------------
  // dump_H_state()
  //
  // Dump the state of the H registers when needed.
  //----------------------------------------------------------------
  task dump_H_state;
    begin
      $display("H0_reg = 0x%08x, H1_reg = 0x%08x, H2_reg = 0x%08x, H3_reg = 0x%08x",
               dut.H0_reg, dut.H1_reg, dut.H2_reg, dut.H3_reg);
      $display("H4_reg = 0x%08x, H5_reg = 0x%08x, H6_reg = 0x%08x, H7_reg = 0x%08x",
               dut.H4_reg, dut.H5_reg, dut.H6_reg, dut.H7_reg);
      $display("");
    end
  endtask // dump_H_state


  //----------------------------------------------------------------
  // reset_dut()
  //
  // Toggle reset to put the DUT into a well known state.
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
      cycle_ctr        = 0;
      error_ctr        = 0;
      tc_ctr           = 0;

      tb_clk           = 0;
      tb_reset_n       = 1;

      tb_init          = 0;
      tb_next          = 0;
      tb_mode          = 1;
      tb_block         = 512'h0;
      tb_state_wr_data = 32'h0;
      tb_state0_we     = 0;
      tb_state1_we     = 0;
      tb_state2_we     = 0;
      tb_state3_we     = 0;
      tb_state4_we     = 0;
      tb_state5_we     = 0;
      tb_state6_we     = 0;
      tb_state7_we     = 0;
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
          $display("*** All %02d test cases completed successfully", tc_ctr);
        end
      else
        begin
          $display("*** %02d test cases did not complete successfully.", error_ctr);
        end
    end
  endtask // display_test_result


  //----------------------------------------------------------------
  // wait_ready()
  //
  // Wait for the ready flag in the dut to be set.
  //
  // Note: It is the callers responsibility to call the function
  // when the dut is actively processing and will in fact at some
  // point set the flag.
  //----------------------------------------------------------------
  task wait_ready;
    begin
      while (!tb_ready)
        begin
          #(CLK_PERIOD);
        end
    end
  endtask // wait_ready


  //----------------------------------------------------------------
  // wait_data_valid()
  //
  // Wait for the data valid in th dut to be set.
  //
  // Note: It is the callers responsibility to call the function
  // when the dut is actively processing and will in fact at some
  // point set the flag.
  //----------------------------------------------------------------
  task wait_data_valid;
    begin
      while (!tb_digest_valid)
        begin
          #(CLK_PERIOD);
        end
    end
  endtask // wait_data_valid


  //----------------------------------------------------------------
  // single_block_test()
  //
  // Run a test case spanning a single data block.
  //----------------------------------------------------------------
  task single_block_test(input [7 : 0]   tc_number,
                         input [511 : 0] block,
                         input [255 : 0] expected);
   begin
     $display("*** TC %0d single block test case started.", tc_number);
     tc_ctr = tc_ctr + 1;

     tb_block = block;
     tb_init = 1;
     #(CLK_PERIOD);
     tb_init = 0;
     wait_ready;

     if (tb_digest == expected)
       begin
         $display("*** TC %0d successful.", tc_number);
         $display("");
       end
     else
       begin
         $display("*** ERROR: TC %0d NOT successful.", tc_number);
         $display("Expected: 0x%064x", expected);
         $display("Got:      0x%064x", tb_digest);
         $display("");

         error_ctr = error_ctr + 1;
       end
   end
  endtask // single_block_test


  //----------------------------------------------------------------
  // double_block_test()
  //
  // Run a test case spanning two data blocks. We check both
  // intermediate and final digest.
  //----------------------------------------------------------------
  task double_block_test(input [7 : 0]   tc_number,
                         input [511 : 0] block1,
                         input [255 : 0] expected1,
                         input [511 : 0] block2,
                         input [255 : 0] expected2);

     reg [255 : 0] db_digest1;
     reg           db_error;
   begin
     $display("*** TC %0d double block test case started.", tc_number);
     db_error = 0;
     tc_ctr = tc_ctr + 1;

     $display("*** TC %0d first block started.", tc_number);
     tb_block = block1;
     tb_init = 1;
     #(CLK_PERIOD);
     tb_init = 0;
     wait_ready;

     db_digest1 = tb_digest;
     $display("*** TC %0d first block done.", tc_number);

     $display("*** TC %0d second block started.", tc_number);
     tb_block = block2;
     dump_dut_state;
     tb_next = 1;
     #(CLK_PERIOD);
     tb_next = 0;
     wait_ready;

     $display("*** TC %0d second block done.", tc_number);

     if (db_digest1 == expected1)
       begin
         $display("*** TC %0d first block successful", tc_number);
         $display("");
       end
     else
       begin
         $display("*** ERROR: TC %0d first block NOT successful", tc_number);
         $display("Expected: 0x%064x", expected1);
         $display("Got:      0x%064x", db_digest1);
         $display("");
         db_error = 1;
       end

     if (tb_digest == expected2)
       begin
         $display("*** TC %0d second block successful", tc_number);
         $display("");
       end
     else
       begin
         $display("*** ERROR: TC %0d second block NOT successful", tc_number);
         $display("Expected: 0x%064x", expected2);
         $display("Got:      0x%064x", tb_digest);
         $display("");
         db_error = 1;
       end

     if (db_error)
       begin
         error_ctr = error_ctr + 1;
       end
   end
  endtask // double_block_test


  //----------------------------------------------------------------
  // state_restore_test()
  //
  // Run a test where the state for the first block is written
  // to the core and then a second block is processed. The result
  // should match the dual block test.
  //----------------------------------------------------------------
  task state_restore_test(input [7 : 0]   tc_number,
                          input [255 : 0] state,
                          input [511 : 0] block,
                          input [255 : 0] expected);

     reg           db_error;
   begin
     $display("*** TC %0d state restore block test case started.", tc_number);
     db_error = 0;
     tc_ctr = tc_ctr + 1;

     $display("*** TC %0d Writing state to core.", tc_number);

     tb_state_wr_data = state[255 : 224];
     tb_state0_we     = 1;
     #(CLK_PERIOD);
     tb_state0_we     = 0;

     tb_state_wr_data = state[223 : 192];
     tb_state1_we     = 1;
     #(CLK_PERIOD);
     tb_state1_we     = 0;

     tb_state_wr_data = state[191 : 160];
     tb_state2_we     = 1;
     #(CLK_PERIOD);
     tb_state2_we     = 0;

     tb_state_wr_data = state[159 : 128];
     tb_state3_we     = 1;
     #(CLK_PERIOD);
     tb_state3_we     = 0;

     tb_state_wr_data = state[127 : 096];
     tb_state4_we     = 1;
     #(CLK_PERIOD);
     tb_state4_we     = 0;

     tb_state_wr_data = state[095 : 0064];
     tb_state5_we     = 1;
     #(CLK_PERIOD);
     tb_state5_we     = 0;

     tb_state_wr_data = state[063 : 0032];
     tb_state6_we     = 1;
     #(CLK_PERIOD);
     tb_state6_we     = 0;

     tb_state_wr_data = state[031 : 000];
     tb_state7_we     = 1;
     #(CLK_PERIOD);
     tb_state7_we     = 0;

     #(CLK_PERIOD);

     $display("*** TC %0d block started.", tc_number);
     tb_block = block;
     tb_next = 1;
     #(CLK_PERIOD);
     tb_next = 0;
     wait_ready;
     $display("*** TC %0d block done.", tc_number);

     if (tb_digest == expected)
       begin
         $display("*** TC %0d block successful", tc_number);
         $display("");
       end
     else
       begin
         $display("*** ERROR: TC %0d block NOT successful", tc_number);
         $display("Expected: 0x%064x", expected);
         $display("Got:      0x%064x", tb_digest);
         $display("");
         db_error = 1;
       end

     if (db_error)
       begin
         error_ctr = error_ctr + 1;
       end
   end
  endtask // state_restore_test


  //----------------------------------------------------------------
  // sha256_core_test
  // The main test functionality.
  //
  // Test cases taken from:
  // http://csrc.nist.gov/groups/ST/toolkit/documents/Examples/SHA256.pdf
  //----------------------------------------------------------------
  initial
    begin : sha256_core_test
      reg [511 : 0] tc1;
      reg [255 : 0] res1;

      reg [511 : 0] tc2_1;
      reg [255 : 0] res2_1;
      reg [511 : 0] tc2_2;
      reg [255 : 0] res2_2;

      reg [255 : 0] tc3_state;
      reg [511 : 0] tc3_block;
      reg [255 : 0] tc3_res;

      $display("   -- Testbench for sha256 core started --");

      init_sim;
      dump_dut_state;
      reset_dut;
      dump_dut_state;

      // TC1: Single block message: "abc".
      tc1 = 512'h61626380000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000018;
      res1 = 256'hBA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD;
      single_block_test(1, tc1, res1);

      // TC2: Double block message.
      // "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
      tc2_1 = 512'h6162636462636465636465666465666765666768666768696768696A68696A6B696A6B6C6A6B6C6D6B6C6D6E6C6D6E6F6D6E6F706E6F70718000000000000000;
      res2_1 = 256'h85E655D6417A17953363376A624CDE5C76E09589CAC5F811CC4B32C1F20E533A;

      tc2_2 = 512'h000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001C0;
      res2_2 = 256'h248D6A61D20638B8E5C026930C3E6039A33CE45964FF2167F6ECEDD419DB06C1;
      double_block_test(2, tc2_1, res2_1, tc2_2, res2_2);

      tc3_state = 256'h85E655D6417A17953363376A624CDE5C76E09589CAC5F811CC4B32C1F20E533A;
      tc3_block = 512'h000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001C0;
      tc3_res = 256'h248D6A61D20638B8E5C026930C3E6039A33CE45964FF2167F6ECEDD419DB06C1;
      state_restore_test(3, tc3_state, tc3_block, tc3_res);

      display_test_result;
      $display("*** Simulation done.");
      $finish;
    end // sha256_core_test
endmodule // tb_sha256_core

//======================================================================
// EOF tb_sha256_core.v
//======================================================================
