//======================================================================
//
// cdc_bus_pulse.v
// ---------------
// Clock Domain Crossing handler for the Cryptech Novena
// FPGA framework design.
//
// This module is based on design suggested on page 27 of the
// paper 'Clock Domain Crossing (CDC) Design & Verification Techniques
// Using SystemVerilog' by Clifford E. Cummings (Sunburst Design, Inc.)
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

module cdc_bus_pulse
  #(parameter   DATA_WIDTH      = 32)     // width of data bus
   (
    input wire                   src_clk, // source domain clock
    input wire [DATA_WIDTH-1:0]  src_din, // data from source clock domain
    input wire                   src_req, // start transfer pulse from source clock domain

    input wire                   dst_clk, // destination domain clock
    output wire [DATA_WIDTH-1:0] dst_dout, // data to destination clock domain
    output wire                  dst_pulse // transfer done pulse to destination clock domain
    );

   //
   // Source Side Registers
   //
   reg                           src_ff         = 1'b0;                 // transfer request flag
   reg [DATA_WIDTH-1:0]          src_latch      = {DATA_WIDTH{1'bX}};   // source data buffer


   //
   // Source Request Handler
   //
   always @(posedge src_clk)
     //
     if (src_req) begin                         // transfer request pulse?
        src_ff          <= ~src_ff;             // toggle transfer request flag...
        src_latch       <= src_din;             // ... and capture data in source buffer
     end


   //
   // Source -> Destination Flag Sync Logic
   //

   /* ISE may decide to infer SRL here, so we explicitly instantiate slice registers. */

   wire flag_sync_first;        // first FF output
   wire flag_sync_second;       // second FF output
   wire flag_sync_third;        // third FF output
   wire flag_sync_pulse;        // flag toggle detector output

   FDCE ff_sync_first
     (
      .C(dst_clk),
      .D(src_ff),               // capture flag from another clock domain
      .Q(flag_sync_first),      // metastability can occur here
      .CLR(1'b0),
      .CE(1'b1)
      );
   FDCE ff_sync_second
     (
      .C(dst_clk),
      .D(flag_sync_first),      // synchronize captured flag to remove metastability
      .Q(flag_sync_second),     // and pass it to another flip-flop
      .CLR(1'b0),
      .CE(1'b1)
      );
   FDCE ff_sync_third
     (
      .C(dst_clk),
      .D(flag_sync_second),     // delay synchronized flag in another flip-flip, because we need
      .Q(flag_sync_third),      // two synchronized flag values (current and delayed) to detect its change
      .CLR(1'b0),
      .CE(1'b1)
      );

   // when delayed flag value differs from its current value, it was changed
   // by the source side, so there must have been a transfer request
   assign flag_sync_pulse = flag_sync_second ^ flag_sync_third;


   //
   // Destination Side Registers
   //
   reg  dst_pulse_reg   = 1'b0;                         // transfer done flag
   reg [DATA_WIDTH-1:0] dst_latch = {DATA_WIDTH{1'bX}}; // destination data buffer

   assign dst_pulse     = dst_pulse_reg;
   assign dst_dout      = dst_latch;

   //
   // Destination Request Handler
   //
   always @(posedge dst_clk) begin
      //
      dst_pulse_reg <= flag_sync_pulse; // generate pulse if flag change was detected
      //
      if (flag_sync_pulse)
        dst_latch <= src_latch;
      /* By the time destination side receives synchronized flag
       * value, data should be stable, we can safely capture and store
       * it in the destination buffer.
       */

   end


endmodule

//======================================================================
// EOF cdc_bus_pulse.v
//======================================================================
