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

module modexps6_montgomery_multiplier
  #(parameter OPERAND_NUM_BITS          = 11,           // 1024 -> 11 bits
    parameter OPERAND_ADDR_WIDTH        =  5)           // 1024 / 32 = 32 -> 5 bits
   (
    input wire                          clk,

    input wire                          ena,
    output wire                         rdy,

    input wire [OPERAND_NUM_BITS-1:0]   operand_width,

    output wire [OPERAND_ADDR_WIDTH :0] x_bram_addr,
    input wire [31:0]                   x_bram_out,

    output wire [OPERAND_ADDR_WIDTH :0] y_bram_addr,
    input wire [31:0]                   y_bram_out,

    output wire [OPERAND_ADDR_WIDTH :0] n_bram_addr,
    input wire [31:0]                   n_bram_out,

    output wire [OPERAND_ADDR_WIDTH :0] z_bram_addr,
    output wire                         z_bram_wr,
    output wire [31:0]                  z_bram_in,
    input wire [31:0]                   z_bram_out,

    input wire [31:0]                   n0_modinv
    );


   //
   // Locals
   //
   localparam   [OPERAND_ADDR_WIDTH:0]  round_count_zero = {1'b0, {OPERAND_ADDR_WIDTH{1'b0}}};
   localparam   [OPERAND_ADDR_WIDTH:0]  bram_addr_zero   = {1'b0, {OPERAND_ADDR_WIDTH{1'b0}}};


   //
   // FSM
   //
   localparam FSM_STATE_IDLE            = 6'd0;

   localparam FSM_STATE_INIT            = 6'd10;

   localparam FSM_STATE_MUL_XY_CALC     = 6'd21;
   localparam FSM_STATE_MUL_XY_PIPELINE = 6'd22;
   localparam FSM_STATE_MUL_XY_REGISTER = 6'd23;
   localparam FSM_STATE_MUL_XY_WRITE    = 6'd24;

   localparam FSM_STATE_MAGIC_CALC      = 6'd31;
   localparam FSM_STATE_MAGIC_PIPELINE  = 6'd32;
   localparam FSM_STATE_MAGIC_REGISTER  = 6'd33;

   localparam FSM_STATE_MUL_MN_CALC     = 6'd41;
   localparam FSM_STATE_MUL_MN_PIPELINE = 6'd42;
   localparam FSM_STATE_MUL_MN_REGISTER = 6'd43;
   localparam FSM_STATE_MUL_MN_WRITE    = 6'd44;

   localparam FSM_STATE_SHIFT           = 6'd50;

   localparam FSM_STATE_ROUND           = 6'd55;

   localparam FSM_STATE_FINAL           = 6'd60;

   reg [5: 0]                           fsm_state = FSM_STATE_IDLE;


   //
   // Trigger
   //
   reg                                  ena_dly = 1'b0;
   always @(posedge clk) ena_dly <= ena;
   wire                                 ena_trig = (ena == 1'b1) && (ena_dly == 1'b0);


   //
   // Ready Register
   //
   reg                                  rdy_reg = 1'b0;
   assign rdy = rdy_reg;


   //
   // Enable / Ready Logic
   //
   always @(posedge clk)
     //
     if (fsm_state == FSM_STATE_FINAL) begin
        //
        rdy_reg <= 1'b1;
        //
     end else if (fsm_state == FSM_STATE_IDLE) begin
        //
        if (rdy_reg && !ena) rdy_reg <= 1'b0;
        //
     end


   //
   // X, Y, N BRAM Interface
   //
   reg [OPERAND_ADDR_WIDTH:0]   x_bram_addr_reg = bram_addr_zero;
   reg [OPERAND_ADDR_WIDTH:0]   y_bram_addr_reg = bram_addr_zero;
   reg [OPERAND_ADDR_WIDTH:0]   n_bram_addr_reg = bram_addr_zero;

   assign x_bram_addr = x_bram_addr_reg;
   assign y_bram_addr = y_bram_addr_reg;
   assign n_bram_addr = n_bram_addr_reg;


   //
   // Z BRAM Interface
   //
   reg [OPERAND_ADDR_WIDTH:0]   z_bram_addr_reg = bram_addr_zero;
   reg                          z_bram_wr_reg           = 1'b0;
   reg [                31:0]   z_bram_in_mux;

   assign z_bram_addr = z_bram_addr_reg;
   assign z_bram_wr = z_bram_wr_reg;
   assign z_bram_in = z_bram_in_mux;


   //
   // Handy Wires
   //
   wire [OPERAND_ADDR_WIDTH-1:0] operand_width_msb = operand_width[OPERAND_NUM_BITS-1:OPERAND_NUM_BITS-OPERAND_ADDR_WIDTH];

   wire [OPERAND_ADDR_WIDTH  :0] bram_addr_last = {operand_width_msb, 1'b1};    // +1


   //
   // Hardware Multiplier (X * Y)
   //
   reg [31: 0]                   multiplier_xy_carry_in;
   wire [31: 0]                  multiplier_xy_out;
   wire [31: 0]                  multiplier_xy_carry_out;

   modexps6_adder64_carry32 dsp_multiplier_xy
     (
      .clk      (clk),
      .t        (/*(z_bram_addr_reg < bram_addr_last) ? */z_bram_out/* : {32{1'b0}}*/),
      .x        (/*(z_bram_addr_reg < bram_addr_last) ? */x_bram_out/* : {32{1'b0}}*/),
      .y        (/*(z_bram_addr_reg < bram_addr_last) ? */y_bram_out/* : {32{1'b0}}*/),
      .s        (multiplier_xy_out),
      .c_in     (multiplier_xy_carry_in),
      .c_out    (multiplier_xy_carry_out)
      );


   //
   // Hardware Multiplier (Magic)
   //
   wire [63: 0]                  multiplier_magic_out;
   reg [31: 0]                   magic_value_reg;

   multiplier_s6 dsp_multiplier_magic
     (
      .clk      (clk),
      .a        (z_bram_out),
      .b        (n0_modinv),
      .p        (multiplier_magic_out)
      );


   //
   // Hardware Multiplier (M * N)
   //
   reg [31: 0]                   multiplier_mn_carry_in;
   wire [31: 0]                  multiplier_mn_out;
   wire [31: 0]                  multiplier_mn_carry_out;

   modexps6_adder64_carry32 dsp_multiplier_mn
     (
      .clk      (clk),
      .t        (z_bram_out),
      .x        (magic_value_reg),
      .y        (/*(z_bram_addr_reg < bram_addr_last) ? */n_bram_out/* : {32{1'b0}}*/),
      .s        (multiplier_mn_out),
      .c_in     (multiplier_mn_carry_in),
      .c_out    (multiplier_mn_carry_out)
      );


   //
   // Z BRAM Input Selector
   //
   always @(*)
     //
     case (fsm_state)

       FSM_STATE_INIT:
         //
         z_bram_in_mux  = {32{1'b0}};

       FSM_STATE_MUL_XY_WRITE:
         //
         if (z_bram_addr_reg < bram_addr_last)  z_bram_in_mux   = multiplier_xy_out;
         else                                                                                           z_bram_in_mux   = multiplier_xy_carry_in;

       FSM_STATE_MUL_MN_WRITE:
         //
         if (z_bram_addr_reg < bram_addr_last)  z_bram_in_mux   = multiplier_mn_out;
         else                                                                                           z_bram_in_mux   = multiplier_mn_carry_in + z_bram_out;

       FSM_STATE_SHIFT:
         //
         z_bram_in_mux  = z_bram_out;

       default:
         //
         z_bram_in_mux  = {32{1'bX}};

     endcase


   //
   // Handy Functions
   //
   function     [OPERAND_ADDR_WIDTH:0]  bram_addr_next_or_zero;
      input [OPERAND_ADDR_WIDTH:0] bram_addr;
      begin
         bram_addr_next_or_zero = (bram_addr < bram_addr_last) ? bram_addr + 1'b1 : bram_addr_zero;
      end
   endfunction

   function     [OPERAND_ADDR_WIDTH:0]  bram_addr_next_or_last;
      input     [OPERAND_ADDR_WIDTH:0]  bram_addr;
      begin
         bram_addr_next_or_last = (bram_addr < bram_addr_last) ? bram_addr + 1'b1 : bram_addr_last;
      end
   endfunction

   function     [OPERAND_ADDR_WIDTH:0]  bram_addr_prev_or_zero;
      input     [OPERAND_ADDR_WIDTH:0]  bram_addr;
      begin
         bram_addr_prev_or_zero = (bram_addr > bram_addr_zero) ? bram_addr - 1'b1 : bram_addr_zero;
      end
   endfunction


   //
   // Round Counter
   //
   reg  [OPERAND_ADDR_WIDTH:0]  round_count                     = round_count_zero;
   wire [OPERAND_ADDR_WIDTH:0]  round_count_last        = {operand_width_msb, 1'b0};
   wire [OPERAND_ADDR_WIDTH:0]  round_count_next        = (round_count < round_count_last) ? round_count + 1'b1 : round_count_zero;


   //
   // Main Logic
   //
   always @(posedge clk)
     //
     case (fsm_state)

       FSM_STATE_INIT: begin
          //
          z_bram_wr_reg         <= (z_bram_addr_reg < bram_addr_last) ? 1'b1 : 1'b0;
          z_bram_addr_reg       <= z_bram_wr_reg ? bram_addr_next_or_zero(z_bram_addr_reg) : bram_addr_zero;
          //
       end

       FSM_STATE_MUL_XY_CALC: begin
          //
          if (z_bram_addr_reg == bram_addr_zero) begin
             //
             multiplier_xy_carry_in <= {32{1'b0}};
          //
       end
          //
       end

       FSM_STATE_MUL_XY_REGISTER: begin
          //
          z_bram_wr_reg <= 1'b1;
          //
       end

       FSM_STATE_MUL_XY_WRITE: begin
          //
          z_bram_wr_reg         <= 1'b0;
          z_bram_addr_reg       <= bram_addr_next_or_zero(z_bram_addr_reg);
          //
          x_bram_addr_reg       <= bram_addr_next_or_zero(x_bram_addr_reg);
          //
          multiplier_xy_carry_in <= multiplier_xy_carry_out;
          //
       end

       FSM_STATE_MUL_MN_CALC: begin
          //
          if (z_bram_addr_reg == bram_addr_zero) begin
             //
             multiplier_mn_carry_in <= {32{1'b0}};
          //
          magic_value_reg <= multiplier_magic_out[31:0];
          //
       end
          //
       end

       FSM_STATE_MUL_MN_REGISTER: begin
          //
          z_bram_wr_reg <= 1'b1;
          //
       end

       FSM_STATE_MUL_MN_WRITE: begin
          //
          z_bram_wr_reg         <= 1'b0;
          z_bram_addr_reg       <= bram_addr_next_or_last(z_bram_addr_reg);
          //
          n_bram_addr_reg       <= bram_addr_next_or_zero(n_bram_addr_reg);
          //
          multiplier_mn_carry_in <= multiplier_mn_carry_out;
          //
       end

       FSM_STATE_SHIFT: begin
          //
          if (z_bram_wr_reg == 1'b0)                                                    z_bram_wr_reg <= 1'b1;
          else if (z_bram_addr_reg == bram_addr_zero)   z_bram_wr_reg <= 1'b0;

          z_bram_addr_reg       <= bram_addr_prev_or_zero(z_bram_addr_reg);
          //
       end

       FSM_STATE_ROUND: begin
          //
          y_bram_addr_reg       <= (round_count < round_count_last) ? bram_addr_next_or_zero(y_bram_addr_reg) : bram_addr_zero;
          //
          round_count <= round_count_next;
          //
       end

     endcase


   //
   // FSM Transition Logic
   //
   always @(posedge clk)
     //
     case (fsm_state)
       //
       FSM_STATE_IDLE:            fsm_state <= (!rdy_reg && ena_trig) ? FSM_STATE_INIT : FSM_STATE_IDLE;

       FSM_STATE_INIT:            fsm_state <= (z_bram_addr < bram_addr_last  ) ? FSM_STATE_INIT        : FSM_STATE_MUL_XY_CALC;
       FSM_STATE_ROUND:           fsm_state <= (round_count < round_count_last) ? FSM_STATE_MUL_XY_CALC : FSM_STATE_FINAL;

       FSM_STATE_MUL_XY_CALC:     fsm_state <= FSM_STATE_MUL_XY_PIPELINE;
       FSM_STATE_MAGIC_CALC:      fsm_state <= FSM_STATE_MAGIC_PIPELINE;
       FSM_STATE_MUL_MN_CALC:     fsm_state <= FSM_STATE_MUL_MN_PIPELINE;

       FSM_STATE_MUL_XY_PIPELINE: fsm_state <= FSM_STATE_MUL_XY_REGISTER;
       FSM_STATE_MAGIC_PIPELINE:  fsm_state <= FSM_STATE_MAGIC_REGISTER;
       FSM_STATE_MUL_MN_PIPELINE: fsm_state <= FSM_STATE_MUL_MN_REGISTER;

       FSM_STATE_MUL_XY_REGISTER: fsm_state <= FSM_STATE_MUL_XY_WRITE;
       FSM_STATE_MAGIC_REGISTER:  fsm_state <= FSM_STATE_MUL_MN_CALC;
       FSM_STATE_MUL_MN_REGISTER: fsm_state <= FSM_STATE_MUL_MN_WRITE;

       FSM_STATE_MUL_XY_WRITE:    fsm_state <= (z_bram_addr < bram_addr_last) ? FSM_STATE_MUL_XY_CALC : FSM_STATE_MAGIC_CALC;
       FSM_STATE_MUL_MN_WRITE:    fsm_state <= (z_bram_addr < bram_addr_last) ? FSM_STATE_MUL_MN_CALC : FSM_STATE_SHIFT;
       FSM_STATE_SHIFT:           fsm_state <= (z_bram_addr > bram_addr_zero) ? FSM_STATE_SHIFT       : FSM_STATE_ROUND;

       FSM_STATE_FINAL:           fsm_state <= FSM_STATE_IDLE;

       default:                   fsm_state <= FSM_STATE_IDLE;

     endcase


endmodule
