//======================================================================
//
// terasic_regs.v
// -------------
// Global registers for the Cryptech Terasic FPGA framework.
//
//
// Author: Pavel Shatov
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

module board_regs
  (
   input wire           clk,
   input wire           rst,

   input wire           cs,
   input wire           we,

   input wire [ 7 : 0]  address,
   input wire [31 : 0]  write_data,
   output wire [31 : 0] read_data
   );


   //----------------------------------------------------------------
   // Internal constant and parameter definitions.
   //----------------------------------------------------------------
   // API addresses.
   localparam ADDR_CORE_NAME0   = 8'h00;
   localparam ADDR_CORE_NAME1   = 8'h01;
   localparam ADDR_CORE_VERSION = 8'h02;
   localparam ADDR_DUMMY_REG    = 8'hFF;    // general-purpose register

   // Core ID constants.
   localparam CORE_NAME0   = 32'h54433547;  // "TC5G"
   localparam CORE_NAME1   = 32'h20202020;  // "    "
   localparam CORE_VERSION = 32'h302e3130;  // "0.10"


   //----------------------------------------------------------------
   // Wires.
   //----------------------------------------------------------------
   reg [31: 0]          tmp_read_data;

   // dummy register to check that you can actually write something
   reg [31: 0] 		reg_dummy;

   
   //----------------------------------------------------------------
   // Concurrent connectivity for ports etc.
   //----------------------------------------------------------------
   assign read_data = tmp_read_data;
   

   //----------------------------------------------------------------
   // Access Handler
   //----------------------------------------------------------------
   always @(posedge clk)
     //
     if (rst)
       reg_dummy <= {32{1'b0}};
     else if (cs) begin
        //
        if (we) begin
           //
           // WRITE handler
           //
           case (address)
             ADDR_DUMMY_REG:
               reg_dummy        <= write_data;
           endcase
           //
        end else begin
           //
           // READ handler
           //
           case (address)
             ADDR_CORE_NAME0:
               tmp_read_data <= CORE_NAME0;
             ADDR_CORE_NAME1:
               tmp_read_data <= CORE_NAME1;
             ADDR_CORE_VERSION:
               tmp_read_data <= CORE_VERSION;
             ADDR_DUMMY_REG:
               tmp_read_data <= reg_dummy;
             //
             default:
               tmp_read_data <= {32{1'b0}};  // read non-existent locations as zeroes
           endcase
           //
        end
        //
     end

endmodule

//======================================================================
// EOF terasic_regs.v
//======================================================================
