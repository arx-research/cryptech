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

module modexps6_montgomery_coeff
  #(parameter MODULUS_NUM_BITS          = 11,           // 1024 -> 11 bits
    parameter OPERAND_ADDR_WIDTH        =  5)           // 1024 / 32 = 32 -> 5 bits
   (
    input wire                           clk,

    input wire                           ena,
    output wire                          rdy,

    input wire [MODULUS_NUM_BITS-1:0]    modulus_width,

    output wire [OPERAND_ADDR_WIDTH :0]  coeff_bram_addr,
    output wire                          coeff_bram_wr,
    output wire [31:0]                   coeff_bram_in,
    input wire [31:0]                    coeff_bram_out,

    output wire [OPERAND_ADDR_WIDTH :0]  nn_bram_addr,
    output wire                          nn_bram_wr,
    output wire [31:0]                   nn_bram_in,

    output wire [OPERAND_ADDR_WIDTH-1:0] modulus_bram_addr,
    input wire [31:0]                    modulus_bram_out,

    output wire [31:0]                   modinv_n0,
    output wire                          modinv_ena,
    input wire                           modinv_rdy
    );


   //
   // Locals
   //
   localparam   [  MODULUS_NUM_BITS  :0] round_count_zero       = {1'b0, {MODULUS_NUM_BITS{1'b0}}};
   localparam   [OPERAND_ADDR_WIDTH  :0] coeff_bram_addr_zero   = {1'b0, {OPERAND_ADDR_WIDTH{1'b0}}};
   localparam   [OPERAND_ADDR_WIDTH-1:0] modulus_bram_addr_zero = {OPERAND_ADDR_WIDTH{1'b0}};


   //
   // FSM
   //
   localparam FSM_STATE_IDLE            = 6'd0;

   localparam FSM_STATE_INIT            = 6'd10;

   localparam FSM_STATE_SHIFT_READ      = 6'd21;
   localparam FSM_STATE_SHIFT_WRITE     = 6'd22;

   localparam FSM_STATE_COMPARE_READ    = 6'd31;
   localparam FSM_STATE_COMPARE_COMPARE = 6'd32;

   localparam FSM_STATE_SUBTRACT_READ   = 6'd41;
   localparam FSM_STATE_SUBTRACT_WRITE  = 6'd42;

   localparam FSM_STATE_ROUND           = 6'd50;

   localparam FSM_STATE_FINAL           = 6'd60;

   reg [5: 0]                            fsm_state = FSM_STATE_IDLE;


   //
   // Trigger
   //
   reg                                   ena_dly = 1'b0;

   wire                                  ena_trig = ena && !ena_dly;

   always @(posedge clk) ena_dly <= ena;


   //
   // Ready Register
   //
   reg                                   rdy_reg = 1'b0;

   assign rdy = rdy_reg;


   //
   // ModInv Control
   //
   reg                                   modinv_ena_reg = 1'b0;
   reg [31: 0]                           modinv_n0_reg;

   assign modinv_ena = modinv_ena_reg;
   assign modinv_n0 = modinv_n0_reg;


   //
   // Enable / Ready Logic
   //
   always @(posedge clk)
     //
     if (fsm_state == FSM_STATE_FINAL) begin
        //
        if (modinv_rdy) rdy_reg <= 1'b1;
        //
     end else if (fsm_state == FSM_STATE_IDLE) begin
        //
        if (rdy_reg && !ena) rdy_reg <= 1'b0;
        //
     end


   //
   // Flags
   //
   reg  reg_shift_carry = 1'b0;
   reg  reg_subtractor_borrow = 1'b0;


   //
   // Round Counter
   //
   reg [MODULUS_NUM_BITS:0] round_count         = round_count_zero;
   wire [MODULUS_NUM_BITS:0] round_count_last   = {modulus_width, 1'b0} + 6'd63;
   wire [MODULUS_NUM_BITS:0] round_count_next   = (round_count < round_count_last) ? round_count + 1'b1 : round_count_zero;


   //
   // Modulus BRAM Interface
   //
   reg [OPERAND_ADDR_WIDTH-1:0] modulus_bram_addr_reg = modulus_bram_addr_zero;

   assign modulus_bram_addr = modulus_bram_addr_reg;


   //
   // Coeff BRAM Interface
   //
   reg [OPERAND_ADDR_WIDTH:0]   coeff_bram_addr_reg     = coeff_bram_addr_zero;
   reg                          coeff_bram_wr_reg               = 1'b0;

   assign coeff_bram_addr = coeff_bram_addr_reg;
   assign coeff_bram_wr = coeff_bram_wr_reg;


   //
   // NN BRAM Interface
   //
   reg [OPERAND_ADDR_WIDTH:0]   nn_bram_addr_reg        = coeff_bram_addr_zero;
   reg                          nn_bram_wr_reg          = 1'b0;

   assign nn_bram_addr = nn_bram_addr_reg;
   assign nn_bram_wr = nn_bram_wr_reg;


   //
   // Hardware Subtractor
   //
   wire [31: 0]                 subtractor_out;
   wire                         subtractor_out_nonzero = |subtractor_out;
   wire                         subtractor_borrow_out;
   wire                         subtractor_borrow_in;

   assign subtractor_borrow_in = (fsm_state == FSM_STATE_COMPARE_COMPARE) ? 1'b0 : reg_subtractor_borrow;

   subtractor_s6 dsp_subtractor
     (
      .a                (coeff_bram_out),
      .b                (modulus_bram_out),
      .s                (subtractor_out),
      .c_in             (subtractor_borrow_in),
      .c_out            (subtractor_borrow_out)
      );


   //
   // Handy Wires
   //
   wire [OPERAND_ADDR_WIDTH-1:0] modulus_width_msb                                              = modulus_width[MODULUS_NUM_BITS-1:MODULUS_NUM_BITS-OPERAND_ADDR_WIDTH];

   wire [OPERAND_ADDR_WIDTH  :0] coeff_bram_addr_last           = {modulus_width_msb, 1'b0};
   wire [OPERAND_ADDR_WIDTH  :0] coeff_bram_addr_next_or_zero   = (coeff_bram_addr_reg < coeff_bram_addr_last) ? coeff_bram_addr_reg + 1'b1 : coeff_bram_addr_zero;
   wire [OPERAND_ADDR_WIDTH  :0] coeff_bram_addr_next_or_last   = (coeff_bram_addr_reg < coeff_bram_addr_last) ? coeff_bram_addr_reg + 1'b1 : coeff_bram_addr_last;
   wire [OPERAND_ADDR_WIDTH  :0] coeff_bram_addr_prev_or_zero   = (coeff_bram_addr_reg > coeff_bram_addr_zero) ? coeff_bram_addr_reg - 1'b1 : coeff_bram_addr_zero;

   wire [OPERAND_ADDR_WIDTH  :0] modulus_bram_addr_last_ext     = coeff_bram_addr_last - 1'b1;

   wire [OPERAND_ADDR_WIDTH-1:0] modulus_bram_addr_last         = modulus_bram_addr_last_ext[OPERAND_ADDR_WIDTH-1:0];
   wire [OPERAND_ADDR_WIDTH-1:0] modulus_bram_addr_next_or_zero = (modulus_bram_addr_reg < modulus_bram_addr_last) ? modulus_bram_addr_reg + 1'b1 : modulus_bram_addr_zero;
   wire [OPERAND_ADDR_WIDTH-1:0] modulus_bram_addr_prev_or_zero = (modulus_bram_addr_reg > modulus_bram_addr_zero) ? modulus_bram_addr_reg - 1'b1 : modulus_bram_addr_zero;


   //
   // Coeff BRAM Input Logic
   //
   reg [31: 0]                   coeff_bram_in_mux;

   assign coeff_bram_in = coeff_bram_in_mux;

   always @(*)
     //
     case (fsm_state)

       FSM_STATE_INIT:
         //
         if (coeff_bram_addr_reg == coeff_bram_addr_zero) coeff_bram_in_mux = 32'h00000001;
         else coeff_bram_in_mux = 32'h00000000;

       FSM_STATE_SHIFT_WRITE:
         //
         coeff_bram_in_mux = {coeff_bram_out[30:0], reg_shift_carry};

       FSM_STATE_SUBTRACT_WRITE:
         //
         if (coeff_bram_addr_reg == coeff_bram_addr_last) coeff_bram_in_mux = 32'h00000000;
         else coeff_bram_in_mux = subtractor_out;

       default:
         //
         coeff_bram_in_mux = {32{1'bX}};

     endcase


   //
   // NN BRAM Input Logic
   //
   reg [31: 0]                   nn_bram_in_mux;

   assign nn_bram_in = nn_bram_in_mux;

   always @(*)
                                  //
     case (fsm_state)

       FSM_STATE_INIT:
         //
         if (coeff_bram_addr_reg == coeff_bram_addr_last) nn_bram_in_mux = {32{1'b0}};
         else nn_bram_in_mux = modulus_bram_out;

         default:
           //
           nn_bram_in_mux       = {32{1'bX}};

      endcase


   //
   // Comparison Functions
   //
   reg                           compare_greater_or_equal;
   reg                           compare_less_than;

   wire                          compare_done = compare_greater_or_equal | compare_less_than;

   always @(*)
                                  //
     if (coeff_bram_addr_reg == coeff_bram_addr_last) compare_greater_or_equal = coeff_bram_out[0];
   //
     else if (coeff_bram_addr_reg == coeff_bram_addr_zero) compare_greater_or_equal = !subtractor_borrow_out;
   //
     else compare_greater_or_equal = !subtractor_borrow_out && subtractor_out_nonzero;

   always @(*)
                              //
     if (coeff_bram_addr_reg == coeff_bram_addr_last) compare_less_than = 1'b0;
   //
     else compare_less_than = subtractor_borrow_out;



   //
   // Main Logic
   //
   always @(posedge clk)
     //
     case (fsm_state)

       FSM_STATE_INIT: begin
          //
          coeff_bram_wr_reg <= (coeff_bram_addr_reg < coeff_bram_addr_last) ? 1'b1 : 1'b0;
          coeff_bram_addr_reg <= coeff_bram_wr_reg ? coeff_bram_addr_next_or_zero : coeff_bram_addr_zero;
          //
          nn_bram_wr_reg <= (coeff_bram_addr_reg < coeff_bram_addr_last) ? 1'b1 : 1'b0;
          nn_bram_addr_reg <= coeff_bram_wr_reg ? coeff_bram_addr_next_or_zero : coeff_bram_addr_zero;
          //
          if (!coeff_bram_wr_reg) begin
             //
             modinv_ena_reg <= 1'b1;
             modinv_n0_reg <= modulus_bram_out;
             //
          end
          //
          if (modulus_bram_addr_reg == modulus_bram_addr_zero) begin
             //
             if (!coeff_bram_wr_reg)
               //
               modulus_bram_addr_reg <= modulus_bram_addr_next_or_zero;
             //
          end else begin
             //
             modulus_bram_addr_reg <= modulus_bram_addr_next_or_zero;
             //
          end
          //
       end

       FSM_STATE_SHIFT_READ: begin
          //
          coeff_bram_wr_reg <= 1'b1;
          //
          if (coeff_bram_addr_reg == coeff_bram_addr_zero)
            //
            reg_shift_carry <= 1'b0;
          //
       end

       FSM_STATE_SHIFT_WRITE: begin
          //
          coeff_bram_wr_reg <= 1'b0;
          coeff_bram_addr_reg <= coeff_bram_addr_next_or_last;
          //
          reg_shift_carry <= coeff_bram_out[31];
          //
       end

       FSM_STATE_COMPARE_COMPARE: begin
          //
          coeff_bram_addr_reg <= compare_done ? coeff_bram_addr_zero : coeff_bram_addr_prev_or_zero;
          //
          modulus_bram_addr_reg <= compare_done ? modulus_bram_addr_zero : ((coeff_bram_addr_reg == coeff_bram_addr_last) ? modulus_bram_addr_last : modulus_bram_addr_prev_or_zero);
          //
       end

       FSM_STATE_SUBTRACT_READ: begin
          //
          coeff_bram_wr_reg <= 1'b1;
          //
          if (coeff_bram_addr_reg == coeff_bram_addr_zero)
            //
            reg_subtractor_borrow <= 1'b0;
          //
       end

       FSM_STATE_SUBTRACT_WRITE: begin
          //
          coeff_bram_wr_reg <= 1'b0;
          coeff_bram_addr_reg <= coeff_bram_addr_next_or_zero;
          //
          modulus_bram_addr_reg <= (coeff_bram_addr_reg == coeff_bram_addr_last) ? modulus_bram_addr_zero : modulus_bram_addr_next_or_zero;
          //
          reg_subtractor_borrow <= subtractor_borrow_out;
          //
       end

       FSM_STATE_ROUND: begin
          //
          round_count <= round_count_next;
          //
       end

       FSM_STATE_FINAL: begin
          //
          if (modinv_rdy) modinv_ena_reg <= 1'b0;
          //
       end

     endcase


   //
   // FSM Transition Logic
   //
   always @(posedge clk)
     //
     case (fsm_state)

       FSM_STATE_IDLE:          fsm_state <= (!rdy_reg && !modinv_rdy && ena_trig) ? FSM_STATE_INIT : FSM_STATE_IDLE;

       FSM_STATE_SHIFT_READ:    fsm_state <= FSM_STATE_SHIFT_WRITE;
       FSM_STATE_COMPARE_READ:  fsm_state <= FSM_STATE_COMPARE_COMPARE;
       FSM_STATE_SUBTRACT_READ: fsm_state <= FSM_STATE_SUBTRACT_WRITE;

       FSM_STATE_INIT:          fsm_state <= (coeff_bram_addr_reg < coeff_bram_addr_last) ? FSM_STATE_INIT          : FSM_STATE_SHIFT_READ;
       FSM_STATE_SHIFT_WRITE:   fsm_state <= (coeff_bram_addr_reg < coeff_bram_addr_last) ? FSM_STATE_SHIFT_READ    : FSM_STATE_COMPARE_READ;
       FSM_STATE_SUBTRACT_WRITE: fsm_state <= (coeff_bram_addr_reg < coeff_bram_addr_last) ? FSM_STATE_SUBTRACT_READ : FSM_STATE_ROUND;

       FSM_STATE_ROUND:         fsm_state <= (round_count < round_count_last) ? FSM_STATE_SHIFT_READ : FSM_STATE_FINAL;

       FSM_STATE_COMPARE_COMPARE: fsm_state <= compare_done ? (compare_greater_or_equal ? FSM_STATE_SUBTRACT_READ : FSM_STATE_ROUND) : FSM_STATE_COMPARE_READ;

       FSM_STATE_FINAL:         fsm_state <= modinv_rdy ? FSM_STATE_IDLE : FSM_STATE_FINAL;

       default:                 fsm_state <= FSM_STATE_IDLE;

     endcase


endmodule
