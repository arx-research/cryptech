//======================================================================
//
// novena_top.v
// ------------
// Top module for the Cryptech Novena FPGA framework. This design
// allow us to run the EIM interface at one clock and cores including
// core selector with the always present global clock.
//
//
// Author: Pavel Shatov
// Copyright (c) 2015, NORDUnet A/S All rights reserved.
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

module novena_fmc_top
  (
   input wire         gclk_p_pin,
   input wire         gclk_n_pin,

   input wire         reset_mcu_b_pin,

   // Cryptech avalanche noise board input
   input wire         ct_noise,

   input wire         fmc_clk,  // clock
   input wire [21: 0] fmc_a,    // address
   inout wire [31: 0] fmc_d,    // data
   input wire         fmc_ne1,  // chip select
   input wire         fmc_noe,  // output enable
   input wire         fmc_nwe,  // write enable
   input wire         fmc_nl,   // latch enable
   output wire        fmc_nwait,// wait

   output wire        apoptosis_pin,
   output wire        led_pin
   );


   //----------------------------------------------------------------
   // Clock Manager
   //
   // Clock manager is used to generate SYS_CLK from GCLK
   // and implement the reset logic.
   // ----------------------------------------------------------------
   wire               sys_clk;
   wire               sys_rst_n;

   novena_clkmgr #
     (
      .CLK_OUT_MUL      (2),    // 2..32
      .CLK_OUT_DIV      (2)     // 1..32
      )
   clkmgr
     (
      .gclk_p           (gclk_p_pin),
      .gclk_n           (gclk_n_pin),

      .reset_mcu_b      (reset_mcu_b_pin),

      .sys_clk          (sys_clk),
      .sys_rst_n        (sys_rst_n)
      );


   //
   // BUFG
   //
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

   wire [21: 0]       sys_fmc_addr;     // address
   wire               sys_fmc_wren;     // write enable
   wire               sys_fmc_rden;     // read enable
   wire [31: 0]       sys_fmc_dout;     // data output (from STM32 to FPGA)
`ifdef test
   reg [31: 0]        sys_fmc_din;      // data input (from FPGA to STM32)
`else
   wire [31: 0]       sys_fmc_din;      // data input (from FPGA to STM32)
`endif

   fmc_arbiter #
     (
      .NUM_ADDR_BITS(22)        // change to 26 when
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
   // A simple utility LED driver that turns on the Novena
   // board LED when the FMC interface is active.
   //----------------------------------------------------------------
   fmc_indicator led
     (
      .sys_clk(sys_clk),
      .sys_rst_n(sys_rst_n),
      .fmc_active(sys_fmc_wren | sys_fmc_rden),
      .led_out(led_pin)
      );


`ifdef test
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
   
   always @(posedge sys_clk)
     //
     if (sys_fmc_wren) begin
	//
	// when writing to address 0, store input data value
	//
	// when writing to non-zero address, store _address_
	// (padded with zeroes) instead of data
	//
	test_reg <= (sys_fmc_addr == {22{1'b0}}) ? sys_fmc_dout : {{10{1'b0}}, sys_fmc_addr};
	//
     end else if (sys_fmc_rden) begin
	//
	// always return current value, ignore address
	//
	sys_fmc_din <= test_reg;

	// when reading from address 0, return the current value
	// when reading from other addresses, return the address
	//sys_fmc_din <= (sys_fmc_addr == {22{1'b0}}) ? test_reg : {{10{1'b0}}, sys_fmc_addr};
	//
     end

`else // !`ifdef test
   //----------------------------------------------------------------
   // Core Selector
   //
   // This multiplexer is used to map different types of cores, such as
   // hashes, RNGs and ciphers to different regions (segments) of memory.
   //----------------------------------------------------------------

   core_selector cores
     (
      .sys_clk(sys_clk),
      .sys_rst_n(sys_rst_n),

      .sys_eim_addr(sys_fmc_addr[16:0]),	// XXX parameterize
      .sys_eim_wr(sys_fmc_wren),
      .sys_eim_rd(sys_fmc_rden),
      .sys_write_data(sys_fmc_dout),
      .sys_read_data(sys_fmc_din),

      .noise(ct_noise)
      );  
`endif
   

   //----------------------------------------------------------------
   // Novena Patch
   //
   // Patch logic to keep the Novena board happy.
   // The apoptosis_pin pin must be kept low or the whole board
   // (more exactly the CPU) will be reset after the FPGA has
   // been configured.
   //----------------------------------------------------------------
   assign apoptosis_pin = 1'b0;


endmodule
