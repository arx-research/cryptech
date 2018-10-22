//======================================================================
//
// eim.v
// ------
// Configuration registers for the eim core.
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
   // Clock and reset.
   input wire           clk,
   input wire           reset_n,

   // Control.
   input wire           cs,
   input wire           we,

   // Data ports.
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
   localparam ADDR_CORE_VERSION = 8'h02;

   // Core ID constants.
   localparam CORE_NAME0   = 32'h65696d20;  // "eim "
   localparam CORE_NAME1   = 32'h20202020;  // "    "
   localparam CORE_VERSION = 32'h302e3130;  // "0.10"


   //----------------------------------------------------------------
   // Wires.
   //----------------------------------------------------------------
   reg [31: 0]          tmp_read_data;
   reg                  write_error;
   reg                  read_error;


   //----------------------------------------------------------------
   // Concurrent connectivity for ports etc.
   //----------------------------------------------------------------
   assign read_data = tmp_read_data;
   assign error     = write_error | read_error;


   //----------------------------------------------------------------
   // storage registers for mapping memory to core interface
   //----------------------------------------------------------------
   always @ (posedge clk)
     begin
        write_error <= 0;

        if (cs && we)
          begin
             // write operations
             case (address)
               // no writeable registers
               default:
                 write_error <= 1;
               endcase
          end
     end

   always @*
     begin
        tmp_read_data = 32'h00000000;
        read_error    = 0;

        if (cs && !we)
          begin
             // read operations
             case (address)
               ADDR_CORE_NAME0:
                 tmp_read_data = CORE_NAME0;
               ADDR_CORE_NAME1:
                 tmp_read_data = CORE_NAME1;
               ADDR_CORE_VERSION:
                 tmp_read_data = CORE_VERSION;
               default:
                 read_error = 1;
             endcase
          end
     end

endmodule

//======================================================================
// EOF eim_regs.v
//======================================================================
