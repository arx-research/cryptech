//======================================================================
//
// Copyright (c) 2016, NORDUnet A/S All rights reserved.
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

module brom_p384_h_x
  (
   input wire 		clk,
   input wire [ 4-1:0] 	b_addr,
   output wire [32-1:0] b_out
   );


   //
   // Output Registers
   //
   reg [31:0] 		bram_reg_b;

   assign b_out = bram_reg_b;


   //
   // Read-Only Port B
   //
   always @(posedge clk)
     //
     case (b_addr)
       4'b0000:	bram_reg_b <= 32'h5295df61;
       4'b0001:	bram_reg_b <= 32'h5b96a9c7;
       4'b0010:	bram_reg_b <= 32'hbe0e64f8;
       4'b0011:	bram_reg_b <= 32'h4fe0e86e;
       4'b0100:	bram_reg_b <= 32'h9fb96e9e;
       4'b0101:	bram_reg_b <= 32'h51d207d1;
       4'b0110:	bram_reg_b <= 32'ha6f434d6;
       4'b0111:	bram_reg_b <= 32'h89025959;
       4'b1000:	bram_reg_b <= 32'hc55b97f0;
       4'b1001:	bram_reg_b <= 32'h69260045;
       4'b1010:	bram_reg_b <= 32'h7ba3d2d9;
       4'b1011:	bram_reg_b <= 32'h08d99905;
     endcase
	  

endmodule
