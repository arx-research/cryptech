`timescale 1ns / 1ps

module modexpa7_simple_fifo #
	(
		parameter	BUS_WIDTH	= 128,
		parameter	DEPTH_BITS	= 2
	)
	(
		input								clk,
		input								rst,
		input								wr_en,
		input								rd_en,
		input		[BUS_WIDTH-1:0]	d_in,
		output	[BUS_WIDTH-1:0]	d_out
	);
	
		//
		// Locals
		//
	localparam						NUM_WORDS = 2 ** DEPTH_BITS;
	
	localparam	[DEPTH_BITS:0]	PTR_ZERO = {DEPTH_BITS{1'b0}};
	localparam	[DEPTH_BITS:0]	PTR_LAST = {DEPTH_BITS{1'b1}};
	
		//
		// Memory
		//
	(* RAM_STYLE="DISTRIBUTED" *)
	reg	[BUS_WIDTH-1:0]	fifo[0:NUM_WORDS-1];
	
		//
		// Pointers
		//
	reg	[DEPTH_BITS-1:0]	ptr_wr;
	reg	[DEPTH_BITS-1:0]	ptr_rd;
	
		//
		// Output
		//
	reg	[BUS_WIDTH-1:0]	d_out_reg;
	assign d_out = d_out_reg;
	
		//
		// Write Pointer
		//
	always @(posedge clk)
		//
		if (wr_en)		ptr_wr <= ptr_wr + 1'b1;	
		else if (rst)	ptr_wr <= PTR_ZERO;

		//
		// Read Pointer
		//
	always @(posedge clk)
		//
		if (rst)				ptr_rd <= PTR_ZERO;
		else if (rd_en)	ptr_rd <= ptr_rd + 1'b1;

		//
		// Read Logic
		//
	always @(posedge clk)
		//
		if (rst)				d_out_reg <= {BUS_WIDTH{1'b0}};
		else if (rd_en)	d_out_reg <= fifo[ptr_rd];
		
		//
		// Write Logic
		//
	always @(posedge clk)
		//
		if (wr_en)	fifo[ptr_wr] <= d_in;
	

endmodule
