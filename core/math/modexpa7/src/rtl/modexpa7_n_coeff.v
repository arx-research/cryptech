//======================================================================
//
// modexpa7_n_coeff.v
// -----------------------------------------------------------------------------
// Montgomery modulus-dependent coefficient calculation block.
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

module modexpa7_n_coeff #
	(
			//
			// This sets the address widths of memory buffers. Internal data
			// width is 32 bits, so for e.g. 2048-bit operands buffers must store
			// 2048 / 32 = 64 words, and these need 6-bit address bus, because
			// 2 ** 6 = 64.
			//
		parameter	OPERAND_ADDR_WIDTH = 6
	)
	(
		input											clk,						// clock
		input											rst_n,					// active-low reset

		input											ena,						// enable input
		output										rdy,						// ready output

		output	[OPERAND_ADDR_WIDTH-1:0]	n_bram_addr,			// modulus memory address
		output	[OPERAND_ADDR_WIDTH-1:0]	n_coeff_bram_addr,	// modulus coefficient memory address

		input		[                32-1:0]	n_bram_out,				// modulus memory output

		output	[                32-1:0]	n_coeff_bram_in,		// modulus coefficient memory input
		output										n_coeff_bram_wr,		// modulus coefficient memory write enable

		input		[OPERAND_ADDR_WIDTH-1:0]	n_num_words				// number of words in modulus
	);
	
		//
		// FSM Declaration
		//
	localparam	[ 7: 0]	FSM_STATE_IDLE		= 8'h00;
	
	localparam	[ 7: 0]	FSM_STATE_INIT_1	= 8'hA1;
	localparam	[ 7: 0]	FSM_STATE_INIT_2	= 8'hA2;
	localparam	[ 7: 0]	FSM_STATE_INIT_3	= 8'hA3;
	localparam	[ 7: 0]	FSM_STATE_INIT_4	= 8'hA4;
	localparam	[ 7: 0]	FSM_STATE_INIT_5	= 8'hA5;
	
	localparam	[ 7: 0]	FSM_STATE_CALC_1	= 8'hB1;
	localparam	[ 7: 0]	FSM_STATE_CALC_2	= 8'hB2;
	localparam	[ 7: 0]	FSM_STATE_CALC_3	= 8'hB3;
	localparam	[ 7: 0]	FSM_STATE_CALC_4	= 8'hB4;
	localparam	[ 7: 0]	FSM_STATE_CALC_5	= 8'hB5;
	
	localparam	[ 7: 0]	FSM_STATE_SAVE_1	= 8'hC1;
	localparam	[ 7: 0]	FSM_STATE_SAVE_2	= 8'hC2;
	localparam	[ 7: 0]	FSM_STATE_SAVE_3	= 8'hC3;
	localparam	[ 7: 0]	FSM_STATE_SAVE_4	= 8'hC4;
	localparam	[ 7: 0]	FSM_STATE_SAVE_5	= 8'hC5;

	localparam	[ 7: 0]	FSM_STATE_STOP		= 8'hFF;
	
	
		//
		// FSM State / Next State
		//
	reg	[ 7: 0]	fsm_state = FSM_STATE_IDLE;
	reg	[ 7: 0]	fsm_next_state;


		//
		// Enable Delay and Trigger
		//
   reg ena_dly = 1'b0;
	
		/* delay enable by one clock cycle */
   always @(posedge clk) ena_dly <= ena;

		/* trigger new operation when enable goes high */
   wire ena_trig = ena && !ena_dly;
	
	
		//
		// Ready Flag Logic
		//
	reg rdy_reg = 1'b1;
	assign rdy = rdy_reg;

   always @(posedge clk or negedge rst_n)
		
			/* reset flag */
		if (rst_n == 1'b0)						rdy_reg <= 1'b1;
		else begin
		
				/* clear flag when operation is started */
			if (fsm_state == FSM_STATE_IDLE)	rdy_reg <= ~ena_trig;
			
				/* set flag after operation is finished */
			if (fsm_state == FSM_STATE_STOP)	rdy_reg <= 1'b1;			
			
		end
		
		
		//
		// Parameters Latch
		//
	reg	[OPERAND_ADDR_WIDTH-1:0]	n_num_words_latch;

		/* save number of words in modulus when new operation starts*/
	always @(posedge clk)
		//
		if (fsm_next_state == FSM_STATE_INIT_1)
			n_num_words_latch <= n_num_words;

		
		//
		// Cycle Counters
		//
		
		/*
		 * Maybe we can cheat and skip calculation of entire T every time.
		 * During the first 32 cycles we only need the first word of T,
		 * during the following 64 cycles the secord word, etc. Needs
		 * further investigation...
		 *
		 */
		 
	reg	[OPERAND_ADDR_WIDTH+4:0]	cyc_cnt;
		
	wire	[OPERAND_ADDR_WIDTH+4:0]	cyc_cnt_zero = {{OPERAND_ADDR_WIDTH{1'b0}}, {5{1'b0}}};
	wire	[OPERAND_ADDR_WIDTH+4:0]	cyc_cnt_last = {n_num_words, 5'b11110};
	wire	[OPERAND_ADDR_WIDTH+4:0]	cyc_cnt_next = cyc_cnt + 1'b1;

		/* handy flag */
	wire	cyc_cnt_done = (cyc_cnt == cyc_cnt_last) ? 1'b1 : 1'b0;

	always @(posedge clk)
		//
		if (fsm_next_state == FSM_STATE_CALC_1)
			//
			case (fsm_state)
				FSM_STATE_INIT_5:	cyc_cnt <= cyc_cnt_zero;
				FSM_STATE_SAVE_5:	cyc_cnt <= !cyc_cnt_done ? cyc_cnt_next : cyc_cnt;
			endcase
			
			
		//
		// Handy Address Values
		//
		
		/* the very first address */
	wire	[OPERAND_ADDR_WIDTH-1:0]	bram_addr_zero = {OPERAND_ADDR_WIDTH{1'b0}};
	
		/* the very last address */
	wire	[OPERAND_ADDR_WIDTH-1:0]	bram_addr_last = n_num_words_latch;
		
		
		//
		// Block Memories
		//
		
		/*
		 * This module uses 8 block memories:
		 *
		 * N       - external input, stores modulus
		 * R       - internal, stores intermediate result
		 * B       - internal, stores current bit mask (see high-level algorithm)
		 * T       - internal, stores the product R * NN (see high-level algorithm)
		 * NN      - internal, stores the quantity ~N + 1 (see high-level algorithm)
		 * RR      - internal, stores a copy of R (see high-level algorithm)
		 * RB      - internal, stores the sum R + B (see high-level algorithm)
		 * N_COEFF - external output, stores the calculated modulus-depentent coefficient
		 *
		 */
		
	reg	[OPERAND_ADDR_WIDTH-1:0]	n_addr;
	reg	[OPERAND_ADDR_WIDTH-1:0]	r_addr;
	reg	[OPERAND_ADDR_WIDTH-1:0]	b_addr;
	reg	[OPERAND_ADDR_WIDTH-1:0]	t_addr;
	reg	[OPERAND_ADDR_WIDTH-1:0]	nn_addr;
	reg	[OPERAND_ADDR_WIDTH-1:0]	rr_addr;
	reg	[OPERAND_ADDR_WIDTH-1:0]	rb_addr;
	reg	[OPERAND_ADDR_WIDTH-1:0]	n_coeff_addr;
	
	reg	[31: 0]	r_data_in;
	reg	[31: 0]	b_data_in;
	reg	[31: 0]	t_data_in;
	reg	[31: 0]	nn_data_in;
	reg	[31: 0]	rr_data_in;
	reg	[31: 0]	rb_data_in;
	reg	[31: 0]	n_coeff_data_in;
	
	wire	[31: 0]	r_data_out;
	wire	[31: 0]	b_data_out;
	wire	[31: 0]	t_data_out;
	wire	[31: 0]	nn_data_out;
	wire	[31: 0]	rr_data_out;
	wire	[31: 0]	rb_data_out;
	
	reg	r_wren;
	reg	b_wren;
	reg	t_wren;
	reg	nn_wren;
	reg	rr_wren;
	reg	rb_wren;
	reg	n_coeff_wren;
		
	bram_1rw_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(OPERAND_ADDR_WIDTH))
	bram_r (.clk(clk), .a_addr(r_addr), .a_wr(r_wren), .a_in(r_data_in), .a_out(r_data_out));

	bram_1rw_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(OPERAND_ADDR_WIDTH))
	bram_b (.clk(clk), .a_addr(b_addr), .a_wr(b_wren), .a_in(b_data_in), .a_out(b_data_out));

	bram_1rw_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(OPERAND_ADDR_WIDTH))
	bram_nn (.clk(clk), .a_addr(nn_addr), .a_wr(nn_wren), .a_in(nn_data_in), .a_out(nn_data_out));		

	bram_1rw_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(OPERAND_ADDR_WIDTH))
	bram_t (.clk(clk), .a_addr(t_addr), .a_wr(t_wren), .a_in(t_data_in), .a_out(t_data_out));

	bram_1rw_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(OPERAND_ADDR_WIDTH))
	bram_rb (.clk(clk), .a_addr(rb_addr), .a_wr(rb_wren), .a_in(rb_data_in), .a_out(rb_data_out));

	bram_1rw_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(OPERAND_ADDR_WIDTH))
	bram_rr (.clk(clk), .a_addr(rr_addr), .a_wr(rr_wren), .a_in(rr_data_in), .a_out(rr_data_out));
			
		/* handy values */
	wire	[OPERAND_ADDR_WIDTH-1:0]	n_addr_next				= n_addr       + 1'b1;
	wire	[OPERAND_ADDR_WIDTH-1:0]	r_addr_next				= r_addr       + 1'b1;
	wire	[OPERAND_ADDR_WIDTH-1:0]	b_addr_next				= b_addr       + 1'b1;
	wire	[OPERAND_ADDR_WIDTH-1:0]	t_addr_next				= t_addr       + 1'b1;
	wire	[OPERAND_ADDR_WIDTH-1:0]	nn_addr_next			= nn_addr      + 1'b1;
	wire	[OPERAND_ADDR_WIDTH-1:0]	rr_addr_next			= rr_addr      + 1'b1;
	wire	[OPERAND_ADDR_WIDTH-1:0]	rb_addr_next			= rb_addr      + 1'b1;
	wire	[OPERAND_ADDR_WIDTH-1:0]	n_coeff_addr_next		= n_coeff_addr + 1'b1;
	
		/* handy flags */
	wire	n_addr_done				= (n_addr       == bram_addr_last) ? 1'b1 : 1'b0;
	wire	r_addr_done				= (r_addr       == bram_addr_last) ? 1'b1 : 1'b0;
	wire	b_addr_done				= (b_addr       == bram_addr_last) ? 1'b1 : 1'b0;
	wire	t_addr_done				= (t_addr       == bram_addr_last) ? 1'b1 : 1'b0;
	wire	nn_addr_done			= (nn_addr      == bram_addr_last) ? 1'b1 : 1'b0;	
	wire	rr_addr_done			= (rr_addr      == bram_addr_last) ? 1'b1 : 1'b0;
	wire	rb_addr_done			= (rb_addr      == bram_addr_last) ? 1'b1 : 1'b0;
	wire	n_coeff_addr_done		= (n_coeff_addr == bram_addr_last) ? 1'b1 : 1'b0;
	
		/* map top-level ports to internal registers */
	assign n_bram_addr			= n_addr;
	assign n_coeff_bram_addr	= n_coeff_addr;
	assign n_coeff_bram_in		= n_coeff_data_in;
	assign n_coeff_bram_wr		= n_coeff_wren;


		//
		// Delayed Flags
		//
	reg	rb_addr_done_dly;
	
		/* delay rb_addr_done flag by one clock cycle (used later) */
	always @(posedge clk) rb_addr_done_dly <= rb_addr_done;
	
	
		//
		// Adder1
		//
		
		/*
		 * This adder is used to calculate NN = ~N + 1.
		 *
		 */
	wire	[31: 0]	add1_s;					// sum output
	wire				add1_c_in;				// carry input
	reg				add1_b_lsb;				// B-input
	reg				add1_c_in_mask;		// flag to not carry anything into the very first word
	reg				add1_c_in_mask_dly;	// delayed carry masking flag
	wire				add1_c_out;				// carry output
	
		/* add masking into carry feedback chain */
	assign add1_c_in = add1_c_out & ~add1_c_in_mask;

		/* feed 1 into port B of adder */
	always @(posedge clk) add1_b_lsb <= (fsm_next_state == FSM_STATE_INIT_2) ? 1'b1 : 1'b0;

		/* mask carry for the very first word of N */
	always @(posedge clk) add1_c_in_mask <= (fsm_next_state == FSM_STATE_INIT_2) ? 1'b1 : 1'b0;

		/* delay carry masking flag by one clock cycle (used later) */
	always @(posedge clk) add1_c_in_mask_dly <= add1_c_in_mask;
	
	modexpa7_adder32 add1_inst
	(
		.clk		(clk),								//
		.ce		(1'b1),								//
		.a			(~n_bram_out),						// ~N
		.b			({{31{1'b0}}, add1_b_lsb}),	//  1
		.c_in		(add1_c_in),						//
		.s			(add1_s),							//
		.c_out	(add1_c_out)						//
	);
	
	
		//
		// Adder2
		//
		
		/*
		 * This adder is used to calculate RB = R + B.
		 *
		 */
	wire	[31: 0]	add2_s;			// sum output
	reg				add2_c_in;		// carry input
	wire				add2_c_out;		// carry output
			
	modexpa7_adder32 add2_inst
	(
		.clk		(clk),
		.ce		(1'b1),
		.a			(r_data_out),
		.b			(b_data_in),
		.c_in		(add2_c_in),
		.s			(add2_s),
		.c_out	(add2_c_out)
	);


		//
		// Multiplier
		//
		
		/*
		 * This multiplier is used to calculate T = R * NN.
		 *
		 */
		 
	reg	[31: 0]	pe_a;
	reg	[31: 0]	pe_b;
	reg	[31: 0]	pe_t;
	reg	[31: 0]	pe_c_in;
	wire	[31: 0]	pe_p;
	wire	[31: 0]	pe_c_out;
		
	modexpa7_systolic_pe pe_mul_inst
	(
		.clk		(clk),
		.a			(pe_a),
		.b			(pe_b),
		.t			(pe_t),
		.c_in		(pe_c_in),
		.p			(pe_p),
		.c_out	(pe_c_out)
	);


		//
		// Multiplier Latency Compensation Logic
		//
		
	localparam SYSTOLIC_PE_LATENCY = 4;
	
		/* shift register to match data propagation delay */
	reg [SYSTOLIC_PE_LATENCY:0] pe_latency;
	wire pe_latency_done = pe_latency[SYSTOLIC_PE_LATENCY];
	
		/* gradually fill the shift register with ones */
	always @(posedge clk)
		//
		if (fsm_state == FSM_STATE_CALC_1)
				pe_latency <= {1'b0, {SYSTOLIC_PE_LATENCY{1'b0}}};
		else	pe_latency <= {pe_latency[SYSTOLIC_PE_LATENCY-1:0], 1'b1};


		//
		// Adder2 Output Delay
		//
	reg	[31: 0]	add2_s_dly[1:SYSTOLIC_PE_LATENCY-1];
	reg				add2_c_out_dly[1:SYSTOLIC_PE_LATENCY+2];	

		/* delay sum */
	integer i;
	always @(posedge clk)
		//
		for (i=1; i<SYSTOLIC_PE_LATENCY; i=i+1)
			add2_s_dly[i] <= (i == 1) ? add2_s : add2_s_dly[i-1];
		
		/* delay adder carry */
	always @(posedge clk)
		//
		for (i=1; i<=(SYSTOLIC_PE_LATENCY+2); i=i+1)
			add2_c_out_dly[i] <= (i == 1) ? add2_c_out : add2_c_out_dly[i-1];

		/* adder carry feedback */
	always @(posedge clk)
		//
		if ((fsm_next_state == FSM_STATE_CALC_3) && (nn_addr == bram_addr_zero))
			add2_c_in <= (r_addr == bram_addr_zero) ? 1'b0 : add2_c_out_dly[SYSTOLIC_PE_LATENCY+2];
			
		//
		// Multiplier Output Delay
		//
	reg	[31: 0]	pe_c_out_dly[1:3];

	always @(posedge clk)
		//
		for (i=1; i<=3; i=i+1)
			pe_c_out_dly[i] <= (i == 1) ? pe_c_out : pe_c_out_dly[i-1];


		//
		// Multiplier Operand Loader
		//
	always @(posedge clk)
		//
		if (fsm_next_state == FSM_STATE_CALC_3) begin
			pe_a    <= r_data_out;
			pe_b    <= nn_data_out;
			pe_t    <= (nn_addr == bram_addr_zero) ? {32{1'b0}} : t_data_out;
			pe_c_in <= (r_addr  == bram_addr_zero) ? {32{1'b0}} : pe_c_out_dly[3];
		end else begin
			pe_a    <= {32{1'bX}};
			pe_b    <= {32{1'bX}};
			pe_t    <= {32{1'bX}};
			pe_c_in <= {32{1'bX}};		
		end
	
	
		//
		// B Shift Carry Logic
		//
		
		/*
		 * B value is repeatedly shifted to the left, so we need carry logic
		 * to save the MSB of the current output word and feed into the LSB
		 * of the next input word.
		 *
		 */
		 
	reg	b_data_out_carry;
	
	always @(posedge clk)
		//
		case (fsm_next_state)
		
				/* mask carry into the very first word */
			FSM_STATE_CALC_2:
				if ((nn_addr == bram_addr_zero) && (b_addr == bram_addr_zero))
					b_data_out_carry <= 1'b0;
					
				/* carry feedback */
			FSM_STATE_CALC_3:
				if (nn_addr == bram_addr_zero)
					b_data_out_carry <= b_data_out[31];
					
		endcase
		
		
		//
		// R Update Flag
		//
	reg	flag_update_r;
	
		/* indices of the target bit of T */
	wire	[                   4:0]	flag_addr_bit  = cyc_cnt_next[4:0];
	wire	[OPERAND_ADDR_WIDTH-1:0]	flag_addr_word	= cyc_cnt_next[OPERAND_ADDR_WIDTH+4:5];
	
		/* update flag when the target bit of T is available */
	always @(posedge clk)
		//
		if (t_wren && (t_addr == flag_addr_word))
			flag_update_r <= t_data_in[flag_addr_bit];
	
	
		//
		// Block Memory Address Logic
		//

	reg	[OPERAND_ADDR_WIDTH-1:0]	r_addr_calc1;
	reg	[OPERAND_ADDR_WIDTH-1:0]	b_addr_calc1;
	reg	[OPERAND_ADDR_WIDTH-1:0]	t_addr_calc1;
	reg	[OPERAND_ADDR_WIDTH-1:0]	nn_addr_calc1;
	reg	[OPERAND_ADDR_WIDTH-1:0]	rr_addr_calc1;
	reg	[OPERAND_ADDR_WIDTH-1:0]	rb_addr_calc1;
	
		/* how to update R duing CALC_1 state */
	always @*
		//
		if (fsm_state == FSM_STATE_INIT_5)					r_addr_calc1 <= bram_addr_zero;
		else begin
			if (r_addr < (n_num_words_latch - nn_addr))	r_addr_calc1 <= r_addr_next;
			else														r_addr_calc1 <= bram_addr_zero;
		end

		/* how to update B, RR, RB duing CALC_1 state */
	always @* begin
		//
		b_addr_calc1  = b_addr;
		rr_addr_calc1 = rr_addr;
		rb_addr_calc1 = rb_addr;
		//
		if ((fsm_state == FSM_STATE_INIT_5)	|| (fsm_state == FSM_STATE_SAVE_5)) begin
			//
			b_addr_calc1  = bram_addr_zero;
			rr_addr_calc1 = bram_addr_zero;
			rb_addr_calc1 = bram_addr_zero;
			//
		end else if (nn_addr == bram_addr_zero) begin
			//
			b_addr_calc1  = !b_addr_done  ? b_addr_next  : b_addr;
			rr_addr_calc1 = !rr_addr_done ? rr_addr_next : rr_addr;
			rb_addr_calc1 = !rb_addr_done ? rb_addr_next : rb_addr;
			//
		end
		//
	end

		/* how to update T duing CALC_1 state */
	always @*
		//
		if ((fsm_state == FSM_STATE_INIT_5) || (fsm_state == FSM_STATE_SAVE_5))
			t_addr_calc1 = bram_addr_zero;
		else begin
			if (r_addr == (n_num_words_latch - nn_addr))
				t_addr_calc1 = nn_addr_next;
			else
				t_addr_calc1 = t_addr_next;
		end

		/* how to update NN duing CALC_1 state */
	always @* begin
		//
		nn_addr_calc1 = nn_addr;
		//
		if ((fsm_state == FSM_STATE_INIT_5) || (fsm_state == FSM_STATE_SAVE_5))
			nn_addr_calc1 = bram_addr_zero;
		else if (r_addr == (n_num_words_latch - nn_addr))
			nn_addr_calc1 = nn_addr_next;
		//
	end


		//
		// Address Update Logic
		//
	always @(posedge clk) begin
		//
		// N
		//
		case (fsm_next_state)
			FSM_STATE_INIT_1:		n_addr <= bram_addr_zero;
			//
			FSM_STATE_INIT_2,
			FSM_STATE_INIT_3,
			FSM_STATE_INIT_4,
			FSM_STATE_INIT_5:		n_addr <= !n_addr_done ? n_addr_next : n_addr;
		endcase
		//
		// R
		//
		case (fsm_next_state)
			FSM_STATE_INIT_4:		r_addr <= bram_addr_zero;
			FSM_STATE_INIT_5:		r_addr <= r_addr_next;
			FSM_STATE_CALC_1:		r_addr <= r_addr_calc1;
			FSM_STATE_SAVE_3:		r_addr <= bram_addr_zero;
			//
			FSM_STATE_SAVE_4,
			FSM_STATE_SAVE_5:		r_addr <= r_addr_next;	
		endcase
		//
		// B
		//
		case (fsm_next_state)
			FSM_STATE_INIT_4:		b_addr <= bram_addr_zero;
			FSM_STATE_INIT_5:		b_addr <= b_addr_next;
			FSM_STATE_CALC_1:		b_addr <= b_addr_calc1;
		endcase
		//
		// T
		//
		case (fsm_next_state)			
			FSM_STATE_CALC_1:		t_addr <= t_addr_calc1;			
		endcase
		//
		// NN
		//
		case (fsm_next_state)
			FSM_STATE_INIT_4:		nn_addr <= bram_addr_zero;
			FSM_STATE_INIT_5:		nn_addr <= nn_addr_next;
			FSM_STATE_CALC_1:		nn_addr <= nn_addr_calc1;
		endcase
		//
		// RR
		//
		case (fsm_next_state)			
			FSM_STATE_CALC_1:		rr_addr <= rr_addr_calc1;
			FSM_STATE_SAVE_1:		rr_addr <= bram_addr_zero;
			//
			FSM_STATE_SAVE_2,
			FSM_STATE_SAVE_3,
			FSM_STATE_SAVE_4:		rr_addr <= !rr_addr_done ? rr_addr_next : rr_addr;	
		endcase		
		//
		// RB
		//
		case (fsm_next_state)			
			FSM_STATE_CALC_1:		rb_addr <= rb_addr_calc1;			
			FSM_STATE_SAVE_1:		rb_addr <= bram_addr_zero;
			//
			FSM_STATE_SAVE_2,
			FSM_STATE_SAVE_3,
			FSM_STATE_SAVE_4:		rb_addr <= !rb_addr_done ? rb_addr_next : rb_addr;
		endcase		
		//
		// N_COEFF
		//
		case (fsm_next_state)			
			FSM_STATE_SAVE_3:		n_coeff_addr <= bram_addr_zero;
			//
			FSM_STATE_SAVE_4,
			FSM_STATE_SAVE_5:		n_coeff_addr <= r_addr_next;
		endcase
		//
	end


		//
		// Block Memory Write Enable Logic
		//
	always @(posedge clk) begin
		//
		// R
		//
		case (fsm_next_state)
			FSM_STATE_INIT_4,
			FSM_STATE_INIT_5,
			FSM_STATE_SAVE_3,
			FSM_STATE_SAVE_4,
			FSM_STATE_SAVE_5:		r_wren <= 1'b1;
			default:					r_wren <= 1'b0;
		endcase
		//
		// B
		//
		case (fsm_next_state)			
			FSM_STATE_INIT_4,
			FSM_STATE_INIT_5:		b_wren <= 1'b1;
			FSM_STATE_CALC_3:		b_wren <= (nn_addr == bram_addr_zero) ? 1'b1 : 1'b0;
			default:					b_wren <= 1'b0;
		endcase
		//
		// T
		//
		case (fsm_next_state)			
			FSM_STATE_CALC_5:		t_wren <= 1'b1;
			default:					t_wren <= 1'b0;
		endcase
		//
		// NN
		//
		case (fsm_next_state)			
			FSM_STATE_INIT_4,
			FSM_STATE_INIT_5:		nn_wren <= 1'b1;
			default:					nn_wren <= 1'b0;
		endcase
		//
		// RR
		//
		case (fsm_next_state)
			FSM_STATE_CALC_5:		rr_wren <= (nn_addr == bram_addr_zero) ? 1'b1 : 1'b0;
			default:					rr_wren <= 1'b0;
		endcase
		//
		// RB
		//
		case (fsm_next_state)
			FSM_STATE_CALC_5:		rb_wren <= (nn_addr == bram_addr_zero) ? 1'b1 : 1'b0;
			default:					rb_wren <= 1'b0;
		endcase
		//
		// N_COEFF
		//
		case (fsm_next_state)
			FSM_STATE_SAVE_3,
			FSM_STATE_SAVE_4,
			FSM_STATE_SAVE_5:		n_coeff_wren <= cyc_cnt_done;
			default:					n_coeff_wren <= 1'b0;
		endcase
		//
	end
	
	
		//
		// Block Memory Input Logic
		//
	always @(posedge clk) begin
		//
		// R
		//
		case (fsm_next_state)
			FSM_STATE_INIT_4,
			FSM_STATE_INIT_5:		r_data_in <= {{31{1'b0}}, add1_c_in_mask_dly};
			//
			FSM_STATE_SAVE_3,
			FSM_STATE_SAVE_4,
			FSM_STATE_SAVE_5:		r_data_in <= flag_update_r ? rb_data_out : rr_data_out;
			default:					r_data_in <= {32{1'bX}};
		endcase
		//
		// B
		//
		case (fsm_next_state)
			FSM_STATE_INIT_4,
			FSM_STATE_INIT_5:		b_data_in <= {{31{1'b0}}, add1_c_in_mask_dly};
			FSM_STATE_CALC_3:		b_data_in <= (nn_addr == bram_addr_zero) ?
				{b_data_out[30:0], b_data_out_carry} : {32{1'bX}};
			default:					b_data_in <= {32{1'bX}};
		endcase
		//
		// T
		//
		case (fsm_next_state)
			FSM_STATE_CALC_5:		t_data_in <= pe_p;
			default:					t_data_in <= {32{1'bX}};
		endcase
		//
		// NN
		//
		case (fsm_next_state)
			FSM_STATE_INIT_4,
			FSM_STATE_INIT_5:		nn_data_in <= add1_s;
			default:					nn_data_in <= {32{1'bX}};
		endcase
		//
		// RR
		//
		case (fsm_next_state)
			FSM_STATE_CALC_5:		rr_data_in <= r_data_out;
			default:					rr_data_in <= {32{1'bX}};
		endcase
		//
		// RB
		//
		case (fsm_next_state)
			FSM_STATE_CALC_5:		rb_data_in <= add2_s_dly[SYSTOLIC_PE_LATENCY-1];
			default:					rb_data_in <= {32{1'bX}};
		endcase
		//
		// N_COEFF
		//
		case (fsm_next_state)
			FSM_STATE_SAVE_3,
			FSM_STATE_SAVE_4,
			FSM_STATE_SAVE_5:		n_coeff_data_in <= flag_update_r ? rb_data_out : rr_data_out;
			default:					n_coeff_data_in <= {32{1'bX}};
		endcase
		//
	end

	
		//
		// FSM Process
		//
	always @(posedge clk or negedge rst_n)
		//
		if (rst_n == 1'b0)	fsm_state <= FSM_STATE_IDLE;
		else						fsm_state <= fsm_next_state;
	
	
		//
		// FSM Transition Logic
		//
	always @* begin
		//
		fsm_next_state = FSM_STATE_STOP;
		//
		case (fsm_state)

			FSM_STATE_IDLE:		if (ena_trig)				fsm_next_state = FSM_STATE_INIT_1;
										else							fsm_next_state = FSM_STATE_IDLE;
												
			FSM_STATE_INIT_1:										fsm_next_state = FSM_STATE_INIT_2;
			FSM_STATE_INIT_2:										fsm_next_state = FSM_STATE_INIT_3;
			FSM_STATE_INIT_3:										fsm_next_state = FSM_STATE_INIT_4;
			FSM_STATE_INIT_4:										fsm_next_state = FSM_STATE_INIT_5;
			FSM_STATE_INIT_5:		if (nn_addr_done)			fsm_next_state = FSM_STATE_CALC_1;
										else							fsm_next_state = FSM_STATE_INIT_5;

			FSM_STATE_CALC_1:										fsm_next_state = FSM_STATE_CALC_2;
			FSM_STATE_CALC_2:										fsm_next_state = FSM_STATE_CALC_3;
			FSM_STATE_CALC_3:										fsm_next_state = FSM_STATE_CALC_4;
			FSM_STATE_CALC_4:		if (pe_latency_done)		fsm_next_state = FSM_STATE_CALC_5;
										else							fsm_next_state = FSM_STATE_CALC_4;
			FSM_STATE_CALC_5:		if (nn_addr_done)			fsm_next_state = FSM_STATE_SAVE_1;
										else							fsm_next_state = FSM_STATE_CALC_1;
			
			FSM_STATE_SAVE_1:										fsm_next_state = FSM_STATE_SAVE_2;
			FSM_STATE_SAVE_2:										fsm_next_state = FSM_STATE_SAVE_3;
			FSM_STATE_SAVE_3:										fsm_next_state = FSM_STATE_SAVE_4;
			FSM_STATE_SAVE_4:		if (rb_addr_done_dly)	fsm_next_state = FSM_STATE_SAVE_5;
										else							fsm_next_state = FSM_STATE_SAVE_4;
			FSM_STATE_SAVE_5:		if (cyc_cnt_done)			fsm_next_state = FSM_STATE_STOP;
										else							fsm_next_state = FSM_STATE_CALC_1;
			
			FSM_STATE_STOP:										fsm_next_state = FSM_STATE_IDLE;

		endcase
	end


endmodule

//======================================================================
// End of file
//======================================================================
