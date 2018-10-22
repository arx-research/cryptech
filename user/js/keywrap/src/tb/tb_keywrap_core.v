//======================================================================
//
// tb_keywrap_mem.v
// ----------------
// Testbench for the keywrap core.
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

module tb_keywrap_core();

  //----------------------------------------------------------------
  // Parameters.
  //----------------------------------------------------------------
  parameter DEBUG = 1;

  parameter CLK_HALF_PERIOD = 1;
  parameter CLK_PERIOD      = 2 * CLK_HALF_PERIOD;

  parameter API_ADDR_BITS  = 8;
  parameter RLEN_BITS      = API_ADDR_BITS - 1;
  parameter CORE_ADDR_BITS = API_ADDR_BITS - 1;
  parameter API_ADDR_MAX   = (2 ** API_ADDR_BITS) - 1;
  parameter CORE_ADDR_MAX  = (2 ** CORE_ADDR_BITS) - 1;

  //----------------------------------------------------------------
  // Variables, regs and wires.
  //----------------------------------------------------------------
  integer cycle_ctr;
  reg [31 : 0]  error_ctr;
  reg [31 : 0]  tc_ctr;

  reg                           tb_clk;
  reg                           tb_reset_n;
  reg                           tb_init;
  reg                           tb_next;
  reg                           tb_encdec;
  wire                          tb_ready;
  wire                          tb_valid;
  reg [(RLEN_BITS - 1) : 0]     tb_rlen;
  reg [255 : 0]                 tb_key;
  reg                           tb_keylen;
  reg  [63 : 0]                 tb_a_init;
  wire [63 : 0]                 tb_a_result;
  reg                           tb_api_we;
  reg [(API_ADDR_BITS - 1) : 0] tb_api_addr;
  reg [31 : 0]                  tb_api_wr_data;
  wire [31 : 0]                 tb_api_rd_data;


  //----------------------------------------------------------------
  // Device Under Test.
  //----------------------------------------------------------------
  keywrap_core #(.MEM_BITS(API_ADDR_BITS))
               dut(
                   .clk(tb_clk),
                   .reset_n(tb_reset_n),

                   .init(tb_init),
                   .next(tb_next),
                   .encdec(tb_encdec),

                   .ready(tb_ready),
                   .valid(tb_valid),

                   .rlen(tb_rlen),
                   .key(tb_key),
                   .keylen(tb_keylen),

                   .a_init(tb_a_init),
                   .a_result(tb_a_result),

                   .api_we(tb_api_we),
                   .api_addr(tb_api_addr),
                   .api_wr_data(tb_api_wr_data),
                   .api_rd_data(tb_api_rd_data)
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
  initial
    begin
      cycle_ctr  = 0;
      tb_clk     = 0;
      tb_reset_n = 0;

      tb_init        = 0;
      tb_next        = 0;
      tb_encdec      = 0;
      tb_rlen        = 13'h0;
      tb_key         = 256'h0;
      tb_keylen      = 0;
      tb_a_init      = 64'h0;
      tb_api_we      = 0;
      tb_api_addr    = 14'h0;
      tb_api_wr_data = 32'h0;


      #(CLK_PERIOD * 10);

      tb_reset_n    = 1;

      #(CLK_PERIOD * 10);
      $finish;
    end

endmodule // tb_keywrap_core

//======================================================================
// EOF tb_keywrap_core.v
//======================================================================
