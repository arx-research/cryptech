//======================================================================
//
// novena_top.v
// ------------
// Top module for the Cryptech Novena FPGA framework with the I2C bus.
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

module novena_top
  (
   // Differential input for 50 MHz general clock.
   input wire 	       gclk_p_pin,
   input wire 	       gclk_n_pin,

   // Reset controlled by the CPU.
   // this must be configured as input w/pullup
   input wire 	       reset_mcu_b_pin,

   // Cryptech avalanche noise board input and LED outputs
   input wire 	       ct_noise,
   output wire [7 : 0] ct_led,

   // I2C interface
   input wire 	       i2c_scl,
   inout wire 	       i2c_sda,

   // Novena utility ports
   output wire         apoptosis_pin,   // Hold low to not restart after config.
   output wire 	       led_pin		// LED on edge close to the FPGA.
   );


   //----------------------------------------------------------------
   // Clock Manager
   //
   // Clock manager is used to generate SYS_CLK from GCLK
   // and implement the reset logic.
   // ----------------------------------------------------------------
   wire                 sys_clk;
   wire                 sys_rst_n;

   novena_clkmgr #
     (
      .CLK_OUT_MUL      (2),    // 2..32
      .CLK_OUT_DIV      (2)     // 1..32
      )
   clkmgr
     (
      .gclk_p(gclk_p_pin),
      .gclk_n(gclk_n_pin),

      .reset_mcu_b(reset_mcu_b_pin),

      .sys_clk(sys_clk),
      .sys_rst_n(sys_rst_n)
      );


   //----------------------------------------------------------------
   // I2C Interface
   //
   // I2C subsystem handles all data transfer to/from CPU via I2C bus.
   //----------------------------------------------------------------
   parameter I2C_DEVICE_ADDR    = 7'h0f;

   wire [16: 0]         sys_eim_addr;
   wire                 sys_eim_wr;
   wire                 sys_eim_rd;

   wire 		sda_pd;
   wire 		sda_int;

   wire 		clk = sys_clk;

   // Coretest connections.
   wire 		coretest_reset_n;
   wire 		coretest_cs;
   wire 		coretest_we;
   wire [15 : 0] 	coretest_address;
   wire [31 : 0] 	coretest_write_data;
   wire [31 : 0] 	coretest_read_data;
   wire 		coretest_error;

   // I2C connections
   wire [6:0] 		i2c_device_addr;
   wire 		i2c_rxd_syn;
   wire [7 : 0] 	i2c_rxd_data;
   wire 		i2c_rxd_ack;
   wire 		i2c_txd_syn;
   wire [7 : 0] 	i2c_txd_data;
   wire 		i2c_txd_ack;

   IOBUF #(.DRIVE(8), .SLEW("SLOW"))
   IOBUF_sda (
	      .IO(i2c_sda),
	      .I(1'b0),
	      .T(!sda_pd),
	      .O(sda_int)
	      );

   i2c_core i2c_core
     (
      .clk(clk),
      .reset(~sys_rst_n),       // active-high reset for this third-party module

      // External data interface
      .SCL(i2c_scl),
      .SDA(sda_int),
      .SDA_pd(sda_pd),
      .i2c_device_addr(i2c_device_addr),

      // Internal receive interface.
      .rxd_syn(i2c_rxd_syn),
      .rxd_data(i2c_rxd_data),
      .rxd_ack(i2c_rxd_ack),

      // Internal transmit interface.
      .txd_syn(i2c_txd_syn),
      .txd_data(i2c_txd_data),
      .txd_ack(i2c_txd_ack)
      );

   coretest coretest
     (
      .clk(clk),
      .reset_n(sys_rst_n),

      .rx_syn(i2c_rxd_syn),
      .rx_data(i2c_rxd_data),
      .rx_ack(i2c_rxd_ack),

      .tx_syn(i2c_txd_syn),
      .tx_data(i2c_txd_data),
      .tx_ack(i2c_txd_ack),

      // Interface to the core being tested.
      .core_reset_n(coretest_reset_n),
      .core_cs(coretest_cs),
      .core_we(coretest_we),
      .core_address(coretest_address),
      .core_write_data(coretest_write_data),
      .core_read_data(coretest_read_data),
      .core_error(coretest_error)
      );

   wire 		select = (i2c_device_addr == I2C_DEVICE_ADDR);
   assign sys_eim_addr = { coretest_address[15:13], 1'b0, coretest_address[12:0] };
   assign sys_eim_wr = select & coretest_cs & coretest_we;
   assign sys_eim_rd = select & coretest_cs & ~coretest_we;


   //----------------------------------------------------------------
   // Core Selector
   //
   // This multiplexer is used to map different types of cores, such as
   // hashes, RNGs and ciphers to different regions (segments) of memory.
   //----------------------------------------------------------------
   core_selector cores
     (
      .sys_clk(clk),
      .sys_rst_n(sys_rst_n),

      .sys_eim_addr(sys_eim_addr),
      .sys_eim_wr(sys_eim_wr),
      .sys_eim_rd(sys_eim_rd),
      .sys_write_data(coretest_write_data),
      .sys_read_data(coretest_read_data),
      .sys_error(coretest_error),

      .noise(ct_noise),
      .debug(ct_led)
      );


   //----------------------------------------------------------------
   // Novena Patch
   //
   // Patch logic to keep the Novena board happy.
   // The apoptosis_pin pin must be kept low or the whole board
   // (more exactly the CPU) will be reset after the FPGA has
   // been configured.
   //----------------------------------------------------------------
   assign apoptosis_pin = 1'b0;

   assign led_pin = 1'b1;

endmodule

//======================================================================
// EOF novena_top.v
//======================================================================
