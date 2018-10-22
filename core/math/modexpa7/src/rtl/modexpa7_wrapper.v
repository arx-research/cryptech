//======================================================================
//
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

module modexpa7_wrapper #
	(
		parameter OPERAND_ADDR_WIDTH		= 5,
		parameter SYSTOLIC_ARRAY_POWER	= 2
	)
	(
		input											clk,
		input											rst_n,

		input											cs,
		input											we,

		input		[OPERAND_ADDR_WIDTH+3:0]	address,
		input		[                32-1:0]	write_data,
		output	[                32-1:0]	read_data
	);


		/*
		 * Address Decoder
		 */
	localparam	ADDR_MSB_REGS	= 1'b0;
	localparam	ADDR_MSB_CORE	= 1'b1;
	
	wire										address_msb = address[OPERAND_ADDR_WIDTH+3];
	wire	[OPERAND_ADDR_WIDTH+2:0]	address_lsb = address[OPERAND_ADDR_WIDTH+2:0];


		/*
		 * Output Mux
		 */
	reg	[31: 0]	read_data_regs;
	wire	[31: 0]	read_data_core;


		/*
		 * Registers
		 */
	localparam	[OPERAND_ADDR_WIDTH+2:0]	ADDR_NAME0				= 'h00;	//
	localparam	[OPERAND_ADDR_WIDTH+2:0]	ADDR_NAME1				= 'h01;	//
	localparam	[OPERAND_ADDR_WIDTH+2:0]	ADDR_VERSION			= 'h02;	//

	localparam	[OPERAND_ADDR_WIDTH+2:0]	ADDR_CONTROL			= 'h08;	// {next, init}
	localparam	[OPERAND_ADDR_WIDTH+2:0]	ADDR_STATUS				= 'h09;	// {valid, ready}
	localparam	[OPERAND_ADDR_WIDTH+2:0]	ADDR_MODE				= 'h10;	// {crt, dummy}
	localparam	[OPERAND_ADDR_WIDTH+2:0]	ADDR_MODULUS_BITS		= 'h11;	// number of bits in modulus
	localparam	[OPERAND_ADDR_WIDTH+2:0]	ADDR_EXPONENT_BITS	= 'h12;	// number of bits in exponent
	localparam	[OPERAND_ADDR_WIDTH+2:0]	ADDR_BUFFER_BITS		= 'h13;	// largest supported number of bits
	localparam	[OPERAND_ADDR_WIDTH+2:0]	ADDR_ARRAY_BITS		= 'h14;	// number of bits in systolic array

	localparam	CONTROL_INIT_BIT	= 0;
	localparam	CONTROL_NEXT_BIT	= 1;

	localparam	STATUS_READY_BIT	= 0;
	localparam	STATUS_VALID_BIT	= 1;
	
	localparam	MODE_DUMMY_BIT		= 0;
	localparam	MODE_CRT_BIT		= 1;

	localparam	CORE_NAME0			= 32'h6D6F6465;	// "mode"
	localparam	CORE_NAME1			= 32'h78706137;	// "xpa7"
	localparam	CORE_VERSION		= 32'h302E3235;	// "0.25"


		/*
		 * Registers
		 */
	reg	[                   1:0]	reg_control;
	reg	[                   1:1]	reg_mode;
	reg	[OPERAND_ADDR_WIDTH+5:0]	reg_modulus_bits;
	reg	[OPERAND_ADDR_WIDTH+5:0]	reg_exponent_bits;


		/*
		 * Wires
		 */
	wire	[ 1:0]	reg_status;


		/*
		 * Internal Quantities
		 */		 
	reg	[OPERAND_ADDR_WIDTH-1:0]	modulus_num_words_core;
	reg	[OPERAND_ADDR_WIDTH+4:0]	exponent_num_bits_core;
 
		 

		/*
		 * ModExpA7
		 */
	modexpa7_top #
	(
		.OPERAND_ADDR_WIDTH		(OPERAND_ADDR_WIDTH),
		.SYSTOLIC_ARRAY_POWER	(SYSTOLIC_ARRAY_POWER)
	)
	modexpa7_core
	(
		.clk						(clk),
		.rst_n					(rst_n),

		.init						(reg_control[CONTROL_INIT_BIT]),
		.ready					(reg_status[STATUS_READY_BIT]),
		
		.next						(reg_control[CONTROL_NEXT_BIT]),
		.valid					(reg_status[STATUS_VALID_BIT]),

		.crt_mode				(reg_mode[MODE_CRT_BIT]),

		.modulus_num_words	(modulus_num_words_core),
		.exponent_num_bits	(exponent_num_bits_core),

		.bus_cs					(cs && (address_msb == ADDR_MSB_CORE)),
		.bus_we					(we),
		.bus_addr				(address_lsb),
		.bus_data_wr			(write_data),
		.bus_data_rd			(read_data_core)
	);
	
	
		/*
		 * Write Checker
		 */

		 // largest supported operand width
	localparam	[OPERAND_ADDR_WIDTH+5:0]	EXPONENT_MIN_BITS	= {{OPERAND_ADDR_WIDTH+4{1'b0}}, 2'b10};
	localparam	[OPERAND_ADDR_WIDTH+5:0]	EXPONENT_MAX_BITS	= {1'b1, {OPERAND_ADDR_WIDTH+5{1'b0}}};
	
	localparam	[OPERAND_ADDR_WIDTH+5:0]	MODULUS_MIN_BITS	= {{OPERAND_ADDR_WIDTH-1{1'b0}}, 7'b1000000};
	localparam	[OPERAND_ADDR_WIDTH+5:0]	MODULUS_MAX_BITS	= {1'b1, {OPERAND_ADDR_WIDTH+5{1'b0}}};
		 
		 //
		 // Limits on modulus_bits:
		 //
		 // Must be 64 .. BUFFER_BITS in steps of 32
		 //
	function	[OPERAND_ADDR_WIDTH+5:0]	check_modulus_bits;
		input	[OPERAND_ADDR_WIDTH+5:0]	num_bits;
		begin
			
				// store input value
			check_modulus_bits = num_bits;
			
				// must be multiple of 32
			check_modulus_bits[4:0] = {5{1'b0}};
			if (check_modulus_bits < num_bits)
				check_modulus_bits = check_modulus_bits + 6'd32;
				
				// too large?
			if (check_modulus_bits > MODULUS_MAX_BITS)
				check_modulus_bits = MODULUS_MAX_BITS;
			
				// too small?
			if (check_modulus_bits < MODULUS_MIN_BITS)
				check_modulus_bits = MODULUS_MIN_BITS;
				
		end
	endfunction

		//
		// Limits on exponent_bits:
		//
		// Must be 2 .. BUFFER_BITS;
		//
		//
	function	[OPERAND_ADDR_WIDTH+5:0]	check_exponent_bits;
		input	[OPERAND_ADDR_WIDTH+5:0]	num_bits;
		begin
			
				// store input value
			check_exponent_bits = num_bits;
			
				// too large?
			if (check_exponent_bits > EXPONENT_MAX_BITS)
				check_exponent_bits = EXPONENT_MAX_BITS;
			
				// too small?
			if (check_exponent_bits < EXPONENT_MIN_BITS)
				check_exponent_bits = EXPONENT_MIN_BITS;
				
			//
		end
	endfunction


		/*
		 * Internal Quantities Generator
		 */


	function	[OPERAND_ADDR_WIDTH-1:0]	get_modulus_num_words_core;
		input	[OPERAND_ADDR_WIDTH+5:0]	num_bits;
		reg	[OPERAND_ADDR_WIDTH+5:0]	num_words_checked;
		begin

				// check number of bits
			num_words_checked = check_modulus_bits(num_bits);
			
				// reduce by 1
			num_words_checked = {{5{1'b0}}, num_words_checked[OPERAND_ADDR_WIDTH+5:5]};
			
				// reduce by 1
			num_words_checked = num_words_checked - 1'b1;
			
				// return
			get_modulus_num_words_core = num_words_checked[OPERAND_ADDR_WIDTH-1:0];

		end
	endfunction

	function	[OPERAND_ADDR_WIDTH+4:0]	get_exponent_num_bits_core;
		input	[OPERAND_ADDR_WIDTH+5:0]	num_bits;
		reg	[OPERAND_ADDR_WIDTH+5:0]	num_bits_checked;
		begin
			
				// check number of bits
			num_bits_checked = check_exponent_bits(num_bits);
			
				// reduce by 1
			num_bits_checked = num_bits_checked - 1'b1;
			
				// return
			get_exponent_num_bits_core = num_bits_checked[OPERAND_ADDR_WIDTH+4:0];
			
		end
	endfunction
												

		/*
		 * Write Interface (External Registers)
		 */
		 
	always @(posedge clk)
		//
		if (rst_n == 1'b0) begin
			//
			reg_control				<= {1'b0, 1'b0};
			reg_modulus_bits		<= 
			reg_exponent_bits		<= {1'b1, {OPERAND_ADDR_WIDTH+4{1'b0}}};
			//
		end else if (cs && (address_msb == ADDR_MSB_REGS) && we)
			//
			case (address_lsb)
				//
				ADDR_CONTROL:			reg_control				<= write_data[ 1: 0];
				ADDR_MODE:				reg_mode					<= write_data[MODE_CRT_BIT];
				ADDR_MODULUS_BITS:	reg_modulus_bits		<= check_modulus_bits(write_data[OPERAND_ADDR_WIDTH+5:0]);
				ADDR_EXPONENT_BITS:	reg_exponent_bits		<= check_exponent_bits(write_data[OPERAND_ADDR_WIDTH+5:0]);
				//
			endcase


		/*
		 * Write Interface (Internal Quantities)
		 */
		 
	always @(posedge clk)
		//
		if (cs && (address_msb == ADDR_MSB_REGS) && we)
			//
			case (address_lsb)
				//
				ADDR_MODULUS_BITS:	modulus_num_words_core <= 
												get_modulus_num_words_core(write_data[OPERAND_ADDR_WIDTH+5:0]);
												
				ADDR_EXPONENT_BITS:	exponent_num_bits_core	<=
												get_exponent_num_bits_core(write_data[OPERAND_ADDR_WIDTH+5:0]);
				//
			endcase


		/*
		 * Read Interface
		 */
		 
	always @(posedge clk)
		//
		if (cs && (address_msb == ADDR_MSB_REGS))
			//
			case (address_lsb)
				//
				ADDR_NAME0:				read_data_regs <= CORE_NAME0;
				ADDR_NAME1:				read_data_regs <= CORE_NAME1;
				ADDR_VERSION:			read_data_regs <= CORE_VERSION;
				
				ADDR_CONTROL:			read_data_regs <= {{30{1'b0}}, reg_control};
				ADDR_MODE:				read_data_regs <= {{30{1'b0}}, reg_mode, 1'b0};
				ADDR_STATUS:			read_data_regs <= {{30{1'b0}}, reg_status};
				
				ADDR_MODULUS_BITS:	read_data_regs <= {{19{1'b0}}, reg_modulus_bits};
				ADDR_EXPONENT_BITS:	read_data_regs <= {{19{1'b0}}, reg_exponent_bits};
				ADDR_BUFFER_BITS:		read_data_regs <= {{26-OPERAND_ADDR_WIDTH  {1'b0}}, 1'b1, {  OPERAND_ADDR_WIDTH+5{1'b0}}};
				ADDR_ARRAY_BITS:		read_data_regs <= {{26-SYSTOLIC_ARRAY_POWER{1'b0}}, 1'b1, {SYSTOLIC_ARRAY_POWER+5{1'b0}}};
				//
				default:					read_data_regs <= {32{1'b0}};
				//
			endcase


		/*
		 * Register / Core Memory Selector
		 */
		 
		// delay the leftmost bit of address
	reg address_msb_dly;
	always @(posedge clk) address_msb_dly = address_msb;

		// output mux
	reg	[31: 0]	read_data_mux;
	assign read_data = read_data_mux;

	always @(*)
		//
		case (address_msb_dly)
			ADDR_MSB_REGS:		read_data_mux = read_data_regs;
			ADDR_MSB_CORE:		read_data_mux = read_data_core;
		endcase


endmodule
