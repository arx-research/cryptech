//======================================================================
//
// tb_fpga_entropy.v
// -----------------
// Testbench for the FPGA Entropy generator core.
//
//
// Author: Joachim Strombergson
// Copyright (c) 2014, Secworks Sweden AB
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


module tb_fpga_entropy();
  
  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  parameter DEBUG           = 1;
  parameter VERBOSE         = 0;

  parameter CLK_HALF_PERIOD = 1;
  parameter CLK_PERIOD      = CLK_HALF_PERIOD * 2;

  
  //----------------------------------------------------------------
  // Register and Wire declarations.
  //----------------------------------------------------------------
  reg [31 : 0]  cycle_ctr;
  reg [31 : 0]  error_ctr;
  reg [31 : 0]  tc_ctr;

  reg           tb_clk;
  reg           tb_reset_n;
  reg           tb_init;
  reg           tb_update;
  reg           tb_seed;
  wire [31 : 0] tb_rnd;


  //----------------------------------------------------------------
  // Device Under Test.
  //----------------------------------------------------------------
  fpga_entropy_core dut(
                        // Clock and reset.
                        .clk(),
                        .reset_n(),
                        
                        .init(tb_init),
                        .update(tb_update),
                        .seed(tb_seed),

                        .rnd(tb_rnd)
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
  // System monitor. Can display status about the dut and TB
  // every cycle.
  //----------------------------------------------------------------
  always
    begin : sys_monitor
      #(CLK_PERIOD);      
      if (DEBUG)
        begin
          dump_dut_state();
          $display("");
        end

      if (VERBOSE)
        begin
          $display("cycle: 0x%016x", cycle_ctr);
        end
      cycle_ctr = cycle_ctr + 1;
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
      $display("Inputs and outputs:");
      $display("init = 0x%01x, update = 0x%01x, seed = 0x%01x",
               tb_init, tb_update, tb_seed);
      $display("rnd= 0x%08x", tb_rnd);
      $display("");
        
      $display("Internal values:");
      $display("shift_reg = 0x%08x, rnd_reg = 0x%08x, bit_ctr_reg = 0x%02x",
               dut.shift_reg, dut.rnd_reg, dut.bit_ctr_reg);

      $display("l5d = 0x%01x, l7d = 0x%01x, l13d = 0x%01x, l41d = 0x01x, l43d = 0x%01x",
               dut.l5d, dut.l7d, dut.l13d, dut.l41d, dut.l43d);
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
      cycle_ctr         = 0;
      error_ctr         = 0;
      tc_ctr            = 0;
      
      tb_clk            = 0;
      tb_reset_n        = 1;
      tb_init           = 1;
      tb_seed           = 1;
      tb_update         = 0;
    end
  endtask // init_sim
                         
    
  //----------------------------------------------------------------
  // fpga_entropy_test
  //
  // The main test functionality. 
  //----------------------------------------------------------------
  initial
    begin : fpga_entropy_test
      $display("   -- Testbench for fpga entropy core started --");

      init_sim();
      dump_dut_state();
      reset_dut();
      dump_dut_state();

      tb_update = 1;
      #(64 * CLK_PERIOD);
      tb_init = 0;
      #(1000 * CLK_PERIOD);

      $display("*** Simulation done. ***");
      $finish;
    end // fpga_entropy_test
endmodule // tb_fpga_entropy

//======================================================================
// EOF tb_fpga_entropy.v
//======================================================================
