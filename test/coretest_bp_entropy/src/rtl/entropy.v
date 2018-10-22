//======================================================================
//
// entropy.v
// ---------
// digital HW based entropy generator. 
//
//
// Author: Bernd Paysan, Joachim Strombergson
// Copyright (c) 2014, Bernd Paysan, Secworks Sweden AB
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

module entropy(input wire          clk, 
               input wire          nreset, 

               input wire          cs,
               input wire          we,
               input wire [7:0]    addr,
               input wire [15:0]   dwrite,
               output wire [31:0]  dread,
               output wire [7 : 0] debug
              );

  //----------------------------------------------------------------
  // Symbolic names.
  //----------------------------------------------------------------
  // Delay in cycles between sampling random values 
  // and updating the debug port.
  // Corresponds to about 1/10s with clock @ 50 MHz.
  parameter DELAY_MAX             = 32'h004c4b40;

  parameter ADDR_ENT_WR_RNG1      = 8'h00;
  parameter ADDR_ENT_WR_RNG2      = 8'h01;
  
  parameter ADDR_ENT_RD_RNG1_RNG2 = 8'h10;
  parameter ADDR_ENT_RD_P         = 8'h11;
  parameter ADDR_ENT_RD_N         = 8'h12;
  parameter ADDR_ENT_MIX          = 8'h20;
  parameter ADDR_ENT_CONCAT       = 8'h21;

  
  //----------------------------------------------------------------
  // Registers.
  //----------------------------------------------------------------
  reg [7:0]    rng1, rng2; // must be inverse to each other
  reg [31 : 0] delay_ctr_reg;  
  reg [31 : 0] delay_ctr_new;  
  reg [7 : 0]  debug_reg;
  reg [31 : 0] mix_reg;
  reg [31 : 0] concat_reg;
 
  
  //----------------------------------------------------------------
  // Wires.
  //----------------------------------------------------------------
  wire [31 : 0] p, n;
  reg [31 : 0] tmp_dread;
  
  
  //----------------------------------------------------------------
  // Module instantiations.
  //----------------------------------------------------------------
  genvar i;
  generate
    for(i=0; i<32; i=i+1) begin: tworoscs
      rosc px(clk, nreset, rng1, rng2, p[i]);
      rosc nx(clk, nreset, rng1, rng2, n[i]);
    end
  endgenerate

  
  //----------------------------------------------------------------
  // Concurrent assignments to connect output ports.
  //----------------------------------------------------------------
  assign dread = tmp_dread;
  assign debug = debug_reg;

  
  //----------------------------------------------------------------
  // reg updates
  //----------------------------------------------------------------
  always @(posedge clk or negedge nreset)
    begin
      if(!nreset) 
        begin
	  rng1          <= 8'h55;
	  rng2          <= 8'haa;
          delay_ctr_reg <= 32'h00000000;
          mix_reg       <= 32'h00000000;
          concat_reg    <= 32'h00000000;
          debug_reg     <= 8'h00;
        end 
      else 
        begin
          delay_ctr_reg <= delay_ctr_new;
          mix_reg       <= n ^ p;
          concat_reg    <= {n[31 : 16] ^ n[15 : 0], p[31 : 16] ^ p[15 : 0]};
          
          if (delay_ctr_reg == 32'h00000000)
            begin
              debug_reg <= n[7 : 0];
            end
          
	  if(cs & we) 
            begin
	      case(addr)
	        ADDR_ENT_WR_RNG1: rng1 <= dwrite[15:8];
	        ADDR_ENT_WR_RNG2: rng2 <= dwrite[7:0];
                default:;
	      endcase
	    end
        end
    end

  
  //----------------------------------------------------------------
  // read_data
  //----------------------------------------------------------------
  always @*
    begin : read_data
      tmp_dread = 16'h0000;

      if(cs & ~we)
        case(addr)
	  ADDR_ENT_RD_RNG1_RNG2: tmp_dread = {16'h0000, rng1, rng2};
	  ADDR_ENT_RD_P:         tmp_dread = p;
	  ADDR_ENT_RD_N:         tmp_dread = n;
	  ADDR_ENT_MIX:          tmp_dread = mix_reg;
	  ADDR_ENT_CONCAT:       tmp_dread = concat_reg;
          default:;
         endcase
    end


  //----------------------------------------------------------------
  // delay_ctr
  //
  // Simple counter that counts to DELAY_MAC. Used to slow down
  // the debug port updates to human speeds.
  //----------------------------------------------------------------
  always @*
    begin : delay_ctr
      if (delay_ctr_reg == DELAY_MAX)
        begin
          delay_ctr_new = 32'h00000000;
        end
      else
        begin
          delay_ctr_new = delay_ctr_reg + 1'b1;
        end
    end // delay_ctr
  
endmodule // entropy

//======================================================================
// EOF entropy.v
//======================================================================
