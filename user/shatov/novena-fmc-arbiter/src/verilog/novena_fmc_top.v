`timescale 1ns / 1ps

module novena_fmc_top
	(
		input		wire 	       	gclk_p_pin,
		input		wire 	       	gclk_n_pin,

		input		wire 	       	reset_mcu_b_pin,
		
		input		wire				fmc_clk,		// clock
		input		wire	[21: 0]	fmc_a,		// address
		inout		wire	[31: 0]	fmc_d,		// data
		input		wire				fmc_ne1,		// chip select
		input		wire				fmc_noe,		// output enable
		input		wire				fmc_nwe,		// write enable
		input		wire				fmc_nl,		// latch enable
		output	wire				fmc_nwait,	// wait
		
		output	wire				apoptosis_pin,
		output	wire				led_pin
	);
	
	
		//
		// Clock Manager
		//
	wire	sys_clk;
	wire  sys_rst;

   novena_clkmgr_new #
	(
		.CLK_OUT_MUL	(2),	// 2..32
		.CLK_OUT_DIV	(2)	// 1..32
	)
	clkmgr
	(
		.gclk_p			(gclk_p_pin),
		.gclk_n			(gclk_n_pin),

		.reset_mcu_b	(reset_mcu_b_pin),

		.sys_clk			(sys_clk),
		.sys_rst			(sys_rst)
	);


		//
		// BUFG
		//
	wire	fmc_clk_bug;
	
   BUFG BUFG_fmc_clk
	(
		.I		(fmc_clk),
		.O		(fmc_clk_bufg)
	);
	
	
	
	//----------------------------------------------------------------
   // FMC Arbiter
   //
   // FMC arbiter handles FMC access and transfers it into
   // `sys_clk' clock domain.
   //----------------------------------------------------------------
	
	wire	[21: 0]	sys_fmc_addr;	// address
	wire				sys_fmc_wren;	// write enable
	wire				sys_fmc_rden;	// read enable
   wire	[31: 0]	sys_fmc_dout;	// data output (from STM32 to FPGA)
	reg	[31: 0]	sys_fmc_din;	// data input (from FPGA to STM32)
		
   fmc_arbiter #
	  (
	   .NUM_ADDR_BITS(22)	// change to 26 when 
	  )
	fmc
     (
		.fmc_clk(fmc_clk_bufg),
		.fmc_a(fmc_a),
		.fmc_d(fmc_d),
		.fmc_ne1(fmc_ne1),
		.fmc_nl(fmc_nl),
		.fmc_nwe(fmc_nwe),
		.fmc_noe(fmc_noe),
		.fmc_nwait(fmc_nwait),

      .sys_clk(sys_clk),

      .sys_addr(sys_fmc_addr),
      .sys_wr_en(sys_fmc_wren),
		.sys_rd_en(sys_fmc_rden),
      .sys_data_out(sys_fmc_dout),
      .sys_data_in(sys_fmc_din)
     );
				
		
	//----------------------------------------------------------------
	// Dummy Register
	//
	// General-purpose register to test FMC interface using STM32
	// demo program instead of core selector logic.
	//
	// This register is a bit tricky, but it allows testing of both
	// data and address buses. Reading from FPGA will always return
	// value, which is currently stored in the test register, 
	// regardless of read transaction address. Writing to FPGA has
	// two variants: a) writing to address 0 will store output data
	// data value in the test register, b) writing to any non-zero
	// address will store _address_ of write transaction in the test
	// register.
	//
	// To test data bus, write some different patterns to address 0,
	// then readback from any address and compare.
	//
	// To test address bus, write anything to some different non-zero
	// addresses, then readback from any address and compare returned
	// value with previously written address.
	//
	//----------------------------------------------------------------
	reg	[31: 0]	test_reg;
	
	always @(posedge sys_clk)
		//
		if (sys_fmc_wren) begin
			//
			// when writing to address 0, store input data value
			//
			// when writing to non-zero address, store _address_
			// (padded with zeroes) instead of data
			//
			test_reg <= (sys_fmc_addr == {22{1'b0}}) ? sys_fmc_dout : {{10{1'b0}}, sys_fmc_addr};
			//
		end else if (sys_fmc_rden) begin
			//
			// always return current value, ignore address
			//
			sys_fmc_din <= test_reg;
			//
		end
	
	
   //----------------------------------------------------------------
   // LED Driver
   //
   // A simple utility LED driver that turns on the Novena
   // board LED when the FMC interface is active.
   //----------------------------------------------------------------
   fmc_indicator led
     (
      .sys_clk(sys_clk),
      .sys_rst(sys_rst),
      .fmc_active(sys_fmc_wren | sys_fmc_rden),
      .led_out(led_pin)
      );
	
			
	//----------------------------------------------------------------
   // Novena Patch
   //
   // Patch logic to keep the Novena board happy.
   // The apoptosis_pin pin must be kept low or the whole board
   // (more exactly the CPU) will be reset after the FPGA has
   // been configured.
   //----------------------------------------------------------------
   assign apoptosis_pin = 1'b0;
	
	
endmodule
