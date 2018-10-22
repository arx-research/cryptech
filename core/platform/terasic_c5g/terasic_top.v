//======================================================================
//
// terasic_top.v
// -------------
// Top module for the Cryptech FPGA framework.
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

module terasic_top
  (
   input wire 	       clk,
   input wire 	       reset_n,
  
   // External interface.
   input wire 	       rxd,
   output wire 	       txd,
  
   output wire [7 : 0] debug
   );

   //----------------------------------------------------------------
   // UART Interface
   //
   // UART subsystem handles all data transfer to/from host via serial.
   //----------------------------------------------------------------

   // UART configuration (set in uart_regs.v)
   reg [15 : 0] 	bit_rate;
   reg [3 : 0] 		data_bits;
   reg [1 : 0] 		stop_bits;

   // UART connections
   wire 		uart_rxd_syn;
   wire [7 : 0] 	uart_rxd_data;
   wire 		uart_rxd_ack;
   wire 		uart_txd_syn;
   wire [7 : 0] 	uart_txd_data;
   wire 		uart_txd_ack;

   uart_core uart_core
     (
      .clk(clk),
      .reset_n(reset_n),

      // Configuration parameters
      .bit_rate(bit_rate),
      .data_bits(data_bits),
      .stop_bits(stop_bits),

      // External data interface
      .rxd(rxd),
      .txd(txd),

      // Internal receive interface.
      .rxd_syn(uart_rxd_syn),
      .rxd_data(uart_rxd_data),
      .rxd_ack(uart_rxd_ack),
      
      // Internal transmit interface.
      .txd_syn(uart_txd_syn),
      .txd_data(uart_txd_data),
      .txd_ack(uart_txd_ack)
      );


   //----------------------------------------------------------------
   // Coretest interface
   //
   // Coretest parses the input datastream into a read/write command,
   // and marshalls the response into an output datastream.
   // ----------------------------------------------------------------

   // Coretest connections.
   wire 		coretest_reset_n;
   wire 		coretest_cs;
   wire 		coretest_we;
   wire [15 : 0] 	coretest_address;
   wire [31 : 0] 	coretest_write_data;
   wire [31 : 0] 	coretest_read_data;

   coretest coretest
     (
      .clk(clk),
      .reset_n(reset_n),
      
      // Interface to communication core
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
      .core_read_data(coretest_read_data)
      );


   //----------------------------------------------------------------
   // Core Selector
   //
   // This multiplexer is used to map different types of cores, such as
   // hashes, RNGs and ciphers to different regions (segments) of memory.
   //----------------------------------------------------------------

   core_selector cores
     (
      .sys_clk(clk),
      .sys_rst(!reset_n),

      .sys_eim_addr({coretest_address[15:13], 1'b0, coretest_address[12:0]}),
      .sys_eim_wr(coretest_cs & coretest_we),
      .sys_eim_rd(coretest_cs & ~coretest_we),

      .sys_write_data(coretest_write_data),
      .sys_read_data(coretest_read_data)
      );  


endmodule
