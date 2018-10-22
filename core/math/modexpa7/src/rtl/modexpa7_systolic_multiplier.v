//======================================================================
//
// modexpa7_systolic_multiplier.v
// -----------------------------------------------------------------------------
// Systolic Montgomery multiplier.
//
// Authors: Pavel Shatov
//
// Copyright (c) 2017, NORDUnet A/S All rights reserved.
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

module modexpa7_systolic_multiplier #
	(
			//
			// This sets the address widths of memory buffers. Internal data
			// width is 32 bits, so for e.g. 2048-bit operands buffers must store
			// 2048 / 32 = 64 words, and these need 6-bit address bus, because
			// 2 ** 6 = 64.
			//
		parameter	OPERAND_ADDR_WIDTH		= 4,
		
			//
			// Explain.
			//
		parameter	SYSTOLIC_ARRAY_POWER		= 2
	)
	(
		input											clk,
		input											rst_n,

		input											ena,
		output										rdy,

		input											reduce_only,

		output	[OPERAND_ADDR_WIDTH-1:0]	a_bram_addr,
		output	[OPERAND_ADDR_WIDTH-1:0]	b_bram_addr,
		output	[OPERAND_ADDR_WIDTH-1:0]	n_bram_addr,
		output	[OPERAND_ADDR_WIDTH-1:0]	n_coeff_bram_addr,
		output	[OPERAND_ADDR_WIDTH-1:0]	r_bram_addr,

		input		[                32-1:0]	a_bram_out,
		input		[                32-1:0]	b_bram_out,
		input		[                32-1:0]	n_bram_out,
		input		[                32-1:0]	n_coeff_bram_out,

		output	[                32-1:0]	r_bram_in,
		output										r_bram_wr,

		input		[OPERAND_ADDR_WIDTH-1:0]	n_num_words
	);
	
		
		/*
		 * Include Settings
		 */
	`include "pe/modexpa7_primitive_switch.v"
	`include "modexpa7_settings.v"
		

		/*
		 * FSM Declaration
		 */
	localparam	[ 7: 0]	FSM_STATE_IDLE				= 8'h00;
	
	localparam	[ 7: 0]	FSM_STATE_LOAD_START		= 8'h11;
	localparam	[ 7: 0]	FSM_STATE_LOAD_SHIFT		= 8'h12;
	localparam	[ 7: 0]	FSM_STATE_LOAD_WRITE		= 8'h13;
	localparam	[ 7: 0]	FSM_STATE_LOAD_FINAL		= 8'h14;

	localparam	[ 7: 0]	FSM_STATE_MULT_START		= 8'h21;
	localparam	[ 7: 0]	FSM_STATE_MULT_CRUNCH	= 8'h22;
	localparam	[ 7: 0]	FSM_STATE_MULT_FINAL		= 8'h23;
	
	localparam	[ 7: 0]	FSM_STATE_ADD_START		= 8'h31;
	localparam	[ 7: 0]	FSM_STATE_ADD_CRUNCH		= 8'h32;
	localparam	[ 7: 0]	FSM_STATE_ADD_UNLOAD		= 8'h33;
	localparam	[ 7: 0]	FSM_STATE_SUB_UNLOAD		= 8'h34;
	localparam	[ 7: 0]	FSM_STATE_ADD_FINAL		= 8'h35;
	
	localparam	[ 7: 0]	FSM_STATE_SAVE_START		= 8'h41;
	localparam	[ 7: 0]	FSM_STATE_SAVE_WRITE		= 8'h42;
	localparam	[ 7: 0]	FSM_STATE_SAVE_FINAL		= 8'h43;
	
	localparam	[ 7: 0]	FSM_STATE_STOP				= 8'hFF;
	
	
		/*
		 * FSM State / Next State / Previous State
		 */
	reg	[ 7: 0]	fsm_state = FSM_STATE_IDLE;
	reg	[ 7: 0]	fsm_next_state;
	//reg	[ 7: 0]	fsm_prev_state;


		/*
		 * Enable Delay and Trigger
		 */
   reg ena_dly = 1'b0;
	
		// delay enable by one clock cycle
   always @(posedge clk) ena_dly <= ena;

		// trigger new operation when enable goes high
   wire ena_trig = ena && !ena_dly;
	
	
		/*
		 * Ready Flag Logic
		 */
	reg rdy_reg = 1'b1;
	assign rdy = rdy_reg;

   always @(posedge clk or negedge rst_n)
		
			// reset flag
		if (rst_n == 1'b0) rdy_reg <= 1'b1;
		else begin
		
				// clear flag when operation is started
			if (fsm_state == FSM_STATE_IDLE)	rdy_reg <= ~ena_trig;
			
				// set flag after operation is finished
			if (fsm_state == FSM_STATE_STOP)	rdy_reg <= 1'b1;			
			
		end
		
		
		/*
		 * Parameters Latch
		 */
	reg	[OPERAND_ADDR_WIDTH-1:0]	n_num_words_latch;
	reg	[OPERAND_ADDR_WIDTH  :0]	p_num_words_latch;
	reg										reduce_only_latch;

		// save number of words in n when new operation starts
	always @(posedge clk)
		//
		if ((fsm_state == FSM_STATE_IDLE) && ena_trig)
			n_num_words_latch <= n_num_words;
			
	always @(posedge clk)
		//
		if ((fsm_state == FSM_STATE_IDLE) && ena_trig)
			reduce_only_latch <= reduce_only;
			
		
		/*
		 * Multiplication Phase
		 */
	localparam	[ 1: 0]	MULT_PHASE_A_B				= 2'd1;
	localparam	[ 1: 0]	MULT_PHASE_AB_N_COEFF	= 2'd2;
	localparam	[ 1: 0]	MULT_PHASE_Q_N				= 2'd3;
	localparam	[ 1: 0]	MULT_PHASE_STALL			= 2'd0;
	
	reg	[ 1: 0]	mult_phase;
	
	wire	mult_phase_ab   = (mult_phase == MULT_PHASE_A_B)   ? 1'b1 : 1'b0;
	wire	mult_phase_done = (mult_phase == MULT_PHASE_STALL) ? 1'b1 : 1'b0;
	
   always @(posedge clk)
		//
		case (fsm_next_state)
			FSM_STATE_LOAD_START:	if (ena_trig)	mult_phase <= MULT_PHASE_A_B;
			FSM_STATE_MULT_FINAL:
				case (mult_phase)
					MULT_PHASE_A_B:						mult_phase <= MULT_PHASE_AB_N_COEFF;
					MULT_PHASE_AB_N_COEFF:				mult_phase <= MULT_PHASE_Q_N;
					MULT_PHASE_Q_N:						mult_phase <= MULT_PHASE_STALL;
				endcase
		endcase
	
			
		/*
		 * Counters
		 */
			
		// handy values
	wire	[SYSTOLIC_ARRAY_POWER-1:0]	load_mult_cnt_zero = {SYSTOLIC_ARRAY_POWER{1'b0}};
	wire	[SYSTOLIC_CNTR_WIDTH-1:0]	load_syst_cnt_zero = {SYSTOLIC_CNTR_WIDTH{1'b0}};

	wire	[SYSTOLIC_ARRAY_POWER-1:0]	load_mult_cnt_last = {SYSTOLIC_ARRAY_POWER{1'b1}};	
	wire	[SYSTOLIC_CNTR_WIDTH-1:0]	load_syst_cnt_last = n_num_words_latch[OPERAND_ADDR_WIDTH-1:SYSTOLIC_ARRAY_POWER];
	
		// counter
	reg	[SYSTOLIC_ARRAY_POWER-1:0]	load_mult_cnt;
	reg	[SYSTOLIC_CNTR_WIDTH-1:0]	load_syst_cnt;
	
		// handy increment value and stop flag
	wire	[SYSTOLIC_ARRAY_POWER-1:0]	load_mult_cnt_next	= load_mult_cnt + 1'b1;
	wire	[SYSTOLIC_CNTR_WIDTH-1:0]	load_syst_cnt_next	= load_syst_cnt + 1'b1;

	wire										load_mult_cnt_done	= (load_mult_cnt == load_mult_cnt_last) ? 1'b1 : 1'b0;
	wire										load_syst_cnt_done	= (load_syst_cnt == load_syst_cnt_last) ? 1'b1 : 1'b0;
			
		
		/*
		 * Loader Count Logic
		 */
	always @(posedge clk) begin
		//
		case (fsm_state)
			FSM_STATE_LOAD_START:	{load_syst_cnt, load_mult_cnt} <= {load_syst_cnt_zero, load_mult_cnt_zero};
			//
			FSM_STATE_LOAD_SHIFT:	load_mult_cnt <= load_mult_cnt_next;
			FSM_STATE_LOAD_WRITE:	load_syst_cnt <= !load_syst_cnt_done ? load_syst_cnt_next : load_syst_cnt;
		endcase
		//
	end
			
				
		/*
		 * Wide Operand Loader
		 */
	
		/*
		 * Explain how parallelized loader works here...
		 *
		 */
	
	
		// loader input
	reg	[SYSTOLIC_CNTR_WIDTH-1:0]	loader_addr_wr;
	wire	[SYSTOLIC_CNTR_WIDTH-1:0]	loader_addr_rd;
	reg										loader_wren;
	reg	[                 32-1:0]	loader_din [0:SYSTOLIC_ARRAY_LENGTH-1];
	
		// loader output
	wire	[                 32-1:0]	loader_dout[0:SYSTOLIC_ARRAY_LENGTH-1];
	
		// array_input
	wire	[32 * SYSTOLIC_ARRAY_LENGTH - 1 : 0]	pe_a_wide;
	wire	[32 * SYSTOLIC_ARRAY_LENGTH - 1 : 0]	pe_b_wide;
			
		// generate parallelized loader		
	genvar i;
	generate for (i=0; i<SYSTOLIC_ARRAY_LENGTH; i=i+1)
		//
		begin : gen_bram_1rw_1ro_readfirst_loader
			//
			bram_1rw_1ro_readfirst #
			(
				.MEM_WIDTH		(32),
				.MEM_ADDR_BITS	(SYSTOLIC_CNTR_WIDTH)
			)
			bram_loader
			(
				.clk		(clk),
				.a_addr	(loader_addr_wr),
				.a_wr		(loader_wren),
				.a_in		(loader_din[i]),
				.a_out	(),
				.b_addr	(loader_addr_rd),
				.b_out	(loader_dout[i])
			);
			//
			assign pe_b_wide[32 * (i + 1) - 1 -: 32] = loader_dout[i];
			//
		end
		//
	endgenerate
			
				
		/*
		 * Block Memory Addresses
		 */
		
		/*
		 * Explain why there are two memory sizes.
		 */
		
		// the very first addresses
	wire	[OPERAND_ADDR_WIDTH-1:0]	bram_addr_zero			= {      {OPERAND_ADDR_WIDTH{1'b0}}};
	wire	[OPERAND_ADDR_WIDTH  :0]	bram_addr_ext_zero	= {1'b0, {OPERAND_ADDR_WIDTH{1'b0}}};
	
		// the very last addresses
	wire	[OPERAND_ADDR_WIDTH-1:0]	bram_addr_last     = {n_num_words_latch};
	wire	[OPERAND_ADDR_WIDTH  :0]	bram_addr_ext_last = {n_num_words_latch, 1'b1};

		// address registers
	wire	[OPERAND_ADDR_WIDTH-1:0]	a_addr;
	reg	[OPERAND_ADDR_WIDTH-1:0]	b_addr;
	reg	[OPERAND_ADDR_WIDTH-1:0]	n_addr;
	wire	[OPERAND_ADDR_WIDTH  :0]	p_addr_ext_wr;
	wire	[OPERAND_ADDR_WIDTH  :0]	ab_addr_ext_wr;
	reg	[OPERAND_ADDR_WIDTH  :0]	ab_addr_ext_rd;
	wire	[OPERAND_ADDR_WIDTH-1:0]	q_addr_wr;
	wire	[OPERAND_ADDR_WIDTH-1:0]	q_addr_rd;
	wire	[OPERAND_ADDR_WIDTH  :0]	qn_addr_ext_wr;
	reg	[OPERAND_ADDR_WIDTH  :0]	qn_addr_ext_rd;
	reg	[OPERAND_ADDR_WIDTH-1:0]	s_addr;
	reg	[OPERAND_ADDR_WIDTH-1:0]	sn_addr;
	reg	[OPERAND_ADDR_WIDTH-1:0]	r_addr;
		
		// handy increment values
	wire	[OPERAND_ADDR_WIDTH-1:0]	b_addr_next				= b_addr         + 1'b1;
	wire	[OPERAND_ADDR_WIDTH-1:0]	n_addr_next				= n_addr         + 1'b1;
	wire	[OPERAND_ADDR_WIDTH  :0]	ab_addr_ext_rd_next	= ab_addr_ext_rd + 1'b1;
	wire	[OPERAND_ADDR_WIDTH-1:0]	q_addr_rd_next			= q_addr_rd      + 1'b1;
	wire	[OPERAND_ADDR_WIDTH  :0]	qn_addr_ext_rd_next	= qn_addr_ext_rd + 1'b1;
	wire	[OPERAND_ADDR_WIDTH-1:0]	s_addr_next				= s_addr         + 1'b1;
	wire	[OPERAND_ADDR_WIDTH-1:0]	sn_addr_next			= sn_addr        + 1'b1;
	wire	[OPERAND_ADDR_WIDTH-1:0]	r_addr_next				= r_addr         + 1'b1;
	
		// write enables
	wire	p_wren;
	wire	ab_wren;
	wire	q_wren;
	wire	qn_wren;
	reg	s_wren;
	reg	sn_wren;
	reg	r_wren;
	
		// data buses
	wire	[31: 0]	p_data_in;
	wire	[31: 0]	ab_data_in;
	wire	[31: 0]	ab_data_out;
	wire	[31: 0]	q_data_in;
	wire	[31: 0]	q_data_out;
	wire	[31: 0]	qn_data_in;
	wire	[31: 0]	qn_data_out;
	wire	[31: 0]	s_data_in;
	wire	[31: 0]	s_data_out;
	wire	[31: 0]	sn_data_in;
	wire	[31: 0]	sn_data_out;
	wire	[31: 0]	r_data_in;
	
		// handy stop flags
	wire	b_addr_done				= (b_addr         == bram_addr_last)     ? 1'b1 : 1'b0;
	wire	n_addr_done				= (n_addr         == bram_addr_last)     ? 1'b1 : 1'b0;
	wire	ab_addr_ext_rd_done	= (ab_addr_ext_rd == bram_addr_ext_last) ? 1'b1 : 1'b0;
	wire	q_addr_rd_done			= (q_addr_rd      == bram_addr_last)     ? 1'b1 : 1'b0;
	wire	qn_addr_ext_rd_done	= (qn_addr_ext_rd == bram_addr_ext_last) ? 1'b1 : 1'b0;
	wire	s_addr_done				= (s_addr         == bram_addr_last)     ? 1'b1 : 1'b0;
	wire	sn_addr_done			= (sn_addr        == bram_addr_last)     ? 1'b1 : 1'b0;
	wire	r_addr_done				= (r_addr         == bram_addr_last)     ? 1'b1 : 1'b0;

		// delayed addresses
	reg	[OPERAND_ADDR_WIDTH-1:0]	b_addr_dly;
	reg	[OPERAND_ADDR_WIDTH-1:0]	n_addr_dly;
	reg	[OPERAND_ADDR_WIDTH  :0]	ab_addr_ext_rd_dly;
	reg	[OPERAND_ADDR_WIDTH : 0]	qn_addr_ext_rd_dly1;
	reg	[OPERAND_ADDR_WIDTH  :0]	qn_addr_ext_rd_dly2;
	reg	[OPERAND_ADDR_WIDTH  :0]	qn_addr_ext_rd_dly3;
	
	always @(posedge clk) b_addr_dly				<= b_addr;
	always @(posedge clk) n_addr_dly				<= n_addr;
	always @(posedge clk) ab_addr_ext_rd_dly	<= ab_addr_ext_rd;
	always @(posedge clk) qn_addr_ext_rd_dly1 <= qn_addr_ext_rd;
	always @(posedge clk) qn_addr_ext_rd_dly2 <= qn_addr_ext_rd_dly1;
	always @(posedge clk) qn_addr_ext_rd_dly3 <= qn_addr_ext_rd_dly2;
				
		// map registers to top-level ports
	assign b_bram_addr = b_addr;
	assign n_bram_addr = n_addr;
	assign r_bram_addr = r_addr;

		// map
	assign ab_addr_ext_wr	= p_addr_ext_wr[OPERAND_ADDR_WIDTH  :0];
	assign q_addr_wr			= p_addr_ext_wr[OPERAND_ADDR_WIDTH-1:0];
	assign qn_addr_ext_wr	= p_addr_ext_wr[OPERAND_ADDR_WIDTH  :0];
	assign r_bram_wr			= r_wren;
	
	assign ab_data_in		= p_data_in;
	assign q_data_in		= p_data_in;
	assign qn_data_in		= p_data_in;
	assign r_bram_in		= r_data_in;
	
	assign ab_wren		= p_wren && (mult_phase == MULT_PHASE_A_B);
	assign q_wren		= p_wren && (mult_phase == MULT_PHASE_AB_N_COEFF);
	assign qn_wren		= p_wren && (mult_phase == MULT_PHASE_Q_N);
		

	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(OPERAND_ADDR_WIDTH+1))
	bram_ab
	(	.clk(clk),
		.a_addr(ab_addr_ext_wr), .a_wr(ab_wren), .a_in(ab_data_in), .a_out(),
		.b_addr(ab_addr_ext_rd), .b_out(ab_data_out)
	);

	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(OPERAND_ADDR_WIDTH))
	bram_q
	(	.clk(clk),
		.a_addr(q_addr_wr), .a_wr(q_wren), .a_in(q_data_in), .a_out(),
		.b_addr(q_addr_rd), .b_out(q_data_out)
	);

	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(OPERAND_ADDR_WIDTH+1))
	bram_qn
	(	.clk(clk),
		.a_addr(qn_addr_ext_wr), .a_wr(qn_wren), .a_in(qn_data_in), .a_out(),
		.b_addr(qn_addr_ext_rd), .b_out(qn_data_out)
	);

	bram_1rw_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(OPERAND_ADDR_WIDTH))
	bram_s
	(	.clk(clk),
		.a_addr(s_addr), .a_wr(s_wren), .a_in(s_data_in), .a_out(s_data_out)
	);

	bram_1rw_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(OPERAND_ADDR_WIDTH))
	bram_sn
	(	.clk(clk),
		.a_addr(sn_addr), .a_wr(sn_wren), .a_in(sn_data_in), .a_out(sn_data_out)
	);

				
		/*
		 * Loader Data Input 
		 */
	integer j;
	
		// shift logic
	always @(posedge clk)
		//
		case (fsm_state)
			//
			FSM_STATE_LOAD_SHIFT: begin
		
					// update the rightmost part of loader buffer
				case (mult_phase)
				
					MULT_PHASE_A_B:
						loader_din[SYSTOLIC_ARRAY_LENGTH-1] <=
							(b_addr_dly <= bram_addr_last) ? b_bram_out : {32{1'b0}};
							
					MULT_PHASE_AB_N_COEFF:
						loader_din[SYSTOLIC_ARRAY_LENGTH-1] <=
							(ab_addr_ext_rd_dly <= {1'b0, bram_addr_last}) ? ab_data_out : {32{1'b0}};
							
					MULT_PHASE_Q_N:
						loader_din[SYSTOLIC_ARRAY_LENGTH-1] <=
							(n_addr_dly <= bram_addr_last) ? n_bram_out : {32{1'b0}};
							
				endcase
				
					// shift the loader buffer to the left
				for (j=1; j<SYSTOLIC_ARRAY_LENGTH; j=j+1)
					loader_din[j-1] <= loader_din[j];
					
			end
			//			
		endcase


		/*
		 * Load Write Enable Logic
		 */
	always @(posedge clk)
		//
		case (fsm_next_state)
			FSM_STATE_LOAD_WRITE:	loader_wren <= 1'b1;
			default:						loader_wren <= 1'b0;
		endcase


		/*
		 * Loader Address Update Logic
		 */

	always @(posedge clk)
		//
		case (fsm_state)
		
			FSM_STATE_LOAD_START:
				//
				loader_addr_wr <= load_syst_cnt_zero;
				
			FSM_STATE_LOAD_WRITE:
				//
				loader_addr_wr <= !load_syst_cnt_done ? load_syst_cnt_next : load_syst_cnt;
					
		endcase

		/*
		 * Flag
		 */
	reg flag_select_s;
	
	assign r_data_in = flag_select_s ? s_data_out : sn_data_out;
	
	
		/*
		 * Memory Address Control Logic
		 */
	always @(posedge clk) begin
		//
		case (fsm_next_state)
		
			FSM_STATE_LOAD_START: begin
				ab_addr_ext_rd		<= bram_addr_ext_zero;
			end
								
			FSM_STATE_LOAD_SHIFT: begin
				ab_addr_ext_rd		<= ab_addr_ext_rd_next;
			end
		
			FSM_STATE_ADD_START: begin
				ab_addr_ext_rd		<= bram_addr_ext_zero;
				qn_addr_ext_rd		<= bram_addr_ext_zero;
			end
			
			FSM_STATE_ADD_CRUNCH: begin
				ab_addr_ext_rd		<= ab_addr_ext_rd_next;
				qn_addr_ext_rd		<= qn_addr_ext_rd_next;
			end
				
		endcase
		//
		case (fsm_next_state)
	
			FSM_STATE_LOAD_START: begin
				b_addr				<= bram_addr_zero;
				n_addr				<= bram_addr_zero;
			end
								
			FSM_STATE_LOAD_SHIFT: begin
				b_addr 				<= b_addr_next;
				n_addr				<= n_addr_next;
			end
					
			FSM_STATE_ADD_CRUNCH,
			FSM_STATE_ADD_UNLOAD: begin
				if (qn_addr_ext_rd_dly1 == {1'b0, bram_addr_last})			n_addr <= bram_addr_zero;
				else if (qn_addr_ext_rd_dly1 >  {1'b0, bram_addr_last})	n_addr <= n_addr_next;				
			end
			
		endcase
		//
	end


		/*
		 * Multiplier Array
		 */
	reg	pe_array_ena;
	wire	pe_array_rdy;

	always @(posedge clk)
		//
		case (fsm_next_state)
			FSM_STATE_MULT_START:	pe_array_ena <= 1'b1;
			default:						pe_array_ena <= 1'b0;
		endcase
		
	always @(posedge clk)
		//
		if (fsm_next_state == FSM_STATE_MULT_START)
			//
			case (mult_phase)
				MULT_PHASE_A_B:			p_num_words_latch <= {n_num_words_latch, 1'b1};
				MULT_PHASE_AB_N_COEFF:	p_num_words_latch <= {1'b0, n_num_words_latch};
				MULT_PHASE_Q_N:			p_num_words_latch <= {n_num_words_latch, 1'b1};
			endcase
			
	assign a_bram_addr = a_addr;
	assign n_coeff_bram_addr = a_addr;
	assign q_addr_rd = a_addr;
	
	reg	[31: 0]	a_data_out;
	
	always @*
		//
		case (mult_phase)
			MULT_PHASE_A_B:			a_data_out = a_bram_out;
			MULT_PHASE_AB_N_COEFF:	a_data_out = n_coeff_bram_out;
			MULT_PHASE_Q_N:			a_data_out = q_data_out;
			default:						a_data_out = {32{1'bX}};
		endcase
	
	modexpa7_systolic_multiplier_array #
	(
		.OPERAND_ADDR_WIDTH		(OPERAND_ADDR_WIDTH),
		.SYSTOLIC_ARRAY_POWER	(SYSTOLIC_ARRAY_POWER)
	)
	systolic_pe_array
	(
		.clk					(clk),
		.rst_n				(rst_n),

		.ena					(pe_array_ena),
		.rdy					(pe_array_rdy),

		.crt					(reduce_only_latch && mult_phase_ab),

		.loader_addr_rd	(loader_addr_rd),
		
		.pe_a_wide			({SYSTOLIC_ARRAY_LENGTH{a_data_out}}),
		.pe_b_wide			(pe_b_wide),
		
		.a_bram_addr		(a_addr),
		
		.p_bram_addr		(p_addr_ext_wr),
		.p_bram_in			(p_data_in),
		.p_bram_wr			(p_wren),

		.n_num_words		(n_num_words_latch),
		.p_num_words		(p_num_words_latch)
	);
	
		/*
		 * Adder
		 */
		 
	reg				add1_ce;					// clock enable
	wire	[31: 0]	add1_s;					// sum output
	wire				add1_c_in;				// carry input
	wire	[31: 0]	add1_a;					// A-input
	wire	[31: 0]	add1_b;					// B-input
	reg				add1_c_in_mask;		// flag to not carry anything into the very first word
	wire				add1_c_out;				// carry output

	modexpa7_adder32 add1_inst
	(
		.clk		(clk),
		.ce		(add1_ce),
		.a			(add1_a),
		.b			(add1_b),
		.c_in		(add1_c_in),
		.s			(add1_s),
		.c_out	(add1_c_out)
	);

		/*
		 * Subtractor
		 */		 
	reg				sub1_ce;					// clock enable
	wire	[31: 0]	sub1_d;					// difference output
	wire				sub1_b_in;				// borrow input
	wire	[31: 0]	sub1_a;					// A-input
	wire	[31: 0]	sub1_b;					// B-input
	reg				sub1_b_in_mask;		// flag to not borrow anything from the very first word
	wire				sub1_b_out;				// borrow output

	modexpa7_subtractor32 sub1_inst
	(
		.clk		(clk),
		.ce		(sub1_ce),
		.a			(sub1_a),
		.b			(sub1_b),
		.b_in		(sub1_b_in),
		.d			(sub1_d),
		.b_out	(sub1_b_out)
	);
	
		// add masking into carry feedback chain
	assign add1_c_in = add1_c_out & ~add1_c_in_mask;

		// add masking into borrow feedback chain
	assign sub1_b_in = sub1_b_out & ~sub1_b_in_mask;

		// mask carry for the very first words of AB and QN
	always @(posedge clk)
		//
		add1_c_in_mask <= (fsm_state == FSM_STATE_ADD_START) ? 1'b1 : 1'b0;

		// mask borrow for the very first words of S and N
	always @(posedge clk)
		//
		sub1_b_in_mask <= add1_c_in_mask;
			
	
		// map adder inputs
	assign add1_a = ab_data_out;
	assign add1_b = qn_data_out;
	
		// map subtractor inputs
	assign sub1_a = add1_s;
	assign sub1_b = (qn_addr_ext_rd_dly2 <= {1'b0, bram_addr_last}) ? 32'd0 : n_bram_out;
	
		// clock enable
	always @(posedge clk) begin
		//
		case (fsm_state)
			FSM_STATE_ADD_START,
			FSM_STATE_ADD_CRUNCH:	add1_ce <= 1'b1;
			default:						add1_ce <= 1'b0;
		endcase
		//
		sub1_ce <= add1_ce;
		//
	end
		
		// map outputs
	assign s_data_in = add1_s;
	assign sn_data_in = sub1_d;
		
		// write enabled
	always @(posedge clk) begin
		//
		case (fsm_state)
			FSM_STATE_ADD_CRUNCH,
			FSM_STATE_ADD_UNLOAD:	s_wren <= 1'b1;
			default:						s_wren <= 1'b0;		
		endcase
		//
		case (fsm_state)
			FSM_STATE_ADD_CRUNCH,
			FSM_STATE_ADD_UNLOAD,
			FSM_STATE_SUB_UNLOAD,
			FSM_STATE_ADD_FINAL:		sn_wren <= s_wren;
			default:						sn_wren <= 1'b0;
		endcase
		//
		case (fsm_state)
			FSM_STATE_SAVE_START,
			FSM_STATE_SAVE_WRITE:	r_wren <= 1'b1;
			default:						r_wren <= 1'b0;
		endcase
		//
	end

		// ...
	always @(posedge clk) begin
		//
		case (fsm_state)
			FSM_STATE_ADD_CRUNCH,
			FSM_STATE_ADD_UNLOAD: begin
					if (qn_addr_ext_rd_dly1 == {1'b0, bram_addr_zero})			s_addr <= bram_addr_zero;
					else if (qn_addr_ext_rd_dly2 >  {1'b0, bram_addr_last})	s_addr <= s_addr_next;
				end
			FSM_STATE_ADD_FINAL:															s_addr <= bram_addr_zero;
			FSM_STATE_SAVE_START,
			FSM_STATE_SAVE_WRITE:														s_addr <= s_addr_next;
		endcase
		//
		case (fsm_state)
			FSM_STATE_ADD_CRUNCH,
			FSM_STATE_ADD_UNLOAD,
			FSM_STATE_SUB_UNLOAD: begin
					if (qn_addr_ext_rd_dly2 == {1'b0, bram_addr_zero})			sn_addr <= bram_addr_zero;
					else if (qn_addr_ext_rd_dly3 >  {1'b0, bram_addr_last})	sn_addr <= sn_addr_next;
				end
			FSM_STATE_ADD_FINAL:															sn_addr <= bram_addr_zero;
			FSM_STATE_SAVE_START,
			FSM_STATE_SAVE_WRITE:														sn_addr <= sn_addr_next;
		endcase
		//
		case (fsm_state)
			FSM_STATE_SAVE_START:	r_addr <= bram_addr_zero;
			FSM_STATE_SAVE_WRITE:	r_addr <= r_addr_next;
		endcase
		//
	end

		
		/*
		 * Flag Update Logic
		 */
	always @(posedge clk)
		//
		if (fsm_state == FSM_STATE_ADD_FINAL)
			flag_select_s <= sub1_b_out & ~add1_c_out;



			
		/*
		 * FSM Process
		 */
	always @(posedge clk or negedge rst_n)
		//
		if (rst_n == 1'b0)	fsm_state <= FSM_STATE_IDLE;
		else						fsm_state <= fsm_next_state;

	//always @(posedge clk)
		//
		//fsm_prev_state <= fsm_state;
	
	
		/*
		 * FSM Transition Logic
		 */
	always @* begin
		//
		fsm_next_state = FSM_STATE_STOP;
		//
		case (fsm_state)
			//
			FSM_STATE_IDLE:				if (ena_trig)					fsm_next_state = FSM_STATE_LOAD_START;
												else								fsm_next_state = FSM_STATE_IDLE;
			//
			FSM_STATE_LOAD_START:											fsm_next_state = FSM_STATE_LOAD_SHIFT;
			FSM_STATE_LOAD_SHIFT:		if (load_mult_cnt_done)		fsm_next_state = FSM_STATE_LOAD_WRITE;
												else								fsm_next_state = FSM_STATE_LOAD_SHIFT;
			FSM_STATE_LOAD_WRITE:		if (load_syst_cnt_done)		fsm_next_state = FSM_STATE_LOAD_FINAL;
												else								fsm_next_state = FSM_STATE_LOAD_SHIFT;
			FSM_STATE_LOAD_FINAL:											fsm_next_state = FSM_STATE_MULT_START;
			//
			FSM_STATE_MULT_START:											fsm_next_state = FSM_STATE_MULT_CRUNCH;
			FSM_STATE_MULT_CRUNCH:		if (pe_array_rdy)				fsm_next_state = FSM_STATE_MULT_FINAL;
												else								fsm_next_state = FSM_STATE_MULT_CRUNCH;
			FSM_STATE_MULT_FINAL:		if (mult_phase_done)			fsm_next_state = FSM_STATE_ADD_START;
												else								fsm_next_state = FSM_STATE_LOAD_START;
			//
			FSM_STATE_ADD_START:												fsm_next_state = FSM_STATE_ADD_CRUNCH;
			FSM_STATE_ADD_CRUNCH:		if (ab_addr_ext_rd_done)	fsm_next_state = FSM_STATE_ADD_UNLOAD;
												else								fsm_next_state = FSM_STATE_ADD_CRUNCH;
			FSM_STATE_ADD_UNLOAD:											fsm_next_state = FSM_STATE_SUB_UNLOAD;
			FSM_STATE_SUB_UNLOAD:											fsm_next_state = FSM_STATE_ADD_FINAL;
			FSM_STATE_ADD_FINAL:												fsm_next_state = FSM_STATE_SAVE_START;
			//
			FSM_STATE_SAVE_START:											fsm_next_state = FSM_STATE_SAVE_WRITE;
			FSM_STATE_SAVE_WRITE:		if (s_addr_done)				fsm_next_state = FSM_STATE_SAVE_FINAL;
												else								fsm_next_state = FSM_STATE_SAVE_WRITE;
			FSM_STATE_SAVE_FINAL:											fsm_next_state = FSM_STATE_STOP;
			//
			FSM_STATE_STOP:													fsm_next_state = FSM_STATE_IDLE;
			//
		endcase
		//
	end


endmodule

//======================================================================
// End of file
//======================================================================
