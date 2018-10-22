//======================================================================
//
// keywrap.v
// --------
// Top level wrapper for the KEY WRAP core.
//
// Since $clog2() is not supported by all tools, and constant
// functions are not supported by some other tools we need to
// do the size to number of bits calculation by hand.
// 8192 bytes = 2048 32 bit words. This requires 11 bits.
// We need additional space for control and status words. But
// since we have filled the address space, we need another MSB
// in the address. Thus ADDR_BITS = 12 bits.
//
// 0x000 - 0x7ff are for control and status.
// 0x800 - 0xfff are for data storage
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

module keywrap #(parameter ADDR_BITS = 13)
               (
                input wire                        clk,
                input wire                        reset_n,

                input wire                        cs,
                input wire                        we,

                input wire  [(ADDR_BITS - 1) : 0] address,
                input wire  [31 : 0]              write_data,
                output wire [31 : 0]              read_data,
                output wire                       error
               );

  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  localparam ADDR_NAME0       = 8'h00;
  localparam ADDR_NAME1       = 8'h01;
  localparam ADDR_VERSION     = 8'h02;

  localparam ADDR_CTRL        = 8'h08;
  localparam CTRL_INIT_BIT    = 0;
  localparam CTRL_NEXT_BIT    = 1;

  localparam ADDR_STATUS      = 8'h09;
  localparam STATUS_READY_BIT = 0;
  localparam STATUS_VALID_BIT = 1;

  localparam ADDR_CONFIG      = 8'h0a;
  localparam CTRL_ENCDEC_BIT  = 0;
  localparam CTRL_KEYLEN_BIT  = 1;

  localparam ADDR_RLEN        = 8'h0c;
  localparam ADDR_A0          = 8'h0e;
  localparam ADDR_A1          = 8'h0f;

  localparam ADDR_KEY0        = 8'h10;
  localparam ADDR_KEY1        = 8'h11;
  localparam ADDR_KEY2        = 8'h12;
  localparam ADDR_KEY3        = 8'h13;
  localparam ADDR_KEY4        = 8'h14;
  localparam ADDR_KEY5        = 8'h15;
  localparam ADDR_KEY6        = 8'h16;
  localparam ADDR_KEY7        = 8'h17;

  localparam CORE_NAME0       = 32'h6b657920; // "key "
  localparam CORE_NAME1       = 32'h77726170; // "wrap"
  localparam CORE_VERSION     = 32'h302e3830; // "0.80"

  localparam MEM_BITS         = ADDR_BITS - 1;
  localparam RLEN_BITS        = ADDR_BITS - 2;
  localparam PAD              = ADDR_BITS - 8;


  //----------------------------------------------------------------
  // Registers including update variables and write enable.
  //----------------------------------------------------------------
  reg init_reg;
  reg init_new;

  reg next_reg;
  reg next_new;

  reg encdec_reg;
  reg keylen_reg;
  reg config_we;

  reg [(RLEN_BITS - 1) : 0] rlen_reg;
  reg                       rlen_we;

  reg [31 : 0] a0_reg;
  reg          a0_we;

  reg [31 : 0] a1_reg;
  reg          a1_we;

  reg [31 : 0] key_reg [0 : 7];
  reg          key_we;

  reg [31 : 0] api_rd_delay_reg;
  reg [31 : 0] api_rd_delay_new;

  reg valid_reg;
  reg ready_reg;


  //----------------------------------------------------------------
  // Wires.
  //----------------------------------------------------------------
  reg [31 : 0]   tmp_read_data;
  reg            tmp_error;

  reg            core_api_we;
  wire [(MEM_BITS - 1) : 0] core_api_addr;
  wire           core_ready;
  wire           core_valid;
  wire [255 : 0] core_key;
  wire [63 : 0]  core_a_init;
  wire [63 : 0]  core_a_result;
  wire [31 : 0]  core_api_rd_data;


  //----------------------------------------------------------------
  // Concurrent connectivity for ports etc.
  //----------------------------------------------------------------
  assign read_data = tmp_read_data;
  assign error     = tmp_error;

  assign core_key = {key_reg[0], key_reg[1], key_reg[2], key_reg[3],
                     key_reg[4], key_reg[5], key_reg[6], key_reg[7]};

  assign core_api_addr = address[(MEM_BITS - 1) : 0];

  assign core_a_init = {a0_reg, a1_reg};


  //----------------------------------------------------------------
  // core instantiation.
  //----------------------------------------------------------------
  keywrap_core #(.MEM_BITS(MEM_BITS))
               core(
                    .clk(clk),
                    .reset_n(reset_n),

                    .init(init_reg),
                    .next(next_reg),
                    .encdec(encdec_reg),

                    .ready(core_ready),
                    .valid(core_valid),

                    .rlen(rlen_reg),

                    .key(core_key),
                    .keylen(keylen_reg),

                    .a_init(core_a_init),
                    .a_result(core_a_result),

                    .api_we(core_api_we),
                    .api_addr(core_api_addr),
                    .api_wr_data(write_data),
                    .api_rd_data(core_api_rd_data)
                   );


  //----------------------------------------------------------------
  // reg_update
  //----------------------------------------------------------------
  always @ (posedge clk or negedge reset_n)
    begin : reg_update
      integer i;

      if (!reset_n)
        begin
          for (i = 0 ; i < 8 ; i = i + 1)
            key_reg[i] <= 32'h0;

          init_reg         <= 1'h0;
          next_reg         <= 1'h0;
          encdec_reg       <= 1'h0;
          keylen_reg       <= 1'h0;
          rlen_reg         <= {RLEN_BITS{1'h0}};
          valid_reg        <= 1'h0;
          ready_reg        <= 1'h0;
          a0_reg           <= 32'h0;
          a1_reg           <= 32'h0;
          api_rd_delay_reg <= 32'h0;
        end
      else
        begin
          ready_reg        <= core_ready;
          valid_reg        <= core_valid;
          init_reg         <= init_new;
          next_reg         <= next_new;
          api_rd_delay_reg <= api_rd_delay_new;

          if (config_we)
            begin
              encdec_reg <= write_data[CTRL_ENCDEC_BIT];
              keylen_reg <= write_data[CTRL_KEYLEN_BIT];
            end

          if (rlen_we)
            rlen_reg <= write_data[12 : 0];

          if (a0_we)
            a0_reg <= write_data;

          if (a1_we)
            a1_reg <= write_data;

          if (key_we)
            key_reg[address[2 : 0]] <= write_data;
        end
    end // reg_update


  //----------------------------------------------------------------
  // api
  //
  // The interface command decoding logic.
  //----------------------------------------------------------------
  always @*
    begin : api
      init_new         = 1'h0;
      next_new         = 1'h0;
      config_we        = 1'h0;
      rlen_we          = 1'h0;
      key_we           = 1'h0;
      core_api_we      = 1'h0;
      a0_we            = 1'h0;
      a1_we            = 1'h0;
      tmp_read_data    = 32'h0;
      tmp_error        = 1'h0;
      api_rd_delay_new = 32'h0;

      // api_mux
      if (address[(ADDR_BITS - 1)])
        tmp_read_data = core_api_rd_data;
      else
        tmp_read_data = api_rd_delay_reg;

      if (cs)
        begin
          if (we)
            begin
              if (address == {{PAD{1'h0}}, ADDR_CTRL})
                begin
                  init_new = write_data[CTRL_INIT_BIT];
                  next_new = write_data[CTRL_NEXT_BIT];
                end

              if (address == {{PAD{1'h0}}, ADDR_CONFIG})
                config_we = 1'h1;

              if (address == {{PAD{1'h0}}, ADDR_RLEN})
                rlen_we = 1'h1;

              if (address == {{PAD{1'h0}}, ADDR_A0})
                a0_we = 1'h1;

              if (address == {{PAD{1'h0}}, ADDR_A1})
                a1_we = 1'h1;

              if ((address >= {{PAD{1'h0}}, ADDR_KEY0}) &&
                   (address <= {{PAD{1'h0}}, ADDR_KEY7}))
                key_we = 1'h1;

              if (address[(ADDR_BITS - 1)])
                core_api_we = 1'h1;
            end // if (we)
          else
            begin
              // Read access
              if (address == {{PAD{1'h0}}, ADDR_NAME0})
                api_rd_delay_new = CORE_NAME0;

              if (address == {{PAD{1'h0}}, ADDR_NAME1})
                api_rd_delay_new = CORE_NAME1;

              if (address == {{PAD{1'h0}}, ADDR_VERSION})
                api_rd_delay_new = CORE_VERSION;

              if (address == {{PAD{1'h0}}, ADDR_CTRL})
                api_rd_delay_new = {28'h0, keylen_reg, encdec_reg, next_reg, init_reg};

              if (address == {{PAD{1'h0}}, ADDR_STATUS})
                api_rd_delay_new = {30'h0, valid_reg, ready_reg};

              if (address == {{PAD{1'h0}}, ADDR_RLEN})
                api_rd_delay_new = {19'h0, rlen_reg};

              if (address == {{PAD{1'h0}}, ADDR_A0})
                api_rd_delay_new = core_a_result[63 : 32];

              if (address == {{PAD{1'h0}}, ADDR_A1})
                api_rd_delay_new = core_a_result[31 : 0];
            end // else: !if(we)
        end // if (cs)
    end // block: api
endmodule // keywrap

//======================================================================
// EOF keywrap.v
//======================================================================
