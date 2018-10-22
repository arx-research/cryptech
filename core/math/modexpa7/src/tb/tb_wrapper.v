`timescale 1ns / 1ps

module tb_wrapper;


		//
		// Test Vectors
		//
	`include "modexp_fpga_model_vectors.v";


		/*
		 * Settings
		 */
	localparam	USE_OPERAND_ADDR_WIDTH		= 7;
	localparam	USE_SYSTOLIC_ARRAY_POWER	= 1;

		/*
		 * Clock (100 MHz)
		 */
	reg clk;
	initial clk = 1'b0;
	always #5 clk = ~clk;
	
		/*
		 * Reset
		 */
	reg rst_n;
		 
		 /*
		  * Access Bus
		  */
	reg											bus_cs;
	reg											bus_we;
	reg	[USE_OPERAND_ADDR_WIDTH+3:0]	bus_addr;
	reg	[                    32-1:0]	bus_wr_data;
	wire	[                    32-1:0]	bus_rd_data;

	modexpa7_wrapper #
	(
		.OPERAND_ADDR_WIDTH		(USE_OPERAND_ADDR_WIDTH),
		.SYSTOLIC_ARRAY_POWER	(USE_SYSTOLIC_ARRAY_POWER)
	)
	uut
	(
		.clk			(clk),
		
		.rst_n		(rst_n),
		
		.cs			(bus_cs), 
		.we			(bus_we), 
		.address		(bus_addr), 
		.write_data	(bus_wr_data), 
		.read_data	(bus_rd_data)
	);

	integer i;
	reg	[31: 0]	tmp;
	reg	[383:0]	shreg;
	reg				poll;
	initial begin
		//
		rst_n = 0;
		//
		bus_cs		= 0;
		bus_we		= 0;
		bus_addr		= 'bX;
		bus_wr_data	= 'bX;
		//
		#200;
		//
		rst_n = 1;
		//
		// read common registers to make sure core header reads out ok
		//
		read_reg('h00, tmp);			// NAME0
		read_reg('h01, tmp);			// NAME1
		read_reg('h02, tmp);			// VERSION
		//
		read_reg('h13, tmp);			// BUFFER_BITS
		read_reg('h14, tmp);			// ARRAY_BITS
		//
		write_reg('h12, 32'd384);	// EXPONENT_BITS
		read_reg ('h12, tmp);
		//
		write_reg('h11, 32'd384);	// MODULUS_BITS
		read_reg ('h11, tmp);
		//
		write_reg('h10, 32'd0);		// MODE
		read_reg ('h10, tmp);
		//
		// fill in 384-bit modulus
		//
		shreg = N_384;
		for (i=0; i<384/32; i=i+1) begin
			write_bank(3'b000, i[USE_OPERAND_ADDR_WIDTH-1:0], shreg[31:0]);
			shreg = shreg >> 32;
		end
		//
		// start precomputation
		//
		write_reg('h08, 32'd0);		// CONTROL.init = 0
		write_reg('h08, 32'd1);		// CONTROL.init = 1
		//
		// wait for precomputation to complete
		//
		poll = 1;
		while (poll) begin
			#10;
			read_reg('h09, tmp);		// tmp = STATUS
			poll = ~tmp[0];			// poll = STATUS.ready
		end
		//
		// move modulus-dependent coefficient and Montgomery factor
		// from "output" to "input" banks
		//
		for (i=0; i<384/32; i=i+1) begin
			read_bank (3'b100, i[USE_OPERAND_ADDR_WIDTH-1:0], tmp);
			write_bank(3'b101, i[USE_OPERAND_ADDR_WIDTH-1:0], tmp);
			read_bank (3'b110, i[USE_OPERAND_ADDR_WIDTH-1:0], tmp);
			write_bank(3'b111, i[USE_OPERAND_ADDR_WIDTH-1:0], tmp);
		end
		//
		// fill in 384-bit message
		//
		shreg = M_384;
		for (i=0; i<384/32; i=i+1) begin
			write_bank(3'b001, i[USE_OPERAND_ADDR_WIDTH-1:0], shreg[31:0]);
			shreg = shreg >> 32;
		end
		//
		// fill in 384-bit exponent
		//
		shreg = D_384;
		for (i=0; i<384/32; i=i+1) begin
			write_bank(3'b010, i[USE_OPERAND_ADDR_WIDTH-1:0], shreg[31:0]);
			shreg = shreg >> 32;
		end
		//
		// start exponentiation
		//
		write_reg('h08, 32'd0);		// CONTROL.next = 0
		write_reg('h08, 32'd2);		// CONTROL.next = 1
		//
		// wait for exponentiation to complete
		//
		poll = 1;
		while (poll) begin
			#10;
			read_reg('h09, tmp);		// tmp = STATUS
			poll = ~tmp[1];			// poll = STATUS.valid
		end
		//
		// read result
		//
		for (i=0; i<384/32; i=i+1) begin
			read_bank(3'b011, i[USE_OPERAND_ADDR_WIDTH-1:0], tmp);
			shreg = {tmp, shreg[383:32]};
		end
		//
	end
	
	task read_reg;
		input		[USE_OPERAND_ADDR_WIDTH+2:0]	addr;
		output	[                    32-1:0]	data;
		begin
			bus_cs = 1;
			bus_addr = {1'b0, addr};
			#10;
			bus_cs = 0;
			bus_addr = 'bX;
			data = bus_rd_data;
		end
	endtask
	
	task read_bank;
		input		[                       2:0]	bank;
		input		[USE_OPERAND_ADDR_WIDTH-1:0]	addr;
		output	[                    32-1:0]	data;
		begin
			bus_cs = 1;
			bus_addr = {1'b1, bank, addr};
			#10;
			bus_cs = 0;
			bus_addr = 'bX;
			data = bus_rd_data;
		end
	endtask

	task write_reg;
		input		[USE_OPERAND_ADDR_WIDTH+2:0]	addr;
		input		[                    32-1:0]	data;
		begin
			bus_cs = 1;
			bus_we = 1;
			bus_addr = {1'b0, addr};
			bus_wr_data = data;
			#10;
			bus_cs = 0;
			bus_we = 0;
			bus_addr = 'bX;
		end
	endtask
	
	task write_bank;
		input		[                       2:0]	bank;
		input		[USE_OPERAND_ADDR_WIDTH-1:0]	addr;
		input		[                    32-1:0]	data;
		begin
			bus_cs = 1;
			bus_we = 1;
			bus_addr = {1'b1, bank, addr};
			bus_wr_data = data;
			#10;
			bus_cs = 0;
			bus_we = 0;
			bus_addr = 'bX;
		end
	endtask
	
endmodule

