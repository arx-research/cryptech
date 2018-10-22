//======================================================================
//
// fpga_entropy.v
// --------------
// Top level wrapper for the fpga entropy generator core.
//
//
// Author: Joachim Strombergson
// Copyright (c) 2014, Secworks Sweden AB
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

module fpga_entropy(
                    input wire           clk,
                    input wire           reset_n,
            
                    // API interface.
                    input wire           cs,
                    input wire           we,
                    input wire [7 : 0]   address,
                    input wire [31 : 0]  write_data,
                    output wire [31 : 0] read_data,
                    output wire          error,

                    // Debug output.
                    output wire [7 : 0]  debug
                   );

  
  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  // API addresses.
  parameter ADDR_CORE_NAME0   = 8'h00;
  parameter ADDR_CORE_NAME1   = 8'h01;
  parameter ADDR_CORE_VERSION = 8'h02;

  parameter ADDR_UPDATE       = 8'h10;
  parameter ADDR_OPA          = 8'h11;

  parameter ADDR_RND_READ     = 8'h20;
  
  // Core ID constants.
  parameter CORE_NAME0   = 32'h66706761;  // "fpga"
  parameter CORE_NAME1   = 32'h5f656e74;  // "_ent"
  parameter CORE_VERSION = 32'h302e3031;  // "0.01"

  // Default value assigned to operand A.
  // Operand B will be the inverse value of operand A.
  parameter DEFAULT_OPA = 32'haaaaaaaa;
  
  // Delay in cycles between updating the debug port with
  // a new random value sampled from the rng core port.
  // Corresponds to about 1/20s with a clock @ 50 MHz.
  parameter DELAY_MAX = 32'h002625a0;

  
  //----------------------------------------------------------------
  // Registers including update variables and write enable.
  //----------------------------------------------------------------
  reg [31 : 0] opa_reg;
  reg [31 : 0] opb_reg;
  reg [31 : 0] opa_new;
  reg          opa_we;

  reg          update_reg;
  reg          update_new;
  reg          update_we;

  reg [31 : 0] delay_ctr_reg;  
  reg [31 : 0] delay_ctr_new;  

  reg [7 : 0]  debug_reg;
  reg          debug_we;
  
  
  //----------------------------------------------------------------
  // Wires.
  //----------------------------------------------------------------
  reg           tmp_error;
  reg [31 : 0]  tmp_read_data;
  wire [31 : 0] core_rnd;
  
  
  //----------------------------------------------------------------
  // Concurrent connectivity for ports etc.
  //----------------------------------------------------------------
  assign read_data = tmp_read_data;
  assign error     = tmp_error;
  assign debug     = debug_reg;
  

  //----------------------------------------------------------------
  // core
  //
  // Instantiation of the uart core.
  //----------------------------------------------------------------
  fpga_entropy_core core(
                         .clk(clk),
                         .reset_n(reset_n),
                         .opa(opa_reg),
                         .opb(opb_reg),
                         .update(update_reg),
                         .rnd(core_rnd)
                        );

  
  //----------------------------------------------------------------
  // reg_update
  //
  // Update functionality for all registers in the core.
  // All registers are positive edge triggered with synchronous
  // active low reset. All registers have write enable.
  //----------------------------------------------------------------
  always @ (posedge clk)
    begin: reg_update
      if (!reset_n)
        begin
          opa_reg    <= DEFAULT_OPA;
          opb_reg    <= ~DEFAULT_OPA;
          update_reg <= 1;
          debug_reg  <= 8'h00;
        end
      else
        begin
          delay_ctr_reg <= delay_ctr_new;

          if (opa_we)
            begin
              opa_reg <= opa_new;
              opb_reg <= ~opa_new;
            end

          if (update_we)
            begin
              update_reg <= update_new;
            end

          if (debug_we)
            begin
              debug_reg <= core_rnd[7 : 0];
            end
        end
    end // reg_update


  //----------------------------------------------------------------
  // delay_ctr
  //
  // Simple counter that counts to DELAY_MAC. Used to slow down
  // the debug port updates to human speeds.
  //----------------------------------------------------------------
  always @*
    begin : delay_ctr
      debug_we = 0;
      
      if (delay_ctr_reg == DELAY_MAX)
        begin
          delay_ctr_new = 32'h00000000;
          debug_we      = 1;
        end
      else
        begin
          delay_ctr_new = delay_ctr_reg + 1'b1;
        end
    end // delay_ctr

  
  //----------------------------------------------------------------
  // api
  //
  // The core API that allows an internal host to control the
  // core functionality.
  //----------------------------------------------------------------
  always @*
    begin: api
      // Default assignments.
      opa_new       = 32'h00000000;
      opa_we        = 0;
      update_new    = 0;
      update_we     = 0;
      tmp_read_data = 32'h00000000;
      tmp_error     = 0;
      
      if (cs)
        begin
          if (we)
            begin
              // Write operations.
              case (address)
                ADDR_UPDATE:
                  begin
                    update_new = write_data[0];
                    update_we  = 1;
                  end

                ADDR_OPA:
                  begin
                    opa_new = write_data;
                    opa_we  = 1;
                  end
                
                default:
                  begin
                    tmp_error = 1;
                  end
              endcase // case (address)
            end
          else
            begin
              // Read operations.
              case (address)
                ADDR_CORE_NAME0:
                  begin
                    tmp_read_data = CORE_NAME0;
                  end

                ADDR_CORE_NAME1:
                  begin
                    tmp_read_data = CORE_NAME1;
                  end

                ADDR_CORE_VERSION:
                  begin
                    tmp_read_data = CORE_VERSION;
                  end
                
                ADDR_UPDATE:
                  begin
                    tmp_read_data = update_reg;
                  end

                ADDR_OPA:
                  begin
                    tmp_read_data = opa_reg;
                  end

                ADDR_RND_READ:
                  begin
                    tmp_read_data = core_rnd;
                  end
                
                default:
                  begin
                    tmp_error = 1;
                  end
              endcase // case (address)
            end
        end
    end
  
endmodule // fpga_entropy

//======================================================================
// EOF fpga_entropy.v
//======================================================================
