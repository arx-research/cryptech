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

module modexps6_top
  #(parameter MAX_MODULUS_WIDTH = 1024)
  (
   input wire                        clk,

   input wire                        init,
   output wire                       ready,

   input wire                        next,
   output wire                       valid,

   input wire [MODULUS_NUM_BITS-1:0] modulus_width,
   input wire [MODULUS_NUM_BITS-1:0] exponent_width,

   input wire                        fast_public_mode,

   input wire                        bus_cs,
   input wire                        bus_we,
   input wire [ADDR_WIDTH_TOTAL-1:0] bus_addr,
   input wire [31:0]                 bus_data_wr,
   output wire [31:0]                bus_data_rd
   );


   //
   // modexps6_clog2()
   //
   function     integer modexps6_clog2;
      input     integer              value;
      integer                        ret;
      begin
         value = value - 1;
         for (ret = 0; value > 0; ret = ret + 1)
           value = value >> 1;
         modexps6_clog2 = ret;
      end
   endfunction


   //
   // Locals
   //
   localparam   OPERAND_ADDR_WIDTH = modexps6_clog2(MAX_MODULUS_WIDTH / 32);
   localparam   MODULUS_NUM_BITS   = modexps6_clog2(MAX_MODULUS_WIDTH + 1);
   localparam   ADDR_WIDTH_TOTAL   = OPERAND_ADDR_WIDTH + 2;

   localparam   [OPERAND_ADDR_WIDTH-1:0] bram_user_addr_zero = {OPERAND_ADDR_WIDTH{1'b0}};
   localparam   [OPERAND_ADDR_WIDTH  :0] bram_core_addr_zero = {1'b0, {OPERAND_ADDR_WIDTH{1'b0}}};

   localparam   [   MODULUS_NUM_BITS:0] round_count_zero = {1'b0, {MODULUS_NUM_BITS{1'b0}}};


   //
   // User Memory
   //
   wire [OPERAND_ADDR_WIDTH-1:0]        ro_modulus_bram_addr;
   wire [                 31:0]         ro_modulus_bram_out;

   reg [OPERAND_ADDR_WIDTH-1:0]         ro_message_bram_addr    = bram_user_addr_zero;
   wire [                 31:0]         ro_message_bram_out;

   reg [OPERAND_ADDR_WIDTH-1:0]         ro_exponent_bram_addr   = bram_user_addr_zero;
   wire [                 31:0]         ro_exponent_bram_out;

   reg [OPERAND_ADDR_WIDTH-1:0]         rw_result_bram_addr     = bram_user_addr_zero;
   wire [                 31:0]         rw_result_bram_out;
   reg                                  rw_result_bram_wr       = 1'b0;
   wire [                 31:0]         rw_result_bram_in;

   modexps6_buffer_user #
     (
      .OPERAND_ADDR_WIDTH       (OPERAND_ADDR_WIDTH)
      )
   mem_user
     (
      .clk                      (clk),

      .bus_cs                   (bus_cs),
      .bus_we                   (bus_we),
      .bus_addr                 (bus_addr),
      .bus_data_wr              (bus_data_wr),
      .bus_data_rd              (bus_data_rd),

      .ro_modulus_bram_addr     (ro_modulus_bram_addr),
      .ro_modulus_bram_out      (ro_modulus_bram_out),

      .ro_message_bram_addr     (ro_message_bram_addr),
      .ro_message_bram_out      (ro_message_bram_out),

      .ro_exponent_bram_addr    (ro_exponent_bram_addr),
      .ro_exponent_bram_out     (ro_exponent_bram_out),

      .rw_result_bram_addr      (rw_result_bram_addr),
      .rw_result_bram_wr        (rw_result_bram_wr),
      .rw_result_bram_in        (rw_result_bram_in)
      );


   //
   // Core (Internal) Memory
   //
   wire [OPERAND_ADDR_WIDTH:0]          rw_coeff_bram_addr;
   wire                                 rw_coeff_bram_wr;
   wire [               31:0]           rw_coeff_bram_in;
   wire [               31:0]           rw_coeff_bram_out;

   reg [OPERAND_ADDR_WIDTH:0]           rw_mm_bram_addr         = bram_core_addr_zero;
   reg                                  rw_mm_bram_wr           = 1'b0;
   reg [               31:0]            rw_mm_bram_in;
   wire [               31:0]           rw_mm_bram_out;

   wire [OPERAND_ADDR_WIDTH:0]          rw_nn_bram_addr;
   wire                                 rw_nn_bram_wr;
   wire [               31:0]           rw_nn_bram_in;

   reg [OPERAND_ADDR_WIDTH:0]           rw_y_bram_addr          = bram_core_addr_zero;
   reg                                  rw_y_bram_wr            = 1'b0;
   reg [              31:0]             rw_y_bram_in;
   wire [              31:0]            rw_y_bram_out;

   wire [OPERAND_ADDR_WIDTH:0]          rw_r_bram_addr;
   wire                                 rw_r_bram_wr;
   wire [              31:0]            rw_r_bram_in;
   wire [              31:0]            rw_r_bram_out;

   reg [OPERAND_ADDR_WIDTH:0]           rw_t_bram_addr          = bram_core_addr_zero;
   reg                                  rw_t_bram_wr            = 1'b0;
   reg [              31:0]             rw_t_bram_in;
   wire [              31:0]            rw_t_bram_out;

   reg [OPERAND_ADDR_WIDTH:0]           ro_coeff_bram_addr      = bram_core_addr_zero;
   wire [               31:0]           ro_coeff_bram_out;

   wire [OPERAND_ADDR_WIDTH:0]          ro_mm_bram_addr;
   wire [               31:0]           ro_mm_bram_out;

   wire [OPERAND_ADDR_WIDTH:0]          ro_nn_bram_addr;
   wire [               31:0]           ro_nn_bram_out;

   reg [OPERAND_ADDR_WIDTH:0]           ro_r_bram_addr          = bram_core_addr_zero;
   wire [               31:0]           ro_r_bram_out;

   wire [OPERAND_ADDR_WIDTH:0]          ro_t_bram_addr;
   wire [              31:0]            ro_t_bram_out;

   modexps6_buffer_core #
     (
      .OPERAND_ADDR_WIDTH       (OPERAND_ADDR_WIDTH)
      )
   mem_core
     (
      .clk                      (clk),

      .rw_coeff_bram_addr       (rw_coeff_bram_addr),
      .rw_coeff_bram_wr         (rw_coeff_bram_wr),
      .rw_coeff_bram_in         (rw_coeff_bram_in),
      .rw_coeff_bram_out        (rw_coeff_bram_out),

      .rw_mm_bram_addr          (rw_mm_bram_addr),
      .rw_mm_bram_wr            (rw_mm_bram_wr),
      .rw_mm_bram_in            (rw_mm_bram_in),
      .rw_mm_bram_out           (rw_mm_bram_out),

      .rw_nn_bram_addr          (rw_nn_bram_addr),
      .rw_nn_bram_wr            (rw_nn_bram_wr),
      .rw_nn_bram_in            (rw_nn_bram_in),

      .rw_y_bram_addr           (rw_y_bram_addr),
      .rw_y_bram_wr             (rw_y_bram_wr),
      .rw_y_bram_in             (rw_y_bram_in),
      .rw_y_bram_out            (rw_y_bram_out),

      .rw_r_bram_addr           (rw_r_bram_addr),
      .rw_r_bram_wr             (rw_r_bram_wr),
      .rw_r_bram_in             (rw_r_bram_in),
      .rw_r_bram_out            (rw_r_bram_out),

      .rw_t_bram_addr           (rw_t_bram_addr),
      .rw_t_bram_wr             (rw_t_bram_wr),
      .rw_t_bram_in             (rw_t_bram_in),
      .rw_t_bram_out            (rw_t_bram_out),

      .ro_coeff_bram_addr       (ro_coeff_bram_addr),
      .ro_coeff_bram_out        (ro_coeff_bram_out),

      .ro_mm_bram_addr          (ro_mm_bram_addr),
      .ro_mm_bram_out           (ro_mm_bram_out),

      .ro_nn_bram_addr          (ro_nn_bram_addr),
      .ro_nn_bram_out           (ro_nn_bram_out),

      .ro_r_bram_addr           (ro_r_bram_addr),
      .ro_r_bram_out            (ro_r_bram_out),

      .ro_t_bram_addr           (ro_t_bram_addr),
      .ro_t_bram_out            (ro_t_bram_out)
      );


   //
   // Small 32-bit ModInv Core
   //
   wire                                 modinv_ena;
   wire                                 modinv_rdy;

   wire [31: 0]                         modinv_n0;
   wire [31: 0]                         modinv_n0_negative = ~modinv_n0 + 1'b1;
   wire [31: 0]                         modinv_n0_modinv;

   modexps6_modinv32 core_modinv32
     (
      .clk                      (clk),

      .ena                      (modinv_ena),
      .rdy                      (modinv_rdy),

      .n0                       (modinv_n0_negative),
      .n0_modinv                (modinv_n0_modinv)
      );


   //
   // Montgomery Coefficient Calculator
   //
   modexps6_montgomery_coeff #
     (
      .MODULUS_NUM_BITS         (MODULUS_NUM_BITS),
      .OPERAND_ADDR_WIDTH       (OPERAND_ADDR_WIDTH)
      )
   core_montgomery_coeff
     (
      .clk                      (clk),

      .ena                      (init),
      .rdy                      (ready),

      .modulus_width            (modulus_width),

      .coeff_bram_addr          (rw_coeff_bram_addr),
      .coeff_bram_wr            (rw_coeff_bram_wr),
      .coeff_bram_in            (rw_coeff_bram_in),
      .coeff_bram_out           (rw_coeff_bram_out),

      .nn_bram_addr             (rw_nn_bram_addr),
      .nn_bram_wr               (rw_nn_bram_wr),
      .nn_bram_in               (rw_nn_bram_in),

      .modulus_bram_addr        (ro_modulus_bram_addr),
      .modulus_bram_out         (ro_modulus_bram_out),

      .modinv_n0                (modinv_n0),
      .modinv_ena               (modinv_ena),
      .modinv_rdy               (modinv_rdy)
      );


   //
   // Montgomery Multiplier
   //
   reg                                  mul_ena = 1'b0;
   wire                                 mul_rdy;

   modexps6_montgomery_multiplier #
     (
      .OPERAND_NUM_BITS         (MODULUS_NUM_BITS),
      .OPERAND_ADDR_WIDTH       (OPERAND_ADDR_WIDTH)
      )
   core_montgomery_multiplier
     (
      .clk                      (clk),

      .ena                      (mul_ena),
      .rdy                      (mul_rdy),

      .operand_width            (modulus_width),

      .x_bram_addr              (ro_t_bram_addr),
      .x_bram_out               (ro_t_bram_out),

      .y_bram_addr              (ro_mm_bram_addr),
      .y_bram_out               (ro_mm_bram_out),

      .n_bram_addr              (ro_nn_bram_addr),
      .n_bram_out               (ro_nn_bram_out),

      .z_bram_addr              (rw_r_bram_addr),
      .z_bram_wr                (rw_r_bram_wr),
      .z_bram_in                (rw_r_bram_in),
      .z_bram_out               (rw_r_bram_out),

      .n0_modinv                (modinv_n0_modinv)
      );


   //
   // FSM
   //
   localparam FSM_STATE_IDLE            = 6'd0;

   localparam FSM_STATE_INIT_LOAD       = 6'd11;
   localparam FSM_STATE_INIT_WAIT       = 6'd12;
   localparam FSM_STATE_INIT_UNLOAD     = 6'd13;

   localparam FSM_STATE_READ_EI         = 6'd20;

   localparam FSM_STATE_ROUND_BEGIN     = 6'd25;

   localparam FSM_STATE_MULTIPLY_LOAD   = 6'd31;
   localparam FSM_STATE_MULTIPLY_WAIT   = 6'd32;
   localparam FSM_STATE_MULTIPLY_UNLOAD = 6'd33;

   localparam FSM_STATE_SQUARE_LOAD     = 6'd41;
   localparam FSM_STATE_SQUARE_WAIT     = 6'd42;
   localparam FSM_STATE_SQUARE_UNLOAD   = 6'd43;

   localparam FSM_STATE_ROUND_END       = 6'd50;

   localparam FSM_STATE_FINAL           = 6'd60;

   reg [5: 0]                           fsm_state = FSM_STATE_IDLE;


   //
   // Trigger
   //
   reg                                  next_dly = 1'b0;
   always @(posedge clk) next_dly <= next;
   wire                                 next_trig = (next == 1'b1) && (next_dly == 1'b0);


   //
   // Valid Register
   //
   reg                                  valid_reg = 1'b0;
   assign valid = valid_reg;


   //
   // Next/ Valid Logic
   //
   always @(posedge clk)
     //
     if (fsm_state == FSM_STATE_FINAL) begin
        //
        valid_reg <= 1'b1;
        //
     end else if (fsm_state == FSM_STATE_IDLE) begin
        //
        if (valid_reg && !next) valid_reg <= 1'b0;
        //
     end


   //
   // Exponent Bit Counter
   //
   reg  [4: 0]  ei_bit_count = 5'd0;
   wire         ei_bit = ro_exponent_bram_out[ei_bit_count];


   //
   // Round Counter
   //
   reg [MODULUS_NUM_BITS:0] round_count         = round_count_zero;
   wire [MODULUS_NUM_BITS:0] round_count_last   = exponent_width - 1'b1;
   wire [MODULUS_NUM_BITS:0] round_count_next   = (round_count < round_count_last) ? round_count + 1'b1 : round_count_zero;


   //
   // Handy Wires
   //
   wire [OPERAND_ADDR_WIDTH-1:0] modulus_width_msb = modulus_width[MODULUS_NUM_BITS-1:MODULUS_NUM_BITS-OPERAND_ADDR_WIDTH];

   wire [OPERAND_ADDR_WIDTH  :0] bram_core_addr_last = {modulus_width_msb, 1'b0};

   wire [OPERAND_ADDR_WIDTH  :0] bram_user_addr_last_ext = bram_core_addr_last - 1'b1;
   wire [OPERAND_ADDR_WIDTH-1:0] bram_user_addr_last = bram_user_addr_last_ext[OPERAND_ADDR_WIDTH-1:0];


   //
   // Handy Functions
   //
   function     [OPERAND_ADDR_WIDTH:0]  bram_core_addr_next_or_zero;
      input [OPERAND_ADDR_WIDTH:0] bram_core_addr;
      begin
         bram_core_addr_next_or_zero = (bram_core_addr < bram_core_addr_last) ? bram_core_addr + 1'b1 : bram_core_addr_zero;
      end
   endfunction

   function     [OPERAND_ADDR_WIDTH-1:0]        bram_user_addr_next_or_zero;
      input     [OPERAND_ADDR_WIDTH-1:0]        bram_user_addr;
      begin
         bram_user_addr_next_or_zero = (bram_user_addr < bram_user_addr_last) ? bram_user_addr + 1'b1 : bram_user_addr_zero;
      end
   endfunction


   //
   // Result BRAM Input
   //
   assign rw_result_bram_in = ei_bit ? ro_r_bram_out : rw_t_bram_out;


   //
   // MM BRAM Input Selector
   //
   always @(*)
     //
     case (fsm_state)

       FSM_STATE_INIT_LOAD:
         //
         rw_mm_bram_in = (rw_mm_bram_addr < bram_core_addr_last) ? ro_message_bram_out : {32{1'b0}};

       FSM_STATE_INIT_UNLOAD:
         //
         rw_mm_bram_in = ro_r_bram_out;

       FSM_STATE_SQUARE_UNLOAD:
         //
         rw_mm_bram_in = ro_r_bram_out;

       default:
         //
         rw_mm_bram_in  = {32{1'bX}};

     endcase


   //
   // Y BRAM Input Selector
   //
   always @(*)
                         //
     case (fsm_state)

       FSM_STATE_INIT_LOAD:
         //
         rw_y_bram_in = (rw_mm_bram_addr == bram_core_addr_zero) ? 32'h00000001 : 32'h00000000;

       FSM_STATE_MULTIPLY_UNLOAD:
         //
         rw_y_bram_in = ei_bit ? ro_r_bram_out : rw_t_bram_out; // RW!

       default:
         //
         rw_y_bram_in   = {32{1'bX}};

     endcase


   //
   // T BRAM Input Selector
   //
   always @(*)
                        //
     case (fsm_state)

       FSM_STATE_INIT_LOAD:
         //
         rw_t_bram_in = ro_coeff_bram_out;

       FSM_STATE_MULTIPLY_LOAD:
         //
         rw_t_bram_in = rw_y_bram_out;

       FSM_STATE_SQUARE_LOAD:
         //
         rw_t_bram_in = rw_mm_bram_out;

       default:
         //
         rw_t_bram_in   = {32{1'bX}};

     endcase


   //
   // Main Logic
   //
   always @(posedge clk)
     //
     case (fsm_state)

       FSM_STATE_INIT_LOAD: begin
          //
          rw_mm_bram_wr         <= (rw_mm_bram_addr < bram_core_addr_last) ? 1'b1 : 1'b0;
          rw_y_bram_wr          <= (rw_mm_bram_addr < bram_core_addr_last) ? 1'b1 : 1'b0;
          rw_t_bram_wr          <= (rw_mm_bram_addr < bram_core_addr_last) ? 1'b1 : 1'b0;
          //
          rw_mm_bram_addr       <= rw_mm_bram_wr ? bram_core_addr_next_or_zero(rw_mm_bram_addr) : bram_core_addr_zero;
          rw_y_bram_addr        <= rw_mm_bram_wr ? bram_core_addr_next_or_zero(rw_mm_bram_addr) : bram_core_addr_zero;
          rw_t_bram_addr        <= rw_mm_bram_wr ? bram_core_addr_next_or_zero(rw_mm_bram_addr) : bram_core_addr_zero;
          //
          if (ro_coeff_bram_addr > bram_core_addr_zero) ro_coeff_bram_addr <= bram_core_addr_next_or_zero(ro_coeff_bram_addr);
          else ro_coeff_bram_addr <= rw_mm_bram_wr ? bram_core_addr_zero : bram_core_addr_next_or_zero(ro_coeff_bram_addr);
          //
          if (ro_message_bram_addr > bram_user_addr_zero) ro_message_bram_addr <= bram_user_addr_next_or_zero(ro_message_bram_addr);
          else ro_message_bram_addr <= rw_mm_bram_wr ? bram_user_addr_zero : bram_user_addr_next_or_zero(ro_message_bram_addr);
          //
       end

       FSM_STATE_INIT_WAIT: begin
          //
          if (mul_ena)  mul_ena <= mul_rdy ? 1'b0 : 1'b1;
          else          mul_ena <= 1'b1;
          //
       end

       FSM_STATE_INIT_UNLOAD: begin
          //
          rw_mm_bram_wr <= (rw_mm_bram_addr < bram_core_addr_last) ? 1'b1 : 1'b0;
          //
          rw_mm_bram_addr <= rw_mm_bram_wr ? bram_core_addr_next_or_zero(rw_mm_bram_addr) : bram_core_addr_zero;
          //
          if (ro_r_bram_addr > bram_core_addr_zero) ro_r_bram_addr <= bram_core_addr_next_or_zero(ro_r_bram_addr);
          else ro_r_bram_addr <= rw_mm_bram_wr ? bram_core_addr_zero : bram_core_addr_next_or_zero(ro_r_bram_addr);
          //
       end

       FSM_STATE_MULTIPLY_LOAD: begin
          //
          rw_t_bram_wr <= (rw_t_bram_addr < bram_core_addr_last) ? 1'b1 : 1'b0;
          //
          rw_t_bram_addr <= rw_t_bram_wr ? bram_core_addr_next_or_zero(rw_t_bram_addr) : bram_core_addr_zero;
          //
          if (rw_y_bram_addr > bram_core_addr_zero) rw_y_bram_addr <= bram_core_addr_next_or_zero(rw_y_bram_addr);
          else rw_y_bram_addr <= rw_t_bram_wr ? bram_core_addr_zero : bram_core_addr_next_or_zero(rw_y_bram_addr);
          //
       end

       FSM_STATE_MULTIPLY_WAIT: begin
          //
          if (mul_ena)  mul_ena <= mul_rdy ? 1'b0 : 1'b1;
          else          mul_ena <= 1'b1;
          //
       end

       FSM_STATE_MULTIPLY_UNLOAD: begin
          //
          rw_y_bram_wr <= (rw_y_bram_addr < bram_core_addr_last) ? 1'b1 : 1'b0;
          //
          rw_y_bram_addr <= rw_y_bram_wr ? bram_core_addr_next_or_zero(rw_y_bram_addr) : bram_core_addr_zero;
          //
          if (ei_bit) begin
             //
             if (ro_r_bram_addr > bram_core_addr_zero) ro_r_bram_addr <= bram_core_addr_next_or_zero(ro_r_bram_addr);
             else ro_r_bram_addr <= rw_y_bram_wr ? bram_core_addr_zero : bram_core_addr_next_or_zero(ro_r_bram_addr);
             //
          end else begin
             //
             if (rw_t_bram_addr > bram_core_addr_zero) rw_t_bram_addr <= bram_core_addr_next_or_zero(rw_t_bram_addr);
             else rw_t_bram_addr <= rw_y_bram_wr ? bram_core_addr_zero : bram_core_addr_next_or_zero(rw_t_bram_addr);
             //
          end
          //
          if (round_count == round_count_last) begin
             //
             if (rw_result_bram_addr == bram_user_addr_zero) begin
                //
                if (rw_y_bram_wr) begin
                   //
                   rw_result_bram_wr <= (rw_y_bram_addr > bram_core_addr_zero) ? 1'b0 : 1'b1;
                   rw_result_bram_addr <= (rw_y_bram_addr > bram_core_addr_zero) ? bram_user_addr_zero : bram_user_addr_next_or_zero(rw_result_bram_addr);
                   //
                end else begin
                   //
                   rw_result_bram_wr <= 1'b1;
                   rw_result_bram_addr <= bram_user_addr_zero;
                   //
                end
                //
             end else begin
                //
                rw_result_bram_wr <= (rw_result_bram_addr < bram_user_addr_last) ? 1'b1 : 1'b0;
                rw_result_bram_addr <= bram_user_addr_next_or_zero(rw_result_bram_addr);
                //
             end
             //
          end
          //
       end

       FSM_STATE_SQUARE_LOAD: begin
          //
          rw_t_bram_wr <= (rw_t_bram_addr < bram_core_addr_last) ? 1'b1 : 1'b0;
          //
          rw_t_bram_addr <= rw_t_bram_wr ? bram_core_addr_next_or_zero(rw_t_bram_addr) : bram_core_addr_zero;
          //
          if (rw_mm_bram_addr > bram_core_addr_zero) rw_mm_bram_addr <= bram_core_addr_next_or_zero(rw_mm_bram_addr);
          else rw_mm_bram_addr <= rw_t_bram_wr ? bram_core_addr_zero : bram_core_addr_next_or_zero(rw_mm_bram_addr);
          //
       end

       FSM_STATE_SQUARE_WAIT: begin
          //
          if (mul_ena)  mul_ena <= mul_rdy ? 1'b0 : 1'b1;
          else          mul_ena <= 1'b1;
          //
       end

       FSM_STATE_SQUARE_UNLOAD: begin
          //
          rw_mm_bram_wr <= (rw_mm_bram_addr < bram_core_addr_last) ? 1'b1 : 1'b0;
          //
          rw_mm_bram_addr <= rw_mm_bram_wr ? bram_core_addr_next_or_zero(rw_mm_bram_addr) : bram_core_addr_zero;
          //
          if (ro_r_bram_addr > bram_core_addr_zero) ro_r_bram_addr <= bram_core_addr_next_or_zero(ro_r_bram_addr);
          else ro_r_bram_addr <= rw_mm_bram_wr ? bram_core_addr_zero : bram_core_addr_next_or_zero(ro_r_bram_addr);
          //
       end

       FSM_STATE_ROUND_END: begin
          //
          round_count <= round_count_next;
          //
          if (round_count < round_count_last) begin
             //
             ei_bit_count <= ei_bit_count + 1'b1;
             //
             if (ei_bit_count == 5'd31)
               //
               ro_exponent_bram_addr <= bram_user_addr_next_or_zero(ro_exponent_bram_addr);
             //
          end else begin
             //
             ei_bit_count <= 5'd0;
             //
             ro_exponent_bram_addr <= bram_user_addr_zero;
             //
          end
          //
       end

     endcase


   //
   // FSM Transition Logic
   //
   always @(posedge clk)
     //
     case (fsm_state)

       FSM_STATE_IDLE:          fsm_state <= (!valid_reg && next_trig) ? FSM_STATE_INIT_LOAD : FSM_STATE_IDLE;

       FSM_STATE_INIT_LOAD:     fsm_state <= (rw_y_bram_addr < bram_core_addr_last) ? FSM_STATE_INIT_LOAD : FSM_STATE_INIT_WAIT;
       FSM_STATE_INIT_WAIT:     fsm_state <= mul_rdy ? FSM_STATE_INIT_UNLOAD : FSM_STATE_INIT_WAIT;
       FSM_STATE_INIT_UNLOAD:   fsm_state <= (rw_mm_bram_addr < bram_core_addr_last) ? FSM_STATE_INIT_UNLOAD : FSM_STATE_READ_EI;

       FSM_STATE_READ_EI:       fsm_state <= FSM_STATE_ROUND_BEGIN;

       FSM_STATE_ROUND_BEGIN:   fsm_state <= (!ei_bit && fast_public_mode && (round_count < round_count_last)) ? FSM_STATE_SQUARE_LOAD : FSM_STATE_MULTIPLY_LOAD;

       FSM_STATE_MULTIPLY_LOAD: fsm_state <= (rw_t_bram_addr < bram_core_addr_last) ? FSM_STATE_MULTIPLY_LOAD : FSM_STATE_MULTIPLY_WAIT;
       FSM_STATE_MULTIPLY_WAIT: fsm_state <= mul_rdy ? FSM_STATE_MULTIPLY_UNLOAD : FSM_STATE_MULTIPLY_WAIT;
       FSM_STATE_MULTIPLY_UNLOAD: fsm_state <= (rw_y_bram_addr < bram_core_addr_last) ? FSM_STATE_MULTIPLY_UNLOAD : FSM_STATE_SQUARE_LOAD;

       FSM_STATE_SQUARE_LOAD:   fsm_state <= (rw_t_bram_addr < bram_core_addr_last) ? FSM_STATE_SQUARE_LOAD : FSM_STATE_SQUARE_WAIT;
       FSM_STATE_SQUARE_WAIT:   fsm_state <= mul_rdy ? FSM_STATE_SQUARE_UNLOAD : FSM_STATE_SQUARE_WAIT;
       FSM_STATE_SQUARE_UNLOAD: fsm_state <= (rw_mm_bram_addr < bram_core_addr_last) ? FSM_STATE_SQUARE_UNLOAD : FSM_STATE_ROUND_END;

       FSM_STATE_ROUND_END:     fsm_state <= (round_count < round_count_last) ? FSM_STATE_READ_EI : FSM_STATE_FINAL;

       FSM_STATE_FINAL:         fsm_state <= FSM_STATE_IDLE;

       default:                 fsm_state <= FSM_STATE_IDLE;

     endcase


endmodule
