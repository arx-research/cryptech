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

module modexps6_buffer_core
  #(parameter OPERAND_ADDR_WIDTH      =  5)     // 1024 / 32 = 32 -> 5 bits
   (
    input wire                        clk,

    input wire [OPERAND_ADDR_WIDTH:0] rw_coeff_bram_addr,
    input wire                        rw_coeff_bram_wr,
    input wire [31:0]                 rw_coeff_bram_in,
    output wire [31:0]                rw_coeff_bram_out,

    input wire [OPERAND_ADDR_WIDTH:0] rw_mm_bram_addr,
    input wire                        rw_mm_bram_wr,
    input wire [31:0]                 rw_mm_bram_in,
    output wire [31:0]                rw_mm_bram_out,

    input wire [OPERAND_ADDR_WIDTH:0] rw_nn_bram_addr,
    input wire                        rw_nn_bram_wr,
    input wire [31:0]                 rw_nn_bram_in,

    input wire [OPERAND_ADDR_WIDTH:0] rw_y_bram_addr,
    input wire                        rw_y_bram_wr,
    input wire [31:0]                 rw_y_bram_in,
    output wire [31:0]                rw_y_bram_out,

    input wire [OPERAND_ADDR_WIDTH:0] rw_r_bram_addr,
    input wire                        rw_r_bram_wr,
    input wire [31:0]                 rw_r_bram_in,
    output wire [31:0]                rw_r_bram_out,

    input wire [OPERAND_ADDR_WIDTH:0] rw_t_bram_addr,
    input wire                        rw_t_bram_wr,
    input wire [31:0]                 rw_t_bram_in,
    output wire [31:0]                rw_t_bram_out,

    input wire [OPERAND_ADDR_WIDTH:0] ro_coeff_bram_addr,
    output wire [31:0]                ro_coeff_bram_out,

    input wire [OPERAND_ADDR_WIDTH:0] ro_mm_bram_addr,
    output wire [31:0]                ro_mm_bram_out,

    input wire [OPERAND_ADDR_WIDTH:0] ro_nn_bram_addr,
    output wire [31:0]                ro_nn_bram_out,

    input wire [OPERAND_ADDR_WIDTH:0] ro_r_bram_addr,
    output wire [31:0]                ro_r_bram_out,

    input wire [OPERAND_ADDR_WIDTH:0] ro_t_bram_addr,
    output wire [31:0]                ro_t_bram_out
    );


   //
   // Montgomery Coefficient
   //
   ram_1rw_1ro_readfirst #
     (
      .MEM_WIDTH        (32),
      .MEM_ADDR_BITS    (OPERAND_ADDR_WIDTH+1)
      )
   mem_coeff
     (
      .clk              (clk),

      .a_addr           (rw_coeff_bram_addr),
      .a_wr             (rw_coeff_bram_wr),
      .a_in             (rw_coeff_bram_in),
      .a_out            (rw_coeff_bram_out),

      .b_addr           (ro_coeff_bram_addr),
      .b_out            (ro_coeff_bram_out)
      );


   //
   // Powers of Message
   //
   ram_1rw_1ro_readfirst #
     (
      .MEM_WIDTH        (32),
      .MEM_ADDR_BITS    (OPERAND_ADDR_WIDTH+1)
      )
   mem_mm
     (
      .clk              (clk),

      .a_addr           (rw_mm_bram_addr),
      .a_wr             (rw_mm_bram_wr),
      .a_in             (rw_mm_bram_in),
      .a_out            (rw_mm_bram_out),

      .b_addr           (ro_mm_bram_addr),
      .b_out            (ro_mm_bram_out)
      );


   //
   // Extended Modulus
   //
   ram_1rw_1ro_readfirst #
     (
      .MEM_WIDTH        (32),
      .MEM_ADDR_BITS    (OPERAND_ADDR_WIDTH+1)
      )
   mem_nn
     (
      .clk              (clk),

      .a_addr           (rw_nn_bram_addr),
      .a_wr             (rw_nn_bram_wr),
      .a_in             (rw_nn_bram_in),
      .a_out            (),

      .b_addr           (ro_nn_bram_addr),
      .b_out            (ro_nn_bram_out)
      );


   //
   // Output
   //
   ram_1rw_1ro_readfirst #
     (
      .MEM_WIDTH        (32),
      .MEM_ADDR_BITS    (OPERAND_ADDR_WIDTH+1)
      )
   mem_y
     (
      .clk              (clk),

      .a_addr           (rw_y_bram_addr),
      .a_wr             (rw_y_bram_wr),
      .a_in             (rw_y_bram_in),
      .a_out            (rw_y_bram_out),

      .b_addr           ({(OPERAND_ADDR_WIDTH+1){1'b0}}),
      .b_out            ()
      );


   //
   // Result of Multiplication
   //
   ram_1rw_1ro_readfirst #
     (
      .MEM_WIDTH        (32),
      .MEM_ADDR_BITS    (OPERAND_ADDR_WIDTH+1)
      )
   mem_r
     (
      .clk              (clk),

      .a_addr           (rw_r_bram_addr),
      .a_wr             (rw_r_bram_wr),
      .a_in             (rw_r_bram_in),
      .a_out            (rw_r_bram_out),

      .b_addr           (ro_r_bram_addr),
      .b_out            (ro_r_bram_out)
      );


   //
   // Temporary Buffer
   //
   ram_1rw_1ro_readfirst #
     (
      .MEM_WIDTH        (32),
      .MEM_ADDR_BITS    (OPERAND_ADDR_WIDTH+1)
      )
   mem_t
     (
      .clk              (clk),

      .a_addr           (rw_t_bram_addr),
      .a_wr             (rw_t_bram_wr),
      .a_in             (rw_t_bram_in),
      .a_out            (rw_t_bram_out),

      .b_addr           (ro_t_bram_addr),
      .b_out            (ro_t_bram_out)
      );


endmodule
