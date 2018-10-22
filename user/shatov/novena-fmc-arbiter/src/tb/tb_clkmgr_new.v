`timescale 1ns / 1ps

module tb_clkmgr_new;


		//
		// Inputs
		//
	reg	gclk = 1'b0;
	reg	gclk_stop;
	wire	gclk_p =  (gclk & ~gclk_stop);
	wire	gclk_n = ~(gclk & ~gclk_stop);
	reg	reset_mcu_b;
	


		//
		// Outputs
		//
	wire sys_clk;
	wire sys_rst;


		//
		// UUT
		//
	novena_clkmgr_new #
	(
		.CLK_OUT_MUL	(4),	// 200 MHz
		.CLK_OUT_DIV	(1)	//
	)
	uut
	(
		.gclk_p			(gclk_p),
		.gclk_n			(gclk_n),
		
		.reset_mcu_b	(reset_mcu_b),
		
		.sys_clk			(sys_clk),
		.sys_rst			(sys_rst)
	);
	
	
		//
		// Clock (50 MHz)
		//
	always #10 gclk = ~gclk;


		//
		// Script
		//
	initial begin
		//
		reset_mcu_b = 0;	// reset active
		gclk_stop = 0;		// gclk running
		//
		#500;
		//
		reset_mcu_b = 1;	// clear reset
		//
		#1000;
		//
		gclk_stop = 1;		// try to stop gclk
		#1000;
		gclk_stop = 0;		// enable gclk again
		//
		#1000;
		//
		reset_mcu_b = 0;	// try to activate reset
		#1000;
		reset_mcu_b = 1;	// clear reset again
		//
	end
      
endmodule

