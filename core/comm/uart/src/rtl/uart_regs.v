//======================================================================
//
// uart.v
// ------
// Configuration registers for the uart core.
//
//
// Author: Joachim Strombergson
// Copyright (c) 2014, NORDUnet A/S
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
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

module comm_regs
  (
   input wire           clk,
   input wire           rst,

   input wire           cs,
   input wire           we,

   input wire [ 7 : 0]  address,
   input wire [31 : 0]  write_data,
   output wire [31 : 0] read_data,
   output wire          error
   );

   
   //----------------------------------------------------------------
   // Internal constant and parameter definitions.
   //----------------------------------------------------------------
   // API addresses.
   localparam ADDR_CORE_NAME0   = 8'h00;
   localparam ADDR_CORE_NAME1   = 8'h01;
   localparam ADDR_CORE_TYPE    = 8'h02;
   localparam ADDR_CORE_VERSION = 8'h03;

   localparam ADDR_BIT_RATE     = 8'h10;
   localparam ADDR_DATA_BITS    = 8'h11;
   localparam ADDR_STOP_BITS    = 8'h12;

   // Core ID constants.
   localparam CORE_NAME0   = 32'h75617274;  // "uart"
   localparam CORE_NAME1   = 32'h20202020;  // "    "
   localparam CORE_TYPE    = 32'h20202031;  // "   1"
   localparam CORE_VERSION = 32'h302e3031;  // "0.01"

   // The default bit rate is based on target clock frequency
   // divided by the bit rate times in order to hit the
   // center of the bits. I.e.
   // Clock: 50 MHz, 9600 bps
   // Divisor = 50*10E6 / 9600 = 5208
   localparam DEFAULT_BIT_RATE  = 16'd5208;
   localparam DEFAULT_DATA_BITS = 4'h8;
   localparam DEFAULT_STOP_BITS = 2'h1;
   

   //----------------------------------------------------------------
   // Registers including update variables and write enable.
   //----------------------------------------------------------------
   reg [31: 0]          tmp_read_data;


   //----------------------------------------------------------------
   // Concurrent connectivity for ports etc.
   //----------------------------------------------------------------
   assign read_data = tmp_read_data;
   

   //----------------------------------------------------------------
   // Access Handler
   //----------------------------------------------------------------
   always @ (posedge clk or posedge rst)
     begin
        if (rst)
          begin
             terasic_top.bit_rate  <= DEFAULT_BIT_RATE;
             terasic_top.data_bits <= DEFAULT_DATA_BITS;
             terasic_top.stop_bits <= DEFAULT_STOP_BITS;
          end
        else if (cs && we)
          begin
             // write operations
             case (address)
               ADDR_BIT_RATE:
                 terasic_top.bit_rate <= write_data[15 : 0];
               ADDR_DATA_BITS:
                 terasic_top.data_bits <= write_data[3 : 0];
               ADDR_STOP_BITS:
                 terasic_top.stop_bits <= write_data[1 : 0];
             endcase
          end
     end
   
   always @*
     begin
        tmp_read_data = 32'h00000000;

        if (cs && !we)
          begin
             // read operations
             case (address)
               ADDR_CORE_NAME0:
                 tmp_read_data = CORE_NAME0;
               ADDR_CORE_NAME1:
                 tmp_read_data = CORE_NAME1;
               ADDR_CORE_TYPE:
                 tmp_read_data = CORE_TYPE;
               ADDR_CORE_VERSION:
                 tmp_read_data = CORE_VERSION;
               ADDR_BIT_RATE:
                 tmp_read_data = {16'h0000, terasic_top.bit_rate};
               ADDR_DATA_BITS:
                 tmp_read_data = {28'h0000000, terasic_top.data_bits};
               ADDR_STOP_BITS:
                 tmp_read_data = {30'h0000000, terasic_top.stop_bits};
             endcase
          end
     end

endmodule // uart

//======================================================================
// EOF uart.v
//======================================================================
