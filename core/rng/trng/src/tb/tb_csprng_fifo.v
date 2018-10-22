//======================================================================
//
// tb_csprng_fifo.v
// ----------------
// Testbench for the csprng output fifo module in the Crytech trng.
//
//
// Author: Joachim Strombergson
// Copyright (c) 2015, NORDUnet A/S
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or
// without modification, are permitted provided that the following
// conditions are met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in
//    the documentation and/or other materials provided with the
//    distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//======================================================================

//------------------------------------------------------------------
// Test module.
//------------------------------------------------------------------
module tb_csprng_fifo();

  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  parameter DEBUG = 0;

  parameter CLK_HALF_PERIOD = 1;
  parameter CLK_PERIOD      = 2 * CLK_HALF_PERIOD;


  //----------------------------------------------------------------
  // Register and Wire declarations.
  //----------------------------------------------------------------
  reg [31 : 0] cycle_ctr;
  reg [31 : 0] error_ctr;
  reg [31 : 0] tc_ctr;

  reg           tb_clk;
  reg           tb_reset_n;

  reg           tb_cs;
  reg           tb_we;
  reg [7 : 0]   tb_address;
  reg [31 : 0]  tb_write_data;
  wire [31 : 0] tb_read_data;
  wire          tb_error;

  reg [511 : 0] tb_csprng_data;
  reg           tb_csprng_data_valid;
  reg           tb_discard;
  wire          tb_more_data;
  wire          tb_rnd_syn;
  wire [31 : 0] tb_rnd_data;
  reg           tb_rnd_ack;

  reg [7 : 0]   i;


  //----------------------------------------------------------------
  // Device Under Test.
  //----------------------------------------------------------------
  trng_csprng_fifo dut(
                       .clk(tb_clk),
                       .reset_n(tb_reset_n),

                       .csprng_data(tb_csprng_data),
                       .csprng_data_valid(tb_csprng_data_valid),
                       .discard(tb_discard),
                       .more_data(tb_more_data),

                       .rnd_syn(tb_rnd_syn),
                       .rnd_data(tb_rnd_data),
                       .rnd_ack(tb_rnd_ack)
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

      #(CLK_PERIOD);

      if (DEBUG)
        begin
          dump_dut_state();
        end
    end


  //----------------------------------------------------------------
  // dump_dut_state()
  //
  // Dump the state of the dump when needed.
  //----------------------------------------------------------------
  task dump_dut_state;
    begin
      $display("cycle: 0x%016x", cycle_ctr);
      $display("State of DUT");
      $display("------------");
      $display("inputs:");
      $display("more_data = 0x%01x, data_valid = 0x%01x",
               tb_more_data, tb_csprng_data_valid);
      $display("input_data = 0x%0128x", tb_csprng_data);
      $display("");
      $display("outputs:");
      $display("rnd_syn = 0x%01x, rnd_ack = 0x%01x, rnd_data = 0x%08x",
               tb_rnd_syn, tb_rnd_ack, tb_rnd_data);
      $display("");
      $display("internals:");
      $display("rd_ptr = 0x%02x, wr_ptr = 0x%02x, fifo_ctr = 0x%02x, mux_ptr = 0x%02x",
               dut.rd_ptr_reg, dut.wr_ptr_reg, dut.fifo_ctr_reg, dut.mux_data_ptr_reg);
      $display("fifo_empty = 0x%01x, fifo_full = 0x%01x", dut.fifo_empty, dut.fifo_full);
      $display("");
    end
  endtask // dump_dut_state


  //----------------------------------------------------------------
  // dump_fifo()
  //
  // Dump the state of the fifo when needed.
  //----------------------------------------------------------------
  task dump_fifo;
    begin
      $display("contents of the fifo");
      $display("--------------------");
      $display("fifo_mem[0] = 0x%0128x", dut.fifo_mem[0]);
      $display("fifo_mem[1] = 0x%0128x", dut.fifo_mem[1]);
      $display("fifo_mem[2] = 0x%0128x", dut.fifo_mem[2]);
      $display("fifo_mem[3] = 0x%0128x", dut.fifo_mem[3]);
      $display("");
      $display("");
    end
  endtask // dump_dut_state


  //----------------------------------------------------------------
  // reset_dut()
  //
  // Toggle reset to put the DUT into a well known state.
  //----------------------------------------------------------------
  task reset_dut;
    begin
      $display("*** Toggle reset.");
      tb_reset_n = 0;

      #(2 * CLK_PERIOD);
      tb_reset_n = 1;
      $display("");
    end
  endtask // reset_dut


  //----------------------------------------------------------------
  // display_test_results()
  //
  // Display the accumulated test results.
  //----------------------------------------------------------------
  task display_test_results;
    begin
      if (error_ctr == 0)
        begin
          $display("*** All %02d test cases completed successfully", tc_ctr);
        end
      else
        begin
          $display("*** %02d tests completed - %02d test cases did not complete successfully.",
                   tc_ctr, error_ctr);
        end
    end
  endtask // display_test_results


  //----------------------------------------------------------------
  // init_sim()
  //
  // Initialize all counters and testbed functionality as well
  // as setting the DUT inputs to defined values.
  //----------------------------------------------------------------
  task init_sim;
    begin
      cycle_ctr       = 0;
      error_ctr       = 0;
      tc_ctr          = 0;

      tb_clk          = 0;
      tb_reset_n      = 1;

      tb_cs           = 0;
      tb_we           = 0;
      tb_address      = 8'h00;
      tb_write_data   = 32'h00000000;

      tb_discard      = 0;
      tb_rnd_ack      = 0;

      for (i = 0 ; i < 16 ; i = i + 1)
        tb_csprng_data[i * 32 +: 32] = 32'h0;
      tb_csprng_data_valid = 0;
    end
  endtask // init_sim


  //----------------------------------------------------------------
  // wait_more_data()
  //
  // Wait for the DUT to signal that it wants more data.
  //----------------------------------------------------------------
  task wait_more_data;
    begin
      while (!tb_more_data)
        #(CLK_PERIOD);
    end
  endtask // wait_more_data


  //----------------------------------------------------------------
  // write_w512()
  //
  // Writes a 512 bit data word into the fifo.
  //----------------------------------------------------------------
  task write_w512(input [7 : 0] b);
    reg [511 : 0] w512;
    reg [31 : 0]  w00;
    reg [31 : 0]  w01;
    reg [31 : 0]  w02;
    reg [31 : 0]  w03;
    reg [31 : 0]  w04;
    reg [31 : 0]  w05;
    reg [31 : 0]  w06;
    reg [31 : 0]  w07;
    reg [31 : 0]  w08;
    reg [31 : 0]  w09;
    reg [31 : 0]  w10;
    reg [31 : 0]  w11;
    reg [31 : 0]  w12;
    reg [31 : 0]  w13;
    reg [31 : 0]  w14;
    reg [31 : 0]  w15;
    begin
      w00 = {(b + 8'd15), (b + 8'd15), (b + 8'd15), (b + 8'd15)};
      w01 = {(b + 8'd14), (b + 8'd14), (b + 8'd14), (b + 8'd14)};
      w02 = {(b + 8'd13), (b + 8'd13), (b + 8'd13), (b + 8'd13)};
      w03 = {(b + 8'd12), (b + 8'd12), (b + 8'd12), (b + 8'd12)};
      w04 = {(b + 8'd11), (b + 8'd11), (b + 8'd11), (b + 8'd11)};
      w05 = {(b + 8'd10), (b + 8'd10), (b + 8'd10), (b + 8'd10)};
      w06 = {(b + 8'd09), (b + 8'd09), (b + 8'd09), (b + 8'd09)};
      w07 = {(b + 8'd08), (b + 8'd08), (b + 8'd08), (b + 8'd08)};

      w08 = {(b + 8'd07), (b + 8'd07), (b + 8'd07), (b + 8'd07)};
      w09 = {(b + 8'd06), (b + 8'd06), (b + 8'd06), (b + 8'd06)};
      w10 = {(b + 8'd05), (b + 8'd05), (b + 8'd05), (b + 8'd05)};
      w11 = {(b + 8'd04), (b + 8'd04), (b + 8'd04), (b + 8'd04)};
      w12 = {(b + 8'd03), (b + 8'd03), (b + 8'd03), (b + 8'd03)};
      w13 = {(b + 8'd02), (b + 8'd02), (b + 8'd02), (b + 8'd02)};
      w14 = {(b + 8'd01), (b + 8'd01), (b + 8'd01), (b + 8'd01)};
      w15 = {(b + 8'd00), (b + 8'd00), (b + 8'd00), (b + 8'd00)};

      w512 = {w00, w01, w02, w03, w04, w05, w06, w07,
              w08, w09, w10, w11, w12, w13, w14, w15};

      wait_more_data();

      dump_dut_state();
      dump_fifo();

      $display("writing to fifo: 0x%0128x", w512);
      tb_csprng_data       = w512;
      tb_csprng_data_valid = 1;
      #(CLK_PERIOD);
      tb_csprng_data_valid = 0;
    end
  endtask // write_w512


  //----------------------------------------------------------------
  // read_w32()
  //
  // read a 32 bit data word from the fifo.
  //----------------------------------------------------------------
  task read_w32;
    begin
      $display("*** Reading from the fifo: 0x%08x", tb_rnd_data);
      tb_rnd_ack = 1;
      #(2 * CLK_PERIOD);
      tb_rnd_ack = 0;
      dump_dut_state();
    end
  endtask // read_w32


  //----------------------------------------------------------------
  // fifo_test()
  //
  // Writes a number of 512-bit words to the FIFO and then
  // extracts 32-bit words and checks that we get the correct
  // words all the time.
  //----------------------------------------------------------------
  task fifo_test;
    reg [7 : 0] i;
    reg [7 : 0] j;

    begin
      $display("*** Test of FIFO by loading known data and then reading out.");

      dump_dut_state();
      dump_fifo();

      i = 8'd0;

      // Filling up the memory with data.
      for (j = 0 ; j < 4 ; j = j + 1)
        begin
          write_w512(i);
          #(2 * CLK_PERIOD);
          i = i + 16;
        end

      dump_dut_state();
      dump_fifo();

      // Read out a number of words from the fifo.
      for (j = 0 ; j < 17 ; j = j + 1)
        begin
          read_w32();
        end

      dump_dut_state();
      dump_fifo();

      // Write another 512-bit word into the fifo.
      write_w512(8'h40);


      // Read out all of the rest of the data.
      while (tb_rnd_syn)
        read_w32();

      dump_fifo();
    end
  endtask // fifo_test


  //----------------------------------------------------------------
  // csprng_test
  //
  // The main test functionality.
  //----------------------------------------------------------------
  initial
    begin : csprng_fifo_test

      $display("   -= Testbench for csprng fifo started =-");
      $display("    ======================================");
      $display("");

      init_sim();
      dump_dut_state();
      reset_dut();
      dump_dut_state();

      #(10 * CLK_PERIOD)

      fifo_test();

      #(100 * CLK_PERIOD)

      display_test_results();

      $display("");
      $display("*** CSPRNG FIFO simulation done. ***");
      $finish;
    end // tb_csprng_fifo_test
endmodule // tb_csprng_fifo

//======================================================================
// EOF tb_csprng_fifo.v
//======================================================================
