//======================================================================
//
// test_core.v
// -----------
// A simple test core used during verification of the coretest test 
// module. The test core provies a few read and write registers
// that can be tested. Additionally it also provides an 8-bit data
// port directly connected to one of the internal registers.
//
//
// Author: Joachim Strombergson
// Copyright (c) 2014 SUNET
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

module test_core(
                 input wire           clk,
                 input wire           reset_n,
                 
                 input wire           cs,
                 input wire           we,
                 input wire [7 : 0]   address,
                 input wire [31 : 0]  write_data,
                 output wire [31 : 0] read_data,
                 output wire          error,

                 output wire [7 : 0]  debug
                );

  
  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  // Programming interface
  parameter ADDR_CORE_ID_0   = 8'h00;
  parameter ADDR_CORE_ID_1   = 8'h01;
  parameter ADDR_CORE_TYPE_0 = 8'h02;
  parameter ADDR_CORE_TYPE_1 = 8'h03;
  parameter ADDR_RW_REG      = 8'h10;
  parameter ADDR_DEBUG_REG   = 8'h20;

  parameter CORE_ID_0   = 32'h74657374; // "test"
  parameter CORE_ID_1   = 32'h636f7265; // "core"
  parameter CORE_TYPE_0 = 32'h30303030; // "00000"
  parameter CORE_TYPE_1 = 32'h30303162; // "001b"
  
  parameter RW_DEFAULT    = 32'h11223344;
  parameter DEBUG_DEFAULT = 8'h55;
  
  
  //----------------------------------------------------------------
  // Registers including update variables and write enable.
  //----------------------------------------------------------------
  reg [31 : 0] rw_reg;
  reg [31 : 0] rw_new;
  reg          rw_we;

  reg [7 : 0]  debug_reg;
  reg [7 : 0]  debug_new;
  reg          debug_we;
  
  
  
  //----------------------------------------------------------------
  // Wires.
  //----------------------------------------------------------------
  reg          tmp_error;
  reg [31 : 0] tmp_read_data;
  
  
  //----------------------------------------------------------------
  // Concurrent connectivity for ports etc.
  //----------------------------------------------------------------
  assign error     = tmp_error;
  assign read_data = tmp_read_data;
  assign debug     = debug_reg;
  
  
  //----------------------------------------------------------------
  // reg_update
  // Update functionality for all registers in the core.
  // All registers are positive edge triggered with synchronous
  // active low reset. All registers have write enable.
  //----------------------------------------------------------------
  always @ (posedge clk)
    begin: reg_update
      if (!reset_n)
        begin
          rw_reg    <= RW_DEFAULT;
          debug_reg <= DEBUG_DEFAULT;
        end
      else
        begin
          if (rw_we)
            begin
              rw_reg <= rw_new;
            end

          if (debug_we)
            begin
              debug_reg <= debug_new;
            end
        end
    end // reg_update

  
  //---------------------------------------------------------------
  // read_write_logic
  //
  // Read and write register logic.
  //---------------------------------------------------------------
  always @*
    begin: read_write_logic
      // Defafult assignments
      rw_new        = 32'h00000000;
      rw_we         = 0;
      debug_new     = 8'h00;
      debug_we      = 0;
      tmp_read_data = 32'h00000000;
      tmp_error     = 0;
      
      if (cs)
        begin
          if (we)
            begin
              // Write operations.
              case (address)
                ADDR_RW_REG:
                  begin
                    rw_new = write_data;
                    rw_we  = 1;
                  end

                ADDR_DEBUG_REG:
                  begin
                    debug_new = write_data[7 : 0];
                    debug_we  = 1;
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
                ADDR_CORE_ID_0:
                  begin
                    tmp_read_data = CORE_ID_0;
                  end
                
                ADDR_CORE_ID_1:
                  begin
                    tmp_read_data = CORE_ID_1;
                  end

                ADDR_CORE_TYPE_0:
                  begin
                    tmp_read_data = CORE_TYPE_0;
                  end

                ADDR_CORE_TYPE_1:
                  begin
                    tmp_read_data = CORE_TYPE_1;
                  end

                ADDR_RW_REG:
                  begin
                    tmp_read_data = rw_reg;
                  end

                ADDR_DEBUG_REG:
                  begin
                    tmp_read_data = {24'h000000, debug_reg};
                  end

                default:
                  begin
                    tmp_error = 1;
                  end
              endcase // case (address)
            end
          
        end
    end // read_write_logic

endmodule // test_core

//======================================================================
// EOF test_core.v
//======================================================================
