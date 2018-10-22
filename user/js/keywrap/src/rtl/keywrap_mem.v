//======================================================================
//
// keywrap_mem.v
// -------------
// Memory for AES KEY WRAP processing.
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

module keywrap_mem #(parameter API_ADDR_BITS = 11)
                   (
                    input wire                            clk,

                    input wire                            api_we,
                    input wire [(API_ADDR_BITS - 1) : 0]  api_addr,
                    input wire [31 : 0]                   api_wr_data,
                    output wire [31 : 0]                  api_rd_data,

                    input wire                            core_we,
                    input wire [(API_ADDR_BITS - 2) : 0]  core_addr,
                    input wire [63 : 0]                   core_wr_data,
                    output wire [63 : 0]                  core_rd_data
                   );

  //----------------------------------------------------------------
  // Parameters.
  //----------------------------------------------------------------
  localparam NUM_BANK_WORDS = 2 ** (API_ADDR_BITS - 1);


  //----------------------------------------------------------------
  // Registers and memories including control signals.
  //----------------------------------------------------------------
  reg [31 : 0]                  mem0 [0 : (NUM_BANK_WORDS - 1)];
  reg [31 : 0]                  mem0_data;
  reg [(API_ADDR_BITS - 2) : 0] mem0_addr;
  reg                           mem0_we;

  reg [31 : 0]                  mem1 [0 : (NUM_BANK_WORDS - 1)];
  reg [31 : 0]                  mem1_data;
  reg [(API_ADDR_BITS - 2) : 0] mem1_addr;
  reg                           mem1_we;


  //----------------------------------------------------------------
  // Wires.
  //----------------------------------------------------------------
  reg [31 : 0] api_rd_data0;
  reg [31 : 0] api_rd_data1;
  reg [31 : 0] muxed_api_rd_data;

  reg [31 : 0] core_rd_data0;
  reg [31 : 0] core_rd_data1;


  //----------------------------------------------------------------
  // Assignments for ports.
  //----------------------------------------------------------------
  assign api_rd_data    = muxed_api_rd_data;
  assign core_rd_data   = {core_rd_data0, core_rd_data1};


  //----------------------------------------------------------------
  // mem0_access
  //----------------------------------------------------------------
  always @(posedge clk)
    begin : mem0_access
      core_rd_data0   <= mem0[core_addr];
      api_rd_data0    <= mem0[api_addr[(API_ADDR_BITS - 1) : 1]];

      if (mem0_we)
        mem0[mem0_addr] <= mem0_data;
    end


  //----------------------------------------------------------------
  // mem1_access
  //----------------------------------------------------------------
  always @(posedge clk)
    begin : mem1_access
      core_rd_data1   <= mem1[core_addr];
      api_rd_data1    <= mem1[api_addr[(API_ADDR_BITS - 1) : 1]];

      if (mem1_we)
        mem1[mem1_addr] <= mem1_data;
    end


  //----------------------------------------------------------------
  // api_rd_data_mux
  //----------------------------------------------------------------
  always @*
    begin : api_rd_data_mux
      if (api_addr[0])
        muxed_api_rd_data = api_rd_data1;
      else
        muxed_api_rd_data = api_rd_data0;
    end


  //----------------------------------------------------------------
  // write_mux
  // Mux that handles priority of writes and selection
  // of memory bank to write api data to.
  //----------------------------------------------------------------
  always @*
    begin : write_mux
      mem0_data = 32'h0;
      mem0_addr = {(API_ADDR_BITS - 1){1'h0}};
      mem0_we   = 1'h0;
      mem1_data = 32'h0;
      mem1_addr = {(API_ADDR_BITS - 1){1'h0}};
      mem1_we   = 1'h0;

      if (core_we)
        begin
          mem0_data = core_wr_data[63 : 32];
          mem0_addr = core_addr;
          mem0_we   = 1'h1;
          mem1_data = core_wr_data[31 : 0];
          mem1_addr = core_addr;
          mem1_we   = 1'h1;
        end

      else if (api_we)
        begin
          mem0_addr = api_addr[(API_ADDR_BITS - 1) : 1];
          mem0_data = api_wr_data;
          mem1_addr = api_addr[(API_ADDR_BITS - 1) : 1];
          mem1_data = api_wr_data;

          if (api_addr[0])
            mem1_we   = 1'h1;
          else
            mem0_we   = 1'h1;
        end
    end
endmodule // keywrap_mem

//======================================================================
// EOF keywrap_mem.v
//======================================================================
