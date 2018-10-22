//======================================================================
//
// alpha_clkmgr.v
// ---------------
// Clock and reset implementation for the Cryptech Alpha
// FPGA framework.
//
//
// Author: Pavel Shatov
// Copyright (c) 2016, NORDUnet A/S All rights reserved.
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

module alpha_clkmgr
  (
   input wire  gclk,            // signal from clock pin

   output wire sys_clk,         // buffered system clock output
   output wire sys_rst_n        // system reset output (async set, sync clear, active-low)
   );


   //
   // Parameters
   //
   parameter    CLK_OUT_MUL     = 20.0;
   parameter    CLK_OUT_DIV     = 20.0;

   //
   // Wrapper for Xilinx-specific MMCM (Mixed Mode Clock Manager) primitive.
   //

   wire        gclk_int;        // buffered input clock
   wire        mmcm_reset;      // reset input
   wire        mmcm_locked;     // output clock valid
   wire        gclk_missing;    // input clock stopped

   clkmgr_mmcm #
     (
			.CLK_OUT_MUL              (CLK_OUT_MUL),	// 2..64
			.CLK_OUT_DIV              (CLK_OUT_DIV)	// 1..128
      )
   mmcm
     (
      .clk_in                   (gclk),
      .reset_in                 (mmcm_reset),

      .gclk_out                 (gclk_int),
      .gclk_missing_out         (gclk_missing),

      .clk_out                  (sys_clk),
      .clk_valid_out            (mmcm_locked)
      );
		


   //
   // MMCM Reset Logic
   //

   /* MMCM should be reset on power-up and when the input clock is stopped.
	 * Note that MMCM requires active-high reset, so the shift register is
	 * preloaded with 1's and then gradually filled with 0's.
    */

   reg [15: 0] mmcm_rst_shreg    = {16{1'b1}};   // 16-bit shift register

   always @(posedge gclk_int or posedge gclk_missing)
     //
     if ((gclk_missing == 1'b1))
       mmcm_rst_shreg    <= {16{1'b1}};
     else
       mmcm_rst_shreg    <= {mmcm_rst_shreg[14:0], 1'b0};

   assign mmcm_reset = mmcm_rst_shreg[15];


   //
   // System Reset Logic
   //

   /* System reset is asserted for 16 cycles whenever MMCM aquires lock. Note
    * that system reset is active-low, so the shift register is preloaded with
    * 0's and gradually filled with 1's.
    */

   reg [15: 0] sys_rst_shreg    = {16{1'b0}};   // 16-bit shift register

   always @(posedge sys_clk or posedge gclk_missing or negedge mmcm_locked)
     //
     if ((gclk_missing == 1'b1) || (mmcm_locked == 1'b0))
       sys_rst_shreg    <= {16{1'b0}};
     else if (mmcm_locked == 1'b1)
       sys_rst_shreg    <= {sys_rst_shreg[14:0], 1'b1};

   assign sys_rst_n = sys_rst_shreg[15];


endmodule

//======================================================================
// EOF alpha_clkmgr.v
//======================================================================
