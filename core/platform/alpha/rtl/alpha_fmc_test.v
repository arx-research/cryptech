//======================================================================
//
// alpha_test.v
// ------------
// Top module for the Cryptech Alpha FPGA framework. This design
// allow us to run the FMC interface at one clock and cores including
// core selector with the always present global clock.
//
//
// Author: Pavel Shatov
// Copyright (c) 2016, NORDUnet A/S All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// - Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
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

`timescale 1ns / 1ps

module alpha_fmc_top
  (
   input wire 	      gclk_pin, // 50 MHz

   input wire 	      ct_noise, // cryptech avalanche noise circuit

   input wire 	      fmc_clk, // clock
   input wire [23: 0] fmc_a, // address
   inout wire [31: 0] fmc_d, // data
   input wire 	      fmc_ne1, // chip select
   input wire 	      fmc_noe, // output enable
   input wire 	      fmc_nwe, // write enable
   input wire 	      fmc_nl, // latch enable
   output wire 	      fmc_nwait,// wait

   output wire 	      mkm_sclk,
   output wire 	      mkm_cs_n,
   input wire 	      mkm_do,
   output wire 	      mkm_di,

   output wire [3: 0] led_pins  // {red, yellow, green, blue}
   );


   //----------------------------------------------------------------
   // Clock Manager
   //
   // Clock manager is used to generate SYS_CLK from GCLK
   // and implement the reset logic.
   // ----------------------------------------------------------------
   wire               sys_clk;
   wire               sys_rst_n;

   alpha_clkmgr #
     (
      .CLK_OUT_MUL      (20.0),    // 2..64
      .CLK_OUT_DIV      (20.0)     // 1..128
      )
   clkmgr
     (
      .gclk      (gclk_pin),
      
      .sys_clk   (sys_clk),
      .sys_rst_n (sys_rst_n)
      );


   //----------------------------------------------------------------
   // BUFG
   //
   // FMC clock must be routed through the global clocking backbone.
   // ----------------------------------------------------------------
   wire               fmc_clk_bug;

   BUFG BUFG_fmc_clk
     (
      .I                (fmc_clk),
      .O                (fmc_clk_bufg)
      );



   //----------------------------------------------------------------
   // FMC Arbiter
   //
   // FMC arbiter handles FMC access and transfers it into
   // `sys_clk' clock domain.
   //----------------------------------------------------------------

   wire [23: 0]       sys_fmc_addr;     // address
   wire               sys_fmc_wren;     // write enable
   wire               sys_fmc_rden;     // read enable
   wire [31: 0]       sys_fmc_dout;     // data output (from STM32 to FPGA)
   reg [31: 0] 	      sys_fmc_din;      // data input (from FPGA to STM32)

   fmc_arbiter #
     (
      .NUM_ADDR_BITS(24)        // change to 26 when Alpha is alive!
      )
   fmc
     (
      .fmc_clk(fmc_clk_bufg),
      .fmc_a(fmc_a),
      .fmc_d(fmc_d),
      .fmc_ne1(fmc_ne1),
      .fmc_nl(fmc_nl),
      .fmc_nwe(fmc_nwe),
      .fmc_noe(fmc_noe),
      .fmc_nwait(fmc_nwait),

      .sys_clk(sys_clk),

      .sys_addr(sys_fmc_addr),
      .sys_wr_en(sys_fmc_wren),
      .sys_rd_en(sys_fmc_rden),
      .sys_data_out(sys_fmc_dout),
      .sys_data_in(sys_fmc_din)
      );


   //----------------------------------------------------------------
   // LED Driver
   //
   // A simple utility LED driver that turns on the Alpha
   // board LED when the FMC interface is active.
   //----------------------------------------------------------------
   fmc_indicator led
     (
      .sys_clk(sys_clk),
      .sys_rst_n(sys_rst_n),
      .fmc_active(sys_fmc_wren | sys_fmc_rden),
      .led_out(led_pins[0])
      );


   //----------------------------------------------------------------
   // Dummy Register
   //
   // General-purpose register to test FMC interface using STM32
   // demo program instead of core selector logic.
   //
   // This register is a bit tricky, but it allows testing of both
   // data and address buses. Reading from FPGA will always return
   // value, which is currently stored in the test register, 
   // regardless of read transaction address. Writing to FPGA has
   // two variants: a) writing to address 0 will store output data
   // data value in the test register, b) writing to any non-zero
   // address will store _address_ of write transaction in the test
   // register.
   //
   // To test data bus, write some different patterns to address 0,
   // then readback from any address and compare.
   //
   // To test address bus, write anything to some different non-zero
   // addresses, then readback from any address and compare returned
   // value with previously written address.
   //
   //----------------------------------------------------------------
   reg [31: 0] 	      test_reg;
   
   
   
   //
   // Noise Capture Register
   //
   reg [31: 0] 	      noise_reg;
   
   always @(posedge sys_clk)
     //
     noise_reg <= {noise_reg[30:0], ct_noise};
   
   
   
   always @(posedge sys_clk)
     //
     if (sys_fmc_wren) begin
	//
	// when writing to address 0, store input data value
	//
	// when writing to non-zero address, store _address_
	// (padded with zeroes) instead of data
	//
	test_reg <= (sys_fmc_addr == {24{1'b0}}) ? sys_fmc_dout : {{8{1'b0}}, sys_fmc_addr};
	//
     end else if (sys_fmc_rden) begin
	//
	// always return current value, ignore address
	//
	sys_fmc_din <= (sys_fmc_addr == {24{1'b1}}) ? noise_reg : test_reg;

	// when reading from address 0, return the current value
	// when reading from other addresses, return the address
	//sys_fmc_din <= (sys_fmc_addr == {22{1'b0}}) ? test_reg : {{10{1'b0}}, sys_fmc_addr};
	//
     end


   //
   // Dummy assignments to bypass unconnected outpins pins check in BitGen
   //
   
   assign led_pins[3:1] = 3'b000;


endmodule
