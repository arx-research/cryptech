//======================================================================
//
// tb_keywrap_mem.v
// ----------------
// Testbench for the keywrap memory.
//
//
// Author: Joachim Strombergson
// Copyright (c) 2018, NORDUnet A/S
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

module tb_keywrap_mem();

  //----------------------------------------------------------------
  // Parameters.
  //----------------------------------------------------------------
  parameter DEBUG = 1;

  parameter CLK_HALF_PERIOD = 1;
  parameter CLK_PERIOD      = 2 * CLK_HALF_PERIOD;

  parameter API_ADDR_BITS  = 4;
  parameter CORE_ADDR_BITS = API_ADDR_BITS - 1;
  parameter API_ADDR_MAX   = (2 ** API_ADDR_BITS) - 1;
  parameter CORE_ADDR_MAX  = (2 ** CORE_ADDR_BITS) - 1;


  //----------------------------------------------------------------
  // Variables, regs and wires.
  //----------------------------------------------------------------
  integer cycle_ctr;
  reg [31 : 0]  error_ctr;
  reg [31 : 0]  tc_ctr;

  reg                            tb_clk;
  reg                            tb_api_we;
  reg [(API_ADDR_BITS - 1) : 0]  tb_api_addr;
  reg [31 : 0]                   tb_api_wr_data;
  wire [31 : 0]                  tb_api_rd_data;
  reg                            tb_core_we;
  reg [(CORE_ADDR_BITS - 1) : 0] tb_core_addr;
  reg [63 : 0]                   tb_core_wr_data;
  wire [63 : 0]                  tb_core_rd_data;


  //----------------------------------------------------------------
  // Device Under Test.
  //----------------------------------------------------------------
  keywrap_mem #(.API_ADDR_BITS(API_ADDR_BITS))
              dut(
                  .clk(tb_clk),
                  .api_we(tb_api_we),
                  .api_addr(tb_api_addr),
                  .api_wr_data(tb_api_wr_data),
                  .api_rd_data(tb_api_rd_data),
                  .core_we(tb_core_we),
                  .core_addr(tb_core_addr),
                  .core_wr_data(tb_core_wr_data),
                  .core_rd_data(tb_core_rd_data)
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
    end


  //----------------------------------------------------------------
  // init_sim()
  //
  // Initialize all counters and testbed functionality as well
  // as setting the DUT inputs to defined values.
  //----------------------------------------------------------------
  task init_sim;
    begin
      cycle_ctr = 0;
      tb_clk    = 0;

      tb_api_we       = 1'h0;
      tb_api_addr     = {API_ADDR_BITS{1'h0}};
      tb_api_wr_data  = 32'h0;
      tb_core_we      = 1'h0;
      tb_core_addr    = {CORE_ADDR_BITS{1'h0}};
      tb_core_wr_data = 64'h0;

      #(CLK_PERIOD);
    end
  endtask // init_sim


  //----------------------------------------------------------------
  // dump_api_mem;
  //----------------------------------------------------------------
  task dump_api_mem;
    begin : dump_api_mem

      reg [13 : 0] addr_ctr;

      $display("Content of the memory:");
      for (addr_ctr = 0 ; addr_ctr < API_ADDR_MAX ; addr_ctr = addr_ctr + 1)
        begin
          tb_api_addr = addr_ctr;
          #(CLK_PERIOD);
          $display("api_mem [0x%08x] = 0x%08x", addr_ctr, tb_api_rd_data);
        end
    end
  endtask // dump_api_mem


  //----------------------------------------------------------------
  // dump_core_mem;
  //----------------------------------------------------------------
  task dump_core_mem;
    begin : dump_core_mem

      reg [12 : 0] addr_ctr;

      $display("Content of the memory:");
      for (addr_ctr = 0 ; addr_ctr <= CORE_ADDR_MAX ; addr_ctr = addr_ctr + 1)
        begin
          tb_core_addr = addr_ctr;
          #(CLK_PERIOD);
          $display("core_mem [0x%08x] = 0x%016x", addr_ctr, tb_core_rd_data);
        end
    end
  endtask // dump_core_mem


  //----------------------------------------------------------------
  // test_core_rmw
  //----------------------------------------------------------------
  task test_core_rmw;
    begin : test_core_rmw
      reg [12 : 0] addr_ctr;

      $display("** TC CORE RMW START");

      for (addr_ctr = 0 ; addr_ctr <= CORE_ADDR_MAX ; addr_ctr = addr_ctr + 1)
        begin
          tb_core_addr = addr_ctr;
          #(CLK_PERIOD);
          tb_core_we = 1;
          tb_core_wr_data = ~tb_core_rd_data;
          #(CLK_PERIOD);
          tb_core_we = 0;
        end

      dump_core_mem();

      $display("** TC CORE RMW END\n");
    end
  endtask // test_core_rmw


  //----------------------------------------------------------------
  // test_api_write;
  //----------------------------------------------------------------
  task test_api_write;
    begin : test_api_write
      reg [API_ADDR_BITS : 0] addr_ctr;

      $display("** TC API WRITE START");

      for (addr_ctr = 0 ; addr_ctr < (API_ADDR_MAX + 1); addr_ctr = addr_ctr + 1)
        begin
          tb_api_we       = 1;
          tb_api_addr     = addr_ctr;
          tb_api_wr_data  = {{(32 - API_ADDR_BITS){1'h0}}, addr_ctr,
                             {(32 - API_ADDR_BITS){1'h1}}, addr_ctr};
          if (DEBUG)
            $display("Writing 0x%08x to address 0x%08x", tb_api_wr_data, tb_api_addr);

          #(CLK_PERIOD);
          tb_api_we       = 0;
        end

      #(CLK_PERIOD);
      dump_core_mem();

      $display("** TC API WRITE END\n");
    end
  endtask // api_write


  //----------------------------------------------------------------
  // test_api_read;
  //----------------------------------------------------------------
  task test_api_read;
    begin : test_api_read

      $display("** TC API READ START");

      dump_api_mem();

      $display("** TC API READ END\n");
    end
  endtask // api_read


  //----------------------------------------------------------------
  // main
  //----------------------------------------------------------------
  initial
    begin : main
      $display("   -= Testbench for Keywrap memory started =-");
      $display("    =========================================");
      $display("");

      init_sim();
      test_api_write();
      test_core_rmw();
      test_api_read();

      $display("");
      $display("*** Keywrap memory simulation done. ***");
      $finish;
    end // main

endmodule // tb_keywrap_mem

//======================================================================
// EOF tb_keywrap_mem.v
//======================================================================
