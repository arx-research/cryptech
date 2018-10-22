//======================================================================
//
// coretest_bp_entropy.v
// ---------------------
// Top level module for the BP FPGA entropy source tester.
//
//
// Author: Joachim Strombergson
// Copyright (c) 2014  Secworks Sweden AB
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

module coretest_bp_entropy(
                           input wire          clk,
                           input wire          reset_n,
                       
                           // External interface.
                           input wire          rxd,
                           output wire         txd,
                           
                           output wire [7 : 0] debug
                          );

  
  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  parameter UART_ADDR_PREFIX   = 8'h00;
  parameter ENT_ADDR_PREFIX    = 8'h10;
  
  
  //----------------------------------------------------------------
  // Wires.
  //----------------------------------------------------------------
  // Coretest connections.
  wire          coretest_reset_n;
  wire          coretest_cs;
  wire          coretest_we;
  wire [15 : 0] coretest_address;
  wire [31 : 0] coretest_write_data;
  reg [31 : 0]  coretest_read_data;
  reg           coretest_error;

  // uart connections
  wire          uart_rxd_syn;
  wire [7 : 0]  uart_rxd_data;
  wire          uart_rxd_ack;
  wire          uart_txd_syn;
  wire [7 : 0]  uart_txd_data;
  wire          uart_txd_ack;
  reg           uart_cs;
  reg           uart_we;
  reg [7 : 0]   uart_address;
  reg [31 : 0]  uart_write_data;
  wire [31 : 0] uart_read_data;
  wire          uart_error;
  wire [7 : 0]  uart_debug;

  reg           ent_cs;
  reg           ent_we;
  reg [7 : 0]   ent_address;
  reg [31 : 0]  ent_write_data;
  wire [31 : 0] ent_read_data;
  wire [7 : 0]  ent_debug;
  
  
  //----------------------------------------------------------------
  // Concurrent assignment.
  //----------------------------------------------------------------
  assign debug = ent_debug;
  
  
  //----------------------------------------------------------------
  // Core instantiations.
  //----------------------------------------------------------------
  coretest coretest(
                    .clk(clk),
                    .reset_n(reset_n),
                         
                    .rx_syn(uart_rxd_syn),
                    .rx_data(uart_rxd_data),
                    .rx_ack(uart_rxd_ack),
                    
                    .tx_syn(uart_txd_syn),
                    .tx_data(uart_txd_data),
                    .tx_ack(uart_txd_ack),
                    
                    // Interface to the core being tested.
                    .core_reset_n(coretest_reset_n),
                    .core_cs(coretest_cs),
                    .core_we(coretest_we),
                    .core_address(coretest_address),
                    .core_write_data(coretest_write_data),
                    .core_read_data(coretest_read_data),
                    .core_error(coretest_error)
                   );


  uart uart(
            .clk(clk),
            .reset_n(reset_n),
            
            .rxd(rxd),
            .txd(txd),

            .rxd_syn(uart_rxd_syn),
            .rxd_data(uart_rxd_data),
            .rxd_ack(uart_rxd_ack),

            .txd_syn(uart_txd_syn),
            .txd_data(uart_txd_data),
            .txd_ack(uart_txd_ack),
            
            .cs(uart_cs),
            .we(uart_we),
            .address(uart_address),
            .write_data(uart_write_data),
            .read_data(uart_read_data),
            .error(uart_error),

            .debug(uart_debug)
           );


  entropy entropy(.clk(clk), 
                  .nreset(reset_n), 
                  .cs(ent_cs),
                  .we(ent_we),
                  .addr(ent_address),
                  .dwrite(ent_write_data),
                  .dread(ent_read_data),
                  .debug(ent_debug)
                 );

  
  //----------------------------------------------------------------
  // address_mux
  //
  // Combinational data mux that handles addressing between
  // cores using the 32-bit memory like interface.
  //----------------------------------------------------------------
  always @*
    begin : address_mux
      // Default assignments.
      coretest_read_data = 32'h00000000;
      coretest_error     = 0;

      uart_cs            = 0;
      uart_we            = 0;
      uart_address       = 8'h00;
      uart_write_data    = 32'h00000000;

      ent_cs             = 0;
      ent_we             = 0;
      ent_address        = 8'h00;
      ent_write_data     = 32'h00000000;

      case (coretest_address[15 : 8])
        UART_ADDR_PREFIX:
          begin
            uart_cs            = coretest_cs;
            uart_we            = coretest_we;
            uart_address       = coretest_address[7 : 0];
            uart_write_data    = coretest_write_data;
            coretest_read_data = uart_read_data;
            coretest_error     = uart_error;
          end
        
        ENT_ADDR_PREFIX:
          begin
            ent_cs             = coretest_cs;
            ent_we             = coretest_we;
            ent_address        = coretest_address[7 : 0];
            ent_write_data     = coretest_write_data[15 : 0];
            coretest_read_data = ent_read_data;
            coretest_error     = 1'b0;
          end
        
        default:
          begin
          end
      endcase // case (coretest_address[15 : 8])
    end // address_mux
  
endmodule // coretest_bp_entropy

//======================================================================
// EOF coretest_bp_entropy.v
//======================================================================
