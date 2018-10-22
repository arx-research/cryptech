//======================================================================
//
// core_selector.v
// ---------------
// Top level wrapper that creates the Cryptech coretest system.
// The wrapper contains instances of external interface, coretest
// and the core to be tested. And if more than one core is
// present the wrapper also includes address and data muxes.
//
//
// Authors: Joachim Strombergson, Paul Selkirk, Pavel Shatov
// Copyright (c) 2014-2015, NORDUnet A/S All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
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

module core_selector
  (
   input wire           sys_clk,
   input wire           sys_rst,
   input wire           sys_ena,

   input wire [13 : 0]  sys_eim_addr,
   input wire           sys_eim_wr,
   input wire           sys_eim_rd,
   output wire [31 : 0] sys_read_data,
   input wire [31 : 0]  sys_write_data
   );

   
   /* In this memory segment (HASHES) we have 14 address bits. Every core has
    * 8-bit internal address space, so we can have up to 2^(14-8) = 64 cores here.
    *
    * Core #0 is not an actual HASH core, but a set of board-level (global)
    * registers, that can be used to get information about hardware (board
    * type, bitstream version and so on).
    *
    * So far we have three cores: SHA-1, SHA-256 and SHA-512.
    */
   
   /*********************************************************
    * To add new HASH core named XXX follow the steps below *
    *********************************************************
    *
    * 1. Add corresponding `define under "List of Available Cores", this will
    *    allow users to exclude your core from implementation to save some
    *    slices in case they don't need it.
    *
    *    `define    USE_CORE_XXX
    *
    *
    * 2. Choose address of your new core and add corresponding line under
    *    "Core Address Table". Core addresses can be in the range from 1 to 63
    *    inclusively. Core address 0 is reserved for a page of global
    *    registers and must not be used.
    *
    *    localparam CORE_ADDR_XXX   = 6'dN;
    *
    *
    * 3. Add instantiation of your new core after all existing cores
    *    surrounded by conditional synthesis directives.
    *    You also need a 32-bit output (read data) bus for your core and an
    *    enable flag. Note that sys_rst in an active-high sync reset signal.
    *
    *   `ifdef USE_CORE_XXX
    *       wire [31: 0]    read_data_xxx;
    *       wire            enable_xxx = sys_ena && (addr_core_num == CORE_ADDR_XXX);
    *       xxx xxx_inst
    *       (
    *       .clk(sys_clk),
    *       .reset_n(~sys_rst),
    *       .cs(enable_xxx & (sys_eim_rd | sys_eim_wr)),
    *       .we(sys_eim_wr),
    *       .address(addr_core_reg),
    *       .write_data(sys_write_data),
    *       .read_data(read_data_xxx),
    *       .error()
    *       );
    *    `endif
    *
    *
    * 4. Add previously created data bus to "Output (Read Data) Multiplexor"
    *    in the end of this file.
    *
    *   `ifdef USE_CORE_XXX
    *       CORE_ADDR_XXX:
    *           sys_read_data_mux = read_data_xxx;
    *   `endif
    *
    */


   //----------------------------------------------------------------
   // Address Decoder
   //----------------------------------------------------------------
   wire [ 5: 0]         addr_core_num   = sys_eim_addr[13: 8];  // upper 6 bits specify core being addressed
   wire [ 7: 0]         addr_core_reg   = sys_eim_addr[ 7: 0];  // lower 8 bits specify register offset in core


   /* We can comment following lines to exclude cores from implementation
    * in case we run out of slices.
    */
   
   //----------------------------------------------------------------
   // List of Available Cores
   //----------------------------------------------------------------
   `define  USE_CORE_SHA1
   `define  USE_CORE_SHA256
   `define  USE_CORE_SHA512
   
   
   //----------------------------------------------------------------
   // Core Address Table
   //----------------------------------------------------------------
   localparam   CORE_ADDR_GLOBAL_REGS   = 6'd0;
   localparam   CORE_ADDR_SHA1          = 6'd1;
   localparam   CORE_ADDR_SHA256        = 6'd2;
   localparam   CORE_ADDR_SHA512        = 6'd3;
   
   
   //----------------------------------------------------------------
   // Global Registers
   //----------------------------------------------------------------
   wire [31: 0]         read_data_global;
   wire                 enable_global = sys_ena && (addr_core_num == CORE_ADDR_GLOBAL_REGS);
   novena_regs novena_regs_inst
     (
      .clk(sys_clk),
      .rst(sys_rst),

      .cs(enable_global & (sys_eim_rd | sys_eim_wr)),
      .we(sys_eim_wr),

      .address(addr_core_reg),
      .write_data(sys_write_data),
      .read_data(read_data_global)
      );
   
   
   //----------------------------------------------------------------
   // SHA-1
   //----------------------------------------------------------------
   `ifdef USE_CORE_SHA1
   wire [31: 0]         read_data_sha1;
   wire                 enable_sha1 = sys_ena && (addr_core_num == CORE_ADDR_SHA1);
   sha1 sha1_inst
     (
      .clk(sys_clk),
      .reset_n(~sys_rst),

      .cs(enable_sha1 & (sys_eim_rd | sys_eim_wr)),
      .we(sys_eim_wr),

      .address(addr_core_reg),
      .write_data(sys_write_data),
      .read_data(read_data_sha1)
      );
   `endif
   
   
   //----------------------------------------------------------------
   // SHA-256
   //----------------------------------------------------------------
   `ifdef USE_CORE_SHA256
   wire [31: 0]         read_data_sha256;
   wire                 enable_sha256 = sys_ena && (addr_core_num == CORE_ADDR_SHA256);
   sha256 sha256_inst
     (
      .clk(sys_clk),
      .reset_n(~sys_rst),

      .cs(enable_sha256 & (sys_eim_rd | sys_eim_wr)),
      .we(sys_eim_wr),

      .address(addr_core_reg),
      .write_data(sys_write_data),
      .read_data(read_data_sha256)
      );
   `endif
   
   
   //----------------------------------------------------------------
   // SHA-512
   //----------------------------------------------------------------
   `ifdef USE_CORE_SHA512
   wire [31: 0]         read_data_sha512;
   wire                 enable_sha512 = sys_ena && (addr_core_num == CORE_ADDR_SHA512);
   sha512 sha512_inst
     (
      .clk(sys_clk),
      .reset_n(~sys_rst),

      .cs(enable_sha512 & (sys_eim_rd | sys_eim_wr)),
      .we(sys_eim_wr),

      .address(addr_core_reg),
      .write_data(sys_write_data),
      .read_data(read_data_sha512)
      );
   `endif
   
   
   //----------------------------------------------------------------
   // Output (Read Data) Multiplexor
   //----------------------------------------------------------------
   reg [31: 0]          sys_read_data_mux;
   assign sys_read_data = sys_read_data_mux;
   
   always @*
     //
     case (addr_core_num)
       //
       CORE_ADDR_GLOBAL_REGS:
         sys_read_data_mux = read_data_global;
   `ifdef USE_CORE_SHA1
       CORE_ADDR_SHA1:
         sys_read_data_mux = read_data_sha1;
   `endif
   `ifdef USE_CORE_SHA256
       CORE_ADDR_SHA256:
         sys_read_data_mux = read_data_sha256;
   `endif
   `ifdef USE_CORE_SHA512
       CORE_ADDR_SHA512:
         sys_read_data_mux = read_data_sha512;
   `endif
       //
       default:
         sys_read_data_mux = {32{1'b0}};
       //
     endcase


endmodule

//======================================================================
// EOF core_selector.v
//======================================================================
