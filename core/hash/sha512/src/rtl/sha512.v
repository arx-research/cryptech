//======================================================================
//
// sha512.v
// --------
// Top level wrapper for the SHA-512 hash function providing
// a simple memory like interface with 32 bit data access.
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

module sha512(
              // Clock and reset.
              input wire           clk,
              input wire           reset_n,

              // Control.
              input wire           cs,
              input wire           we,

              // Data ports.
              input wire  [7 : 0]  address,
              input wire  [31 : 0] write_data,
              output wire [31 : 0] read_data,
              output wire          error
             );

  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  parameter ADDR_NAME0           = 8'h00;
  parameter ADDR_NAME1           = 8'h01;
  parameter ADDR_VERSION         = 8'h02;

  parameter ADDR_CTRL            = 8'h08;
  parameter CTRL_INIT_BIT        = 0;
  parameter CTRL_NEXT_BIT        = 1;
  parameter CTRL_MODE_LOW_BIT    = 2;
  parameter CTRL_MODE_HIGH_BIT   = 3;
  parameter CTRL_WORK_FACTOR_BIT = 7;

  parameter ADDR_STATUS          = 8'h09;
  parameter STATUS_READY_BIT     = 0;
  parameter STATUS_VALID_BIT     = 1;

  parameter ADDR_WORK_FACTOR_NUM = 8'h0a;

  parameter ADDR_BLOCK0          = 8'h10;
  parameter ADDR_BLOCK31         = 8'h2f;

  parameter ADDR_DIGEST0         = 8'h40;
  parameter ADDR_DIGEST1         = 8'h41;
  parameter ADDR_DIGEST2         = 8'h42;
  parameter ADDR_DIGEST3         = 8'h43;
  parameter ADDR_DIGEST4         = 8'h44;
  parameter ADDR_DIGEST5         = 8'h45;
  parameter ADDR_DIGEST6         = 8'h46;
  parameter ADDR_DIGEST7         = 8'h47;
  parameter ADDR_DIGEST8         = 8'h48;
  parameter ADDR_DIGEST9         = 8'h49;
  parameter ADDR_DIGEST10        = 8'h4a;
  parameter ADDR_DIGEST11        = 8'h4b;
  parameter ADDR_DIGEST12        = 8'h4c;
  parameter ADDR_DIGEST13        = 8'h4d;
  parameter ADDR_DIGEST14        = 8'h4e;
  parameter ADDR_DIGEST15        = 8'h4f;

  parameter CORE_NAME0         = 32'h73686132; // "sha2"
  parameter CORE_NAME1         = 32'h2d353132; // "-512"
  parameter CORE_VERSION       = 32'h302e3831; // "0.81"

  parameter MODE_SHA_512_224   = 2'h0;
  parameter MODE_SHA_512_256   = 2'h1;
  parameter MODE_SHA_384       = 2'h2;
  parameter MODE_SHA_512       = 2'h3;

  parameter DEFAULT_WORK_FACTOR_NUM = 32'h000f0000;


  //----------------------------------------------------------------
  // Registers including update variables and write enable.
  //----------------------------------------------------------------
  reg init_reg;
  reg init_new;
  reg init_we;
  reg init_set;

  reg next_reg;
  reg next_new;
  reg next_we;
  reg next_set;

  reg work_factor_reg;
  reg work_factor_new;
  reg work_factor_we;

  reg [1 : 0] mode_reg;
  reg [1 : 0] mode_new;
  reg         mode_we;

  reg [31 : 0] work_factor_num_reg;
  reg          work_factor_num_we;

  reg ready_reg;

  reg [31 : 0] block_reg [0 : 31];
  reg          block_we;

  reg [511 : 0] digest_reg;
  reg           digest_valid_reg;


  //----------------------------------------------------------------
  // Wires.
  //----------------------------------------------------------------
  wire            core_ready;
  wire [1023 : 0] core_block;
  wire [511 : 0]  core_digest;
  wire            core_digest_valid;

  reg [4 : 0]     block_addr;

  reg             state00_we;
  reg             state01_we;
  reg             state02_we;
  reg             state03_we;
  reg             state04_we;
  reg             state05_we;
  reg             state06_we;
  reg             state07_we;
  reg             state08_we;
  reg             state09_we;
  reg             state10_we;
  reg             state11_we;
  reg             state12_we;
  reg             state13_we;
  reg             state14_we;
  reg             state15_we;

  reg [31 : 0]    tmp_read_data;
  reg             tmp_error;


  //----------------------------------------------------------------
  // Concurrent connectivity for ports etc.
  //----------------------------------------------------------------
  assign core_block = {block_reg[00], block_reg[01], block_reg[02], block_reg[03],
                       block_reg[04], block_reg[05], block_reg[06], block_reg[07],
                       block_reg[08], block_reg[09], block_reg[10], block_reg[11],
                       block_reg[12], block_reg[13], block_reg[14], block_reg[15],
                       block_reg[16], block_reg[17], block_reg[18], block_reg[19],
                       block_reg[20], block_reg[21], block_reg[22], block_reg[23],
                       block_reg[24], block_reg[25], block_reg[26], block_reg[27],
                       block_reg[28], block_reg[29], block_reg[30], block_reg[31]};

  assign read_data = tmp_read_data;
  assign error     = tmp_error;


  //----------------------------------------------------------------
  // core instantiation.
  //----------------------------------------------------------------
  sha512_core core(
                   .clk(clk),
                   .reset_n(reset_n),

                   .init(init_reg),
                   .next(next_reg),
                   .mode(mode_reg),

                   .work_factor(work_factor_reg),
                   .work_factor_num(work_factor_num_reg),

                   .block(core_block),

                   .ready(core_ready),

                   .state_wr_data(write_data),
                   .state00_we(state00_we),
                   .state01_we(state01_we),
                   .state02_we(state02_we),
                   .state03_we(state03_we),
                   .state04_we(state04_we),
                   .state05_we(state05_we),
                   .state06_we(state06_we),
                   .state07_we(state07_we),
                   .state08_we(state08_we),
                   .state09_we(state09_we),
                   .state10_we(state10_we),
                   .state11_we(state11_we),
                   .state12_we(state12_we),
                   .state13_we(state13_we),
                   .state14_we(state14_we),
                   .state15_we(state15_we),

                   .digest(core_digest),
                   .digest_valid(core_digest_valid)
                  );


  //----------------------------------------------------------------
  // reg_update
  //
  // Update functionality for all registers in the core.
  // All registers are positive edge triggered with asynchronous
  // active low reset. All registers have write enable.
  //----------------------------------------------------------------
  always @ (posedge clk or negedge reset_n)
    begin : reg_update
      integer i;

      if (!reset_n)
        begin
          for (i = 0 ; i < 32 ; i = i + 1)
            block_reg[i] <= 32'h0;

          init_reg            <= 1'h0;
          next_reg            <= 1'h0;
          mode_reg            <= MODE_SHA_512;
          work_factor_reg     <= 1'h0;
          work_factor_num_reg <= DEFAULT_WORK_FACTOR_NUM;
          ready_reg           <= 1'h0;
          digest_reg          <= 512'h0;
          digest_valid_reg    <= 1'h0;
        end
      else
        begin
          ready_reg        <= core_ready;
          digest_valid_reg <= core_digest_valid;
          init_reg         <= init_new;
          next_reg         <= next_new;

          if (mode_we)
            mode_reg <= mode_new;

          if (work_factor_we)
            work_factor_reg <= work_factor_new;

          if (work_factor_num_we)
            work_factor_num_reg <= write_data;

          if (core_digest_valid)
            digest_reg <= core_digest;

          if (block_we)
            block_reg[block_addr] <= write_data;
        end
    end // reg_update


  //----------------------------------------------------------------
  // api_logic
  //
  // Implementation of the api logic. If cs is enabled will either
  // try to write to or read from the internal registers.
  //----------------------------------------------------------------
  always @*
    begin : api_logic
      init_new           = 1'h0;
      next_new           = 1'h0;
      mode_new           = MODE_SHA_512;
      mode_we            = 1'h0;
      work_factor_new    = 1'h0;
      work_factor_we     = 1'h0;
      work_factor_num_we = 1'h0;
      block_we           = 1'h0;
      state00_we         = 1'h0;
      state01_we         = 1'h0;
      state02_we         = 1'h0;
      state03_we         = 1'h0;
      state04_we         = 1'h0;
      state05_we         = 1'h0;
      state06_we         = 1'h0;
      state07_we         = 1'h0;
      state08_we         = 1'h0;
      state09_we         = 1'h0;
      state10_we         = 1'h0;
      state11_we         = 1'h0;
      state12_we         = 1'h0;
      state13_we         = 1'h0;
      state14_we         = 1'h0;
      state15_we         = 1'h0;
      tmp_read_data      = 32'h00000000;
      tmp_error          = 1'h0;

      block_addr = address[4 : 0] - ADDR_BLOCK0[4 : 0];

      if (cs)
        begin
          if (we)
            begin
              if (core_ready)
                begin
                  if ((address >= ADDR_BLOCK0) && (address <= ADDR_BLOCK31))
                    block_we = 1'h1;

                  case (address)
                    ADDR_CTRL:
                      begin
                        init_new        = write_data[CTRL_INIT_BIT];
                        next_new        = write_data[CTRL_NEXT_BIT];
                        mode_new        = write_data[CTRL_MODE_HIGH_BIT : CTRL_MODE_LOW_BIT];
                        mode_we         = 1'h1;
                        work_factor_new = write_data[CTRL_WORK_FACTOR_BIT];
                        work_factor_we  = 1'h1;
                      end

                    ADDR_WORK_FACTOR_NUM:
                      begin
                        work_factor_num_we = 1;
                      end

                    ADDR_DIGEST0:
                      state00_we = 1;

                    ADDR_DIGEST1:
                      state01_we = 1;

                    ADDR_DIGEST2:
                      state02_we = 1;

                    ADDR_DIGEST3:
                      state03_we = 1;

                    ADDR_DIGEST4:
                      state04_we = 1;

                    ADDR_DIGEST5:
                      state05_we = 1;

                    ADDR_DIGEST6:
                      state06_we = 1;

                    ADDR_DIGEST7:
                      state07_we = 1;

                    ADDR_DIGEST8:
                      state08_we = 1;

                    ADDR_DIGEST9:
                      state09_we = 1;

                    ADDR_DIGEST10:
                      state10_we = 1;

                    ADDR_DIGEST11:
                      state11_we = 1;

                    ADDR_DIGEST12:
                      state12_we = 1;

                    ADDR_DIGEST13:
                      state13_we = 1;

                    ADDR_DIGEST14:
                      state14_we = 1;

                    ADDR_DIGEST15:
                      state15_we = 1;

                    default:
                      tmp_error = 1;
                  endcase // case (address)
                end // if (core_ready)
            end // if (we)

          else
            begin
              if (core_ready)
                if ((address >= ADDR_DIGEST0) && (address <= ADDR_DIGEST15))
                  tmp_read_data = digest_reg[(15 - (address - ADDR_DIGEST0)) * 32 +: 32];

              if ((address >= ADDR_BLOCK0) && (address <= ADDR_BLOCK31))
                tmp_read_data = block_reg[address[4 : 0]];

              case (address)
                ADDR_NAME0:
                  tmp_read_data = CORE_NAME0;

                ADDR_NAME1:
                  tmp_read_data = CORE_NAME1;

                ADDR_VERSION:
                  tmp_read_data = CORE_VERSION;

                ADDR_CTRL:
                  tmp_read_data = {24'h000000, work_factor_reg, 3'b000, mode_reg, next_reg, init_reg};

                ADDR_STATUS:
                  tmp_read_data = {28'h0000000, 2'b00, digest_valid_reg, ready_reg};

                ADDR_WORK_FACTOR_NUM:
                  tmp_read_data = work_factor_num_reg;

                default:
                  tmp_error = 1;
              endcase // case (address)
            end
        end
    end // addr_decoder
endmodule // sha512

//======================================================================
// EOF sha512.v
//======================================================================
