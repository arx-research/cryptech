//======================================================================
//
// blockmem_rw32ptr_r64.v
// ----------------------
// Synchronous block memory with two separate ports. Port one is
// called the api port and port two is called the internal port.
//
// The api port contains its own address generator and the api port
// uses 32 bit data width. The address is automatically increased when
// the cs signal is set. The address is reset to zero when the rst
// signal is asserted.
//
// The second data port is called the internal port. The internal
// port uses 64 bit data width.
//
// and an internal port. The api port contains apointer
// and 32 bit data width. The internal port
// For port 1 the address is implicit and instead given by the
// internal pointer.
//
// Copyright (c) 2015, NORDUnet A/S All rights reserved.
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

module blockmem_rw32ptr_r64(
                            input wire           clk,
                            input wire           reset_n,

                            input wire           api_rst,
                            input wire           api_cs,
                            input wire           api_wr,
                            input wire  [31 : 0] api_wr_data,
                            output wire [31 : 0] api_rd_data,

                            input wire  [06 : 0] internal_addr,
                            output wire [63 : 0] internal_rd_data
                           );


  //----------------------------------------------------------------
  // Regs and memories.
  //----------------------------------------------------------------
  reg [31 : 0] mem0 [0 : 127];
  wire         mem0_we;

  reg [31 : 0] mem1 [0 : 127];
  wire         mem1_we;

  reg [7 : 0]  ptr_reg;
  reg [7 : 0]  ptr_new;
  reg          ptr_we;


  //----------------------------------------------------------------
  // Wires.
  //----------------------------------------------------------------
  reg [31 : 0] tmp0_api_rd_data;
  reg [31 : 0] tmp1_api_rd_data;
  reg [31 : 0] tmp0_int_rd_data;
  reg [31 : 0] tmp1_int_rd_data;


  //----------------------------------------------------------------
  // Assignments.
  //----------------------------------------------------------------
  assign api_rd_data      = ptr_reg[0] ? tmp1_api_rd_data : tmp0_api_rd_data;
  assign internal_rd_data = {tmp1_int_rd_data, tmp0_int_rd_data};

  assign mem0_we = api_wr & ~ptr_reg[0];
  assign mem1_we = api_wr & ptr_reg[0];


  //----------------------------------------------------------------
  // mem0_updates
  //----------------------------------------------------------------
  always @ (posedge clk)
    begin : update_mem0
      if (mem0_we)
        mem0[ptr_reg[7 : 1]] <= api_wr_data;

      tmp0_api_rd_data <= mem0[ptr_reg[7 : 1]];
      tmp0_int_rd_data <= mem0[internal_addr];
    end

  always @ (posedge clk)
    begin : update_mem1
      if (mem1_we)
        mem1[ptr_reg[7 : 1]] <= api_wr_data;

      tmp1_api_rd_data <= mem1[ptr_reg[7 : 1]];
      tmp1_int_rd_data <= mem1[internal_addr];
    end


  //----------------------------------------------------------------
  // reg_update
  //----------------------------------------------------------------
  always @ (posedge clk or negedge reset_n)
    begin : reg_update
      if (!reset_n)
        ptr_reg <= 8'h00;
      else
        if (ptr_we)
          ptr_reg <= ptr_new;
    end


  //----------------------------------------------------------------
  // ptr_logic
  //----------------------------------------------------------------
  always @*
    begin : ptr_logic
      ptr_new = 8'h00;
      ptr_we  = 1'b0;

      if (api_rst)
        begin
          ptr_new = 8'h00;
          ptr_we  = 1'b1;
        end

      if (api_cs)
        begin
          ptr_new = ptr_reg + 1'b1;
          ptr_we  = 1'b1;
        end
    end

endmodule // blockmem_rw32ptr_r64

//======================================================================
// EOF blockmem_rw32ptr_r64
//======================================================================
