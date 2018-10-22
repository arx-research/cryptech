//======================================================================
//
// single_rosc.v
// -------------
// Top level module for testing the digital ring oscillator. This
// module basically instantiates an oscillator with registered inputs
// and outputs to allow synthesis tools to do good placement and
// timing of the oscillator parts.
//
// The module is used to test how the oscillator is mapped to
// resources in different FPGAs, and what happens when the size
// of the oscillator increases.
// idea of using carry chain in adders as inverter by Bernd Paysan.
//
//
//
// Author: Joachim Strombergson
// Copyright (c) 2015, NORDUnet A/S All rights reserved.
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

module single_rosc
             (
              input wire                   clk,
              input wire                   reset_n,

              input wire                   we,

              input wire [(WIDTH - 1) : 0] opa,
              input wire [(WIDTH - 1) : 0] opb,

              output wire                  dout
             );

  parameter WIDTH = 3;

  //----------------------------------------------------------------
  // Registers.
  //----------------------------------------------------------------
  reg [(WIDTH - 1) : 0] opa_reg;
  reg [(WIDTH - 1) : 0] opb_reg;

  reg                   we_reg;

  reg                   dout_reg;

  // Ugly in-line Xilinx attribute to preserve the registers.
  (* equivalent_register_removal = "no" *)
  wire                  dout_new;


  //----------------------------------------------------------------
  // Concurrent assignment.
  //----------------------------------------------------------------
  assign dout = dout_reg;


  //----------------------------------------------------------------
  // oscillator instance.
  //----------------------------------------------------------------
  rosc #(.WIDTH(WIDTH)) osc(
                            .clk(clk),
                            .we(we_reg),
                            .reset_n(reset_n),
                            .opa(opa_reg),
                            .opb(opb_reg),
                            .dout(dout_new)
                            );


  //----------------------------------------------------------------
  // reg_update
  //----------------------------------------------------------------
     always @ (posedge clk or negedge reset_n)
       begin
         if (!reset_n)
           begin
             opa_reg  <= {WIDTH{1'b0}};
             opb_reg  <= {WIDTH{1'b0}};
             we_reg   <= 1'b0;
             dout_reg <= 1'b0;
           end
         else
           begin
             we_reg   <= we;
             opa_reg  <= opa;
             opb_reg  <= opb;
             dout_reg <= dout_new;
           end
       end

endmodule // single_rosc

//======================================================================
// EOF single_rosc.v
//======================================================================
