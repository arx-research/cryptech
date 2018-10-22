//======================================================================
//
// coretest_hashes.v
// -----------------
// Top level wrapper that creates the Cryptech coretest system.
// The wrapper contains instances of external interface, coretest
// and the core to be tested. And if more than one core is
// present the wrapper also includes address and data muxes.
//
//
// Author: Pavel Shatov
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

module eim_memory
  (
   input wire          sys_clk,
   input wire          sys_rst,

   input wire [16: 0]  sys_eim_addr,
   input wire          sys_eim_wr,
   input wire          sys_eim_rd,
   output wire [31: 0] sys_read_data,
   input wire [31: 0]  sys_write_data
   );
   
   
   /* Three upper bits of address [16:14] are used to select memory segment.
    * There can be eight segments. So far segment 0 is used for hashes,
    * segment 1 is reserved for random number generators, segment 2 is reserved
    * for chiphers. Other segments are not used so far.
    */
   
   /* Every segment has its own memory map, take at look at corresponding
    * selectors for more information.
    */
   
   //----------------------------------------------------------------
   // Segment Decoder
   //----------------------------------------------------------------
   localparam   SEGMENT_ADDR_HASHES     = 3'd0;
   localparam   SEGMENT_ADDR_RNGS       = 3'd1;
   localparam   SEGMENT_ADDR_CIPHERS    = 3'd2;

   wire [ 2: 0] addr_segment            = sys_eim_addr[16:14];  //  3 upper bits are decoded here
   wire [13: 0] addr_segment_int        = sys_eim_addr[13: 0];  // 14 lower bits are decoded individually
   // in corresponding segment selectors
   
   wire [31: 0] segment_hashes_read_data;               // data read from HASHES segment
   wire [31: 0] segment_rngs_read_data;                 // data read from RNGS segment
   wire [31: 0] segment_ciphers_read_data;              // data read from CIPHERS segment
   
   wire         segment_enable_hashes   = (addr_segment == SEGMENT_ADDR_HASHES)  ? 1'b1 : 1'b0; // HASHES segment is being addressed
   wire         segment_enable_rngs     = (addr_segment == SEGMENT_ADDR_RNGS)    ? 1'b1 : 1'b0; // RNGS segment is being addressed
   wire         segment_enable_ciphers  = (addr_segment == SEGMENT_ADDR_CIPHERS) ? 1'b1 : 1'b0; // CIPHERS segment is being addressed
   
   
   //----------------------------------------------------------------
   // Output (Read Data) Bus
   //----------------------------------------------------------------
   reg [31: 0]  sys_read_data_reg;
   assign sys_read_data = sys_read_data_reg;
   
   always @*
     //
     case (addr_segment)
       SEGMENT_ADDR_HASHES:     sys_read_data_reg = segment_hashes_read_data;
       SEGMENT_ADDR_RNGS:       sys_read_data_reg = segment_rngs_read_data;
       SEGMENT_ADDR_CIPHERS:    sys_read_data_reg = segment_ciphers_read_data;
       default:                 sys_read_data_reg = {32{1'b0}};
     endcase
   
   
   
   //----------------------------------------------------------------
   // HASH Core Selector
   //
   // This selector is used to map core registers into
   // EIM address space and select which core to send EIM read and
   // write operations to.
   //----------------------------------------------------------------
   core_selector segment_cores
     (
      .sys_clk(sys_clk),
      .sys_rst(sys_rst),

      .sys_ena(segment_enable_hashes),          // only enable active selector

      .sys_eim_addr(addr_segment_int),          // we only connect 14 lower bits of address here,
      // because we have already decoded 3 upper bits earlier,
      // every segment can have its own address decoder.
      .sys_eim_wr(sys_eim_wr),
      .sys_eim_rd(sys_eim_rd),                                          

      .sys_write_data(sys_write_data),
      .sys_read_data(segment_hashes_read_data)  // output from HASHES segment
      );
   
   
   //----------------------------------------------------------------
   // RNG Selector
   //
   // This selector is used to map random number generator registers into
   // EIM address space and select which RNG to send EIM read and
   // write operations to. So far there are no RNG cores.
   //----------------------------------------------------------------
   rng_selector segment_rngs
     (
      .sys_clk(sys_clk),
      .sys_rst(sys_rst),

      .sys_ena(segment_enable_rngs),            // only enable active selector

      .sys_eim_addr(addr_segment_int),          // we only connect 14 lower bits of address here,
      // because we have already decoded 3 upper bits earlier,
      // every segment can have its own address decoder.
      .sys_eim_wr(sys_eim_wr),
      .sys_eim_rd(sys_eim_rd),          

      .sys_write_data(sys_write_data),
      .sys_read_data(segment_rngs_read_data)    // output from RNGS segment
      );
   
   
   //----------------------------------------------------------------
   // CIPHER Selector
   //
   // This selector is used to map cipher registers into
   // EIM address space and select which CIPHER to send EIM read and
   // write operations to. So far there are no CIPHER cores.
   //----------------------------------------------------------------
   cipher_selector segment_ciphers
     (
      .sys_clk(sys_clk),
      .sys_rst(sys_rst),

      .sys_ena(segment_enable_ciphers),         // only enable active selector

      .sys_eim_addr(addr_segment_int),          // we only connect 14 lower bits of address here,
      // because we have already decoded 3 upper bits earlier,
      // every segment can have its own address decoder.
      .sys_eim_wr(sys_eim_wr),
      .sys_eim_rd(sys_eim_rd),          

      .sys_write_data(sys_write_data),
      .sys_read_data(segment_ciphers_read_data) // output from CIPHERS segment
      );
   
   
endmodule


//======================================================================
// EOF eim_memory.v
//======================================================================
