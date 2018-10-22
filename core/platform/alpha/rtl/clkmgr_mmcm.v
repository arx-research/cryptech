//======================================================================
//
// clkmgr_mmcm.v
// ---------------
// Xilinx MMCM primitive wrapper to avoid using Clocking Wizard IP core.
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

module clkmgr_mmcm
  (
   input wire  clk_in,
   input wire  reset_in,
  
   output wire gclk_out,
   output wire gclk_missing_out,

   output wire clk_out,
   output wire clk_valid_out
   );
   
   
   //
   // Parameters
   //
   parameter    CLK_OUT_MUL     = 20.0;    // multiply factor for output clock frequency (2..64)
   parameter    CLK_OUT_DIV     = 20.0;    // divide factor for output clock frequency (1..128)
   
   
   //
   // IBUFG
   //
 
   (* BUFFER_TYPE="NONE" *)
	wire        clk_in_ibufg;
	
	IBUFG IBUFG_gclk
   (
		.I (clk_in),
		.O	(clk_in_ibufg)
	);


	//
	// MMCME2_ADV
	//
	wire	mmcm_clkout0;
	wire	mmcm_locked;
	wire	mmcm_clkfbout;
	wire	mmcm_clkfbout_bufg;
	
	MMCME2_ADV #
	(
		.CLKIN1_PERIOD        (20.000),
		.REF_JITTER1          (0.010),

		.STARTUP_WAIT         ("FALSE"),
		.BANDWIDTH            ("OPTIMIZED"),
		.COMPENSATION         ("ZHOLD"),

		.DIVCLK_DIVIDE        (1),
		
		.CLKFBOUT_MULT_F      (CLK_OUT_MUL),
		.CLKFBOUT_PHASE       (0.000),
		.CLKFBOUT_USE_FINE_PS ("FALSE"),
		
		.CLKOUT0_DIVIDE_F     (CLK_OUT_DIV),
		.CLKOUT0_PHASE        (0.000),
		.CLKOUT0_USE_FINE_PS  ("FALSE"),
		.CLKOUT0_DUTY_CYCLE   (0.500),
		
		.CLKOUT4_CASCADE      ("FALSE")
	)
	MMCME2_ADV_inst
	(
		.CLKIN1              (clk_in_ibufg),
		.CLKIN2              (1'b0),
		.CLKINSEL            (1'b1),

		.CLKFBIN             (mmcm_clkfbout_bufg),
		.CLKFBOUT            (mmcm_clkfbout),
		.CLKFBOUTB           (),
		
		.CLKINSTOPPED        (gclk_missing_out),
		.CLKFBSTOPPED        (),

		.CLKOUT0             (mmcm_clkout0),
		.CLKOUT0B            (),
		
		.CLKOUT1             (),
		.CLKOUT1B            (),
		
		.CLKOUT2             (),
		.CLKOUT2B            (),
		
		.CLKOUT3             (),
		.CLKOUT3B            (),
		
		.CLKOUT4             (),
		.CLKOUT5             (),
		.CLKOUT6             (),
		
		.DCLK                (1'b0),
		.DEN                 (1'b0),		
		.DWE                 (1'b0),
		.DADDR               (7'd0),
		.DI                  (16'h0000),
		.DO                  (),
		.DRDY                (),
		
		.PSCLK               (1'b0),
		.PSEN                (1'b0),
		.PSINCDEC            (1'b0),
		.PSDONE              (),
		
		.LOCKED              (mmcm_locked),
		.PWRDWN              (1'b0),
		.RST                 (reset_in)
	);


	//
	// Mapping
	//
	assign	gclk_out        = clk_in_ibufg;
	assign	clk_valid_out   = mmcm_locked;

	
	//
	// BUFGs
	//
	BUFG BUFG_gclk
   (
		.I	(mmcm_clkout0),
		.O	(clk_out)
    );

	BUFG BUFG_feedback
	(
		.I	(mmcm_clkfbout),
		.O	(mmcm_clkfbout_bufg)
	);

  

endmodule

//======================================================================
// EOF clkmgr_mmcm.v
//======================================================================
