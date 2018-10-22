//======================================================================
//
// Copyright (c) 2015, NORDUnet A/S All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// - Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
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

`timescale 1ns / 1ps

module modexps6_buffer_user
  #(parameter OPERAND_ADDR_WIDTH        =  5)     // 1024 / 32 = 32 -> 5 bits
   (
    input wire                          clk,

    input wire                          bus_cs,
    input wire                          bus_we,
    input wire [ADDR_WIDTH_TOTAL-1:0]   bus_addr,
    input wire [31:0]                   bus_data_wr,
    output wire [31:0]                  bus_data_rd,

    input wire [OPERAND_ADDR_WIDTH-1:0] ro_modulus_bram_addr,
    output wire [31:0]                  ro_modulus_bram_out,

    input wire [OPERAND_ADDR_WIDTH-1:0] ro_message_bram_addr,
    output wire [31:0]                  ro_message_bram_out,

    input wire [OPERAND_ADDR_WIDTH-1:0] ro_exponent_bram_addr,
    output wire [31:0]                  ro_exponent_bram_out,

    input wire [OPERAND_ADDR_WIDTH-1:0] rw_result_bram_addr,
    input wire                          rw_result_bram_wr,
    input wire [31:0]                   rw_result_bram_in
    );


   //
   // Locals
   //
   localparam   ADDR_WIDTH_TOTAL                = OPERAND_ADDR_WIDTH + 2;

   localparam   [1: 0]  BUS_ADDR_BANK_MODULUS   = 2'b00;
   localparam   [1: 0]  BUS_ADDR_BANK_MESSAGE   = 2'b01;
   localparam   [1: 0]  BUS_ADDR_BANK_EXPONENT  = 2'b10;
   localparam   [1: 0]  BUS_ADDR_BANK_RESULT    = 2'b11;

   //
   // Address Decoder
   //
   wire [OPERAND_ADDR_WIDTH-1:0]        bus_addr_operand_word = bus_addr[OPERAND_ADDR_WIDTH-1:0];
   wire [                  1:0]         bus_addr_operand_bank = bus_addr[ADDR_WIDTH_TOTAL-1:ADDR_WIDTH_TOTAL-2];


   //
   // Modulus Memory
   //
   wire [31: 0]                         bus_data_rd_modulus;

   ram_1rw_1ro_readfirst #
     (
      .MEM_WIDTH        (32),
      .MEM_ADDR_BITS    (OPERAND_ADDR_WIDTH)
      )
   mem_modulus
     (
      .clk              (clk),

      .a_addr           (bus_addr_operand_word),
      .a_wr             (bus_cs & bus_we & (bus_addr_operand_bank == BUS_ADDR_BANK_MODULUS)),
      .a_in             (bus_data_wr),
      .a_out            (bus_data_rd_modulus),

      .b_addr           (ro_modulus_bram_addr),
      .b_out            (ro_modulus_bram_out)
      );


   //
   // Message Memory
   //
   wire [31: 0]         bus_data_rd_message;

   ram_1rw_1ro_readfirst #
     (
      .MEM_WIDTH        (32),
      .MEM_ADDR_BITS    (OPERAND_ADDR_WIDTH)
      )
   mem_message
     (
      .clk              (clk),

      .a_addr           (bus_addr_operand_word),
      .a_wr             (bus_cs & bus_we & (bus_addr_operand_bank == BUS_ADDR_BANK_MESSAGE)),
      .a_in             (bus_data_wr),
      .a_out            (bus_data_rd_message),

      .b_addr           (ro_message_bram_addr),
      .b_out            (ro_message_bram_out)
      );


   //
   // Exponent Memory
   //
   wire [31: 0]         bus_data_rd_exponent;

   ram_1rw_1ro_readfirst #
     (
      .MEM_WIDTH        (32),
      .MEM_ADDR_BITS    (OPERAND_ADDR_WIDTH)
      )
   mem_exponent
     (
      .clk              (clk),

      .a_addr           (bus_addr_operand_word),
      .a_wr             (bus_cs & bus_we & (bus_addr_operand_bank == BUS_ADDR_BANK_EXPONENT)),
      .a_in             (bus_data_wr),
      .a_out            (bus_data_rd_exponent),

      .b_addr           (ro_exponent_bram_addr),
      .b_out            (ro_exponent_bram_out)
      );


   //
   // Result Memory
   //
   wire [31: 0]         bus_data_rd_result;

   ram_1rw_1ro_readfirst #
     (
      .MEM_WIDTH        (32),
      .MEM_ADDR_BITS    (OPERAND_ADDR_WIDTH)
      )
   mem_result
     (
      .clk              (clk),

      .a_addr           (rw_result_bram_addr),
      .a_wr             (rw_result_bram_wr),
      .a_in             (rw_result_bram_in),
      .a_out            (),

      .b_addr           (bus_addr_operand_word),
      .b_out            (bus_data_rd_result)
      );


   //
   // Output Selector
   //
   reg [1: 0]           bus_addr_operand_bank_prev;
   always @(posedge clk) bus_addr_operand_bank_prev = bus_addr_operand_bank;

   reg [31: 0]          bus_data_rd_mux;
   assign bus_data_rd = bus_data_rd_mux;

   always @(*)
     //
     case (bus_addr_operand_bank_prev)
       //
       BUS_ADDR_BANK_MODULUS:   bus_data_rd_mux = bus_data_rd_modulus;
       BUS_ADDR_BANK_MESSAGE:   bus_data_rd_mux = bus_data_rd_message;
       BUS_ADDR_BANK_EXPONENT:  bus_data_rd_mux = bus_data_rd_exponent;
       BUS_ADDR_BANK_RESULT:    bus_data_rd_mux = bus_data_rd_result;
       //
       default:                 bus_data_rd_mux = {32{1'bX}};
       //
     endcase


endmodule
