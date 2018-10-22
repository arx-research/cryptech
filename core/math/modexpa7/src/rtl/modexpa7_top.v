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

`timescale 1ns / 1ps

module modexpa7_top #
	(
		parameter OPERAND_ADDR_WIDTH		= 7,
		parameter SYSTOLIC_ARRAY_POWER	= 1
	)
	(
		input											clk,
		input											rst_n,
		
		input											init,
		output										ready,

		input											next,
		output										valid,

		input											crt_mode,

		input		[OPERAND_ADDR_WIDTH-1:0]	modulus_num_words,
		input		[OPERAND_ADDR_WIDTH+4:0]	exponent_num_bits,

		input 										bus_cs,
		input 										bus_we,
		input		[OPERAND_ADDR_WIDTH+2:0]	bus_addr,
		input		[                32-1:0]	bus_data_wr,
		output 	[                32-1:0]	bus_data_rd
	);
	
	
		/*
		 * FSM Declaration
		 */
		 
	localparam	[ 2: 0]	FSM_STATE_IDLE					= 3'd0;
	
	localparam	[ 2: 0]	FSM_STATE_PRECALC_START		= 3'd1;
	localparam	[ 2: 0]	FSM_STATE_PRECALC_CRUNCH	= 3'd2;
	localparam	[ 2: 0]	FSM_STATE_PRECALC_FINAL		= 3'd3;
		
	localparam	[ 2: 0]	FSM_STATE_EXPONENT_START	= 3'd4;
	localparam	[ 2: 0]	FSM_STATE_EXPONENT_CRUNCH	= 3'd5;
	localparam	[ 2: 0]	FSM_STATE_EXPONENT_FINAL	= 3'd6;
	
	localparam	[ 7: 0]	FSM_STATE_STOP					= 3'd7;
	
	
		/*
		 * FSM State / Next State
		 */
		 
	reg	[ 7: 0]	fsm_state = FSM_STATE_IDLE;
	reg	[ 7: 0]	fsm_next_state;


		/*
		 * Enable Delay (Trigger)
		 */
		 
   reg init_dly = 1'b0;
   reg next_dly = 1'b0;

		// delay init and next by one clock cycle
   always @(posedge clk) init_dly <= init;
	always @(posedge clk) next_dly <= next;

		// trigger new operation when one of the control inputs goes from low to high
   wire init_trig = init && !init_dly;
   wire next_trig = next && !next_dly;
	
	
		/*
		 * Ready and Valid Flags Logic
		 */
		 
	reg ready_reg = 1'b0;
	reg valid_reg = 1'b0;
	
	assign ready = ready_reg;
	assign valid = valid_reg;
	
	reg	init_trig_latch;
	reg	next_trig_latch;
	
	always @(posedge clk)
		//
		if (fsm_state == FSM_STATE_IDLE)
			//
			case ({next_trig, init_trig})
				2'b00:	{next_trig_latch, init_trig_latch} <= 2'b00;		// do nothing
				2'b01:	{next_trig_latch, init_trig_latch} <= 2'b01;		// precalculate
				2'b10:	{next_trig_latch, init_trig_latch} <= 2'b10;		// exponentiate
				2'b11:	{next_trig_latch, init_trig_latch} <= 2'b01;		// 'init' has priority over 'next'
			endcase

		// ready flag logic
   always @(posedge clk or negedge rst_n)
		//
		if (rst_n == 1'b0)									ready_reg <= 1'b0;	// reset flag to default state
		else case (fsm_state)
			FSM_STATE_IDLE:	if (init_trig)				ready_reg <= 1'b0;	// clear flag when operation is started
			FSM_STATE_STOP:	if (init_trig_latch)		ready_reg <= 1'b1;	// set flag after operation is finished
		endcase

		// valid flag logic
   always @(posedge clk or negedge rst_n)
		//
		if (rst_n == 1'b0)									valid_reg <= 1'b0;	// reset flag to default state
		else case (fsm_state)
			FSM_STATE_IDLE:	if (next_trig)				valid_reg <= 1'b0;	// clear flag when operation is started
			FSM_STATE_STOP:	if (next_trig_latch)		valid_reg <= 1'b1;	// set flag after operation is finished
		endcase

	
		/*
		 * Parameters Latch
		 */
	reg	[OPERAND_ADDR_WIDTH-1:0]	modulus_num_words_latch;
	reg	[OPERAND_ADDR_WIDTH+4:0]	exponent_num_bits_latch;

		// save number of words in modulus when pre-calculation has been triggered,
		// i.e. user has apparently loaded a new modulus into the core
		//
		// we also need to update modulus length when user wants to exponentiate,
		// because he could have done precomputation for some modulus, then used
		// a different length modulus and then reverted back the original modulus
		// without doing precomputation (dammit, spent whole day chasing this bug :(
	always @(posedge clk)
		//
		if ((fsm_next_state == FSM_STATE_PRECALC_START) ||
			 (fsm_next_state == FSM_STATE_EXPONENT_START))
			modulus_num_words_latch <= modulus_num_words;

		// save number of bits in exponent when exponentiation has been triggered,
		// i.e. user has loaded a new message into the core and wants to exponentiate
	always @(posedge clk)
		//
		if (fsm_next_state == FSM_STATE_EXPONENT_START)
			exponent_num_bits_latch <= exponent_num_bits;

	
		/*
		 * Split bus address into bank/word parts.
		 */
	wire	[                 3 - 1 : 0]	bus_addr_bank = bus_addr[OPERAND_ADDR_WIDTH+2:OPERAND_ADDR_WIDTH];
	wire	[OPERAND_ADDR_WIDTH - 1 : 0]	bus_addr_word = bus_addr[OPERAND_ADDR_WIDTH-1:0];
	
	
		/*
		 * Define bank offsets.
		 */
	localparam	[ 2: 0]	BANK_MODULUS					= 3'b000;	// 0
	localparam	[ 2: 0]	BANK_MESSAGE					= 3'b001;	// 1
	localparam	[ 2: 0]	BANK_EXPONENT					= 3'b010;	// 2
	localparam	[ 2: 0]	BANK_RESULT						= 3'b011;	// 3
	localparam	[ 2: 0]	BANK_MODULUS_COEFF_OUT		= 3'b100;	// 5
	localparam	[ 2: 0]	BANK_MODULUS_COEFF_IN		= 3'b101;	// 4
	localparam	[ 2: 0]	BANK_MONTGOMERY_FACTOR_OUT	= 3'b110;	// 7
	localparam	[ 2: 0]	BANK_MONTGOMERY_FACTOR_IN	= 3'b111;	// 6
	
	
		/*
		 * Instantiate user-accessible memories.
		 *
		 * We have four block memories: N for modulus, M for message, D for exponent
		 * and R for result. Memories N, M and D and writeable from the user's side,
		 * memory R is writeable from the core's side and is read-only by user.
		 *
		 * Note, that the core does squaring and multiplication simultaneously, so
		 * there are two identical systolic multipliers inside. It's better to have two
		 * copies of modulus to give router some freedom in placing the multipliers,
		 * that's why there are actually two identical block memories N1 and N2 instead of N.
		 * User reads from the first one, but writes to both of them. Note that the synthesis
		 * tool might get too clever and find out that N1 and N2 are identical and decide
		 * to throw one of them away, use (* KEEP="TRUE" *) or something like that then.
		 *
		 * We also need N3 and N4, because during pre-computation F and N_COEFF are calculated
		 * at the same time, so we need two more copies of modulus to allow different words
		 * of it to be read at the same time.
		 */
	
	wire	[OPERAND_ADDR_WIDTH-1:0]	core_n1_addr;
	wire	[OPERAND_ADDR_WIDTH-1:0]	core_n2_addr;
	wire	[OPERAND_ADDR_WIDTH-1:0]	core_n3_addr;
	wire	[OPERAND_ADDR_WIDTH-1:0]	core_n4_addr;
	wire	[OPERAND_ADDR_WIDTH-1:0]	core_m_addr;
	wire	[OPERAND_ADDR_WIDTH-1:0]	core_d_addr;
	wire	[OPERAND_ADDR_WIDTH-1:0]	core_r_addr;
	
	wire	[                32-1:0]	core_n1_data;
	wire	[                32-1:0]	core_n2_data;
	wire	[                32-1:0]	core_n3_data;
	wire	[                32-1:0]	core_n4_data;
	wire	[                32-1:0]	core_m_data;
	wire	[                32-1:0]	core_d_data;
	wire	[                32-1:0]	core_r_data;
	
	wire	[                32-1:0]	user_n_data;
	wire	[                32-1:0]	user_m_data;
	wire	[                32-1:0]	user_d_data;
	wire	[                32-1:0]	user_r_data;
	
	wire										core_r_wren;
	wire										user_n_wren = bus_cs && bus_we && (bus_addr_bank == BANK_MODULUS);
	wire										user_m_wren = bus_cs && bus_we && (bus_addr_bank == BANK_MESSAGE);
	wire										user_d_wren = bus_cs && bus_we && (bus_addr_bank == BANK_EXPONENT);
	
	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(OPERAND_ADDR_WIDTH))
	bram_n1 (.clk(clk),
		.a_addr(bus_addr_word), .a_out(user_n_data), .a_wr(user_n_wren), .a_in(bus_data_wr), 
		.b_addr(core_n1_addr),  .b_out(core_n1_data));

	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(OPERAND_ADDR_WIDTH))
	bram_n2 (.clk(clk),
		.a_addr(bus_addr_word), .a_out(), .a_wr(user_n_wren), .a_in(bus_data_wr), 
		.b_addr(core_n2_addr),  .b_out(core_n2_data));

	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(OPERAND_ADDR_WIDTH))
	bram_n3 (.clk(clk),
		.a_addr(bus_addr_word), .a_out(), .a_wr(user_n_wren), .a_in(bus_data_wr), 
		.b_addr(core_n3_addr),  .b_out(core_n3_data));

	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(OPERAND_ADDR_WIDTH))
	bram_n4 (.clk(clk),
		.a_addr(bus_addr_word), .a_out(), .a_wr(user_n_wren), .a_in(bus_data_wr), 
		.b_addr(core_n4_addr),  .b_out(core_n4_data));

	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(OPERAND_ADDR_WIDTH))
	bram_m (.clk(clk),
		.a_addr(bus_addr_word), .a_out(user_m_data), .a_wr(user_m_wren), .a_in(bus_data_wr), 
		.b_addr(core_m_addr),   .b_out(core_m_data));

	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(OPERAND_ADDR_WIDTH))
	bram_d (.clk(clk),
		.a_addr(bus_addr_word), .a_out(user_d_data), .a_wr(user_d_wren), .a_in(bus_data_wr), 
		.b_addr(core_d_addr), .b_out(core_d_data));

	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(OPERAND_ADDR_WIDTH))
	bram_r (.clk(clk),
		.a_addr(core_r_addr), .a_out(), .a_wr(core_r_wren), .a_in(core_r_data), 
		.b_addr(bus_addr_word), .b_out(user_r_data));

		
		/*
		 * Instantiate more block memories.
		 *
		 * Fast modular exponentiation requires two pre-calculated helper quantities: Montgomery
		 * factor F and modulus-dependent speed-up coefficient N_COEFF. This core has two separate
		 * buffers for each of those quantities, during pre-computation F and N_COEFF are written to
		 * the "output" buffers, so that user can retrieve them and store along with the key for
		 * future use. During exponentiation F and N_COEFF are read from the "input" buffers and
		 * must be supplied by user along with the modulus.
		 *
		 * Note, that there are actually two identical input block memories N_COEFF1 and N_COEFF2
		 * instead of just one N_COEFF, read the explanation above. F is only used by one of
		 * the multipliers, so we don't need F1 and F2.
		 */

	wire	[OPERAND_ADDR_WIDTH-1:0]	core_f_addr_wr;
	wire	[OPERAND_ADDR_WIDTH-1:0]	core_f_addr_rd;
	wire	[OPERAND_ADDR_WIDTH-1:0]	core_n_coeff_addr_wr;
	wire	[OPERAND_ADDR_WIDTH-1:0]	core_n_coeff1_addr_rd;
	wire	[OPERAND_ADDR_WIDTH-1:0]	core_n_coeff2_addr_rd;
	
	wire	[                32-1:0]	core_f_data_wr;
	wire	[                32-1:0]	core_f_data_rd;
	wire	[                32-1:0]	core_n_coeff_data_wr;
	wire	[                32-1:0]	core_n_coeff1_data_rd;
	wire	[                32-1:0]	core_n_coeff2_data_rd;
		
	wire										core_f_wren;
	wire										core_n_coeff_wren;

	wire	[                32-1:0]	user_f_out_data;
	wire	[                32-1:0]	user_f_in_data;
	wire	[                32-1:0]	user_n_coeff_out_data;
	wire	[                32-1:0]	user_n_coeff_in_data;

	wire										user_f_in_wren       = bus_cs && bus_we && (bus_addr_bank == BANK_MONTGOMERY_FACTOR_IN);
	wire										user_n_coeff_in_wren = bus_cs && bus_we && (bus_addr_bank == BANK_MODULUS_COEFF_IN);

	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(OPERAND_ADDR_WIDTH))
	bram_f_out (.clk(clk),
		.a_addr(core_f_addr_wr), .a_out(), .a_wr(core_f_wren), .a_in(core_f_data_wr),
		.b_addr(bus_addr_word), .b_out(user_f_out_data));

	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(OPERAND_ADDR_WIDTH))
	bram_f_in (.clk(clk),
		.a_addr(bus_addr_word), .a_out(user_f_in_data), .a_wr(user_f_in_wren), .a_in(bus_data_wr),
		.b_addr(core_f_addr_rd), .b_out(core_f_data_rd));

	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(OPERAND_ADDR_WIDTH))
	bram_n_coeff_out (.clk(clk),
		.a_addr(core_n_coeff_addr_wr), .a_out(), .a_wr(core_n_coeff_wren), .a_in(core_n_coeff_data_wr),
		.b_addr(bus_addr_word), .b_out(user_n_coeff_out_data));

	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(OPERAND_ADDR_WIDTH))
	bram_n_coeff_in1 (.clk(clk),
		.a_addr(bus_addr_word), .a_out(user_n_coeff_in_data), .a_wr(user_n_coeff_in_wren), .a_in(bus_data_wr),
		.b_addr(core_n_coeff1_addr_rd), .b_out(core_n_coeff1_data_rd));

	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(OPERAND_ADDR_WIDTH))
	bram_n_coeff_in2 (.clk(clk),
		.a_addr(bus_addr_word), .a_out(), .a_wr(user_n_coeff_in_wren), .a_in(bus_data_wr),
		.b_addr(core_n_coeff2_addr_rd), .b_out(core_n_coeff2_data_rd));
		

		/*
		 * Montgomery factor calculation module.
		 */
	(* EQUIVALENT_REGISTER_REMOVAL="NO" *)
	reg	precalc_f_ena = 1'b0;
	wire	precalc_r_rdy;
	
	modexpa7_factor #
	(
		.OPERAND_ADDR_WIDTH	(OPERAND_ADDR_WIDTH)
	)
	precalc_f
	(
		.clk				(clk),
		.rst_n			(rst_n),

		.ena				(precalc_f_ena),
		.rdy				(precalc_r_rdy),

		.n_bram_addr	(core_n3_addr),
		.f_bram_addr	(core_f_addr_wr),

		.n_bram_out		(core_n3_data),

		.f_bram_in		(core_f_data_wr),
		.f_bram_wr		(core_f_wren),

		.n_num_words	(modulus_num_words_latch)
	);
	
	
		/*
		 * Modulus-depentent coefficient calculation module.
		 */
	(* EQUIVALENT_REGISTER_REMOVAL="NO" *)	 
	reg	precalc_n_coeff_ena = 1'b0;
	wire	precalc_n_coeff_rdy;
	
	modexpa7_n_coeff #
	(
		.OPERAND_ADDR_WIDTH	(OPERAND_ADDR_WIDTH)
	)
	precalc_n_coeff
	(
		.clk						(clk),
		.rst_n					(rst_n),

		.ena						(precalc_n_coeff_ena),
		.rdy						(precalc_n_coeff_rdy),

		.n_bram_addr			(core_n4_addr),
		.n_coeff_bram_addr	(core_n_coeff_addr_wr),

		.n_bram_out				(core_n4_data),

		.n_coeff_bram_in		(core_n_coeff_data_wr),
		.n_coeff_bram_wr		(core_n_coeff_wren),

		.n_num_words			(modulus_num_words_latch)
	);		 

		/*
		 * Exponentiation module.
		 */
		 
	reg	exponent_ena = 1'b0;
	wire	exponent_rdy;
		 
	modexpa7_exponentiator #
	(
		.OPERAND_ADDR_WIDTH		(OPERAND_ADDR_WIDTH),
		.SYSTOLIC_ARRAY_POWER	(SYSTOLIC_ARRAY_POWER)
	)
	exponent_r
	(
		.clk						(clk),
		.rst_n					(rst_n),

		.ena						(exponent_ena),
		.rdy						(exponent_rdy),
		
		.crt						(crt_mode),
		
		.m_bram_addr			(core_m_addr),
		.d_bram_addr			(core_d_addr),
		.f_bram_addr			(core_f_addr_rd),
		.n1_bram_addr			(core_n1_addr),
		.n2_bram_addr			(core_n2_addr),
		.n_coeff1_bram_addr	(core_n_coeff1_addr_rd),
		.n_coeff2_bram_addr	(core_n_coeff2_addr_rd),
		.r_bram_addr			(core_r_addr),

		.m_bram_out				(core_m_data),
		.d_bram_out				(core_d_data),
		.f_bram_out				(core_f_data_rd),
		.n1_bram_out			(core_n1_data),
		.n2_bram_out			(core_n2_data),
		.n_coeff1_bram_out	(core_n_coeff1_data_rd),
		.n_coeff2_bram_out	(core_n_coeff2_data_rd),

		.r_bram_in				(core_r_data),
		.r_bram_wr				(core_r_wren),

		.m_num_words			(modulus_num_words_latch),
		.d_num_bits				(exponent_num_bits_latch)
	);
	
	
		/*
		 * Sub-Module Enable Logic
		 */
		 
	always @(posedge clk) begin
		precalc_f_ena			<= (fsm_next_state == FSM_STATE_PRECALC_START)  ? 1'b1 : 1'b0;
		precalc_n_coeff_ena	<= (fsm_next_state == FSM_STATE_PRECALC_START)  ? 1'b1 : 1'b0;
		exponent_ena			<= (fsm_next_state == FSM_STATE_EXPONENT_START) ? 1'b1 : 1'b0;
	end
		
		
	
	
		/*
		 * FSM Process
		 */
		 
	always @(posedge clk or negedge rst_n)
		//
		if (rst_n == 1'b0)	fsm_state <= FSM_STATE_IDLE;
		else						fsm_state <= fsm_next_state;
	
	
		/*
		 * FSM Transition Logic
		 */
		 
		 // handy flag that tells whether both pre-calculations modules are idle
	wire	precalc_rdy = precalc_n_coeff_rdy && precalc_r_rdy;
	
	always @* begin
		//
		fsm_next_state = FSM_STATE_STOP;
		//
		case (fsm_state)
			//
			FSM_STATE_IDLE:					if (init_trig)			fsm_next_state = FSM_STATE_PRECALC_START;		// init has priority over next
													else if (next_trig)	fsm_next_state = FSM_STATE_EXPONENT_START;
													else						fsm_next_state = FSM_STATE_IDLE;
			//
			FSM_STATE_PRECALC_START:									fsm_next_state = FSM_STATE_PRECALC_CRUNCH;
			FSM_STATE_PRECALC_CRUNCH:		if (precalc_rdy)		fsm_next_state = FSM_STATE_PRECALC_FINAL;
													else						fsm_next_state = FSM_STATE_PRECALC_CRUNCH;
			FSM_STATE_PRECALC_FINAL:									fsm_next_state = FSM_STATE_STOP;
			//
			FSM_STATE_EXPONENT_START:									fsm_next_state = FSM_STATE_EXPONENT_CRUNCH;
			FSM_STATE_EXPONENT_CRUNCH:		if (exponent_rdy)		fsm_next_state = FSM_STATE_EXPONENT_FINAL;
													else						fsm_next_state = FSM_STATE_EXPONENT_CRUNCH;
			FSM_STATE_EXPONENT_FINAL:									fsm_next_state = FSM_STATE_STOP;
			//
			FSM_STATE_STOP:												fsm_next_state = FSM_STATE_IDLE;
			//
		endcase
		//
	end


		/*
		 * Bus read mux.
		 */
		 
		// delay bus_addr_bank by 1 clock cycle to remember from where we've just been reading
	reg	[2: 0]	bus_addr_bank_dly;
	always @(posedge clk)
		if (bus_cs) bus_addr_bank_dly <= bus_addr_bank;

		// map mux to output port
	reg	[31: 0]	bus_data_rd_mux;
	assign bus_data_rd = bus_data_rd_mux;

		// select the right data word
	always @(*)
		//
		case (bus_addr_bank_dly)
			//
			BANK_MODULUS:						bus_data_rd_mux = user_n_data;
			BANK_MESSAGE:						bus_data_rd_mux = user_m_data;
			BANK_EXPONENT:						bus_data_rd_mux = user_d_data;
			BANK_RESULT:						bus_data_rd_mux = user_r_data;
			//
			BANK_MODULUS_COEFF_OUT:			bus_data_rd_mux = user_n_coeff_out_data;
			BANK_MODULUS_COEFF_IN:			bus_data_rd_mux = user_n_coeff_in_data;
			BANK_MONTGOMERY_FACTOR_OUT:	bus_data_rd_mux = user_f_out_data;
			BANK_MONTGOMERY_FACTOR_IN:		bus_data_rd_mux = user_f_in_data;
			//
		endcase

endmodule
