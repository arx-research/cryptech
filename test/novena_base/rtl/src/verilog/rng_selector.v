//======================================================================
//
// rng_selector.v
// -----------------
// Top level wrapper that creates the Cryptech coretest system.
// The wrapper contains instances of external interface, coretest
// and the core to be tested. And if more than one core is
// present the wrapper also includes address and data muxes.
//
//
// Authors: Joachim Strombergson, Paul Selkirk, Pavel Shatov
// Copyright (c) 2014-2015, NORDUnet A/S All rights reserved.
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

module rng_selector
  (
   input wire           sys_clk,
   input wire           sys_rst,
   input wire           sys_ena,

   input wire [13: 0]   sys_eim_addr,
   input wire           sys_eim_wr,
   input wire           sys_eim_rd,
   output wire [31 : 0] sys_read_data,
   input wire [31 : 0]  sys_write_data
   );

   
   //
   // Output Register
   //
   reg [31: 0]          tmp_read_data;
   assign sys_read_data = tmp_read_data;
   

   /* So far we have no RNG cores, let's make some dummy 32-bit registers here
    * to prevent ISE from complaining that we don't use input ports.
    */
   
   reg [31: 0]          reg_dummy_first;
   reg [31: 0]          reg_dummy_second;
   reg [31: 0]          reg_dummy_third;
   
   always @(posedge sys_clk)
     //
     if (sys_rst) begin
        reg_dummy_first  <= {8{4'hA}};
        reg_dummy_second <= {8{4'hB}};
        reg_dummy_third  <= {8{4'hC}};
     end else if (sys_ena) begin
        //
        if (sys_eim_wr) begin
           //
           // WRITE handler
           //
           case (sys_eim_addr)
             14'd0: reg_dummy_first     <= sys_write_data;
             14'd1: reg_dummy_second    <= sys_write_data;
             14'd2: reg_dummy_third     <= sys_write_data;
           endcase
           //
        end
        //
        if (sys_eim_rd) begin
           //
           // READ handler
           //
           case (sys_eim_addr)
             14'd0: tmp_read_data       <= reg_dummy_first;
             14'd1: tmp_read_data       <= reg_dummy_second;
             14'd2: tmp_read_data       <= reg_dummy_third;
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
// EOF core_selector.v
//======================================================================
