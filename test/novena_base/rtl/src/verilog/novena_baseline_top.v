//======================================================================
//
// novena_baseline_top.v
// ---------------------
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

module novena_baseline_top
  (
   // Differential input for 50 MHz general clock.
   input wire          gclk_p_pin,
   input wire          gclk_n_pin,

   // Reset controlled by the CPU.
   // this must be configured as input w/pullup
   input wire          reset_mcu_b_pin,

   // Cryptech avalanche noise board input and LED outputs
   input wire          ct_noise,
   output wire [7 : 0] ct_led,

   // EIM interface
   input wire          eim_bclk, // EIM burst clock. Started by the CPU.
   input wire          eim_cs0_n, // Chip select (active low).
   inout wire [15 : 0] eim_da, // Bidirectional address and data port.
   input wire [18: 16] eim_a, // MSB part of address port.                     
   input wire          eim_lba_n, // Latch address signal (active low).
   input wire          eim_wr_n, // write enable signal (active low).
   input wire          eim_oe_n, // output enable signal (active low).
   output wire         eim_wait_n, // Data wait signal (active low).
                       
                       // Novena utility ports
                       apoptosis_pin, // Hold low to not restart after config.
                       led_pin                      // LED on edge close to the FPGA.
   );


   //----------------------------------------------------------------
   // Clock Manager
   //
   // Clock manager is used to buffer BCLK, generate SYS_CLK
   // from GCLK and implement the reset logic.
   //----------------------------------------------------------------
   wire                 sys_clk;
   wire                 sys_rst;
   wire                 eim_bclk_buf;

   novena_clkmgr clkmgr
     (
      .gclk_p(gclk_p_pin),
      .gclk_n(gclk_n_pin),

      .reset_mcu_b(reset_mcu_b_pin),

      .sys_clk(sys_clk),
      .sys_rst(sys_rst),

      .bclk_in(eim_bclk),
      .bclk_out(eim_bclk_buf)
      );


   //----------------------------------------------------------------
   // EIM Arbiter
   //
   // EIM arbiter handles EIM access and transfers it into
   // `sys_clk' clock domain.
   //----------------------------------------------------------------
   wire [16: 0]         sys_eim_addr;
   wire                 sys_eim_wr;
   wire                 sys_eim_rd;
   wire [31: 0]         sys_eim_dout;
   wire [31: 0]         sys_eim_din;

   eim_arbiter eim
     (
      .eim_bclk(eim_bclk_buf),
      .eim_cs0_n(eim_cs0_n),
      .eim_da(eim_da),
      .eim_a(eim_a),
      .eim_lba_n(eim_lba_n),
      .eim_wr_n(eim_wr_n),
      .eim_oe_n(eim_oe_n),
      .eim_wait_n(eim_wait_n),

      .sys_clk(sys_clk),

      .sys_addr(sys_eim_addr),
      .sys_wren(sys_eim_wr),
      .sys_data_out(sys_eim_dout),
      .sys_rden(sys_eim_rd),
      .sys_data_in(sys_eim_din)
      );


   //----------------------------------------------------------------
   // Memory Mapper
   //
   // This multiplexer is used to map different types of cores, such as
   // hashes, RNGs and ciphers to different regions (segments) of memory.
   //----------------------------------------------------------------
   eim_memory mem
     (
      .sys_clk(sys_clk),
      .sys_rst(sys_rst),

      .sys_eim_addr(sys_eim_addr),
      .sys_eim_wr(sys_eim_wr),
      .sys_eim_rd(sys_eim_rd),

      .sys_write_data(sys_eim_dout),
      .sys_read_data(sys_eim_din)
      );  


   //----------------------------------------------------------------
   // LED Driver
   //
   // A simple utility LED driver that turns on the Novena
   // board LED when the EIM interface is active.
   //----------------------------------------------------------------
   eim_indicator led
     (
      .sys_clk(sys_clk),
      .sys_rst(sys_rst),
      .eim_active(sys_eim_wr | sys_eim_rd),
      .led_out(led_pin)
      );


   //----------------------------------------------------------------
   // Cryptech Logic
   //
   // Logic specific to the Cryptech use of the Novena.
   // Currently we just hard wire the LED outputs.
   //----------------------------------------------------------------
   assign ct_led = {8{ct_noise}};


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

//======================================================================
// EOF novena_baseline_top.v
//======================================================================
