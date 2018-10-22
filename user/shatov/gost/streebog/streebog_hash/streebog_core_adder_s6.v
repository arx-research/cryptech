`timescale 1ns / 1ps

module streebog_core_adder_s6
	(
		clk,
		ena, rdy,
		x, y, sum
	);


		//
		// Ports
		//
	input		wire				clk;	// core clock
	input		wire				ena;	// start addition flag
	output	wire				rdy;	// addition done flag (sum is valid)
	input		wire	[511:0]	x;		// item x
	input		wire	[511:0]	y;		// item y
	output	wire	[511:0]	sum;	// x+y


		/*
		 * ISE cannot synthesize adders using fabric that are more than 256 bits wide. Items X and Y are 512-bit wide, so
		 * Spartan-6 DSP blocks are used to overcome this issue. Every DSP block is configured to add 32 bits at a time, 
		 * so total of 512/32=16 DSP blocks are required to implement addition. Every DSP block is configured to expose
		 * carry input and output ports. Overflow at 512-bit boundary should be ignored according to the specification,
		 * that's why only 15 intermediate carry lines are required.
		 *
		 *     +-------------------+-------------------+-         -+-------------------+
		 * [X] |         511 : 480 |         479 : 448 |    ...    |          31 :   0 |
		 *     +------*------------+------*------------+-         -+------*------------+
		 *            |                   |                               |
		 *     +------|------------+------|------------+-         -+------|------------+
		 * [Y] |      |  511 : 480 |      |  479 : 448 |    ...    |      |   31 :   0 |
		 *     +------|-----*------+------|------------+-         -+------|------------+
		 *            |     |             |     |                         |     |
		 *            |     |             |     |                         |     |
		 *            v     v             v     v                         v     v
		 *          +---+-+---+         +---+-+---+                     +---+-+---+
		 *          | A | | B |         | A | | B |                     | A | | B |
		 *          +---------+         +---+-+---+                     +---+-+---+
		 *          | DSP #15 |         | DSP #15 |                     | DSP  #0 |
		 *          |---------|         |---------|                     |---------|
		 *          |  Carry  |         |  Carry  |                     |  Carry  |
		 *      X --<-Out  In-<--C[14]--<-Out  In-<--C[13]- ... -C[ 0]--<-Out  In-<-- 0
		 *          +---------+         +---------+                     +---------+
		 *          |    S    |         |    S    |                     |    S    |
		 *          +---------+         +---------+                     +---------+
		 *               |                   |                               |
		 *               v                   v                               v
		 *     +---------*---------+---------*---------+-         -+---------*---------+
		 * [Z] |         511 : 480 |         479 : 448 |    ...    |          31 :   0 |
		 *     +-------------------+-------------------+-         -+-------------------+
		 *
		 */


		//
		// Internals
		//
	wire	[511:0]	z;				// concatenated outputs of adders
	wire	[14:0]	z_carry;		// carry lines
	reg	[511:0]	sum_reg;		// output register
	
	assign sum = sum_reg;


		//
		// Shift Register
		//
	
		/*
		 * This shift register is re-loaded with "walking one" bit pattern whenever enable
		 * input is active and adder core is ready. The most significant bit [17] acts as a
		 * ready flag. Lower 16 bits [15:0] control DSP blocks (Clock Enable). Intermediate
		 * bit [16] is required to compensate for 1-cycle latency of DSP blocks.
		 *
		 */
	
	reg	[17: 0]	ce_shreg	= {1'b1, 1'b0, 16'h0000};
	
	assign rdy = ce_shreg[17];
	
	
		//
		// Shift Register Logic
		//
	always @(posedge clk)
		//
		if (! rdy)		ce_shreg	<= {ce_shreg[16:0], 1'b0};
		else if (ena)	ce_shreg	<= {1'b0, 1'b0, 16'h0001};
	
	
		//
		// Output Register Logic
		//
	always @(posedge clk)
		//
		if (ce_shreg[16] == 1'b1) sum_reg <= z;
		

		//
		// LSB Adder
		//
	adder_s6 adder_s6_lsb
	(
		.clk		(clk),				//
		.ce		(ce_shreg[0]),		// clock enable [0]
		.a			(x[ 31:  0]),		//
		.b			(y[ 31:  0]),		//
		.s			(z[ 31:  0]),		//
		.c_in		(1'b0),				// carry input tied to 0
		.c_out	(z_carry[0])		// carry[0] to next adder
	);
	
	
		//
		// MSB Adder
		//
	adder_s6 adder_s6_msb
	(
		.clk		(clk),				//
		.ce		(ce_shreg[15]),	// clock enable [15]
		.a			(x[511:480]),		//
		.b			(y[511:480]),		//
		.s			(z[511:480]),		//
		.c_in		(z_carry[14]),		// carry[14] from previous adder
		.c_out	()						// carry output not connected
	);	


		//
		// Intermediate Adders
		//
	genvar i;
	generate for (i=1; i<=14; i=i+1)
		begin: gen_adder_s6
			adder_s6 adder_s6_int
			(
				.clk		(clk),					//
				.ce		(ce_shreg[i]),			// clock enable [1..14]
				.a			(x[32*i+31:32*i]),	//
				.b			(y[32*i+31:32*i]),	//
				.s			(z[32*i+31:32*i]),	//
				.c_in		(z_carry[i-1]),		// carry[0..13] from previous adder
				.c_out	(z_carry[i])			// carry[1..14] to next adder
			);
		end
	endgenerate
	
	
endmodule
