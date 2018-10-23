//======================================================================
//
// mkmif.v
// -------
// Top level wrapper for the Master Key Memory (MKM) interface.
// The interface is implemented to use the Microchip 23K640 serial
// sram as external storage. The core acts as a SPI Master for the
// external memory including SPI clock generation.
//
// The current version of the core does not provide any functionality
// to protect against remanence.
//
//
// Author: Joachim Strombergson
// Copyright (c) 2016, NORDUnet A/S All rights reserved.
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

module mkmif(
             input wire           clk,
             input wire           reset_n,

             output wire          spi_sclk,
             output wire          spi_cs_n,
             input wire           spi_do,
             output wire          spi_di,

             input wire           cs,
             input wire           we,
             input wire  [7 : 0]  address,
             input wire  [31 : 0] write_data,
             output wire [31 : 0] read_data
            );


  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  localparam ADDR_NAME0       = 8'h00;
  localparam ADDR_NAME1       = 8'h01;
  localparam ADDR_VERSION     = 8'h02;
  localparam ADDR_CTRL        = 8'h08;
  localparam CTRL_READ_BIT    = 0;
  localparam CTRL_WRITE_BIT   = 1;
  localparam CTRL_INIT_BIT    = 2;
  localparam ADDR_STATUS      = 8'h09;
  localparam STATUS_READY_BIT = 0;
  localparam STATUS_VALID_BIT = 1;
  localparam ADDR_SCLK_DIV    = 8'h0a;
  localparam ADDR_EMEM_ADDR   = 8'h10;
  localparam ADDR_EMEM_DATA   = 8'h20;

  localparam DEFAULT_SCLK_DIV = 16'h0020;

  localparam CORE_NAME0   = 32'h6d6b6d69; // "mkmi"
  localparam CORE_NAME1   = 32'h66202020; // "f   "
  localparam CORE_VERSION = 32'h302e3130; // "0.10"


  //----------------------------------------------------------------
  // Registers including update variables and write enable.
  //----------------------------------------------------------------
  reg          read_op_reg;
  reg          read_op_new;
  reg          write_op_reg;
  reg          write_op_new;
  reg          init_op_reg;
  reg          init_op_new;

  reg [15 : 0] addr_reg;
  reg          addr_we;

  reg [15 : 0] sclk_div_reg;
  reg          sclk_div_we;

  reg [31 : 0] write_data_reg;
  reg          write_data_we;


  //----------------------------------------------------------------
  // Wires.
  //----------------------------------------------------------------
  wire          core_ready;
  wire          core_valid;
  wire [31 : 0] core_read_data;
  reg [31 : 0]  tmp_read_data;


  //----------------------------------------------------------------
  // Concurrent connectivity for ports etc.
  //----------------------------------------------------------------
  assign read_data = tmp_read_data;


  //----------------------------------------------------------------
  // core
  //----------------------------------------------------------------
  mkmif_core core(
                  .clk(clk),
                  .reset_n(reset_n),

                  .spi_sclk(spi_sclk),
                  .spi_cs_n(spi_cs_n),
                  .spi_do(spi_do),
                  .spi_di(spi_di),

                  .read_op(read_op_reg),
                  .write_op(write_op_reg),
                  .init_op(init_op_reg),
                  .ready(core_ready),
                  .valid(core_valid),
                  .sclk_div(sclk_div_reg),
                  .addr(addr_reg),
                  .write_data(write_data_reg),
                  .read_data(core_read_data)
                 );


  //----------------------------------------------------------------
  // reg_update
  // Update functionality for all registers in the core.
  // All registers are positive edge triggered with asynchronous
  // active low reset.
  //----------------------------------------------------------------
  always @ (posedge clk or negedge reset_n)
    begin
      if (!reset_n)
        begin
          read_op_reg    <= 1'h0;
          write_op_reg   <= 1'h0;
          addr_reg       <= 16'h0;
          sclk_div_reg   <= DEFAULT_SCLK_DIV;
          write_data_reg <= 32'h0;
        end
      else
        begin
          read_op_reg  <= read_op_new;
          write_op_reg <= write_op_new;
          init_op_reg  <= init_op_new;

          if (sclk_div_we)
            sclk_div_reg <= write_data[15 : 0];

          if (addr_we)
            addr_reg <= write_data[15 : 0];

          if (write_data_we)
            write_data_reg <= write_data;
        end
    end // reg_update


  //----------------------------------------------------------------
  // api
  //----------------------------------------------------------------
  always @*
    begin : api
      read_op_new   = 0;
      write_op_new  = 0;
      init_op_new   = 0;
      addr_we       = 0;
      sclk_div_we   = 0;
      write_data_we = 0;
      tmp_read_data = 32'h00000000;

      if (cs)
        begin
          if (we)
            begin
              case (address)
                ADDR_CTRL:
                  begin
                    read_op_new  = write_data[CTRL_READ_BIT];
                    write_op_new = write_data[CTRL_WRITE_BIT];
                    init_op_new  = write_data[CTRL_INIT_BIT];
                  end

                ADDR_SCLK_DIV:
                  sclk_div_we = 1;

                ADDR_EMEM_ADDR:
                  addr_we = 1;

                ADDR_EMEM_DATA:
                  write_data_we = 1;

                default:
                  begin
                  end
              endcase // case (address)
            end // if (we)

          else
            begin
              case (address)
                ADDR_NAME0:
                  tmp_read_data = CORE_NAME0;

                ADDR_NAME1:
                  tmp_read_data = CORE_NAME1;

                ADDR_VERSION:
                  tmp_read_data = CORE_VERSION;

                ADDR_STATUS:
                    tmp_read_data = {30'h0, {core_valid, core_ready}};

                ADDR_SCLK_DIV:
                  tmp_read_data = {16'h0, sclk_div_reg};

                ADDR_EMEM_ADDR:
                  tmp_read_data = {16'h0, addr_reg};

                ADDR_EMEM_DATA:
                  begin
                    tmp_read_data = core_read_data;
                  end

                default:
                  begin
                  end
              endcase // case (address)
            end
        end
    end // api
endmodule // mkmif

//======================================================================
// EOF mkmif.v
//======================================================================
