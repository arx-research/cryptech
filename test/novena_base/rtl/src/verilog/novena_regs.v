//======================================================================
//
// novena_regs.v
// -------------
// Global registers for the Cryptech Novena FPGA framework.
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

module novena_regs
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
   // Board-Level Registers
   //----------------------------------------------------------------
   localparam   ADDR_BOARD_TYPE         = 8'h00;        // board id
   localparam   ADDR_FIRMWARE_VER       = 8'h01;        // bitstream version
   localparam   ADDR_DUMMY_REG          = 8'hFF;        // general-purpose register


   //----------------------------------------------------------------
   // Constants
   //----------------------------------------------------------------
   localparam   NOVENA_BOARD_TYPE       = 32'h50565431;         // PVT1
   localparam   NOVENA_DESIGN_VER       = 32'h00_01_00_0b;      // v0.1.0b


   //
   // Output Register
   //
   reg [31: 0]          tmp_read_data;
   assign read_data = tmp_read_data;
   

   /* This dummy register can be used by users to check that they can actually
    * write something.
    */
   
   reg [31: 0]          reg_dummy;
   
   
   //
   // Access Handler
   //
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
             ADDR_BOARD_TYPE:
               tmp_read_data    <= NOVENA_BOARD_TYPE;
             ADDR_FIRMWARE_VER:
               tmp_read_data    <= NOVENA_DESIGN_VER;
             ADDR_DUMMY_REG:
               tmp_read_data    <= reg_dummy;
             //
             default:
               tmp_read_data    <= {32{1'b0}};  // read non-existent locations as zeroes
           endcase
           //
        end
        //
     end

endmodule

//======================================================================
// EOF novena_regs.v
//======================================================================
