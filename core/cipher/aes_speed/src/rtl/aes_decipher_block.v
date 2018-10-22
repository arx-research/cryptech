//======================================================================
//
// aes_decipher_block.v
// --------------------
// The AES decipher round. A pure combinational module that implements
// the initial round, main round and final round logic for
// decciper operations.
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

module aes_decipher_block(
                          input wire            clk,
                          input wire            reset_n,

                          input wire            next,

                          input wire            keylen,
                          output wire [3 : 0]   round,
                          input wire [127 : 0]  round_key,

                          input wire [127 : 0]  block,
                          output wire [127 : 0] new_block,
                          output wire           ready
                         );


  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  localparam AES_128_BIT_KEY = 1'h0;
  localparam AES_256_BIT_KEY = 1'h1;

  localparam AES128_ROUNDS = 4'ha;
  localparam AES256_ROUNDS = 4'he;

  localparam NO_UPDATE    = 2'h0;
  localparam INIT_UPDATE  = 2'h1;
  localparam MAIN_UPDATE  = 2'h2;
  localparam FINAL_UPDATE = 2'h3;

  localparam CTRL_IDLE  = 2'h0;
  localparam CTRL_INIT  = 2'h1;
  localparam CTRL_MAIN  = 2'h2;


  //----------------------------------------------------------------
  // Gaolis multiplication functions for Inverse MixColumn.
  //----------------------------------------------------------------
  function [7 : 0] gm2(input [7 : 0] op);
    begin
      gm2 = {op[6 : 0], 1'b0} ^ (8'h1b & {8{op[7]}});
    end
  endfunction // gm2

  function [7 : 0] gm3(input [7 : 0] op);
    begin
      gm3 = gm2(op) ^ op;
    end
  endfunction // gm3

  function [7 : 0] gm4(input [7 : 0] op);
    begin
      gm4 = gm2(gm2(op));
    end
  endfunction // gm4

  function [7 : 0] gm8(input [7 : 0] op);
    begin
      gm8 = gm2(gm4(op));
    end
  endfunction // gm8

  function [7 : 0] gm09(input [7 : 0] op);
    begin
      gm09 = gm8(op) ^ op;
    end
  endfunction // gm09

  function [7 : 0] gm11(input [7 : 0] op);
    begin
      gm11 = gm8(op) ^ gm2(op) ^ op;
    end
  endfunction // gm11

  function [7 : 0] gm13(input [7 : 0] op);
    begin
      gm13 = gm8(op) ^ gm4(op) ^ op;
    end
  endfunction // gm13

  function [7 : 0] gm14(input [7 : 0] op);
    begin
      gm14 = gm8(op) ^ gm4(op) ^ gm2(op);
    end
  endfunction // gm14

  function [31 : 0] inv_mixw(input [31 : 0] w);
    reg [7 : 0] b0, b1, b2, b3;
    reg [7 : 0] mb0, mb1, mb2, mb3;
    begin
      b0 = w[31 : 24];
      b1 = w[23 : 16];
      b2 = w[15 : 08];
      b3 = w[07 : 00];

      mb0 = gm14(b0) ^ gm11(b1) ^ gm13(b2) ^ gm09(b3);
      mb1 = gm09(b0) ^ gm14(b1) ^ gm11(b2) ^ gm13(b3);
      mb2 = gm13(b0) ^ gm09(b1) ^ gm14(b2) ^ gm11(b3);
      mb3 = gm11(b0) ^ gm13(b1) ^ gm09(b2) ^ gm14(b3);

      inv_mixw = {mb0, mb1, mb2, mb3};
    end
  endfunction // mixw

  function [127 : 0] inv_mixcolumns(input [127 : 0] data);
    reg [31 : 0] w0, w1, w2, w3;
    reg [31 : 0] ws0, ws1, ws2, ws3;
    begin
      w0 = data[127 : 096];
      w1 = data[095 : 064];
      w2 = data[063 : 032];
      w3 = data[031 : 000];

      ws0 = inv_mixw(w0);
      ws1 = inv_mixw(w1);
      ws2 = inv_mixw(w2);
      ws3 = inv_mixw(w3);

      inv_mixcolumns = {ws0, ws1, ws2, ws3};
    end
  endfunction // inv_mixcolumns

  function [127 : 0] inv_shiftrows(input [127 : 0] data);
    reg [31 : 0] w0, w1, w2, w3;
    reg [31 : 0] ws0, ws1, ws2, ws3;
    begin
      w0 = data[127 : 096];
      w1 = data[095 : 064];
      w2 = data[063 : 032];
      w3 = data[031 : 000];

      ws0 = {w0[31 : 24], w3[23 : 16], w2[15 : 08], w1[07 : 00]};
      ws1 = {w1[31 : 24], w0[23 : 16], w3[15 : 08], w2[07 : 00]};
      ws2 = {w2[31 : 24], w1[23 : 16], w0[15 : 08], w3[07 : 00]};
      ws3 = {w3[31 : 24], w2[23 : 16], w1[15 : 08], w0[07 : 00]};

      inv_shiftrows = {ws0, ws1, ws2, ws3};
    end
  endfunction // inv_shiftrows

  function [127 : 0] addroundkey(input [127 : 0] data, input [127 : 0] rkey);
    begin
      addroundkey = data ^ rkey;
    end
  endfunction // addroundkey


  //----------------------------------------------------------------
  // Registers including update variables and write enable.
  //----------------------------------------------------------------
  reg [127 : 0] block_reg;
  reg [127 : 0] block_new;
  reg           block_we;

  reg [3 : 0]   round_ctr_reg;
  reg [3 : 0]   round_ctr_new;
  reg           round_ctr_we;
  reg           round_ctr_set;
  reg           round_ctr_dec;

  reg           ready_reg;
  reg           ready_new;
  reg           ready_we;

  reg [1 : 0]   dec_ctrl_reg;
  reg [1 : 0]   dec_ctrl_new;
  reg           dec_ctrl_we;


  //----------------------------------------------------------------
  // Wires.
  //----------------------------------------------------------------
  reg [31 : 0]  sboxw0;
  reg [31 : 0]  sboxw1;
  reg [31 : 0]  sboxw2;
  reg [31 : 0]  sboxw3;
  wire [31 : 0] new_sboxw0;
  wire [31 : 0] new_sboxw1;
  wire [31 : 0] new_sboxw2;
  wire [31 : 0] new_sboxw3;
  reg [1 : 0]   update_type;


  //----------------------------------------------------------------
  // Inverse S-boxes.
  //----------------------------------------------------------------
  aes_inv_sbox inv_sbox_inst0(.sword(sboxw0), .new_sword(new_sboxw0));
  aes_inv_sbox inv_sbox_inst1(.sword(sboxw1), .new_sword(new_sboxw1));
  aes_inv_sbox inv_sbox_inst2(.sword(sboxw2), .new_sword(new_sboxw2));
  aes_inv_sbox inv_sbox_inst3(.sword(sboxw3), .new_sword(new_sboxw3));


  //----------------------------------------------------------------
  // Concurrent connectivity for ports etc.
  //----------------------------------------------------------------
  assign new_block = block_reg;
  assign round     = round_ctr_reg;
  assign ready     = ready_reg;


  //----------------------------------------------------------------
  // reg_update
  //
  // Update functionality for all registers in the core.
  // All registers are positive edge triggered with synchronous
  // active low reset. All registers have write enable.
  //----------------------------------------------------------------
  always @ (posedge clk or negedge reset_n)
    begin: reg_update
      if (!reset_n)
        begin
          block_reg     <= 128'h0;
          round_ctr_reg <= 4'h0;
          ready_reg     <= 1'b1;
          dec_ctrl_reg  <= CTRL_IDLE;
        end
      else
        begin
          if (block_we)
            block_reg <= block_new;

          if (round_ctr_we)
            round_ctr_reg <= round_ctr_new;

          if (ready_we)
            ready_reg <= ready_new;

          if (dec_ctrl_we)
            dec_ctrl_reg <= dec_ctrl_new;
        end
    end // reg_update


  //----------------------------------------------------------------
  // round_logic
  //
  // The logic needed to implement init, main and final rounds.
  //----------------------------------------------------------------
  always @*
    begin : round_logic
      reg [127 : 0] subbytes_block, inv_shiftrows_block, inv_mixcolumns_block;
      reg [127 : 0] addkey_block;

      inv_shiftrows_block  = 128'h0;
      inv_mixcolumns_block = 128'h0;
      addkey_block         = 128'h0;
      block_new            = 128'h0;
      block_we             = 1'b0;

      sboxw0         = block_reg[127 : 96];
      sboxw1         = block_reg[95  : 64];
      sboxw2         = block_reg[63  : 32];
      sboxw3         = block_reg[31  :  0];
      subbytes_block = {new_sboxw0, new_sboxw1, new_sboxw2, new_sboxw3};

      case (update_type)
        INIT_UPDATE:
          begin
            addkey_block        = addroundkey(block, round_key);
            inv_shiftrows_block = inv_shiftrows(addkey_block);
            block_new           = inv_shiftrows_block;
            block_we            = 1'b1;
          end

        MAIN_UPDATE:
          begin
            addkey_block         = addroundkey(subbytes_block, round_key);
            inv_mixcolumns_block = inv_mixcolumns(addkey_block);
            inv_shiftrows_block  = inv_shiftrows(inv_mixcolumns_block);
            block_new            = inv_shiftrows_block;
            block_we             = 1'b1;
          end

        FINAL_UPDATE:
          begin
            block_new = addroundkey(subbytes_block, round_key);
            block_we  = 1'b1;
          end

        default:
          begin
          end
      endcase // case (update_type)
    end // round_logic


  //----------------------------------------------------------------
  // round_ctr
  //
  // The round counter with reset and increase logic.
  //----------------------------------------------------------------
  always @*
    begin : round_ctr
      round_ctr_new = 4'h0;
      round_ctr_we  = 1'b0;

      if (round_ctr_set)
        begin
          if (keylen == AES_256_BIT_KEY)
            begin
              round_ctr_new = AES256_ROUNDS;
            end
          else
            begin
              round_ctr_new = AES128_ROUNDS;
            end
          round_ctr_we  = 1'b1;
        end
      else if (round_ctr_dec)
        begin
          round_ctr_new = round_ctr_reg - 1'b1;
          round_ctr_we  = 1'b1;
        end
    end // round_ctr


  //----------------------------------------------------------------
  // decipher_ctrl
  //
  // The FSM that controls the decipher operations.
  //----------------------------------------------------------------
  always @*
    begin: decipher_ctrl
      round_ctr_dec = 1'b0;
      round_ctr_set = 1'b0;
      ready_new     = 1'b0;
      ready_we      = 1'b0;
      update_type   = NO_UPDATE;
      dec_ctrl_new  = CTRL_IDLE;
      dec_ctrl_we   = 1'b0;

      case(dec_ctrl_reg)
        CTRL_IDLE:
          begin
            if (next)
              begin
                round_ctr_set = 1'b1;
                ready_new     = 1'b0;
                ready_we      = 1'b1;
                dec_ctrl_new  = CTRL_INIT;
                dec_ctrl_we   = 1'b1;
              end
          end

        CTRL_INIT:
          begin
            round_ctr_dec = 1'b1;
            update_type   = INIT_UPDATE;
            dec_ctrl_new  = CTRL_MAIN;
            dec_ctrl_we   = 1'b1;
          end

        CTRL_MAIN:
          begin
            if (round_ctr_reg > 0)
              begin
                round_ctr_dec = 1'b1;
                update_type   = MAIN_UPDATE;
              end
            else
              begin
                update_type  = FINAL_UPDATE;
                ready_new    = 1'b1;
                ready_we     = 1'b1;
                dec_ctrl_new = CTRL_IDLE;
                dec_ctrl_we  = 1'b1;
              end
          end

        default:
          begin
            // Empty. Just here to make the synthesis tool happy.
          end
      endcase // case (dec_ctrl_reg)
    end // decipher_ctrl
endmodule // aes_decipher_block

//======================================================================
// EOF aes_decipher_block.v
//======================================================================
