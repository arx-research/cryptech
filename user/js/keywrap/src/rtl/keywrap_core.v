//======================================================================
//
// keywrap_core.v
// --------------
// Core that tries to implement AES KEY WRAP as specified in
// RFC 3394 and extended with padding in RFC 5649.
// Experimental core at the moment. Does Not Work.
// The maximum wrap object size is 64 kByte.
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

module keywrap_core #(parameter MEM_BITS = 11)
                    (
                     input wire                       clk,
                     input wire                       reset_n,

                     input wire                       init,
                     input wire                       next,
                     input wire                       encdec,

                     output wire                      ready,
                     output wire                      valid,

                     input wire [(MEM_BITS - 2) : 0]  rlen,

                     input wire [255 : 0]             key,
                     input wire                       keylen,

                     input wire  [63 : 0]             a_init,
                     output wire [63 : 0]             a_result,

                     input wire                       api_we,
                     input wire  [(MEM_BITS - 1) : 0] api_addr,
                     input wire [31 : 0]              api_wr_data,
                     output wire [31 : 0]             api_rd_data
                    );


  //----------------------------------------------------------------
  // Paramenters and local defines.
  //----------------------------------------------------------------
  localparam MAX_ITERATIONS = 6 - 1;

  localparam CTRL_IDLE          = 4'h0;
  localparam CTRL_INIT_WAIT     = 4'h1;
  localparam CTRL_NEXT_WSTART   = 4'h2;
  localparam CTRL_NEXT_USTART   = 4'h3;
  localparam CTRL_NEXT_LOOP0    = 4'h4;
  localparam CTRL_NEXT_LOOP     = 4'h5;
  localparam CTRL_NEXT_WAIT     = 4'h6;
  localparam CTRL_NEXT_UPDATE   = 4'h7;
  localparam CTRL_NEXT_WCHECK   = 4'h8;
  localparam CTRL_NEXT_UCHECK   = 4'h9;
  localparam CTRL_NEXT_FINALIZE = 4'ha;


  //----------------------------------------------------------------
  // Registers and memories including control signals.
  //----------------------------------------------------------------
  reg [63 : 0] a_reg;
  reg [63 : 0] a_new;
  reg          a_we;
  reg          init_a;

  reg          ready_reg;
  reg          ready_new;
  reg          ready_we;

  reg          valid_reg;
  reg          valid_new;
  reg          valid_we;

  reg [(MEM_BITS - 2) : 0] block_ctr_reg;
  reg [(MEM_BITS - 2) : 0] block_ctr_new;
  reg                      block_ctr_we;
  reg                      block_ctr_dec;
  reg                      block_ctr_inc;
  reg                      block_ctr_rst;
  reg                      block_ctr_set;

  reg [2 : 0]  iteration_ctr_reg;
  reg [2 : 0]  iteration_ctr_new;
  reg          iteration_ctr_we;
  reg          iteration_ctr_inc;
  reg          iteration_ctr_dec;
  reg          iteration_ctr_set;
  reg          iteration_ctr_rst;

  reg [3 : 0]  keywrap_core_ctrl_reg;
  reg [3 : 0]  keywrap_core_ctrl_new;
  reg          keywrap_core_ctrl_we;


  //----------------------------------------------------------------
  // Wires.
  //----------------------------------------------------------------
  reg            aes_init;
  reg            aes_next;
  wire           aes_ready;
  wire           aes_valid;
  reg  [127 : 0] aes_block;
  wire [127 : 0] aes_result;

  reg            update_state;

  reg                      core_we;
  reg [(MEM_BITS - 2) : 0] core_addr;
  reg [63 : 0]             core_wr_data;
  wire [63 : 0]            core_rd_data;


  //----------------------------------------------------------------
  // Instantiations.
  //----------------------------------------------------------------
  keywrap_mem #(.API_ADDR_BITS(MEM_BITS))
              mem(
                  .clk(clk),

                  .api_we(api_we),
                  .api_addr(api_addr),
                  .api_wr_data(api_wr_data),
                  .api_rd_data(api_rd_data),

                  .core_we(core_we),
                  .core_addr(core_addr),
                  .core_wr_data(core_wr_data),
                  .core_rd_data(core_rd_data)
                 );


  aes_core aes(
               .clk(clk),
               .reset_n(reset_n),

               .encdec(encdec),
               .init(aes_init),
               .next(aes_next),

               .key(key),
               .keylen(keylen),

               .block(aes_block),

               .ready(aes_ready),
               .result(aes_result),
               .result_valid(aes_valid)
              );


  //----------------------------------------------------------------
  // Assignments for ports.
  //----------------------------------------------------------------
  assign a_result = a_reg;
  assign ready    = ready_reg;
  assign valid    = valid_reg;


  //----------------------------------------------------------------
  // reg_update
  //----------------------------------------------------------------
  always @ (posedge clk or negedge reset_n)
    begin: reg_update
      if (!reset_n)
        begin
          a_reg                 <= 64'h0;
          ready_reg             <= 1'h1;
          valid_reg             <= 1'h1;
          block_ctr_reg         <= {(MEM_BITS - 1){1'h0}};
          iteration_ctr_reg     <= 3'h0;
          keywrap_core_ctrl_reg <= CTRL_IDLE;
       end

      else
        begin
          if (a_we)
            a_reg <= a_new;

          if (ready_we)
            ready_reg <= ready_new;

          if (valid_we)
            valid_reg <= valid_new;

          if (block_ctr_we)
            block_ctr_reg <= block_ctr_new;

          if (iteration_ctr_we)
            iteration_ctr_reg <= iteration_ctr_new;

          if (keywrap_core_ctrl_we)
            keywrap_core_ctrl_reg <= keywrap_core_ctrl_new;
        end
    end // reg_update


  //----------------------------------------------------------------
  // keywrap_logic
  //
  // Main logic for the key wrap functionality.
  //----------------------------------------------------------------
  always @*
    begin : keywrap_logic
      reg [63 : 0] xor_val;

      a_new     = 64'h0;
      a_we      = 1'h0;
      core_addr = block_ctr_reg;
      core_we   = 1'h0;

      xor_val = (rlen * iteration_ctr_reg) + {51'h0, (block_ctr_reg + 1'h1)};

      if (encdec)
        aes_block = {a_reg, core_rd_data};
      else
        aes_block = {(a_reg ^ xor_val), core_rd_data};

      core_wr_data = aes_result[63 : 0];

      if (init_a)
        begin
          a_new = a_init;
          a_we  = 1'h1;
        end

      if (update_state)
        begin
          core_we = 1'h1;

          if (encdec)
            a_new   = aes_result[127 : 64] ^ xor_val;
          else
            a_new   = aes_result[127 : 64];
          a_we    = 1'h1;
        end
    end


  //----------------------------------------------------------------
  // block_ctr
  //----------------------------------------------------------------
  always @*
    begin : block_ctr
      block_ctr_new = {(MEM_BITS - 1){1'h0}};
      block_ctr_we  = 1'h0;

      if (block_ctr_rst)
        begin
          block_ctr_new = {(MEM_BITS - 1){1'h0}};
          block_ctr_we  = 1'h1;
        end

      if (block_ctr_set)
        begin
          block_ctr_new = (rlen - 1'h1);
          block_ctr_we  = 1'h1;
        end

      if (block_ctr_dec)
        begin
          block_ctr_new = block_ctr_reg - 1'h1;
          block_ctr_we  = 1'h1;
        end

      if (block_ctr_inc)
        begin
          block_ctr_new = block_ctr_reg + 1'h1;
          block_ctr_we  = 1'h1;
        end
    end


  //----------------------------------------------------------------
  // iteration_ctr
  //----------------------------------------------------------------
  always @*
    begin : iteration_ctr
      iteration_ctr_new = 3'h0;
      iteration_ctr_we  = 1'h0;

      if (iteration_ctr_rst)
        begin
          iteration_ctr_new = 3'h0;
          iteration_ctr_we  = 1'h1;
        end

      if (iteration_ctr_set)
        begin
          iteration_ctr_new = MAX_ITERATIONS;
          iteration_ctr_we  = 1'h1;
        end

      if (iteration_ctr_dec)
        begin
          iteration_ctr_new = iteration_ctr_reg - 1'h1;
          iteration_ctr_we  = 1'h1;
        end

      if (iteration_ctr_inc)
        begin
          iteration_ctr_new = iteration_ctr_reg + 1'h1;
          iteration_ctr_we  = 1'h1;
        end

    end


  //----------------------------------------------------------------
  // keywrap_core_ctrl
  //----------------------------------------------------------------
  always @*
    begin : keywrap_core_ctrl
      ready_new             = 1'h0;
      ready_we              = 1'h0;
      valid_new             = 1'h0;
      valid_we              = 1'h0;
      init_a                = 1'h0;
      update_state          = 1'h0;
      aes_init              = 1'h0;
      aes_next              = 1'h0;
      block_ctr_dec         = 1'h0;
      block_ctr_inc         = 1'h0;
      block_ctr_rst         = 1'h0;
      block_ctr_set         = 1'h0;
      iteration_ctr_inc     = 1'h0;
      iteration_ctr_dec     = 1'h0;
      iteration_ctr_set     = 1'h0;
      iteration_ctr_rst     = 1'h0;
      keywrap_core_ctrl_new = CTRL_IDLE;
      keywrap_core_ctrl_we  = 1'h0;


      case (keywrap_core_ctrl_reg)
        CTRL_IDLE:
          begin
            if (init)
              begin
                aes_init              = 1'h1;
                ready_new             = 1'h0;
                ready_we              = 1'h1;
                valid_new             = 1'h0;
                valid_we              = 1'h1;
                keywrap_core_ctrl_new = CTRL_INIT_WAIT;
                keywrap_core_ctrl_we  = 1'h1;
              end

            if (next)
              begin
                ready_new             = 1'h0;
                ready_we              = 1'h1;
                valid_new             = 1'h0;
                valid_we              = 1'h1;
                init_a                = 1'h1;

                if (encdec)
                  keywrap_core_ctrl_new = CTRL_NEXT_WSTART;
                else
                  keywrap_core_ctrl_new = CTRL_NEXT_USTART;
                keywrap_core_ctrl_we  = 1'h1;
              end
          end


        CTRL_INIT_WAIT:
          begin
            if (aes_ready)
              begin
                ready_new             = 1'h1;
                ready_we              = 1'h1;
                keywrap_core_ctrl_new = CTRL_IDLE;
                keywrap_core_ctrl_we  = 1'h1;
              end
          end


        CTRL_NEXT_WSTART:
          begin
            block_ctr_rst         = 1'h1;
            iteration_ctr_rst     = 1'h1;
            keywrap_core_ctrl_new = CTRL_NEXT_LOOP0;
            keywrap_core_ctrl_we  = 1'h1;
          end


        CTRL_NEXT_USTART:
          begin
            block_ctr_set         = 1'h1;
            iteration_ctr_set     = 1'h1;
            keywrap_core_ctrl_new = CTRL_NEXT_LOOP0;
            keywrap_core_ctrl_we  = 1'h1;
          end


        CTRL_NEXT_LOOP0:
          begin
            keywrap_core_ctrl_new = CTRL_NEXT_LOOP;
            keywrap_core_ctrl_we  = 1'h1;
          end


        CTRL_NEXT_LOOP:
          begin
            aes_next              = 1'h1;
            keywrap_core_ctrl_new = CTRL_NEXT_WAIT;
            keywrap_core_ctrl_we  = 1'h1;
          end


        CTRL_NEXT_WAIT:
          begin
            if (aes_ready)
              begin
                keywrap_core_ctrl_new = CTRL_NEXT_UPDATE;
                keywrap_core_ctrl_we  = 1'h1;
              end
          end


        CTRL_NEXT_UPDATE:
          begin
            update_state = 1'h1;

            if (encdec)
              keywrap_core_ctrl_new = CTRL_NEXT_WCHECK;
            else
              keywrap_core_ctrl_new = CTRL_NEXT_UCHECK;
            keywrap_core_ctrl_we  = 1'h1;
          end


        CTRL_NEXT_WCHECK:
          begin
            if (block_ctr_reg < (rlen - 1'h1))
              begin
                block_ctr_inc         = 1'h1;
                keywrap_core_ctrl_new = CTRL_NEXT_LOOP0;
                keywrap_core_ctrl_we  = 1'h1;
              end

            else if (iteration_ctr_reg < MAX_ITERATIONS)
              begin
                block_ctr_rst         = 1'h1;
                iteration_ctr_inc     = 1'h1;
                keywrap_core_ctrl_new = CTRL_NEXT_LOOP0;
                keywrap_core_ctrl_we  = 1'h1;
              end

            else
              begin
                keywrap_core_ctrl_new = CTRL_NEXT_FINALIZE;
                keywrap_core_ctrl_we  = 1'h1;
              end
          end


        CTRL_NEXT_UCHECK:
          begin
            if (block_ctr_reg > 0)
              begin
                block_ctr_dec         = 1'h1;
                keywrap_core_ctrl_new = CTRL_NEXT_LOOP0;
                keywrap_core_ctrl_we  = 1'h1;
              end

            else if (iteration_ctr_reg > 0)
              begin
                block_ctr_set         = 1'h1;
                iteration_ctr_dec     = 1'h1;
                keywrap_core_ctrl_new = CTRL_NEXT_LOOP0;
                keywrap_core_ctrl_we  = 1'h1;
              end

            else
              begin
                keywrap_core_ctrl_new = CTRL_NEXT_FINALIZE;
                keywrap_core_ctrl_we  = 1'h1;
              end
          end


        CTRL_NEXT_FINALIZE:
          begin
            ready_new             = 1'h1;
            ready_we              = 1'h1;
            valid_new             = 1'h1;
            valid_we              = 1'h1;
            keywrap_core_ctrl_new = CTRL_IDLE;
            keywrap_core_ctrl_we  = 1'h1;
          end

        default:
          begin

          end
      endcase // case (keywrap_core_ctrl_reg)
    end // keywrap_core_ctrl

endmodule // keywrap_core

//======================================================================
// EOF keywrap_core.v
//======================================================================
