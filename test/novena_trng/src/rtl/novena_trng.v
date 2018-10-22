//======================================================================
//
// novena_trng.v
// -------------
// Cryptech TRNG for the Novena platform.
// This top level file is based on the novena_fpga design by
// Andrew "bunnie" Huang, see below.
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

// Copyright (c) 2013, Andrew "bunnie" Huang
//
// See the NOTICE file distributed with this work for additional
// information regarding copyright ownership.  The copyright holder
// licenses this file to you under the Apache License, Version 2.0
// (the "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// code distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//////////////////////////////////////////////////////////////////////////////

/// note: must set "-g UnusedPin:Pullnone" to avoid conflicts with unused pins

`timescale 1ns / 1ps

module novena_trng(
	           // CPU side mapping
	           input wire [15:0] EIM_DA,
	           output reg EIM_A16,  // relay of the trigger output
	           output reg EIM_A17,  // relay of the trigger data (read path)

	           // connector side mapping
	           //input wire F_LVDS_N3, // output of trigger
	           //input wire F_DX2,       // output of trigger
	           //output wire F_LVDS_N5, // trigger reset
	           output wire F_LVDS_P4,   // trigger reset
	           //inout wire F_LVDS_P5, // trigger data (bidir)
	           //input wire F_DX18,      // trigger data in from sticker (DUT->CPU)
	           output wire F_LVDS_P11, // trigger data out to sticker (CPU->DUT)
	           //output wire F_LVDS_N8, // trigger clock
	           //output wire F_DX14,      // trigger clock

	           output wire F_LVDS_N7, // drive TPI data line
	           output wire F_LVDS_P7, // drive TPI signal lines

	           output wire F_DX15,  // 1 = drive 5V, 0 = drive 3V to DUT

	           output wire F_LVDS_CK1_N,
	           output wire F_LVDS_CK1_P,
	           output wire F_LVDS_N11,

	           output wire F_LVDS_N0,
	           output wire F_LVDS_P0,
	           // output wire F_DX1,

	           output wire F_LVDS_N15,
	           output wire F_LVDS_P15,
	           output wire F_LVDS_NC,

	           //input wire F_DX11,

                   // Noise input ports.
                   // DX3 is for rev01 of the noise board
                   // DX7 is used for rev02 of the noise board
                   // DX15 is used fpr rev02 as non-schmitt triggered source
	           // input wire F_DX3,
                   input wire F_DX7,

                   // Cryptech noise board v2 LEDs 00..07
                   // are connected to these pins.
	           output wire F_DX0,
	           output wire F_DX3,
	           output wire F_DX2,
	           output wire F_DX11,
	           output wire F_DX1,
	           output wire F_DX17,
	           output wire F_DX14,
	           output wire F_DX18,

	           input wire CLK2_N,
	           input wire CLK2_P,
	           output wire FPGA_LED2,

	           input wire I2C3_SCL,
	           inout wire I2C3_SDA,

	           input wire RESETBMCU,
	           // output wire F_DX17,  // dummy
	           output wire APOPTOSIS
                  );


  //----------------------------------------------------------------
  // Wires.
  //----------------------------------------------------------------
  wire clk;

  wire [7 : 0] test_debug;

  IBUFGDS clkibufgds(
		     .I(CLK2_P),
		     .IB(CLK2_N),
		     .O(clk)
	             );

  //----------------------------------------------------------------
  // Assignments.
  //----------------------------------------------------------------
  assign FPGA_LED2 = test_debug[0];

  // Assign LEDs to debug
  assign F_DX0  = test_debug[0];
  assign F_DX3  = test_debug[1];
  assign F_DX2  = test_debug[2];
  assign F_DX11 = test_debug[3];
  assign F_DX1  = test_debug[4];
  assign F_DX17 = test_debug[5];
  assign F_DX14 = test_debug[6];
  assign F_DX18 = test_debug[7];

  assign APOPTOSIS = 1'b0;
  assign F_DX15 = 1'b1; //+5V P_DUT

  // OE on bank to drive signals; signal not inverted in software
  assign F_LVDS_P7 = !EIM_DA[3];
  // OE on bank to drive the data; signal not inverted in software
  assign F_LVDS_N7 = !EIM_DA[4];
  assign F_LVDS_P4 = 1'b0;
  assign F_LVDS_P11 = 1'b0;
  assign F_LVDS_CK1_N = 1'b0;
  assign F_LVDS_CK1_P = 1'b0;
  assign F_LVDS_N11 = 1'b0;
  assign F_LVDS_N0 = 1'b0;
  assign F_LVDS_P0 = 1'b0;
  // assign F_DX1 = 1'b0;
  assign F_LVDS_N15 = 1'b0;
  assign F_LVDS_P15 = 1'b0;
  assign F_LVDS_NC = 1'b0;


  ////////////////////////////////////
  ///// I2C register set
  ////////////////////////////////////
  wire       SDA_pd;
  wire       SDA_int;
  reg        clk25;
  reg        reset_n;

  initial begin
    clk25   <= 1'b0;
    reset_n <= 1'b0;
  end

  always @ (posedge clk) begin
    clk25 <= ~clk25;
    reset_n <= 1'b1;
    EIM_A16 <= 1'b0;
    EIM_A17 <= 1'b0;
  end

  IOBUF #(
	  .DRIVE(8),
	  .SLEW("SLOW")
	  ) IOBUF_sda (
		       .IO(I2C3_SDA),
		       .I(1'b0),
		       .T(!SDA_pd),
		       .O(SDA_int)
	               );

   coretest_trng coretest_trng_inst(
		                    .clk(clk),
		                    .reset_n(reset_n),

                                    .noise(F_DX7),
                                    .debug(test_debug),
		                    .SCL(I2C3_SCL),
		                    .SDA(SDA_int),
		                    .SDA_pd(SDA_pd)
		                   );

endmodule // novena_trng

//======================================================================
// EOF novena_trng.v
//======================================================================
