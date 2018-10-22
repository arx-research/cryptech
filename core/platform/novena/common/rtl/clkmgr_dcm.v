//======================================================================
//
// clkmgr_dcm.v
// ---------------
// Xilinx DCM_SP primitive wrapper to avoid using Clocking Wizard IP core.
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

module clkmgr_dcm
  (
   input wire  clk_in_p,
   input wire  clk_in_n,
   input wire  reset_in,
  
   output wire gclk_out,
   output wire gclk_missing_out,

   output wire clk_out,
   output wire clk_valid_out
   );
   
   
   //
   // Parameters
   //
   parameter    CLK_OUT_MUL     = 2;    // multiply factor for output clock frequency (2..32)
   parameter    CLK_OUT_DIV     = 2;    // divide factor for output clock frequency (1..32)
   
   
   //
   // IBUFGDS
   //
   /* Xilinx-specific primitive to handle LVDS input signal. */
   (* BUFFER_TYPE="NONE" *)
   wire        clk_in;
   
   IBUFGDS IBUFGDS_gclk
     (
      .I(clk_in_p),
      .IB(clk_in_n),
      .O(clk_in)
      );

   //
   // DCM_SP
   //
   /* Xilinx-specific primitive. */
   wire        dcm_clk_0;
   wire        dcm_clk_feedback;
   wire        dcm_clk_fx;
   wire        dcm_locked_int;
   wire [ 7: 0] dcm_status_int;
   
   DCM_SP #
     (
      .STARTUP_WAIT             ("FALSE"),
      .DESKEW_ADJUST            ("SYSTEM_SYNCHRONOUS"),
      .CLK_FEEDBACK             ("1X"),
      
      .PHASE_SHIFT              (0),
      .CLKOUT_PHASE_SHIFT       ("NONE"),
      
      .CLKIN_PERIOD             (20.0), // 50 MHz => 20 ns
      .CLKIN_DIVIDE_BY_2        ("FALSE"),
      
      .CLKDV_DIVIDE             (5.000),
      .CLKFX_MULTIPLY           (CLK_OUT_MUL),
      .CLKFX_DIVIDE             (CLK_OUT_DIV)
      )
   DCM_SP_inst
     (
      .RST                      (reset_in),
      
      .CLKIN                    (clk_in),
      .CLKFB                    (dcm_clk_feedback),
      .CLKDV                    (),
      
      .CLK0                     (dcm_clk_0),
      .CLK90                    (),
      .CLK180                   (),
      .CLK270                   (),
      
      .CLK2X                    (),
      .CLK2X180                 (),
      
      .CLKFX                    (dcm_clk_fx),
      .CLKFX180                 (),
      
      .PSCLK                    (1'b0),
      .PSEN                     (1'b0),
      .PSINCDEC                 (1'b0),
      .PSDONE                   (),

      .LOCKED                   (dcm_locked_int),
      .STATUS                   (dcm_status_int),

      .DSSEN                    (1'b0)
      );


   //
   // Mapping
   //
   assign       gclk_out        = clk_in;
   assign       gclk_missing_out= dcm_status_int[1];
   assign       clk_valid_out   = dcm_locked_int & ((dcm_status_int[2:1] == 2'b00) ? 1'b1 : 1'b0);


   //
   // Feedback
   //
   /* DCM_SP requires BUFG primitive in its feedback path. */
   BUFG BUFG_feedback
     (
      .I                (dcm_clk_0),
      .O                (dcm_clk_feedback)
      );

   //
   // Output Buffer
   //
   /* Connect system clock to global clocking network. */
   BUFG BUFG_output
     (
      .I                (dcm_clk_fx),
      .O                (clk_out)
      );


endmodule

//======================================================================
// EOF clkmgr_dcm.v
//======================================================================
