//======================================================================
//
// fpga_entropy_core.v
// -------------------
// fpga entropy generation core.
//
//
// Author: Joachim Strombergson
// Copyright (c) 2014, Secworks Sweden AB
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or 
// without modification, are permitted provided that the following 
// conditions are met: 
// 
// 1. Redistributions of source code must retain the above copyright 
//    notice, this list of conditions and the following disclaimer. 
// 
// 2. Redistributions in binary form must reproduce the above copyright 
//    notice, this list of conditions and the following disclaimer in 
//    the documentation and/or other materials provided with the 
//    distribution. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//======================================================================

module fpga_entropy_core(
                         // Clock and reset.
                         input wire           clk,
                         input wire           reset_n,

                         input wire [31 : 0]  opa,
                         input wire [31 : 0]  opb,
                         input wire           update,

                         output wire [31 : 0] rnd
                        );

  
  //----------------------------------------------------------------
  // Registers including update variables and write enable.
  //----------------------------------------------------------------
  reg [31 : 0] shift_reg;
  reg          shift_we;
  reg [31 : 0] rnd_reg;
  reg [4 : 0]  bit_ctr_reg;
  reg          rnd_ctr_reg;

  reg          bit0_reg;
  reg          bit1_reg;
  reg          bit_new;
  

  //----------------------------------------------------------------
  // Wires.
  //----------------------------------------------------------------
  wire dout02;
  wire dout03;
  wire dout07;
  wire dout13;
  wire dout41;
  wire dout43;

  
  //----------------------------------------------------------------
  // Concurrent connectivity for ports etc.
  //----------------------------------------------------------------
  assign rnd = rnd_reg;
  
             
  //----------------------------------------------------------------
  // module instantiations.
  //----------------------------------------------------------------
  bp_osc #(.WIDTH(2)) osc02(.clk(clk),
                            .reset_n(reset_n),
                            .opa(opa[1 : 0]),
                            .opb(opb[1 : 0]),
                            .dout(dout02)
                           );

  bp_osc #(.WIDTH(3)) osc03(.clk(clk),
                            .reset_n(reset_n),
                            .opa(opa[2 : 0]),
                            .opb(opb[2 : 0]),
                            .dout(dout03)
                           );

  bp_osc #(.WIDTH(7)) osc07(.clk(clk), 
                            .reset_n(reset_n),
                            .opa(opa[6 : 0]),
                            .opb(opb[6 : 0]),
                            .dout(dout07)
                            );

  bp_osc #(.WIDTH(13)) osc13(.clk(clk),
                             .reset_n(reset_n),
                             .opa(opa[12 : 0]),
                             .opb(opb[12 : 0]),
                             .dout(dout13)
                            );

  bp_osc #(.WIDTH(41)) osc41(.clk(clk),
                             .reset_n(reset_n),
                             .opa({opa[8 : 0], opa[31 : 0]}),
                             .opb({opb[8 : 0], opb[31 : 0]}),
                             .dout(dout41)
                            );

  bp_osc #(.WIDTH(43)) osc43(.clk(clk),
                            .reset_n(reset_n),
                            .opa({opa[10 : 0], opa[31 : 0]}), 
                            .opb({opb[10 : 0], opb[31 : 0]}),
                            .dout(dout43)
                           );
  
  
  //----------------------------------------------------------------
  // reg_update
  //
  // Update functionality for all registers in the core.
  // All registers are positive edge triggered with synchronous
  // active low reset. All registers have write enable.
  //----------------------------------------------------------------
  always @ (posedge clk)
    begin
      if (!reset_n)
        begin
          shift_reg   <= 32'h00000000;
          rnd_reg     <= 32'h00000000;
          bit_ctr_reg <= 5'h00;
          rnd_ctr_reg <= 1'b0;
          bit0_reg    <= 1'b0;
          bit1_reg    <= 1'b0;
        end
      else
        begin
          rnd_ctr_reg <= ~rnd_ctr_reg;

          if (rnd_ctr_reg)
            begin
              bit0_reg <= bit_new;
            end

          if (!rnd_ctr_reg)
            begin
              bit1_reg <= bit_new;
            end

          if (shift_we)
            begin
              shift_reg   <= {shift_reg[30 : 0], bit0_reg};
              bit_ctr_reg <= bit_ctr_reg + 1'b1;
            end
          
          if (bit_ctr_reg == 5'h1f)
            begin
              rnd_reg <= shift_reg;
            end
        end
    end // reg_update


  //----------------------------------------------------------------
  // rnd_gen
  //
  // Logic that implements the actual random bit value generator
  // Note: This logic also performs von Neumann decorrelation.
  //----------------------------------------------------------------
  always @*
    begin : rnd_gen
      shift_we = 1'b0;
      bit_new  = dout02 ^ dout03 ^ dout07 ^ dout13 ^ dout41 ^ dout43;

      if (update && rnd_ctr_reg)
        begin
          shift_we = bit0_reg ^ bit1_reg;
        end
    end

endmodule // fpga_entropy_core

//======================================================================
// EOF fpga_entropy_core.v
//======================================================================
