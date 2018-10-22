`timescale 1ns / 1ps

module tb_demo_adder;

		//
		// Inputs
		//
	reg	gclk;
	wire	gclk_p	=  gclk;
	wire	gclk_n	= ~gclk;
	reg	reset_mcu_b;

		//
		// Outputs
		//
	wire	led;

		//
		// EIM
		//
	reg				eim_cs_n;
	reg				eim_bclk;
	reg				eim_lba_n;
	wire	[15: 0]	eim_da;
	reg	[15: 0]	eim_da_out;
	reg				eim_da_drive;
	reg	[18:16]	eim_a;
	reg				eim_oe_n;
	reg				eim_wr_n;
	wire				eim_wait_n;
	
	assign eim_da = (eim_da_drive == 1'b1) ? eim_da_out : {16{1'bZ}};

		//
		// UUT
		//
	novena_baseline_top uut
	(
		.gclk_p_pin			(gclk_p), 
		.gclk_n_pin			(gclk_n), 
		
		.eim_bclk			(eim_bclk),
		.eim_cs0_n			(eim_cs_n),
		.eim_da				(eim_da),
		.eim_a				(eim_a),
		.eim_lba_n			(eim_lba_n),
		.eim_wr_n			(eim_wr_n),
		.eim_oe_n			(eim_oe_n),
		.eim_wait_n			(eim_wait_n),
		
		.reset_mcu_b_pin	(reset_mcu_b),
		
		.led_pin				(led_pin),
		.apoptosis_pin		(apoptosis_pin)
	);

		//
		// CLK2 (50 MHz)
		//
	always #10 gclk = ~gclk;		
		
		//
		// Initialize EIM
		//
	initial begin
		eim_cs_n			= 1'b1;
		eim_bclk			= 1'b0;
		eim_lba_n		= 1'b1;
		eim_da_out		= {16{1'bX}};
		eim_da_drive	= 1'b1;
		eim_a				= 3'bXXX;
		eim_oe_n			= 1'b1;
		eim_wr_n			= 1'b1;
	end
		
		//
		// Test Logic
		//
	reg	[31: 0]	eim_rd = {32{1'bX}};
	
	initial begin
		gclk				= 1'b0;
		reset_mcu_b		= 1'b1;
		//
		#2000;
		//
		eim_read(19'h10000, eim_rd);				// read Z				<-- should be 0xBB77B7B7
		//
		#10000;
		//
		/*
		eim_write({12'h321, 2'd0, 2'b00}, 32'hAA_55_A5_A5);	// write X
		#100;
		eim_write({12'h321, 2'd1, 2'b00}, 32'h11_22_12_12);	// write Y
		#100;
		eim_read( {12'h321, 2'd3, 2'b00}, eim_rd);				// read {STS, CTL}	<-- should be 0x0000_0000
		#100;
		eim_rd = eim_rd + 1'b1;
		eim_write({12'h321, 2'd3, 2'b00}, eim_rd);				// write {STS, CTL}	<-- STS is ignored by adder
		#100;
		eim_read( {12'h321, 2'd3, 2'b00}, eim_rd);				// read {STS, CTL}	<-- should be 0x0001_0001
		#100;
		eim_read( {12'h321, 2'd2, 2'b00}, eim_rd);				// read Z				<-- should be 0xBB77B7B7
		*/
	end	
		
		//
		// Write Access
		//
	integer wr;
	task eim_write;
		input [18: 0] addr;
		input [31: 0] data;
		begin
			#15	eim_cs_n		= 1'b0;
					eim_lba_n	= 1'b0;
					eim_da_out	= addr[15: 0];
					eim_a			= addr[18:16];
					eim_wr_n		= 1'b0;
			#15	eim_bclk		= 1'b1;
			#15	eim_bclk		= 1'b0;
					eim_lba_n	= 1'b1;
					eim_da_out	= data[15:0];
					eim_a			= 3'bXXX;
			#15	eim_bclk		= 1'b1;
			#15	eim_bclk		= 1'b0;
					eim_da_out	= data[31:16];
			#15	eim_bclk		= 1'b1;
			#15	eim_bclk		= 1'b0;
					eim_da_out	= {16{1'bX}};
			while (eim_wait_n == 1'b0) begin
			#15	eim_bclk		= 1'b1;
			#15	eim_bclk		= 1'b0;
			end					
			#15	eim_cs_n		= 1'b1;
					eim_wr_n		= 1'b1;
			#30;
		end
	endtask;
	
	
		//
		// Read Access
		//
	task eim_read;
		input  [18: 0] addr;
		output [31: 0] data;
		begin
			#15	eim_cs_n			= 1'b0;
					eim_lba_n		= 1'b0;
					eim_da_out		= addr[15: 0];
					eim_a				= addr[18:16];
			#15	eim_bclk			= 1'b1;
			
			#15	eim_bclk			= 1'b0;
					eim_lba_n		= 1'b1;
					eim_oe_n			= 1'b0;
					eim_da_drive	= 1'b0;
					eim_a				= 3'bXXX;
			#15;
			while (eim_wait_n == 1'b0) begin
					eim_bclk		= 1'b1;
			#15	eim_bclk		= 1'b0;
			#15;
			end
					eim_bclk			= 1'b1;
			#15	eim_bclk			= 1'b0;
			#15	eim_bclk			= 1'b1;
					data[15: 0]		= eim_da;
			#15	eim_bclk			= 1'b0;
			#15	eim_bclk			= 1'b1;
					data[31:16]		= eim_da;
			#15	eim_bclk			= 1'b0;
					eim_da_out		= {16{1'bX}};

			#15	eim_cs_n			= 1'b1;
					eim_oe_n			= 1'b1;
					eim_da_drive	= 1'b1;
			#30;
		end
	endtask;	

      
endmodule

