//======================================================================
//
// novena_clkmgr_new.v
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

module novena_clkmgr_new
  (
		input wire  gclk_p, // signal from clock pins
		input wire  gclk_n, //

		input wire  reset_mcu_b, // cpu reset (async, active-low)

		output wire sys_clk, // buffered system clock output
		output wire sys_rst  // system reset output (sync, active-high)
   );


		//
		// Parameters
		//
	parameter	CLK_OUT_MUL		= 2;
	parameter	CLK_OUT_DIV		= 2;

		//
		// IBUFGDS
		//
   (* BUFFER_TYPE="NONE" *)
   wire        gclk;

   IBUFGDS IBUFGDS_gclk
	(
		.I(gclk_p),
		.IB(gclk_n),
      .O(gclk)
	);


		//
		// DCM
		//
   wire        dcm_reset;     // dcm reset
   wire        dcm_locked;    // output clock valid
   wire        gclk_missing;	// no input clock

   clkmgr_dcm_new #
	(
		.CLK_OUT_MUL			(CLK_OUT_MUL),
		.CLK_OUT_DIV			(CLK_OUT_DIV)
	)
	dcm
	(
		.clk_in					(gclk),
		.reset_in				(dcm_reset),
		.gclk_missing_out		(gclk_missing),

		.clk_out					(sys_clk),
		.clk_valid_out			(dcm_locked)
	);


   //
   // DCM Reset Logic
   //

   /* DCM should be reset on power-up, when input clock is stopped or when the
    * CPU gets reset.
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

   /* System reset is asserted for 16 cycles whenever DCM aquires lock. */

   reg [15: 0] sys_rst_shreg    = {16{1'b1}};   // 16-bit shift register

   always @(posedge sys_clk or negedge reset_mcu_b or posedge gclk_missing or negedge dcm_locked)
     //
     if ((reset_mcu_b == 1'b0) || (gclk_missing == 1'b1) || (dcm_locked == 1'b0))
       sys_rst_shreg    <= {16{1'b1}};
     else if (dcm_locked == 1'b1)
       sys_rst_shreg    <= {sys_rst_shreg[14:0], 1'b0};

   assign sys_rst = sys_rst_shreg[15];


endmodule

//======================================================================
// EOF novena_clkmgr_new.v
//======================================================================
