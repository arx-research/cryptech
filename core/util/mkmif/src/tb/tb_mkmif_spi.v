//======================================================================
//
// tb_mkmif_spi.v
// --------------
// Testbench for the mkmif SPI module.
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

module tb_mkmif_spi();

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
  wire          tb_spi_cs_n;
  reg           tb_spi_do;
  wire          tb_spi_di;
  reg           tb_set;
  reg           tb_start;
  reg [2 : 0]   tb_length;
  reg [15 : 0]  tb_divisor;
  wire          tb_ready;
  reg [55 : 0]  tb_write_data;
  wire [31 : 0] tb_read_data;
  reg           tb_dump_state;


  //----------------------------------------------------------------
  // mkmif device under test.
  //----------------------------------------------------------------
  mkmif_spi dut(
                .clk(tb_clk),
                .reset_n(tb_reset_n),

                .spi_sclk(tb_spi_sclk),
                .spi_cs_n(tb_spi_cs_n),
                .spi_do(tb_spi_di),
                .spi_di(tb_spi_di),

                .set(tb_set),
                .start(tb_start),
                .length(tb_length),
                .divisor(tb_divisor),
                .ready(tb_ready),
                .wr_data(tb_write_data),
                .rd_data(tb_read_data)
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
  // dump_ports
  // Dump the status of the dut ports.
  //----------------------------------------------------------------
  task dump_ports;
    begin
      $display("");
    end
  endtask // dump_ports


  //----------------------------------------------------------------
  // dump_state
  // Dump the internal MKMIF state to std out.
  //----------------------------------------------------------------
  task dump_state;
    begin
      $display("Dut state:");
      $display("data_reg: 0x%014x", dut.data_reg);
      $display("clk_ctr:   0x%08x", dut.clk_ctr_reg);
      $display("bit_ctr:   0x%02x", dut.bit_ctr_reg);
      $display("ctrl:      0x%02x, done: 0x%01x, ready: 0x%01x",
               dut.spi_ctrl_reg, dut.bit_ctr_done, tb_ready);
      $display("Output:");
      $display("en: 0x%01x, sclk: 0x%01x, di: 0x%01x, do: 0x%01x",
               tb_spi_cs_n, tb_spi_sclk, tb_spi_di, tb_spi_di);
      $display("read data: 0x%08x", tb_read_data);
      $display("");
    end
  endtask // dump_state


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
          dump_state();
        end
    end
  endtask // read_word


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
      tb_set        = 0;
      tb_start      = 0;
      tb_length     = 3'h7;
      tb_divisor    = 16'h0008;
      tb_write_data = 32'haa55aa55;

      tb_dump_state = 0;
    end
  endtask // tb_init


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
    end
  endtask // toggle_reset


  //----------------------------------------------------------------
  // transmit_data
  // Test case for testing that the dut will transmit data
  // on the interface.
  //----------------------------------------------------------------
  task transmit_data;
    begin
      $display("  -- Trying to transmit data.");
      tb_set = 1;
      tb_write_data = 56'hdeadbeeff18244;
      #(2 * CLK_PERIOD);
      $display("Contents of data reg in dut after set: 0x%14x",
               dut.data_reg);

      #(2 * CLK_PERIOD);
      tb_divisor    = 16'h8;
      tb_length     = 3'h4;
      tb_start      = 1;
      #(2 * CLK_PERIOD);
      tb_start      = 0;

      wait_ready();

      $display("  -- Transmit data test done..");
    end
  endtask // transmit_data


  //----------------------------------------------------------------
  // mkmif_spi_test
  // The main test functionality.
  //----------------------------------------------------------------
  initial
    begin : mkmif_spi_test
      $display("   -- Test of mkmif spi started --");

      tb_init();
      toggle_reset();
      transmit_data();

      $display("");
      $display("   -- Test of mkmif spi completed --");
      $display("Tests executed: %04d", test_ctr);
      $display("Tests failed:   %04d", error_ctr);
      $finish;
    end // mkmif_spi_test

endmodule // tb_mkmif_spi

//======================================================================
// EOF tb_mkmif_spi.v
//======================================================================
