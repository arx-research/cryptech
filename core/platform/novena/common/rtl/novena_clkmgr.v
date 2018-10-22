//======================================================================
//
// novena_clkmgr.v
// ---------------
// Clock and reset implementation for the Cryptech Novena
// FPGA framework.
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

module novena_clkmgr
  (
   input wire  gclk_p,          // signal from clock pins
   input wire  gclk_n,          //

   input wire  reset_mcu_b,     // cpu reset (async, active-low)

   output wire sys_clk,         // buffered system clock output
   output wire sys_rst_n        // system reset output (async set, sync clear, active-low)
   );


   //
   // Parameters
   //
   parameter    CLK_OUT_MUL     = 2;
   parameter    CLK_OUT_DIV     = 2;

   //
   // Wrapper for Xilinx-specific DCM (Digital Clock Manager) primitive.
   //

   wire        gclk;            // buffered input clock
   wire        dcm_reset;       // dcm reset
   wire        dcm_locked;      // output clock valid
   wire        gclk_missing;    // no input clock

   clkmgr_dcm #
     (
      .CLK_OUT_MUL              (CLK_OUT_MUL),
      .CLK_OUT_DIV              (CLK_OUT_DIV)
      )
   dcm
     (
      .clk_in_p                 (gclk_p),
      .clk_in_n                 (gclk_n),
      .reset_in                 (dcm_reset),

      .gclk_out                 (gclk),
      .gclk_missing_out         (gclk_missing),

      .clk_out                  (sys_clk),
      .clk_valid_out            (dcm_locked)
      );


   //
   // DCM Reset Logic
   //

   /* DCM should be reset on power-up, when input clock is stopped or when the
    * CPU gets reset. Note that DCM requires active-high reset, so the shift
    * register is preloaded with 1's and gradually filled with 0's.
    */

   reg [15: 0] dcm_rst_shreg    = {16{1'b1}};   // 16-bit shift register

   always @(posedge gclk or negedge reset_mcu_b or posedge gclk_missing)
     //
     if ((reset_mcu_b == 1'b0) || (gclk_missing == 1'b1))
       dcm_rst_shreg    <= {16{1'b1}};
     else
       dcm_rst_shreg    <= {dcm_rst_shreg[14:0], 1'b0};

   assign dcm_reset = dcm_rst_shreg[15];


   //
   // System Reset Logic
   //

   /* System reset is asserted for 16 cycles whenever DCM aquires lock. Note
    * that system reset is active-low, so the shift register is preloaded with
    * 0's and gradually filled with 1's.
    */

   reg [15: 0] sys_rst_shreg    = {16{1'b0}};   // 16-bit shift register

   always @(posedge sys_clk or negedge reset_mcu_b or posedge gclk_missing or negedge dcm_locked)
     //
     if ((reset_mcu_b == 1'b0) || (gclk_missing == 1'b1) || (dcm_locked == 1'b0))
       sys_rst_shreg    <= {16{1'b0}};
     else if (dcm_locked == 1'b1)
       sys_rst_shreg    <= {sys_rst_shreg[14:0], 1'b1};

   assign sys_rst_n = sys_rst_shreg[15];


endmodule

//======================================================================
// EOF novena_clkmgr.v
//======================================================================
