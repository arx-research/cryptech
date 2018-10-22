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

module modexps6_adder64_carry32
  (
   input wire          clk,
   input wire [31: 0]  t,
   input wire [31: 0]  x,
   input wire [31: 0]  y,
   output wire [31: 0] s,
   input wire [31: 0]  c_in,
   output wire [31: 0] c_out
   );


   //
   // Multiplier
   //
   wire [63: 0]        multiplier_out;

   multiplier_s6 dsp_multiplier
     (
      .clk             (clk),
      .a               (x),
      .b               (y),
      .p               (multiplier_out)
      );


   //
   // Carry and T
   //
   wire [63: 0]        t_ext = {{32{1'b0}}, t};
   wire [63: 0]        c_ext = {{32{1'b0}}, c_in};


   //
   // Sum
   //
   wire [63: 0]        sum = multiplier_out + c_in + t;


   //
   // Output
   //
   assign s = sum[31: 0];
   assign c_out = sum[63:32];

   /*
    reg [31: 0] s_reg;
    reg [31: 0] c_out_reg;

    assign s = s_reg;
    assign c_out = c_out_reg;

    always @(posedge clk) begin
      //
      s_reg            <= sum[31: 0];
      c_out_reg        <= sum[63:32];
      //
    end
    */


endmodule
