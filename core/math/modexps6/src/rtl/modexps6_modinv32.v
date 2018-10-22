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

module modexps6_modinv32
  (
   input wire          clk,

   input wire          ena,
   output wire         rdy,

   input wire [31: 0]  n0,
   output wire [31: 0] n0_modinv
   );


   //
   // Trigger
   //
   reg                 ena_dly = 1'b0;
   wire                ena_trig = ena && !ena_dly;
   always @(posedge clk) ena_dly <= ena;


   //
   // Ready Register
   //
   reg                 rdy_reg = 1'b0;
   assign rdy = rdy_reg;


   //
   // Counter
   //
   reg [7: 0]          cnt = 8'd0;
   wire [7: 0]         cnt_zero = 8'd0;
   wire [7: 0]         cnt_last = 8'd132;
   wire [7: 0]         cnt_next = cnt + 1'b1;
   wire [1: 0]         cnt_phase = cnt[1:0];
   wire [5: 0]         cnt_cycle = cnt[7:2];

   always @(posedge clk)
     //
     if (cnt == cnt_zero) cnt <= (!rdy_reg && ena_trig) ? cnt_next : cnt_zero;
     else cnt <= (cnt == cnt_last) ? cnt_zero : cnt_next;


   //
   // Enable / Ready Logic
   //
   always @(posedge clk)
     //
     if (cnt == cnt_last) rdy_reg <= 1'b1;
     else if ((cnt == cnt_zero) && (rdy_reg && !ena)) rdy_reg <= 1'b0;


   //
   // Output Register
   //
   reg [31: 0]         n0_modinv_reg;
   assign n0_modinv = n0_modinv_reg;


   //
   // Multiplier
   //
   wire [63: 0]        multiplier_out;
   wire [31: 0]        multiplier_out_masked = multiplier_out[31: 0] & {mask_reg, 1'b1};

   multiplier_s6 dsp_multiplier
     (
      .clk              (clk),
      .a                (n0),
      .b                (n0_modinv_reg),
      .p                (multiplier_out)
      );


   //
   // Mask and Power
   //
   reg [30: 0]         mask_reg;
   reg [31: 0]         power_reg;

   always @(posedge clk)
     //
     if (cnt_phase == 2'd1) begin
        //
        if (cnt_cycle == 6'd0) begin
           //
           mask_reg <= 31'd0;
           power_reg <= 32'd1;
           //
           n0_modinv_reg <= 32'd0;
           //
        end else begin
           //
           mask_reg <= { mask_reg[29:0], 1'b1};
           power_reg <= {power_reg[30:0], 1'b0};
           //
           if (multiplier_out_masked != 32'd1)
             //
             n0_modinv_reg <= n0_modinv_reg + power_reg;
           //
        end
        //
     end


endmodule
