//======================================================================
//
// coretest_entropy.v
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

module coretest_entropy(
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
  parameter I2C_ADDR_PREFIX       = 8'h00;
  parameter AVALANCHE_ADDR_PREFIX = 8'h20;
  parameter ROSC_ADDR_PREFIX      = 8'h30;


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

  // sha1 connections.
  reg           sha1_cs;
  reg           sha1_we;
  reg [7 : 0]   sha1_address;
  reg [31 : 0]  sha1_write_data;
  wire [31 : 0] sha1_read_data;
  wire          sha1_error;
  wire [7 : 0]  sha1_debug;

  // Avalanche noise based entropy provider connections.
  wire          avalanche_noise;
  reg           avalanche_cs;
  reg           avalanche_we;
  reg [7 : 0]   avalanche_address;
  reg [31 : 0]  avalanche_write_data;
  wire [31 : 0] avalanche_read_data;
  wire          avalanche_error;
  wire [7 : 0]  avalanche_debug;
  wire          avalanche_debug_update;
  wire          avalanche_discard;
  wire          avalanche_test_mode;
  wire          avalanche_security_error;
  wire          avalanche_entropy_enabled;
  wire [31 : 0] avalanche_entropy_data;
  wire          avalanche_entropy_valid;
  wire          avalanche_entropy_ack;


  // ROSC based entropy providr connections.
  reg           rosc_cs;
  reg           rosc_we;
  reg [7 : 0]   rosc_address;
  reg [31 : 0]  rosc_write_data;
  wire [31 : 0] rosc_read_data;
  wire          rosc_error;
  wire [7 : 0]  rosc_debug;
  wire          rosc_debug_update;
  wire          rosc_discard;
  wire          rosc_test_mode;
  wire          rosc_security_error;
  wire          rosc_entropy_enabled;
  wire [31 : 0] rosc_entropy_data;
  wire          rosc_entropy_valid;
  wire          rosc_entropy_ack;


  //----------------------------------------------------------------
  // Concurrent assignment.
  //----------------------------------------------------------------
  assign debug = avalanche_debug;

  assign avalanche_noise         = noise;
  assign avalanche_discard       = 0;
  assign avalanche_test_mode     = 0;
  assign avalanche_entropy_ack   = 1;
  assign avalanche_debug_update  = 1;

  assign rosc_discard       = 0;
  assign rosc_test_mode     = 0;
  assign rosc_entropy_ack   = 1;
  assign rosc_debug_update  = 1;


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


  avalanche_entropy avalanche_inst(
                                   .clk(clk),
                                   .reset_n(reset_n),

                                   .noise(avalanche_noise),

                                   .cs(avalanche_cs),
                                   .we(avalanche_we),
                                   .address(avalanche_address),
                                   .write_data(avalanche_write_data),
                                   .read_data(avalanche_read_data),
                                   .error(avalanche_error),

                                   .discard(avalanche_discard),
                                   .test_mode(avalanche_test_mode),
                                   .security_error(avalanche_security_error),

                                   .entropy_enabled(avalanche_entropy_enabled),
                                   .entropy_data(avalanche_entropy_data),
                                   .entropy_valid(avalanche_entropy_valid),
                                   .entropy_ack(avalanche_entropy_ack),

                                   .debug(avalanche_debug),
                                   .debug_update(avalanche_debug_update)
                                  );


  rosc_entropy rosc_inst(
                         .clk(clk),
                         .reset_n(reset_n),

                         .cs(rosc_cs),
                         .we(rosc_we),
                         .address(rosc_address),
                         .write_data(rosc_write_data),
                         .read_data(rosc_read_data),
                         .error(rosc_error),

                         .discard(rosc_discard),
                         .test_mode(rosc_test_mode),
                         .security_error(rosc_security_error),

                         .entropy_enabled(rosc_entropy_enabled),
                         .entropy_data(rosc_entropy_data),
                         .entropy_valid(rosc_entropy_valid),
                         .entropy_ack(rosc_entropy_ack),

                         .debug(rosc_debug),
                         .debug_update(rosc_debug_update)
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
      coretest_read_data   = 32'h00000000;
      coretest_error       = 0;

      i2c_cs               = 0;
      i2c_we               = 0;
      i2c_address          = 8'h00;
      i2c_write_data       = 32'h00000000;

      avalanche_cs         = 0;
      avalanche_we         = 0;
      avalanche_address    = 8'h00;
      avalanche_write_data = 32'h00000000;

      rosc_cs              = 0;
      rosc_we              = 0;
      rosc_address         = 8'h00;
      rosc_write_data      = 32'h00000000;


      case (coretest_address[15 : 8])
        I2C_ADDR_PREFIX:
          begin
            i2c_cs             = coretest_cs;
            i2c_we             = coretest_we;
            i2c_address        = coretest_address[7 : 0];
            i2c_write_data     = coretest_write_data;
            coretest_read_data = i2c_read_data;
            coretest_error     = i2c_error;
          end


        AVALANCHE_ADDR_PREFIX:
          begin
            avalanche_cs          = coretest_cs;
            avalanche_we          = coretest_we;
            avalanche_address     = coretest_address[7 : 0];
            avalanche_write_data  = coretest_write_data;
            coretest_read_data    = avalanche_read_data;
            coretest_error        = avalanche_error;
          end


        ROSC_ADDR_PREFIX:
          begin
            rosc_cs            = coretest_cs;
            rosc_we            = coretest_we;
            rosc_address       = coretest_address[7 : 0];
            rosc_write_data    = coretest_write_data;
            coretest_read_data = rosc_read_data;
            coretest_error     = rosc_error;
          end


        default:
          begin
          end
      endcase // case (coretest_address[15 : 8])
    end // address_mux

endmodule // coretest_entropy

//======================================================================
// EOF coretest_entropy.v
//======================================================================
