//======================================================================
//
// tb_coretest.v
// -------------
// Testbench for coretest. Generates commands and observes responses.
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
// Simulator directives.
//------------------------------------------------------------------
`timescale 1ns/10ps


module tb_coretest();
  
  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  parameter DEBUG           = 0;
  parameter VERBOSE         = 0;
  parameter CMD_MONITOR     = 1;
  parameter REC_MONITOR     = 0;

  parameter CLK_HALF_PERIOD = 1;
  parameter CLK_PERIOD      = CLK_HALF_PERIOD * 2;

  // Command and response constants.
  parameter SOC       = 8'h55;
  parameter EOC       = 8'haa;
  parameter RESET_CMD = 8'h01;
  parameter READ_CMD  = 8'h10; 
  parameter WRITE_CMD = 8'h11; 

  parameter SOR      = 8'haa;
  parameter EOR      = 8'h55;
  parameter UNKNOWN  = 8'hfe;
  parameter ERROR    = 8'hfd;
  parameter READ_OK  = 8'h7f;
  parameter WRITE_OK = 8'h7e;
  parameter RESET_OK = 8'h7d;
  
  parameter MAX_MEM  = 16'h00ff;

  
  //----------------------------------------------------------------
  // Register and Wire declarations.
  //----------------------------------------------------------------
  reg [31 : 0]  cycle_ctr;
  reg [31 : 0]  error_ctr;
  reg [31 : 0]  tc_ctr;

  reg           tb_clk;
  reg           tb_reset_n;

  reg           tb_rx_syn;
  reg [7 : 0]   tb_rx_data;
  wire          tb_rx_ack;
  wire          tb_tx_syn;
  wire [7 : 0]  tb_tx_data;
  reg           tb_tx_ack;

  wire          tb_core_reset_;
  wire          tb_core_cs;
  wire          tb_core_we;
  wire [15 : 0] tb_core_address;
  wire [31 : 0] tb_core_write_data;
  reg [31 : 0]  tb_core_read_data;
  reg           tb_core_error;

  reg [7 : 0]   received_tx_data;
  
  reg [31 : 0]  test_mem [0 : (MAX_MEM - 1'b1)];


  //----------------------------------------------------------------
  // Device Under Test.
  //----------------------------------------------------------------
  coretest dut(
               .clk(tb_clk),
               .reset_n(tb_reset_n),
                
               // Interface to communication core
               .rx_syn(tb_rx_syn),
               .rx_data(tb_rx_data),
               .rx_ack(tb_rx_ack),
               
               .tx_syn(tb_tx_syn),
               .tx_data(tb_tx_data),
               .tx_ack(tb_tx_ack),
                
               // Interface to the core being tested.
               .core_reset_n(tb_core_reset_n),
               .core_cs(tb_core_cs),
               .core_we(tb_core_we),
               .core_address(tb_core_address),
               .core_write_data(tb_core_write_data),
               .core_read_data(tb_core_read_data),
               .core_error(tb_core_error)
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
  // command_monitor
  //
  // Observes any read/write or reset commands generated
  // by the DUT.
  //----------------------------------------------------------------
  always
    begin : command_monitor
      #(CLK_PERIOD);      
      if (CMD_MONITOR)
        begin
          if (!tb_core_reset_n)
            begin
              $display("Core is being reset by coretest.");
            end

          if (tb_core_cs)
            begin
              if (tb_core_we)
                begin
                  $display("Core is being written to: address 0x%08x = 0x%08x",
                           tb_core_address, tb_core_write_data);
                end
              else
                begin
                  $display("Core is being read from: address 0x%08x = 0x%08x",
                           tb_core_address, tb_core_read_data);
                end
            end
        end
    end


  //----------------------------------------------------------------
  // receive_logic
  //
  // The logic needed to the correct handshake expected by the DUT
  // when it is sending bytes.
  //----------------------------------------------------------------
  always @ (posedge tb_clk)
    begin : receive_logic
      if (tb_tx_syn)
        begin
          if (REC_MONITOR)
            begin
              $display("Receiving byte 0x%02x from the DUT.", tb_tx_data);
            end
          #(2 * CLK_PERIOD);
          tb_tx_ack = 1;

          #(2 * CLK_PERIOD);
          tb_tx_ack = 0;
        end
    end // receive_logic


  //----------------------------------------------------------------
  // test_mem_logic
  //
  // The logic needed to implement the test memory. We basically
  // implement a simple memory to allow read and write operations
  // via commands to the DUT to really be executed.
  //----------------------------------------------------------------
  always @ (posedge tb_clk)
    begin : test_mem_logic
      if (tb_core_cs)
        begin
          if (tb_core_we)
            begin
              if (tb_core_address < MAX_MEM)
                begin
                  $display("Writing to test_mem[0x%08x] = 0x%08x",
                           tb_core_address, tb_core_write_data);
                  test_mem[tb_core_address] = tb_core_write_data;
                end
              else
                begin
                  $display("Writing to incorrect address 0x%08x",
                           tb_core_address);
                  tb_core_error = 1;
                end
            end
          else
            begin
              if (tb_core_address < MAX_MEM)
                begin
                  $display("Reading from test_mem[0x%08x] = 0x%08x",
                           tb_core_address, tb_core_read_data);
                  tb_core_read_data = test_mem[tb_core_address];
                end
              else
                begin
                  $display("Reading from incorrect address 0x%08x",
                           tb_core_address);
                  tb_core_error = 1;
                end
            end
        end
      else
        begin
          tb_core_read_data = 32'h00000000;
          tb_core_error     = 0;
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
      $display("Inputs and outputs:");
      $display("rx_syn = 0x%01x, rx_data = 0x%02x, rx_ack = 0x%01x",
               dut.rx_syn, dut.rx_data, dut.rx_ack);
      $display("tx_syn = 0x%01x, tx_data = 0x%02x, tx_ack = 0x%01x",
               dut.tx_syn, dut.tx_data, dut.tx_ack);
      $display("cs = 0x%01x, we = 0x%01x, address = 0x%04x, write_data = 0x%08x, read_data = 0x%08x, error = 0x%01x",
               dut.core_cs, dut.core_we, dut.core_address, dut.core_write_data, dut.core_read_data, dut.core_error);
      $display("");

      $display("RX chain signals:");
      $display("rx_buffer_wr_ptr = 0x%02x, rx_buffer_rd_ptr = 0x%02x, rx_buffer_ctr = 0x%02x, rx_buffer_empty = 0x%01x,  rx_buffer_full = 0x%01x",
               dut.rx_buffer_wr_ptr_reg, dut.rx_buffer_rd_ptr_reg,  dut.rx_buffer_ctr_reg, dut.rx_buffer_empty, dut.rx_buffer_full);
      $display("");

      $display("Control signals and FSM state:");
      $display("test_engine_reg = 0x%02x, cmd_reg = 0x%02x",
               dut.test_engine_reg, dut.cmd_reg);
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
    reg [8 : 0] i;
    
    begin
      cycle_ctr         = 0;
      error_ctr         = 0;
      tc_ctr            = 0;
      
      tb_clk            = 0;
      tb_reset_n        = 1;
      tb_rx_syn         = 0;
      tb_rx_data        = 8'h00;
      tb_tx_ack         = 0;

      for (i = 0 ; i < 256 ; i = i + 1)
        begin
          test_mem[i[7 : 0]] = {4{i[7 : 0]}};
        end
    end
  endtask // init_sim


  //----------------------------------------------------------------
  // send_byte
  //
  // Send a byte of data to the DUT.
  //----------------------------------------------------------------
  task send_byte(input [7 : 0] data);
    integer i;
    begin

      if (VERBOSE)
        begin
          $display("*** Sending byte 0x%02x to the dut.", data);
          $display("*** Setting RX data and RX SYN.");
        end
      tb_rx_data = data;
      tb_rx_syn  = 1;

      while (!tb_rx_ack)
        begin
          #CLK_PERIOD;
          if (VERBOSE)
            begin
              $display("*** Waiting for RX ACK.");
            end
        end

      if (VERBOSE)
        begin
          $display("*** RX ACK seen. Dropping SYN.");
        end
      tb_rx_syn  = 0;

      #(2 * CLK_PERIOD);
    end
  endtask // send_byte

  
  //----------------------------------------------------------------
  // send_reset_command
  //
  // Generates a reset command to the dut.
  //----------------------------------------------------------------
  task send_reset_command();
    begin
      $display("*** Sending reset command.");
      send_byte(SOC);
      send_byte(RESET_CMD);
      send_byte(EOC);
      $display("*** Sending reset command done.");
    end
  endtask // send_write_command

  
  //----------------------------------------------------------------
  // send_read_command
  //
  // Generates a read command to the dut.
  //----------------------------------------------------------------
  task send_read_command(input [15 : 0] addr);
    begin
      $display("*** Sending read command: address 0x%04x.", addr);
      send_byte(SOC);
      send_byte(READ_CMD);
      send_byte(addr[15 : 8]);
      send_byte(addr[7 : 0]);
      send_byte(EOC);
      $display("*** Sending read command done.");
    end
  endtask // send_write_command

  
  //----------------------------------------------------------------
  // send_write_command
  //
  // Generates a write command to the dut.
  //----------------------------------------------------------------
  task send_write_command(input [15 : 0] addr, input [31 : 0] data);
    begin
      $display("*** Sending write command: address 0x%04x = 0x%08x.", addr, data);
      send_byte(SOC);
      send_byte(WRITE_CMD);
      send_byte(addr[15 : 8]);
      send_byte(addr[7 : 0]);
      send_byte(data[31 : 24]);
      send_byte(data[23 : 16]);
      send_byte(data[15 : 8]);
      send_byte(data[7 : 0]);
      send_byte(EOC);
      $display("*** Sending write command done.");
    end
  endtask // send_write_command
  
  
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
  // coretest_test
  // The main test functionality. 
  //----------------------------------------------------------------
  initial
    begin : coretest_test
      $display("   -- Testbench for coretest started --");

      init_sim();
      dump_dut_state();
      reset_dut();
      dump_dut_state();

      #(64 * CLK_PERIOD);

      send_reset_command();

      send_read_command(16'h0023);
      send_read_command(16'h0055);

      send_write_command(16'h00aa, 32'h1337beef);
      send_read_command(16'h00aa);
      send_write_command(16'h0010, 32'h55aa55aa);
      send_read_command(16'h0010);

      #(200 * CLK_PERIOD);

      display_test_result();
      $display("*** Simulation done.");
      $finish;
    end // coretest_test
endmodule // tb_coretest

//======================================================================
// EOF tb_coretest.v
//======================================================================
