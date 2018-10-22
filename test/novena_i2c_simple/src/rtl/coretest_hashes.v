//======================================================================
//
// coretest_hashes.v
// -----------------
// Top level wrapper that creates the Cryptech coretest system.
// The wrapper contains instances of the external interface and the
// cores to be tested, as well as address and data muxes.
//
//
// Authors: Joachim Strombergson, Paul Selkirk
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

module coretest_hashes(
                       input wire          clk,
                       input wire          reset_n,

                       // External interface.
                       input wire          SCL,
                       input wire          SDA,
                       output wire         SDA_pd,

                       output wire [7 : 0] debug
                      );


  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  parameter I2C_SHA1_ADDR       = 7'h1e;
  parameter I2C_SHA256_ADDR     = 7'h1f;
  parameter I2C_SHA512_224_ADDR = 7'h20;
  parameter I2C_SHA512_256_ADDR = 7'h21;
  parameter I2C_SHA384_ADDR     = 7'h22;
  parameter I2C_SHA512_ADDR     = 7'h23;

  parameter MODE_SHA_512_224    = 2'h0;
  parameter MODE_SHA_512_256    = 2'h1;
  parameter MODE_SHA_384        = 2'h2;
  parameter MODE_SHA_512        = 2'h3;


  //----------------------------------------------------------------
  // Wires.
  //----------------------------------------------------------------
  // i2c connections
  wire          i2c_rxd_syn;
  wire [7 : 0]  i2c_rxd_data;
  reg           i2c_rxd_ack;
  reg           i2c_txd_syn;
  reg  [7 : 0]  i2c_txd_data;
  wire          i2c_txd_ack;
  wire [6 : 0]  i2c_device_addr;

  // sha1 connections.
  reg           sha1_rxd_syn;
  reg  [7 : 0]  sha1_rxd_data;
  wire          sha1_rxd_ack;
  wire          sha1_txd_syn;
  wire [7 : 0]  sha1_txd_data;
  reg           sha1_txd_ack;

  // sha256 connections.
  reg           sha256_rxd_syn;
  reg  [7 : 0]  sha256_rxd_data;
  wire          sha256_rxd_ack;
  wire          sha256_txd_syn;
  wire [7 : 0]  sha256_txd_data;
  reg           sha256_txd_ack;

  // sha512 connections.
  reg  [1 : 0]  sha512_mode;
  reg           sha512_rxd_syn;
  reg  [7 : 0]  sha512_rxd_data;
  wire          sha512_rxd_ack;
  wire          sha512_txd_syn;
  wire [7 : 0]  sha512_txd_data;
  reg           sha512_txd_ack;


  //----------------------------------------------------------------
  // Core instantiations.
  //----------------------------------------------------------------
  i2c_core i2c(
               .clk(clk),
               .reset(!reset_n), // active high

               .SCL(SCL),
               .SDA(SDA),
               .SDA_pd(SDA_pd),
               .i2c_device_addr(i2c_device_addr),

               .rxd_syn(i2c_rxd_syn),
               .rxd_data(i2c_rxd_data),
               .rxd_ack(i2c_rxd_ack),

               .txd_syn(i2c_txd_syn),
               .txd_data(i2c_txd_data),
               .txd_ack(i2c_txd_ack)
               );

  sha1 sha1(
            .clk(clk),
            .reset_n(reset_n),

            .rx_syn(sha1_rxd_syn),
            .rx_data(sha1_rxd_data),
            .rx_ack(sha1_rxd_ack),

            .tx_syn(sha1_txd_syn),
            .tx_data(sha1_txd_data),
            .tx_ack(sha1_txd_ack)
           );

  sha256 sha256(
                .clk(clk),
                .reset_n(reset_n),

                .rx_syn(sha256_rxd_syn),
                .rx_data(sha256_rxd_data),
                .rx_ack(sha256_rxd_ack),

                .tx_syn(sha256_txd_syn),
                .tx_data(sha256_txd_data),
                .tx_ack(sha256_txd_ack)
                );

  sha512 sha512(
                .clk(clk),
                .reset_n(reset_n),

                .mode(sha512_mode),

                .rx_syn(sha512_rxd_syn),
                .rx_data(sha512_rxd_data),
                .rx_ack(sha512_rxd_ack),

                .tx_syn(sha512_txd_syn),
                .tx_data(sha512_txd_data),
                .tx_ack(sha512_txd_ack)
                );

  //----------------------------------------------------------------
  // address_mux
  //
  // Combinational data mux that handles addressing between
  // cores using the I2C device address.
  //----------------------------------------------------------------
  always @*
    begin : address_mux
      // Default assignments.
      i2c_rxd_ack     = 0;
      i2c_txd_syn     = 0;
      i2c_txd_data    = 8'h00;

      sha1_rxd_syn    = 0;
      sha1_rxd_data   = 8'h00;
      sha1_txd_ack    = 0;

      sha256_rxd_syn  = 0;
      sha256_rxd_data = 8'h00;
      sha256_txd_ack  = 0;

      sha512_rxd_syn  = 0;
      sha512_rxd_data = 8'h00;
      sha512_txd_ack  = 0;
      sha512_mode     = 2'b00;

      case (i2c_device_addr)
        I2C_SHA1_ADDR:
          begin
            sha1_rxd_syn    = i2c_rxd_syn;
            sha1_rxd_data   = i2c_rxd_data;
            i2c_rxd_ack     = sha1_rxd_ack;
            i2c_txd_syn     = sha1_txd_syn;
            i2c_txd_data    = sha1_txd_data;
            sha1_txd_ack    = i2c_txd_ack;
          end

        I2C_SHA256_ADDR:
          begin
            sha256_rxd_syn  = i2c_rxd_syn;
            sha256_rxd_data = i2c_rxd_data;
            i2c_rxd_ack     = sha256_rxd_ack;
            i2c_txd_syn     = sha256_txd_syn;
            i2c_txd_data    = sha256_txd_data;
            sha256_txd_ack  = i2c_txd_ack;
          end

        I2C_SHA512_224_ADDR:
          begin
            sha512_rxd_syn  = i2c_rxd_syn;
            sha512_rxd_data = i2c_rxd_data;
            i2c_rxd_ack     = sha512_rxd_ack;
            i2c_txd_syn     = sha512_txd_syn;
            i2c_txd_data    = sha512_txd_data;
            sha512_txd_ack  = i2c_txd_ack;
            sha512_mode     = MODE_SHA_512_224;
          end

        I2C_SHA512_256_ADDR:
          begin
            sha512_rxd_syn  = i2c_rxd_syn;
            sha512_rxd_data = i2c_rxd_data;
            i2c_rxd_ack     = sha512_rxd_ack;
            i2c_txd_syn     = sha512_txd_syn;
            i2c_txd_data    = sha512_txd_data;
            sha512_txd_ack  = i2c_txd_ack;
            sha512_mode     = MODE_SHA_512_256;
          end

        I2C_SHA384_ADDR:
          begin
            sha512_rxd_syn  = i2c_rxd_syn;
            sha512_rxd_data = i2c_rxd_data;
            i2c_rxd_ack     = sha512_rxd_ack;
            i2c_txd_syn     = sha512_txd_syn;
            i2c_txd_data    = sha512_txd_data;
            sha512_txd_ack  = i2c_txd_ack;
            sha512_mode     = MODE_SHA_384;
          end

        I2C_SHA512_ADDR:
          begin
            sha512_rxd_syn  = i2c_rxd_syn;
            sha512_rxd_data = i2c_rxd_data;
            i2c_rxd_ack     = sha512_rxd_ack;
            i2c_txd_syn     = sha512_txd_syn;
            i2c_txd_data    = sha512_txd_data;
            sha512_txd_ack  = i2c_txd_ack;
            sha512_mode     = MODE_SHA_512;
          end

        default:
          begin
          end
      endcase // case (i2c_device_addr)
    end // address_mux

endmodule // coretest_hashes

//======================================================================
// EOF coretest_hashes.v
//======================================================================
