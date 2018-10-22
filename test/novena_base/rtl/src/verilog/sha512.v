//======================================================================
//
// sha512.v
// ------
// Top level wrapper for the SHA-512 hash function providing
// a simple memory like interface with 32 bit data access.
//
// Authors: Joachim Strömbergson, Paul Selkirk
// Copyright (c) 2014-2015, NORDUnet A/S All rights reserved.
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
            input wire [7 : 0]   address,
            input wire [31 : 0]  write_data,
            output wire [31 : 0] read_data
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

   parameter ADDR_BLOCK           = 8'h10;

   parameter ADDR_DIGEST          = 8'h40;

   parameter CORE_NAME0           = 32'h73686132; // "sha2"
   parameter CORE_NAME1           = 32'h2d353132; // "-512"
   parameter CORE_VERSION         = 32'h302e3830; // "0.80"

   parameter MODE_SHA_512_224     = 2'h0;
   parameter MODE_SHA_512_256     = 2'h1;
   parameter MODE_SHA_384         = 2'h2;
   parameter MODE_SHA_512         = 2'h3;

   parameter DEFAULT_WORK_FACTOR_NUM = 32'h000f0000;

   parameter BLOCK_BITS           = 1024;
   parameter DIGEST_BITS          = 512;
   parameter BLOCK_WORDS          = BLOCK_BITS / 32;
   parameter DIGEST_WORDS         = DIGEST_BITS / 32;

   //----------------------------------------------------------------
   // Registers.
   //----------------------------------------------------------------
   reg [0 : BLOCK_BITS - 1]    block_reg;
   reg [0 : DIGEST_BITS - 1]   digest_reg;
   reg                         init_reg;
   reg                         next_reg;
   reg [1 : 0]                 mode_reg;
   reg                         work_factor_reg;
   reg [31 : 0]                work_factor_num_reg;

   reg [31 : 0]                tmp_read_data;
   reg [31 : 0]                tmp_read_data_reg;

   //----------------------------------------------------------------
   // Wires.
   //----------------------------------------------------------------
   wire                        core_init;
   wire                        core_next;
   wire                        core_ready;
   wire [1 : 0]                core_mode;
   wire                        core_work_factor;
   wire [31 : 0]               core_work_factor_num;
   wire [0 : BLOCK_BITS - 1]   core_block;
   wire [0 : DIGEST_BITS - 1]  core_digest;
   wire                        core_digest_valid;

   wire [31 : 0]               core_name0   = CORE_NAME0;
   wire [31 : 0]               core_name1   = CORE_NAME1;
   wire [31 : 0]               core_version = CORE_VERSION;
   wire [31 : 0]               core_ctrl;
   wire [31 : 0]               core_status;

   //----------------------------------------------------------------
   // Concurrent connectivity for ports etc.
   //----------------------------------------------------------------
   assign core_init   = init_reg;
   assign core_next   = next_reg;
   assign core_mode = mode_reg;
   assign core_work_factor = work_factor_reg;
   assign core_work_factor_num = work_factor_num_reg;
   assign core_ctrl   = {24'h000000, work_factor_reg, 3'b000,
                         mode_reg, next_reg, init_reg};
   assign core_status = { 30'b0, core_digest_valid, core_ready };
   assign core_block  = block_reg;

   assign read_data   = tmp_read_data_reg;

   //----------------------------------------------------------------
   // core instantiation.
   //----------------------------------------------------------------
   sha512_core core(
                    .clk(clk),
                    .reset_n(reset_n),

                    .init(core_init),
                    .next(core_next),
                    .mode(core_mode),

                    .work_factor(core_work_factor),
                    .work_factor_num(core_work_factor_num),

                    .block(core_block),

                    .ready(core_ready),

                    .digest(core_digest),
                    .digest_valid(core_digest_valid)
                  );


   //----------------------------------------------------------------
   // latch in digest when ready
   //----------------------------------------------------------------
   always @(posedge clk)
      begin
         if (core_digest_valid)
           digest_reg <= core_digest;
      end

   //----------------------------------------------------------------
   // storage registers for mapping memory to core interface
   //----------------------------------------------------------------
   always @(posedge clk)
     begin
        init_reg <= 0;
        next_reg <= 0;
        mode_reg            <= MODE_SHA_512;
        work_factor_reg     <= 0;
        work_factor_num_reg <= DEFAULT_WORK_FACTOR_NUM;

        if (cs && we)
          begin
             // write operations
             if ((address >= ADDR_BLOCK) &&
                 (address < ADDR_BLOCK + BLOCK_WORDS))
               block_reg[((address - ADDR_BLOCK) * 32)+:32] <= write_data;
             else if (address == ADDR_CTRL)
               begin
                  init_reg <= write_data[CTRL_INIT_BIT];
                  next_reg <= write_data[CTRL_NEXT_BIT];
                  mode_reg        <= write_data[CTRL_MODE_HIGH_BIT : CTRL_MODE_LOW_BIT];
                  work_factor_reg <= write_data[CTRL_WORK_FACTOR_BIT];
               end
             else if (address == ADDR_WORK_FACTOR_NUM)
               begin
                  work_factor_num_reg <= write_data;
               end
          end
     end

   always @*
     begin
        tmp_read_data = 32'h00000000;

        if (cs && !we)
          begin
             // read operations
             if ((address >= ADDR_BLOCK) &&
                 (address < ADDR_BLOCK + BLOCK_WORDS))
               tmp_read_data = block_reg[((address - ADDR_BLOCK) * 32)+:32];
             else if ((address >= ADDR_DIGEST) &&
                      (address < ADDR_DIGEST + DIGEST_WORDS))
               tmp_read_data = digest_reg[((address - ADDR_DIGEST) * 32)+:32];
             else
               case (address)
                 ADDR_NAME0:
                   tmp_read_data = core_name0;
                 ADDR_NAME1:
                   tmp_read_data = core_name1;
                 ADDR_VERSION:
                   tmp_read_data = core_version;
                 ADDR_CTRL:
                   tmp_read_data = core_ctrl;
                 ADDR_STATUS:
                   tmp_read_data = core_status;
                 ADDR_WORK_FACTOR_NUM:
                   tmp_read_data = work_factor_num_reg;
               endcase
          end
     end

   always @(posedge clk)
     begin
        tmp_read_data_reg <= tmp_read_data;
     end

endmodule // sha512
