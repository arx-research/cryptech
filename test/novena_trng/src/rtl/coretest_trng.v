//======================================================================
//
// coretest_trng.v
// -----------------
// Top level wrapper that creates the Cryptech coretest system.
// The wrapper contains instances of external interface, coretest
// and the cores to be tested. And if more than one core is
// present the wrapper also includes address and data muxes.
//
//
// Author: Joachim Strombergson
// Copyright (c) 2014, SUNET
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

module coretest_trng(
                       input wire          clk,
                       input wire          reset_n,

                       input               noise,

                       // External interface.
                       input wire          SCL,
                       input wire          SDA,
                       output wire         SDA_pd,

                       output wire [7 : 0] debug
                      );


  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  parameter I2C_ADDR_PREFIX  = 4'h0;
  parameter TRNG_ADDR_PREFIX = 4'h1;


  //----------------------------------------------------------------
  // Wires.
  //----------------------------------------------------------------
  // Coretest connections.
  wire          coretest_reset_n;
  wire          coretest_cs;
  wire          coretest_we;
  wire [15 : 0] coretest_address;
  wire [31 : 0] coretest_write_data;
  wire [31 : 0] coretest_read_data;
  wire          coretest_error;

  // i2c connections
  wire          i2c_rxd_syn;
  wire [7 : 0]  i2c_rxd_data;
  wire          i2c_rxd_ack;
  wire          i2c_txd_syn;
  wire [7 : 0]  i2c_txd_data;
  wire          i2c_txd_ack;
  reg           i2c_cs;
  reg           i2c_we;
  reg [7 : 0]   i2c_address;
  reg [31 : 0]  i2c_write_data;
  wire [31 : 0] i2c_read_data;
  wire          i2c_error;
  wire [7 : 0]  i2c_debug;

  // trng connections.
  reg           trng_cs;
  reg           trng_we;
  reg [7 : 0]   trng_address;
  reg [31 : 0]  trng_write_data;
  wire [31 : 0] trng_read_data;
  wire          trng_error;
  wire [7 : 0]  trng_debug;
  wire          trng_debug_update;
  wire          trng_security_error;


  //----------------------------------------------------------------
  // Concurrent assignment.
  //----------------------------------------------------------------
  assign debug              = trng_debug;
  assign trng_debug_update  = 1;


  //----------------------------------------------------------------
  // Core instantiations.
  //----------------------------------------------------------------
  coretest coretest_inst(
                         .clk(clk),
                         .reset_n(reset_n),

                         .rx_syn(i2c_rxd_syn),
                         .rx_data(i2c_rxd_data),
                         .rx_ack(i2c_rxd_ack),

                         .tx_syn(i2c_txd_syn),
                         .tx_data(i2c_txd_data),
                         .tx_ack(i2c_txd_ack),

                         .core_reset_n(coretest_reset_n),
                         .core_cs(coretest_cs),
                         .core_we(coretest_we),
                         .core_address(coretest_address),
                         .core_write_data(coretest_write_data),
                         .core_read_data(coretest_read_data),
                         .core_error(coretest_error)
                        );


  i2c i2c_inst(
               .clk(clk),
               .reset_n(!reset_n),

               .SCL(SCL),
               .SDA(SDA),
               .SDA_pd(SDA_pd),
               .i2c_device_addr(8'h1E),

               .rxd_syn(i2c_rxd_syn),
               .rxd_data(i2c_rxd_data),
               .rxd_ack(i2c_rxd_ack),

               .txd_syn(i2c_txd_syn),
               .txd_data(i2c_txd_data),
               .txd_ack(i2c_txd_ack),

               .cs(i2c_cs),
               .we(i2c_we),
               .address(i2c_address),
               .write_data(i2c_write_data),
               .read_data(i2c_read_data),
               .error(i2c_error),

               .debug(i2c_debug)
              );


  trng trng_inst(
                 // Clock and reset.
                 .clk(clk),
                 .reset_n(reset_n),

                 .avalanche_noise(noise),

                 .cs(coretest_cs),
                 .we(coretest_we),
                 .address(coretest_address),
                 .write_data(coretest_write_data),
                 .read_data(coretest_read_data),
                 .error(coretest_error),

                 .debug(trng_debug),
                 .debug_update(trng_debug_update),

                 .security_error(trng_security_error)
                );


  //----------------------------------------------------------------
  // address_mux
  //
  // Combinational data mux that handles addressing between
  // cores using the 32-bit memory like interface.
  //----------------------------------------------------------------
//  always @*
//    begin : address_mux
//      // Default assignments.
//      coretest_read_data   = 32'h00000000;
//      coretest_error       = 0;
//
//      i2c_cs               = 0;
//      i2c_we               = 0;
//      i2c_address          = 8'h00;
//      i2c_write_data       = 32'h00000000;

//      trng_cs              = 0;
//      trng_we              = 0;
//      trng_address         = 8'h00;
//      trng_write_data      = 32'h00000000;

//      trng_cs            = coretest_cs;
//      trng_we            = coretest_we;
//      trng_address       = coretest_address[11 : 0];
//      trng_write_data    = coretest_write_data;
//      coretest_read_data = trng_read_data;
//      coretest_error     = trng_error;
//
//      case (coretest_address[13 : 12])
//        I2C_ADDR_PREFIX:
//          begin
//            i2c_cs             = coretest_cs;
//            i2c_we             = coretest_we;
//            i2c_address        = coretest_address[7 : 0];
//            i2c_write_data     = coretest_write_data;
//            coretest_read_data = i2c_read_data;
//            coretest_error     = i2c_error;
//          end
//
//        TRNG_ADDR_PREFIX:
//          begin
//            trng_cs            = coretest_cs;
//            trng_we            = coretest_we;
//            trng_address       = coretest_address[11 : 0];
//            trng_write_data    = coretest_write_data;
//            coretest_read_data = trng_read_data;
//            coretest_error     = trng_error;
//          end
//
//        default:
//          begin
//          end
//      endcase // case (coretest_address[15 : 12])
//    end // address_mux

endmodule // coretest_trng

//======================================================================
// EOF coretest_trng.v
//======================================================================
