//======================================================================
//
// sha512.v
// --------
// Top level wrapper for the SHA-512 hash function providing
// a simple write()/read() interface with 8 bit data access.
//
//
// Authors: Joachim Strombergson, Paul Selkirk
// Copyright (c) 2014, SUNET
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

module sha512(
              // Clock and reset.
              input wire          clk,
              input wire          reset_n,

              // SHA-512 mode
              input wire [1 : 0]  mode,

              // Interface to communication core
              input wire          rx_syn,
              input wire [7 : 0]  rx_data,
              output wire         rx_ack,

              output wire         tx_syn,
              output wire [7 : 0] tx_data,
              input wire          tx_ack
              );

   //----------------------------------------------------------------
   // Internal constant and parameter definitions.
   //----------------------------------------------------------------
   parameter RX_INIT = 0;
   parameter RX_SYN  = 1;
   parameter RX_ACK  = 2;
   parameter RX_WAIT = 3;

   parameter TX_INIT = 0;
   parameter TX_SYN  = 1;
   parameter TX_ACK  = 2;

   parameter MODE_SHA_512_224   = 2'h0;
   parameter MODE_SHA_512_256   = 2'h1;
   parameter MODE_SHA_384       = 2'h2;
   parameter MODE_SHA_512       = 2'h3;

   parameter BLOCK_BITS           = 1024;
   parameter BLOCK_BYTES          = BLOCK_BITS / 8;
   parameter DIGEST_BITS          = 512;
   parameter DIGEST_BYTES         = DIGEST_BITS / 8;
   parameter DIGEST_BYTES_512_224 = 224 / 8;
   parameter DIGEST_BYTES_512_256 = 256 / 8;
   parameter DIGEST_BYTES_384     = 384 / 8;
   parameter DIGEST_BYTES_512     = 512 / 8;

   //----------------------------------------------------------------
   // Registers including update variables and write enable.
   //----------------------------------------------------------------
   reg                          rx_ack_reg;
   reg                          rx_ack_new;
   reg                          rx_ack_we;

   reg [6 : 0]                  rx_ptr_reg;
   reg [7 : 0]                  rx_ptr_new;
   reg                          rx_ptr_we;

   reg [1 : 0]                  rx_ctrl_reg;
   reg [1 : 0]                  rx_ctrl_new;
   reg                          rx_ctrl_we;

   reg [7 : 0]                  block_reg [0 : BLOCK_BYTES - 1];
   reg [7 : 0]                  block_new;
   reg                          block_we;

   reg                          init_reg;
   reg                          init_new;
   reg                          init_we;

   reg                          next_reg;
   reg                          next_new;
   reg                          next_we;

   reg                          initnext_reg;
   reg                          initnext_new;
   reg                          initnext_we;

   reg                          tx_syn_reg;
   reg                          tx_syn_new;
   reg                          tx_syn_we;

   reg [6 : 0]                  tx_ptr_reg;
   reg [7 : 0]                  tx_ptr_new;
   reg                          tx_ptr_we;

   reg [1 : 0]                  tx_ctrl_reg;
   reg [1 : 0]                  tx_ctrl_new;
   reg                          tx_ctrl_we;

   reg                          tx_active_reg;
   reg                          tx_active_new;
   reg                          tx_active_we;

   wire [7 : 0]                 digest_reg [0 : DIGEST_BYTES - 1];

   //----------------------------------------------------------------
   // Wires.
   //----------------------------------------------------------------
   wire                         core_init;
   wire                         core_next;
   wire                         core_ready;
   wire [BLOCK_BITS - 1 : 0]    core_block;
   wire [DIGEST_BITS - 1 : 0]   core_digest;
   wire                         core_digest_valid;

   //----------------------------------------------------------------
   // Concurrent connectivity for ports etc.
   //----------------------------------------------------------------
   assign core_init  = init_reg;
   assign core_next  = next_reg;
   assign core_block = {block_reg[0], block_reg[1], block_reg[2], block_reg[3],
                        block_reg[4], block_reg[5], block_reg[6], block_reg[7],
                        block_reg[8], block_reg[9], block_reg[10], block_reg[11],
                        block_reg[12], block_reg[13], block_reg[14], block_reg[15],
                        block_reg[16], block_reg[17], block_reg[18], block_reg[19],
                        block_reg[20], block_reg[21], block_reg[22], block_reg[23],
                        block_reg[24], block_reg[25], block_reg[26], block_reg[27],
                        block_reg[28], block_reg[29], block_reg[30], block_reg[31],
                        block_reg[32], block_reg[33], block_reg[34], block_reg[35],
                        block_reg[36], block_reg[37], block_reg[38], block_reg[39],
                        block_reg[40], block_reg[41], block_reg[42], block_reg[43],
                        block_reg[44], block_reg[45], block_reg[46], block_reg[47],
                        block_reg[48], block_reg[49], block_reg[50], block_reg[51],
                        block_reg[52], block_reg[53], block_reg[54], block_reg[55],
                        block_reg[56], block_reg[57], block_reg[58], block_reg[59],
                        block_reg[60], block_reg[61], block_reg[62], block_reg[63],
                        block_reg[64], block_reg[65], block_reg[66], block_reg[67],
                        block_reg[68], block_reg[69], block_reg[70], block_reg[71],
                        block_reg[72], block_reg[73], block_reg[74], block_reg[75],
                        block_reg[76], block_reg[77], block_reg[78], block_reg[79],
                        block_reg[80], block_reg[81], block_reg[82], block_reg[83],
                        block_reg[84], block_reg[85], block_reg[86], block_reg[87],
                        block_reg[88], block_reg[89], block_reg[90], block_reg[91],
                        block_reg[92], block_reg[93], block_reg[94], block_reg[95],
                        block_reg[96], block_reg[97], block_reg[98], block_reg[99],
                        block_reg[100], block_reg[101], block_reg[102], block_reg[103],
                        block_reg[104], block_reg[105], block_reg[106], block_reg[107],
                        block_reg[108], block_reg[109], block_reg[110], block_reg[111],
                        block_reg[112], block_reg[113], block_reg[114], block_reg[115],
                        block_reg[116], block_reg[117], block_reg[118], block_reg[119],
                        block_reg[120], block_reg[121], block_reg[122], block_reg[123],
                        block_reg[124], block_reg[125], block_reg[126], block_reg[127]};

   assign rx_ack     = rx_ack_reg;
   assign tx_syn     = tx_syn_reg;
   assign tx_data    = digest_reg[tx_ptr_reg];

   assign digest_reg[0]  = core_digest[511 : 504];
   assign digest_reg[1]  = core_digest[503 : 496];
   assign digest_reg[2]  = core_digest[495 : 488];
   assign digest_reg[3]  = core_digest[487 : 480];
   assign digest_reg[4]  = core_digest[479 : 472];
   assign digest_reg[5]  = core_digest[471 : 464];
   assign digest_reg[6]  = core_digest[463 : 456];
   assign digest_reg[7]  = core_digest[455 : 448];
   assign digest_reg[8]  = core_digest[447 : 440];
   assign digest_reg[9]  = core_digest[439 : 432];
   assign digest_reg[10] = core_digest[431 : 424];
   assign digest_reg[11] = core_digest[423 : 416];
   assign digest_reg[12] = core_digest[415 : 408];
   assign digest_reg[13] = core_digest[407 : 400];
   assign digest_reg[14] = core_digest[399 : 392];
   assign digest_reg[15] = core_digest[391 : 384];
   assign digest_reg[16] = core_digest[383 : 376];
   assign digest_reg[17] = core_digest[375 : 368];
   assign digest_reg[18] = core_digest[367 : 360];
   assign digest_reg[19] = core_digest[359 : 352];
   assign digest_reg[20] = core_digest[351 : 344];
   assign digest_reg[21] = core_digest[343 : 336];
   assign digest_reg[22] = core_digest[335 : 328];
   assign digest_reg[23] = core_digest[327 : 320];
   assign digest_reg[24] = core_digest[319 : 312];
   assign digest_reg[25] = core_digest[311 : 304];
   assign digest_reg[26] = core_digest[303 : 296];
   assign digest_reg[27] = core_digest[295 : 288];
   assign digest_reg[28] = core_digest[287 : 280];
   assign digest_reg[29] = core_digest[279 : 272];
   assign digest_reg[30] = core_digest[271 : 264];
   assign digest_reg[31] = core_digest[263 : 256];
   assign digest_reg[32] = core_digest[255 : 248];
   assign digest_reg[33] = core_digest[247 : 240];
   assign digest_reg[34] = core_digest[239 : 232];
   assign digest_reg[35] = core_digest[231 : 224];
   assign digest_reg[36] = core_digest[223 : 216];
   assign digest_reg[37] = core_digest[215 : 208];
   assign digest_reg[38] = core_digest[207 : 200];
   assign digest_reg[39] = core_digest[199 : 192];
   assign digest_reg[40] = core_digest[191 : 184];
   assign digest_reg[41] = core_digest[183 : 176];
   assign digest_reg[42] = core_digest[175 : 168];
   assign digest_reg[43] = core_digest[167 : 160];
   assign digest_reg[44] = core_digest[159 : 152];
   assign digest_reg[45] = core_digest[151 : 144];
   assign digest_reg[46] = core_digest[143 : 136];
   assign digest_reg[47] = core_digest[135 : 128];
   assign digest_reg[48] = core_digest[127 : 120];
   assign digest_reg[49] = core_digest[119 : 112];
   assign digest_reg[50] = core_digest[111 : 104];
   assign digest_reg[51] = core_digest[103 : 96];
   assign digest_reg[52] = core_digest[95 : 88];
   assign digest_reg[53] = core_digest[87 : 80];
   assign digest_reg[54] = core_digest[79 : 72];
   assign digest_reg[55] = core_digest[71 : 64];
   assign digest_reg[56] = core_digest[63 : 56];
   assign digest_reg[57] = core_digest[55 : 48];
   assign digest_reg[58] = core_digest[47 : 40];
   assign digest_reg[59] = core_digest[39 : 32];
   assign digest_reg[60] = core_digest[31 : 24];
   assign digest_reg[61] = core_digest[23 : 16];
   assign digest_reg[62] = core_digest[15 : 8];
   assign digest_reg[63] = core_digest[7 : 0];

   //----------------------------------------------------------------
   // core instantiation.
   //----------------------------------------------------------------
   sha512_core core(
                    .clk(clk),
                    .reset_n(reset_n),

                    .init(core_init),
                    .next(core_next),
                    .mode(mode),

                    .block(core_block),

                    .ready(core_ready),

                    .digest(core_digest),
                    .digest_valid(core_digest_valid)
                    );

   //----------------------------------------------------------------
   // reg_update
   // Update functionality for all registers in the core.
   // All registers are positive edge triggered with synchronous
   // active low reset. All registers have write enable.
   //----------------------------------------------------------------

   always @ (posedge clk)
     begin: reg_update
        if (!reset_n)
          begin: reset_reg
             reg[7:0] i;
             for (i = 0; i < BLOCK_BYTES; i = i + 1)
               block_reg[i] <= 0;
             rx_ack_reg   <= 0;
             rx_ptr_reg   <= 0;
             rx_ctrl_reg  <= RX_INIT;
             tx_syn_reg   <= 0;
             tx_ptr_reg   <= 0;
             tx_ctrl_reg  <= TX_INIT;
             initnext_reg <= 0;
          end
        else
          begin
             if (rx_ack_we)
               begin
                  rx_ack_reg <= rx_ack_new;
               end

             if (rx_ptr_we)
               begin
                  rx_ptr_reg <= rx_ptr_new[6:0];
               end

             if (rx_ctrl_we)
               begin
                  rx_ctrl_reg <= rx_ctrl_new;
               end

             if (block_we)
               begin
                  block_reg[rx_ptr_reg] <= block_new;
               end

             if (init_we)
               begin
                  init_reg <= init_new;
               end

             if (next_we)
               begin
                  next_reg <= next_new;
               end

             if (initnext_we)
               begin
                  initnext_reg <= initnext_new;
               end

             if (tx_syn_we)
               begin
                  tx_syn_reg <= tx_syn_new;
               end

             if (tx_ptr_we)
               begin
                  tx_ptr_reg <= tx_ptr_new[6:0];
               end

             if (tx_ctrl_we)
               begin
                  tx_ctrl_reg <= tx_ctrl_new;
               end

             if (tx_active_we)
               begin
                  tx_active_reg <= tx_active_new;
               end
          end
     end // reg_update

   //----------------------------------------------------------------
   // rx_engine
   //----------------------------------------------------------------
   always @*
     begin: rx_engine
        rx_ack_new = 0;
        rx_ack_we = 0;
        rx_ptr_new = 0;
        rx_ptr_we = 0;
        rx_ctrl_new = 0;
        rx_ctrl_we = 0;
        block_new = 0;
        block_we = 0;
        init_new = 0;
        init_we = 0;
        next_new = 0;
        next_we = 0;
        initnext_new = 0;
        initnext_we = 0;

        if (tx_active_reg)
          begin
             initnext_we = 1;
          end

        case (rx_ctrl_reg)
          RX_INIT:
            if (core_ready)
              begin
                 rx_ctrl_new = RX_SYN;
                 rx_ctrl_we = 1;
              end
          RX_SYN:
            if (rx_syn)
              begin
                 rx_ack_new = 1;
                 rx_ack_we = 1;
                 block_new = rx_data;
                 block_we = 1;
                 rx_ctrl_new = RX_ACK;
                 rx_ctrl_we = 1;
              end
          RX_ACK:
            if (!rx_syn)
              begin
                 rx_ack_new = 0;
                 rx_ack_we = 1;
                 rx_ptr_new = rx_ptr_reg + 1;
                 rx_ptr_we = 1;
                 rx_ctrl_new = RX_SYN;
                 rx_ctrl_we = 1;
                 if (rx_ptr_new == BLOCK_BYTES)
                   begin
                      rx_ptr_new = 0;
                      rx_ctrl_new = RX_WAIT;
                      if (initnext_reg == 0)
                        begin
                           init_new = 1;
                           init_we = 1;
                           initnext_new = 1;
                           initnext_we = 1;
                        end
                      else
                        begin
                           next_new = 1;
                           next_we = 1;
                        end
                   end
              end
          RX_WAIT:
            if (!core_ready)
              begin
                 init_new = 0;
                 init_we = 1;
                 next_new = 0;
                 next_we = 1;
                 rx_ctrl_new = RX_INIT;
                 rx_ctrl_we = 1;
              end
        endcase // case (rx_ctrl_reg)
     end // rx_engine

   //----------------------------------------------------------------
   // tx_engine
   //----------------------------------------------------------------
   always @*
     begin: tx_engine
        tx_syn_new = 0;
        tx_syn_we = 0;
        tx_ptr_new = 0;
        tx_ptr_we = 0;
        tx_ctrl_new = 0;
        tx_ctrl_we = 0;
        tx_active_new = 0;
        tx_active_we = 0;

        case (tx_ctrl_reg)
          TX_INIT:
            if (core_digest_valid)
              begin
                 tx_ctrl_new = TX_SYN;
                 tx_ctrl_we = 1;
              end
          TX_SYN:
            begin
               tx_syn_new = 1;
               tx_syn_we = 1;
               tx_ctrl_new = TX_ACK;
               tx_ctrl_we = 1;
            end
          TX_ACK:
            if (!core_digest_valid)
              begin
                 tx_syn_new = 0;
                 tx_syn_we = 1;
                 tx_ptr_new = 0;
                 tx_ptr_we = 1;
                 tx_ctrl_new = TX_INIT;
                 tx_ctrl_we = 1;
              end
            else if (tx_ack)
              begin
                 tx_active_new = 1;
                 tx_active_we = 1;
                 tx_syn_new = 0;
                 tx_syn_we = 1;
                 tx_ptr_new = tx_ptr_reg + 1;
                 tx_ptr_we = 1;
                 tx_ctrl_new = TX_SYN;
                 tx_ctrl_we = 1;
                 if (((mode == MODE_SHA_512_224) && (tx_ptr_new == DIGEST_BYTES_512_224)) ||
                     ((mode == MODE_SHA_512_256) && (tx_ptr_new == DIGEST_BYTES_512_256)) ||
                     ((mode == MODE_SHA_384) && (tx_ptr_new == DIGEST_BYTES_384)) ||
                     ((mode == MODE_SHA_512) && (tx_ptr_new == DIGEST_BYTES_512)))
                   begin
                      tx_active_new = 0;
                      tx_ptr_new = 0;
                      tx_ctrl_new = TX_INIT;
                   end
              end
        endcase // case (tx_ctrl_reg)
     end // tx_engine

endmodule // sha512

//======================================================================
// EOF sha512.v
//======================================================================
