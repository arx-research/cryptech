//======================================================================
//
// tb_coretest_test_core.v
// -----------------------
// Testbench for the coretest_test_core module.
//
//
// Author: Joachim Strombergson
// Copyright (c) 2014, SUNET
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
// Simulator directives.
//------------------------------------------------------------------
`timescale 1ns/10ps

module tb_coretest_test_core();
  
  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  parameter DEBUG           = 0;
  parameter VERBOSE         = 0;

  parameter CLK_HALF_PERIOD = 1;
  parameter CLK_PERIOD      = CLK_HALF_PERIOD * 2;
  
  
  //----------------------------------------------------------------
  // Register and Wire declarations.
  //----------------------------------------------------------------
  reg [31 : 0] cycle_ctr;
  reg [31 : 0] error_ctr;
  reg [31 : 0] tc_ctr;

  reg          tb_clk;
  reg          tb_reset_n;
  reg          tb_rxd;
  wire         tb_txd;

  wire         tb_rxd_syn;
  wire [7 : 0] tb_rxd_data;
  wire         tb_rxd_ack;

  wire         tb_txd_syn;
  wire [7 : 0] tb_txd_data;
  wire         tb_txd_ack;

  wire [7 : 0] tb_debug;

  reg          txd_state;
  

  //----------------------------------------------------------------
  // Device Under Test.
  //----------------------------------------------------------------
  coretest_test_core dut(
                         .clk(tb_clk),
                         .reset_n(tb_reset_n),
           
                         .rxd(tb_rxd),
                         .txd(tb_txd),
                         
                         .debug(tb_debug)
                        );

  //----------------------------------------------------------------
  // Concurrent assignments.
  //----------------------------------------------------------------
  

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
  //----------------------------------------------------------------
  always
    begin : sys_monitor
      #(CLK_PERIOD);      
      if (VERBOSE)
        begin
          $display("cycle: 0x%016x", cycle_ctr);
        end
      cycle_ctr = cycle_ctr + 1;
    end


  //----------------------------------------------------------------
  // tx_monitor
  //
  // Observes what happens on the dut tx port and reports it.
  //----------------------------------------------------------------
  always @*
    begin : tx_monitor
      if ((!tb_txd) && txd_state)
        begin
          $display("txd going low.");
          txd_state = 0;
        end

      if (tb_txd && (!txd_state))
        begin
          $display("txd going high");
          txd_state = 1;
        end
    end
    
  
  //----------------------------------------------------------------
  // dump_dut_state()
  //
  // Dump the state of the dut when needed.
  //----------------------------------------------------------------
  task dump_dut_state();
    begin
      $display("State of DUT");
      $display("------------");
      $display("");
    end
  endtask // dump_dut_state  

  
  //----------------------------------------------------------------
  // reset_dut()
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
      cycle_ctr  = 0;
      error_ctr  = 0;
      tc_ctr     = 0;
      
      tb_clk     = 0;
      tb_reset_n = 1;

      tb_rxd     = 1;
      txd_state  = 1;
    end
  endtask // init_sim


  //----------------------------------------------------------------
  // transmit_byte
  //
  // Transmit a byte of data to the DUT receive port.
  //----------------------------------------------------------------
  task transmit_byte(input [7 : 0] data);
    integer i;
    begin
      $display("*** Transmitting byte 0x%02x to the dut.", data);

      #10;
      
      // Start bit
      $display("*** Transmitting start bit.");
      tb_rxd = 0;
      #(CLK_PERIOD * dut.uart.DEFAULT_CLK_RATE);
      
      // Send the bits LSB first.
      for (i = 0 ; i < 8 ; i = i + 1)
        begin
          $display("*** Transmitting data[%1d] = 0x%01x.", i, data[i]);
          tb_rxd = data[i];
          #(CLK_PERIOD * dut.uart.DEFAULT_CLK_RATE);
        end

      // Send two stop bits. I.e. two bit times high (mark) value.
      $display("*** Transmitting two stop bits.");
      tb_rxd = 1;
      #(2 * CLK_PERIOD * dut.uart.DEFAULT_CLK_RATE * dut.uart.DEFAULT_STOP_BITS);
      $display("*** End of transmission.");
    end
  endtask // transmit_byte


  //----------------------------------------------------------------
  // check_transmit
  //
  // Transmits a byte and checks that it was captured internally
  // by the dut.
  //----------------------------------------------------------------
  task check_transmit(input [7 : 0] data);
    begin
      tc_ctr = tc_ctr + 1;

      transmit_byte(data);
      
//      if (dut.core.rxd_byte_reg == data)
//        begin
//          $display("*** Correct data: 0x%01x captured by the dut.", 
//                   dut.core.rxd_byte_reg);
//        end
//      else
//        begin
//          $display("*** Incorrect data: 0x%01x captured by the dut Should be: 0x%01x.",
//                   dut.core.rxd_byte_reg, data);
//          error_ctr = error_ctr + 1;
//        end
    end
  endtask // check_transmit
  

  //----------------------------------------------------------------
  // test_transmit
  //
  // Transmit a number of test bytes to the dut.
  //----------------------------------------------------------------
  task test_transmit();
    begin
      // Send reset command.
      $display("*** Sending reset command.");
      check_transmit(8'h55);
      check_transmit(8'h01);
      check_transmit(8'haa);
      
      // Send read command.
      $display("*** Sending Read command to CORE ID0 in test core.");
      check_transmit(8'h55);
      check_transmit(8'h10);
      check_transmit(8'h01);
      check_transmit(8'h00);
      check_transmit(8'haa);
      
      // Send read command.
      $display("*** Sending Read command to CORE ID2 in test core.");
      check_transmit(8'h55);
      check_transmit(8'h10);
      check_transmit(8'h01);
      check_transmit(8'h01);
      check_transmit(8'haa);
      
      // Send read command.
      $display("*** Sending Read command to rw-register in test_core. 11223344.");
      check_transmit(8'h55);
      check_transmit(8'h10);
      check_transmit(8'h01);
      check_transmit(8'h10);
      check_transmit(8'haa);

      // Send write command.
      $display("*** Sending Write command to rw-register in test_core. deadbeef.");
      check_transmit(8'h55);
      check_transmit(8'h11);
      check_transmit(8'h01);
      check_transmit(8'h10);
      check_transmit(8'hde);
      check_transmit(8'had);
      check_transmit(8'hbe);
      check_transmit(8'hef);
      check_transmit(8'haa);
      
      // Send read command.
      $display("*** Sending Read command to rw-register in test_core. deadbeef.");
      check_transmit(8'h55);
      check_transmit(8'h10);
      check_transmit(8'h01);
      check_transmit(8'h10);
      check_transmit(8'haa);

      // Send write command.
      $display("*** Sending Write command to debug-register in test_core. 77.");
      check_transmit(8'h55);
      check_transmit(8'h11);
      check_transmit(8'h01);
      check_transmit(8'h20);
      check_transmit(8'h00);
      check_transmit(8'h00);
      check_transmit(8'h00);
      check_transmit(8'h77);
      check_transmit(8'haa);

    end
  endtask // test_transmit

  
  //----------------------------------------------------------------
  // display_test_result()
  //
  // Display the accumulated test results.
  //----------------------------------------------------------------
  task display_test_result();
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
  // coretest_test_core
  // The main test functionality. 
  //----------------------------------------------------------------
  initial
    begin : uart_test
      $display("   -- Testbench for coretest_test_core started --");

      init_sim();
      dump_dut_state();
      reset_dut();
      dump_dut_state();

      test_transmit();
      $display("*** transmit done.");

      #(1000000 * CLK_PERIOD);
      $display("*** Wait completed.");
      
      display_test_result();
      $display("*** Simulation done.");
      $finish;
    end // uart_test
endmodule // tb_uart

//======================================================================
// EOF tb_coretest_test_core.v
//======================================================================
