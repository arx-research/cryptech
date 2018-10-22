//======================================================================
//
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

module modexps6_wrapper
  (
   input wire          clk,
   input wire          reset_n,

   input wire          cs,
   input wire          we,

   input wire [9: 0]   address,
   input wire [31: 0]  write_data,
   output wire [31: 0] read_data
   );


   //
   // Address Decoder
   //
   localparam ADDR_MSB_REGS     = 1'b0;
   localparam ADDR_MSB_CORE     = 1'b1;
   wire                address_msb = address[9];
   wire [8: 0]         address_lsb = address[8:0];


   //
   // Output Mux
   //
   wire [31: 0]        read_data_regs;
   wire [31: 0]        read_data_core;


   //
   // Registers
   //
   localparam ADDR_NAME0        = 9'h000;
   localparam ADDR_NAME1        = 9'h001;
   localparam ADDR_VERSION      = 9'h002;

   localparam ADDR_CONTROL      = 9'h008;               // {next, init}
   localparam ADDR_STATUS       = 9'h009;               // {valid, ready}
   localparam ADDR_MODE         = 9'h010;               // 0 = slow secure, 1 = fast unsafe (public)
   localparam ADDR_MODULUS_BITS = 9'h011;               //
   localparam ADDR_EXPONENT_BITS = 9'h012;              //
   localparam ADDR_GPIO_REG     = 9'h020;               //

   localparam CONTROL_INIT_BIT  = 0;
   localparam CONTROL_NEXT_BIT  = 1;

   localparam STATUS_READY_BIT  = 0;
   localparam STATUS_VALID_BIT  = 1;

   localparam CORE_NAME0        = 32'h6D6F6465; // "mode"
   localparam CORE_NAME1        = 32'h78707336; // "xps6"
   localparam CORE_VERSION      = 32'h302E3130; // "0.10"


   //
   // Registers
   //
   reg [1: 0]          reg_control;
   reg                 reg_mode;
   reg [12: 0]         reg_modulus_width;
   reg [12: 0]         reg_exponent_width;
   reg [31: 0]         reg_gpio;


   //
   // Wires
   //
   wire [1: 0]         reg_status;


   //
   // ModExpS6
   //
   modexps6_top #
     (
      .MAX_MODULUS_WIDTH        (4096)
      )
   modexps6_core
     (
      .clk                      (clk),

      .init                     (reg_control[CONTROL_INIT_BIT]),
      .ready                    (reg_status[STATUS_READY_BIT]),
      .next                     (reg_control[CONTROL_NEXT_BIT]),
      .valid                    (reg_status[STATUS_VALID_BIT]),

      .modulus_width            (reg_modulus_width),
      .exponent_width           (reg_exponent_width),

      .fast_public_mode         (reg_mode),

      .bus_cs                   (cs && (address_msb == ADDR_MSB_CORE)),
      .bus_we                   (we),
      .bus_addr                 (address_lsb),
      .bus_data_wr              (write_data),
      .bus_data_rd              (read_data_core)
      );


   //
   // Read Latch
   //
   reg [31: 0]         tmp_read_data;


   //
   // Read/Write Interface
   //
   always @(posedge clk)
     //
     if (!reset_n) begin
        //
        reg_control             <= 2'b00;
        reg_mode                <= 1'b0;
        reg_modulus_width       <= 13'd1024;
        reg_exponent_width      <= 13'd1024;
        //
     end else if (cs && (address_msb == ADDR_MSB_REGS)) begin
        //
        if (we) begin
           //
           // Write Handler
           //
           case (address_lsb)
             //
             ADDR_CONTROL:      reg_control             <= write_data[1: 0];
             ADDR_MODE:         reg_mode                <= write_data[0];
             ADDR_MODULUS_BITS: reg_modulus_width       <= write_data[12: 0];
             ADDR_EXPONENT_BITS: reg_exponent_width     <= write_data[12: 0];
             ADDR_GPIO_REG:     reg_gpio                <= write_data;
             //
           endcase
           //
        end else begin
           //
           // Read Handler
           //
           case (address)
             //
             ADDR_NAME0:        tmp_read_data <= CORE_NAME0;
             ADDR_NAME1:        tmp_read_data <= CORE_NAME1;
             ADDR_VERSION:      tmp_read_data <= CORE_VERSION;
             ADDR_CONTROL:      tmp_read_data <= {{30{1'b0}}, reg_control};
             ADDR_STATUS:       tmp_read_data <= {{30{1'b0}}, reg_status};
             ADDR_MODE:         tmp_read_data <= {{31{1'b0}}, reg_mode};
             ADDR_MODULUS_BITS: tmp_read_data <= {{19{1'b0}}, reg_modulus_width};
             ADDR_EXPONENT_BITS: tmp_read_data <= {{19{1'b0}}, reg_exponent_width};
             ADDR_GPIO_REG:     tmp_read_data <= reg_gpio;
             //
             default:           tmp_read_data <= 32'h00000000;
             //
           endcase
           //
        end
        //
     end


   //
   // Register / Core Memory Selector
   //
   reg address_msb_last;
   always @(posedge clk) address_msb_last = address_msb;

   reg [31: 0] read_data_mux;
   assign read_data = read_data_mux;

   always @(*)
     //
     case (address_msb_last)
       //
       ADDR_MSB_REGS:           read_data_mux = tmp_read_data;
       ADDR_MSB_CORE:           read_data_mux = read_data_core;
       //
     endcase


endmodule
