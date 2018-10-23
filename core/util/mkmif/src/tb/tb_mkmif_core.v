//======================================================================
//
// tb_mkmif_core.v
// ---------------
// Testbench for the mkmif core module.
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

module tb_mkmif_core();

  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  parameter DEBUG = 1;

  parameter CLK_HALF_PERIOD = 2;
  parameter CLK_PERIOD      = 2 * CLK_HALF_PERIOD;


  //----------------------------------------------------------------
  // Register and Wire declarations.
  //----------------------------------------------------------------
  reg [31 : 0] cycle_ctr;
  reg [31 : 0] test_ctr;
  reg [31 : 0] error_ctr;

  reg           tb_clk;
  reg           tb_reset_n;
  wire          tb_spi_sclk;
  wire          tb_spi_do;
  wire          tb_spi_di;
  reg           tb_read_op;
  reg           tb_write_op;
  reg           tb_init_op;
  wire          tb_ready;
  wire          tb_valid;
  reg  [15 : 0] tb_sclk_div;
  reg  [15 : 0] tb_addr;
  reg  [31 : 0] tb_write_data;
  wire [31 : 0] tb_read_data;

  reg           tb_display_state;
  reg [31 : 0]  read_data;


  //----------------------------------------------------------------
  // Concurrent connectivity.
  //----------------------------------------------------------------
  // We loop back the inverted SPI serial transmit data from
  // the DUT as the data from the memory (do) to the DUT.
  assign tb_spi_do = ~tb_spi_di;


  //----------------------------------------------------------------
  // mkmif device under test.
  //----------------------------------------------------------------
  mkmif_core dut(
                 .clk(tb_clk),
                 .reset_n(tb_reset_n),

                 .spi_sclk(tb_spi_sclk),
                 .spi_cs_n(tb_cs_n),
                 .spi_do(tb_spi_do),
                 .spi_di(tb_spi_di),

                 .read_op(tb_read_op),
                 .write_op(tb_write_op),
                 .init_op(tb_init_op),
                 .ready(tb_ready),
                 .valid(tb_valid),
                 .sclk_div(tb_sclk_div),
                 .addr(tb_addr),
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

      if (tb_display_state)
        begin
          $display("cycle = %8x:", cycle_ctr);
          dump_state();
        end
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
      $display("mkmif_ctrl_reg: 0x%02x", dut.mkmif_ctrl_reg);
      $display("sclk: 0x%01x, cs_n: 0x%01x, di: 0x%01x, do: 0x%01x, nxt: 0x%01x",
               tb_spi_sclk, tb_cs_n, tb_spi_di, tb_spi_do, dut.spi.data_nxt);
      $display("spi_ctrl_reg: 0x%01x, spi_clk_ctr: 0x%04x, spi_bit_ctr: 0x%02x",
               dut.spi.spi_ctrl_reg, dut.spi.clk_ctr_reg, dut.spi.bit_ctr_reg);
      $display("spi length: 0x%02x, spi divisor: 0x%04x, spi set: 0x%01x, spi start: 0x%01x, spi ready: 0x%01x",
               dut.spi.length_reg, dut.spi.divisor_reg, dut.spi.set, dut.spi.start, dut.spi.ready);
      $display("read data: 0x%08x, write_data: 0x%014x", dut.spi.rd_data, dut.spi.wr_data);
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
      tb_read_op    = 0;
      tb_write_op   = 0;
      tb_init_op    = 0;
      tb_sclk_div   = 16'h0004;
      tb_addr       = 16'h0010;
      tb_write_data = 32'haa55aa55;

      tb_display_state = 1;
    end
  endtask // tb_init


  //----------------------------------------------------------------
  // wait_ready()
  //
  // Wait for ready word to be set in the DUT API.
  //----------------------------------------------------------------
  task wait_ready;
    reg ready;
    begin
      ready = 0;

      while (tb_ready == 0)
        begin
          #(CLK_PERIOD);
        end
    end
  endtask // read_word


  //----------------------------------------------------------------
  // toggle_reset
  // Toggle the reset.
  //----------------------------------------------------------------
  task toggle_reset;
    begin
      $display("  -- Toggling reset.");
      dump_state();
      #(2 * CLK_PERIOD);
      tb_reset_n = 0;
      #(10 * CLK_PERIOD);
      @(negedge tb_clk)
      tb_reset_n = 1;
      dump_state();
      $display("  -- Toggling of reset done.");
      $display("");
    end
  endtask // toggle_reset


  //----------------------------------------------------------------
  // write_test
  //----------------------------------------------------------------
  task write_test;
    begin
      $display("  -- Write Test started.");
      inc_test_ctr();
      wait_ready();
      tb_sclk_div      = 16'h0004;
      tb_addr          = 16'h0012;
      tb_write_data    = 32'hdeadbeef;
      tb_write_op      = 1;
      #(2 * CLK_PERIOD);
      tb_write_op      = 0;
      wait_ready();
      $display("  -- Write Test done.");
      $display("");
    end
  endtask // write_test


  //----------------------------------------------------------------
  // mkmif_core_test
  // The main test functionality.
  //----------------------------------------------------------------
  initial
    begin : mkmif__core_test
      $display("   -- Test of mkmif core started --");

      tb_init();
      toggle_reset();
      write_test();

      $display("");
      $display("   -- Test of mkmif core completed --");
      $display("Tests executed: %04d", test_ctr);
      $display("Tests failed:   %04d", error_ctr);
      $finish;
    end // mkmif_core_test

endmodule // tb_mkmif_core

//======================================================================
// EOF tb_mkmif_core.v
//======================================================================
