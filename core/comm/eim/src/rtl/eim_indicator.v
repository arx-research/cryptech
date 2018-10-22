//======================================================================
//
// eim_indicator.v
// ---------------
// A simple LED indicator to show that the EIM is alive.
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

module eim_indicator
  (
   input wire  sys_clk,
   input wire  sys_rst_n,
   input wire  eim_active,
   output wire led_out
   );

   //
   // Parameters
   //
   localparam   CNT_BITS                = 24;   // led will be dim for 2**(24-1) = 8388608 ticks, which is ~100 ms @ 80 MHz.

   //
   // Counter
   //
   reg [CNT_BITS-1:0] cnt;

   always @(posedge sys_clk)
     //
     if (!sys_rst_n)                    cnt <= {CNT_BITS{1'b0}};
     else if (cnt > {CNT_BITS{1'b0}})   cnt <= cnt - 1'b1;
     else if (eim_active)               cnt <= {CNT_BITS{1'b1}};

   assign led_out = ~cnt[CNT_BITS-1];

endmodule

//======================================================================
// EOF eim_indicator.v
//======================================================================
