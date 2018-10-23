//======================================================================
//
// tb_mkmif.v
// ------------
// Testbench for the mkmif top level wrapper.
//
//
// Author: Joachim Strombergson
// Copyright (c) 2016, NORDUnet A/S All rights reserved.
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
// Compiler directives.
//------------------------------------------------------------------
`timescale 1ns/100ps

module tb_mkmif();

  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  parameter DEBUG = 1;

  parameter CLK_HALF_PERIOD = 2;
  parameter CLK_PERIOD      = 2 * CLK_HALF_PERIOD;

  localparam ADDR_NAME0        = 8'h00;
  localparam ADDR_NAME1        = 8'h01;
  localparam ADDR_VERSION      = 8'h02;
  localparam ADDR_CTRL         = 8'h08;
  localparam ADDR_STATUS       = 8'h09;
  localparam ADDR_CONFIG       = 8'h0a;
  localparam ADDR_EMEM_ADDR    = 8'h10;
  localparam ADDR_EMEM_DATA    = 8'h20;

  localparam CORE_NAME0   = 32'h6d6b6d69; // "mkmi"
  localparam CORE_NAME1   = 32'h66202020; // "f   "
  localparam CORE_VERSION = 32'h302e3130; // "0.10"


  //----------------------------------------------------------------
  // Register and Wire declarations.
  //----------------------------------------------------------------
  reg [31 : 0] cycle_ctr;
  reg [31 : 0] test_ctr;
  reg [31 : 0] error_ctr;

  reg           tb_clk;
  reg           tb_reset_n;
  wire          tb_spi_sclk;
  wire          tb_spi_cs_n;
  reg           tb_spi_do;
  wire          tb_spi_di;
  reg           tb_cs;
  reg           tb_we;
  reg [7 : 0]   tb_address;
  reg [31 : 0]  tb_write_data;
  wire [31 : 0] tb_read_data;
  reg           tb_dump_state;
  wire          tb_error;
  reg [31 : 0]  read_data;


  //----------------------------------------------------------------
  // mkmif device under test.
  //----------------------------------------------------------------
  mkmif dut(
            .clk(tb_clk),
            .reset_n(tb_reset_n),

            .spi_sclk(tb_spi_sclk),
            .spi_cs_n(tb_spi_cs_n),
            .spi_do(tb_spi_di),
            .spi_di(tb_spi_di),

            .cs(tb_cs),
            .we(tb_we),
            .address(tb_address),
            .write_data(tb_write_data),
            .read_data(tb_read_data)
           );


  //----------------------------------------------------------------
  // clk_gen
  // Clock generator process.
  //----------------------------------------------------------------
  always
    begin : clk_gen
      #CLK_HALF_PERIOD tb_clk = !tb_clk;
    end // clk_gen


  //--------------------------------------------------------------------
  // dut_monitor
  // Monitor for observing the inputs and outputs to the dut.
  // Includes the cycle counter.
  //--------------------------------------------------------------------
  always @ (posedge tb_clk)
    begin : dut_monitor
      cycle_ctr = cycle_ctr + 1;

      if (DEBUG)
        $display("cycle = %8x:", cycle_ctr);

      if (tb_dump_state)
        dump_state();
    end // dut_monitor


  //----------------------------------------------------------------
  // inc_test_ctr
  //----------------------------------------------------------------
  task inc_test_ctr;
    begin
      test_ctr = test_ctr +1;
    end
  endtask // inc_test_ctr


  //----------------------------------------------------------------
  // inc_error_ctr
  //----------------------------------------------------------------
  task inc_error_ctr;
    begin
      error_ctr = error_ctr +1;
    end
  endtask // inc_error_ctr



  //----------------------------------------------------------------
  // dump_state
  // Dump the internal MKMIF state to std out.
  //----------------------------------------------------------------
  task dump_state;
    begin

      $display("mkmif_core_ctrl_reg: 0x%02x, core ready: 0x%01x, core valid: 0x%01x",
               dut.core.mkmif_ctrl_reg, dut.core_ready, dut.core_valid);
      $display("sclk: 0x%01x, cs_n: 0x%01x, di: 0x%01x, do: 0x%01x, nxt: 0x%01x",
               tb_spi_sclk, tb_spi_cs_n, tb_spi_di, tb_spi_do, dut.core.spi.data_nxt);
      $display("spi_ctrl_reg: 0x%01x, spi_clk_ctr: 0x%04x, spi_bit_ctr: 0x%02x",
               dut.core.spi.spi_ctrl_reg, dut.core.spi.clk_ctr_reg, dut.core.spi.bit_ctr_reg);
      $display("spi length: 0x%02x, spi divisor: 0x%04x, spi set: 0x%01x, spi start: 0x%01x, spi ready: 0x%01x",
               dut.core.spi.length_reg, dut.core.spi.divisor_reg, dut.core.spi.set, dut.core.spi.start, dut.core.spi.ready);
      $display("read data: 0x%08x, write_data: 0x%014x", dut.core.spi.rd_data, dut.core.spi.wr_data);
      $display("spi data reg: 0x%014x", dut.core.spi.data_reg);
      $display("");
    end
  endtask // dump_state


  //----------------------------------------------------------------
  // tb_init
  // Initialize varibles, dut inputs at start.
  //----------------------------------------------------------------
  task tb_init;
    begin
      test_ctr      = 0;
      error_ctr     = 0;
      cycle_ctr     = 0;
      tb_clk        = 0;
      tb_reset_n    = 1;
      tb_spi_do     = 0;
      tb_cs         = 1'b0;
      tb_we         = 1'b0;
      tb_address    = 8'h00;
      tb_write_data = 32'h00;
      tb_dump_state = 1;
    end
  endtask // tb_init


  //----------------------------------------------------------------
  // toggle_reset
  // Toggle the reset.
  //----------------------------------------------------------------
  task toggle_reset;
    begin
      $display("  --- Toggling reset started.");
      dump_state();
      #(2 * CLK_PERIOD);
      tb_reset_n = 0;
      #(10 * CLK_PERIOD);
      @(negedge tb_clk)
      tb_reset_n = 1;
      dump_state();
      $display("  --- Toggling of reset done.");
      $display("");
    end
  endtask // toggle_reset


  //----------------------------------------------------------------
  // read_word()
  //
  // Read a data word from the given address in the DUT.
  // the word read will be available in the global variable
  // read_data.
  //----------------------------------------------------------------
  task read_word(input [7 : 0]  address);
    begin
      tb_address = address;
      tb_cs = 1;
      tb_we = 0;
      #(CLK_PERIOD);
      read_data = tb_read_data;
      tb_cs = 0;

      if (DEBUG)
        begin
          $display("*** Reading 0x%08x from 0x%02x.", read_data, address);
          $display("");
        end
    end
  endtask // read_word


  //----------------------------------------------------------------
  // write_word()
  //
  // Write the given word to the DUT using the DUT interface.
  //----------------------------------------------------------------
  task write_word(input [7 : 0]  address,
                  input [31 : 0] word);
    begin
      if (DEBUG)
        begin
          $display("*** Writing 0x%08x to 0x%02x.", word, address);
          $display("");
        end

      tb_address = address;
      tb_write_data = word;
      tb_cs = 1;
      tb_we = 1;
      #(CLK_PERIOD);
      tb_cs = 0;
      tb_we = 0;
    end
  endtask // write_word


  //----------------------------------------------------------------
  // wait_ready()
  //
  // Wait for ready word to be set in the DUT API.
  //----------------------------------------------------------------
  task wait_ready;
    reg ready;
    begin
      ready = 0;

      while (ready == 0)
        begin
          read_word(ADDR_STATUS);
          ready = read_data & 32'h00000001;
        end
    end
  endtask // read_word


  //----------------------------------------------------------------
  // check_name_version()
  //
  // Read the name and version from the DUT.
  //----------------------------------------------------------------
  task check_name_version;
    reg [31 : 0] name0;
    reg [31 : 0] name1;
    reg [31 : 0] version;
    begin
      inc_test_ctr();

      $display("  -- Test of reading name and version started.");

      read_word(ADDR_NAME0);
      name0 = read_data;
      read_word(ADDR_NAME1);
      name1 = read_data;
      read_word(ADDR_VERSION);
      version = read_data;

      if ((name0 == CORE_NAME0) && (name1 == CORE_NAME1) && (version == CORE_VERSION))
        $display("Correct name and version read from dut.");
      else
        begin
          inc_error_ctr();
          $display("Error:");
          $display("Got name:      %c%c%c%c%c%c%c%c",
                   name0[31 : 24], name0[23 : 16], name0[15 : 8], name0[7 : 0],
                   name1[31 : 24], name1[23 : 16], name1[15 : 8], name1[7 : 0]);
          $display("Expected name: %c%c%c%c%c%c%c%c",
                   CORE_NAME0[31 : 24], CORE_NAME0[23 : 16], CORE_NAME0[15 : 8], CORE_NAME0[7 : 0],
                   CORE_NAME1[31 : 24], CORE_NAME1[23 : 16], CORE_NAME1[15 : 8], CORE_NAME1[7 : 0]);

          $display("Got version:      %c%c%c%c",
                   version[31 : 24], version[23 : 16], version[15 : 8], version[7 : 0]);
          $display("Expected version: %c%c%c%c",
                   CORE_VERSION[31 : 24], CORE_VERSION[23 : 16], CORE_VERSION[15 : 8], CORE_VERSION[7 : 0]);

          $display("  -- Test of reading name and version done.");
          $display("");
        end
    end
  endtask // check_name_version


  //----------------------------------------------------------------
  // write_test
  //
  // Try to write a few words of data.
  //----------------------------------------------------------------
  task write_test;
    begin
      inc_test_ctr();
      $display("  -- Test of writing words to the memory started.");

      wait_ready();
      $display("Ready has been set. Starting write commands.");
      write_word(ADDR_EMEM_ADDR, 16'h0010);
      write_word(ADDR_EMEM_DATA, 32'hdeadbeef);
      write_word(ADDR_CTRL, 32'h2);
      #(10 * CLK_PERIOD);
      wait_ready();
      read_word(ADDR_EMEM_DATA);
      $display("First write completed. Read: 0x%08x", read_data);

      write_word(ADDR_EMEM_ADDR, 16'h0020);
      write_word(ADDR_EMEM_DATA, 32'haa55aa55);
      write_word(ADDR_CTRL, 32'h2);
      #(10 * CLK_PERIOD);
      wait_ready();
      read_word(ADDR_EMEM_DATA);
      $display("Second write completed. Read: 0x%08x", read_data);

//      write_word(ADDR_EMEM_ADDR, 16'h0100);
//      write_word(ADDR_EMEM_DATA, 32'h004488ff);
//      write_word(ADDR_CTRL, 32'h2);
//      #(1000 * CLK_PERIOD);
//      wait_ready();

      $display("  -- Test of writing words to the memory done.");
      $display("");
    end
  endtask // write_test


  //----------------------------------------------------------------
  // read_test
  //
  // Try to read a few words of data.
  //----------------------------------------------------------------
  task read_test;
    begin
      inc_test_ctr();
      $display("  -- Test of reading from the memory started.");

//      wait_ready();
//      $display("Ready has been set. Starting write commands.");
//      write_word(ADDR_EMEM_ADDR, 16'h0010);
//      write_word(ADDR_EMEM_DATA, 32'hdeadbeef);
//      write_word(ADDR_CTRL, 32'h2);
//      #(1000 * CLK_PERIOD);
//      wait_ready();

//      write_word(ADDR_EMEM_ADDR, 16'h0020);
//      write_word(ADDR_EMEM_DATA, 32'haa55aa55);
//      write_word(ADDR_CTRL, 32'h2);
//      #(1000 * CLK_PERIOD);
//      wait_ready();
//
//      write_word(ADDR_EMEM_ADDR, 16'h0100);
//      write_word(ADDR_EMEM_DATA, 32'h004488ff);
//      write_word(ADDR_CTRL, 32'h2);
//      #(1000 * CLK_PERIOD);
//      wait_ready();

      $display("  -- Test of reading from the memory done.");
      $display("");
    end
  endtask // read_test


  //----------------------------------------------------------------
  // mkmif_test
  // The main test functionality.
  //----------------------------------------------------------------
  initial
    begin : mkmif_test
      $display("   --*** Test of mkmif started ***--");

      tb_init();
      toggle_reset();
      check_name_version();
      write_test();
      read_test();

      $display("");
      $display("   --*** Test of mkmif completed ***--");
      $display("Tests executed: %04d", test_ctr);
      $display("Tests failed:   %04d", error_ctr);
      $finish;
    end // mkmif_test

endmodule // tb_mkmif

//======================================================================
// EOF tb_mkmif.v
//======================================================================
