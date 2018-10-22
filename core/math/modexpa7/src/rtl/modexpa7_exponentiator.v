//======================================================================
//
// modexpa7_exponentiator.v
// -----------------------------------------------------------------------------
// Modular Montgomery Exponentiator.
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

module modexpa7_exponentiator #
	(
			//
			// This sets the address widths of memory buffers. Internal data
			// width is 32 bits, so for e.g. 2048-bit operands buffers must store
			// 2048 / 32 = 64 words, and these need 5-bit address bus, because
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
		
		input											crt,
		
		output	[OPERAND_ADDR_WIDTH-1:0]	m_bram_addr,
		output	[OPERAND_ADDR_WIDTH-1:0]	d_bram_addr,
		output	[OPERAND_ADDR_WIDTH-1:0]	f_bram_addr,
		output	[OPERAND_ADDR_WIDTH-1:0]	n1_bram_addr,
		output	[OPERAND_ADDR_WIDTH-1:0]	n2_bram_addr,
		output	[OPERAND_ADDR_WIDTH-1:0]	n_coeff1_bram_addr,
		output	[OPERAND_ADDR_WIDTH-1:0]	n_coeff2_bram_addr,
		output	[OPERAND_ADDR_WIDTH-1:0]	r_bram_addr,

		input		[                32-1:0]	m_bram_out,
		input		[                32-1:0]	d_bram_out,
		input		[                32-1:0]	f_bram_out,
		input		[                32-1:0]	n1_bram_out,
		input		[                32-1:0]	n2_bram_out,
		input		[                32-1:0]	n_coeff1_bram_out,
		input		[                32-1:0]	n_coeff2_bram_out,

		output	[                32-1:0]	r_bram_in,
		output										r_bram_wr,

		input		[OPERAND_ADDR_WIDTH-1:0]	m_num_words,
		input		[OPERAND_ADDR_WIDTH+4:0]	d_num_bits
	);
	
	
		//
		// FSM Declaration
		//
	localparam	[ 7: 0]	FSM_STATE_EXP_IDLE		= 8'h00;
	//
	localparam	[ 7: 0]	FSM_STATE_EXP_INIT_1		= 8'hA1;
	localparam	[ 7: 0]	FSM_STATE_EXP_INIT_2		= 8'hA2;
	localparam	[ 7: 0]	FSM_STATE_EXP_INIT_3		= 8'hA3;
	localparam	[ 7: 0]	FSM_STATE_EXP_INIT_4		= 8'hA4;

	localparam	[ 7: 0]	FSM_STATE_EXP_LOAD_1		= 8'hB1;
	localparam	[ 7: 0]	FSM_STATE_EXP_LOAD_2		= 8'hB2;
	localparam	[ 7: 0]	FSM_STATE_EXP_LOAD_3		= 8'hB3;
	localparam	[ 7: 0]	FSM_STATE_EXP_LOAD_4		= 8'hB4;

	localparam	[ 7: 0]	FSM_STATE_EXP_CALC_1		= 8'hC1;
	localparam	[ 7: 0]	FSM_STATE_EXP_CALC_2		= 8'hC2;
	localparam	[ 7: 0]	FSM_STATE_EXP_CALC_3		= 8'hC3;

	localparam	[ 7: 0]	FSM_STATE_EXP_FILL_1		= 8'hD1;
	localparam	[ 7: 0]	FSM_STATE_EXP_FILL_2		= 8'hD2;
	localparam	[ 7: 0]	FSM_STATE_EXP_FILL_3		= 8'hD3;
	localparam	[ 7: 0]	FSM_STATE_EXP_FILL_4		= 8'hD4;

	localparam	[ 7: 0]	FSM_STATE_EXP_NEXT		= 8'hE0;

	localparam	[ 7: 0]	FSM_STATE_EXP_SAVE_1		= 8'hF1;
	localparam	[ 7: 0]	FSM_STATE_EXP_SAVE_2		= 8'hF2;
	localparam	[ 7: 0]	FSM_STATE_EXP_SAVE_3		= 8'hF3;
	localparam	[ 7: 0]	FSM_STATE_EXP_SAVE_4		= 8'hF4;
	//
	localparam	[ 7: 0]	FSM_STATE_MUL_INIT_1		= 8'h11;
	localparam	[ 7: 0]	FSM_STATE_MUL_INIT_2		= 8'h12;
	localparam	[ 7: 0]	FSM_STATE_MUL_INIT_3		= 8'h13;
	localparam	[ 7: 0]	FSM_STATE_MUL_INIT_4		= 8'h14;

	localparam	[ 7: 0]	FSM_STATE_MUL_CALC_1		= 8'h21;
	localparam	[ 7: 0]	FSM_STATE_MUL_CALC_2		= 8'h22;
	localparam	[ 7: 0]	FSM_STATE_MUL_CALC_3		= 8'h23;
	//
	localparam	[ 7: 0]	FSM_STATE_CRT_INIT_A_1	= 8'h31;
	localparam	[ 7: 0]	FSM_STATE_CRT_INIT_A_2	= 8'h32;
	localparam	[ 7: 0]	FSM_STATE_CRT_INIT_A_3	= 8'h33;
	localparam	[ 7: 0]	FSM_STATE_CRT_INIT_A_4	= 8'h34;

	localparam	[ 7: 0]	FSM_STATE_CRT_CALC_A_1	= 8'h41;
	localparam	[ 7: 0]	FSM_STATE_CRT_CALC_A_2	= 8'h42;
	localparam	[ 7: 0]	FSM_STATE_CRT_CALC_A_3	= 8'h43;
	//
	localparam	[ 7: 0]	FSM_STATE_CRT_INIT_B_1	= 8'h51;
	localparam	[ 7: 0]	FSM_STATE_CRT_INIT_B_2	= 8'h52;
	localparam	[ 7: 0]	FSM_STATE_CRT_INIT_B_3	= 8'h53;
	localparam	[ 7: 0]	FSM_STATE_CRT_INIT_B_4	= 8'h54;

	localparam	[ 7: 0]	FSM_STATE_CRT_CALC_B_1	= 8'h61;
	localparam	[ 7: 0]	FSM_STATE_CRT_CALC_B_2	= 8'h62;
	localparam	[ 7: 0]	FSM_STATE_CRT_CALC_B_3	= 8'h63;
	//
	localparam	[ 7: 0]	FSM_STATE_CRT_INIT_C_1	= 8'h71;
	localparam	[ 7: 0]	FSM_STATE_CRT_INIT_C_2	= 8'h72;
	localparam	[ 7: 0]	FSM_STATE_CRT_INIT_C_3	= 8'h73;
	localparam	[ 7: 0]	FSM_STATE_CRT_INIT_C_4	= 8'h74;

	localparam	[ 7: 0]	FSM_STATE_CRT_CALC_C_1	= 8'h81;
	localparam	[ 7: 0]	FSM_STATE_CRT_CALC_C_2	= 8'h82;
	localparam	[ 7: 0]	FSM_STATE_CRT_CALC_C_3	= 8'h83;
	//
	localparam	[ 7: 0]	FSM_STATE_EXP_STOP		= 8'hFF;


	/*
	 *  //
	 *
	 *  MUL_INIT:		P1 <= F
	 *             	P2 <= F
	 *             	P3 <= F
	 *            		T2 <= M
	 *
	 *  MUL_CALC:		TP = T2 * P3
	 *
	 *  //
	 *
	 *  CRT_INIT_A:	T2 <= M
	 *
	 *  CRT_CALC_A:	TP = T2 * P3 ("reduce only")
	 *
	 *  CRT_INIT_B:  	P1 <= F
	 *						P2 <= F
	 *						P3 <= F
	 *						T2 <= TP
	 *
	 *  CRT_CALC_B:	TP = T2 * P3
	 *
	 *  CRT_INIT_C:  	T2 <= TP
	 *
	 *  CRT_CALC_C:	TP = T2 * P3
	 *
	 *  //
	 *
	 *  EXP_INIT:		P1 <= TP
	 *						P2 <= TP
	 *						P3 <= TP
	 *						T1 <= 1
	 *						T2 <= 1
	 *
	 *  EXP_LOAD:		T0 <= T1
	 *
	 *  EXP_CALC:		PP = P1 * P2
	 *						TP = T2 * P3
	 *
	 *  EXP_FILL:		P1 <= PP
	 *						P2 <= PP
	 *						P3 <= PP
	 *						T1 <= D[i] ? TP : T0
	 *						T2 <= D[i] ? TP : T0
	 *
	 *  EXP_SAVE:		R  <=  T1
	 *
	 *  //
	 *
	 */

	
		//
		// FSM State / Next State
		//
	reg	[ 7: 0]	fsm_state = FSM_STATE_EXP_IDLE;
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
		if (rst_n == 1'b0) rdy_reg <= 1'b1;
		else begin
		
				/* clear flag when operation is started */
			if (fsm_state == FSM_STATE_EXP_IDLE)	rdy_reg <= ~ena_trig;
			
				/* set flag after operation is finished */
			if (fsm_state == FSM_STATE_EXP_STOP)	rdy_reg <= 1'b1;			
			
		end
		
		
		//
		// Parameters Latch
		//
	reg	[OPERAND_ADDR_WIDTH-1:0]	m_num_words_latch;
	reg	[OPERAND_ADDR_WIDTH+4:0]	d_num_bits_latch;

		/* save number of words in a and b when new operation starts */
	always @(posedge clk)
		//
		if ((fsm_state == FSM_STATE_EXP_IDLE) && ena_trig)
			{m_num_words_latch, d_num_bits_latch} <= {m_num_words, d_num_bits};
			

		//
		// Block Memory Addresses
		//
		
		/*
		 * Explain what every memory does.
		 *
		 */
		
		/* the very first addresses */
	wire	[OPERAND_ADDR_WIDTH-1:0]	bram_addr_zero			= {{OPERAND_ADDR_WIDTH{1'b0}}};
	
		/* the very last addresses */
	wire	[OPERAND_ADDR_WIDTH-1:0]	bram_addr_last			= {m_num_words_latch};
	wire	[OPERAND_ADDR_WIDTH-1:0]	bram_addr_last_crt	=
		{m_num_words_latch[OPERAND_ADDR_WIDTH-2:0], 1'b1};

		/* address registers */
	reg	[OPERAND_ADDR_WIDTH-1:0]	m_addr;
	reg	[OPERAND_ADDR_WIDTH-1:0]	d_addr;
	reg	[OPERAND_ADDR_WIDTH-1:0]	f_addr;
	reg	[OPERAND_ADDR_WIDTH-1:0]	r_addr;
	reg	[OPERAND_ADDR_WIDTH-1:0]	t0_addr;
	reg	[OPERAND_ADDR_WIDTH-1:0]	t1_addr;
	reg	[OPERAND_ADDR_WIDTH-1:0]	t2_addr_wr;
	wire	[OPERAND_ADDR_WIDTH-1:0]	t2_addr_rd;
	reg	[OPERAND_ADDR_WIDTH-1:0]	p_addr_wr;
	wire	[OPERAND_ADDR_WIDTH-1:0]	p1_addr_rd;
	wire	[OPERAND_ADDR_WIDTH-1:0]	p2_addr_rd;
	wire	[OPERAND_ADDR_WIDTH-1:0]	p3_addr_rd;
	wire	[OPERAND_ADDR_WIDTH-1:0]	pp_addr_wr;
	reg	[OPERAND_ADDR_WIDTH-1:0]	pp_addr_rd;
	wire	[OPERAND_ADDR_WIDTH-1:0]	tp_addr_wr;
	reg	[OPERAND_ADDR_WIDTH-1:0]	tp_addr_rd;
		
		/* handy increment values */
	wire	[OPERAND_ADDR_WIDTH-1:0]	m_addr_next			= m_addr + 1'b1;
	wire	[OPERAND_ADDR_WIDTH-1:0]	d_addr_next			= d_addr + 1'b1;
	wire	[OPERAND_ADDR_WIDTH-1:0]	f_addr_next			= f_addr + 1'b1;
	wire	[OPERAND_ADDR_WIDTH-1:0]	r_addr_next			= r_addr + 1'b1;
	wire	[OPERAND_ADDR_WIDTH-1:0]	t0_addr_next		= t0_addr + 1'b1;
	wire	[OPERAND_ADDR_WIDTH-1:0]	t1_addr_next		= t1_addr + 1'b1;
	wire	[OPERAND_ADDR_WIDTH-1:0]	t2_addr_wr_next	= t2_addr_wr + 1'b1;
	wire	[OPERAND_ADDR_WIDTH-1:0]	p_addr_wr_next		= p_addr_wr + 1'b1;
	wire	[OPERAND_ADDR_WIDTH-1:0]	pp_addr_rd_next	= pp_addr_rd + 1'b1;
	wire	[OPERAND_ADDR_WIDTH-1:0]	tp_addr_rd_next	= tp_addr_rd + 1'b1;
	
		/* handy stop flags */
	wire	m_addr_done				= (m_addr     == bram_addr_last)     ? 1'b1 : 1'b0;
	wire	m_addr_done_crt		= (m_addr     == bram_addr_last_crt) ? 1'b1 : 1'b0;
	wire	d_addr_done				= (d_addr     == bram_addr_last)     ? 1'b1 : 1'b0;
	wire	f_addr_done				= (f_addr     == bram_addr_last)     ? 1'b1 : 1'b0;
	wire	r_addr_done				= (r_addr     == bram_addr_last)     ? 1'b1 : 1'b0;
	wire	t0_addr_done			= (t0_addr    == bram_addr_last)     ? 1'b1 : 1'b0;
	wire	t1_addr_done			= (t1_addr    == bram_addr_last)     ? 1'b1 : 1'b0;
	wire	t2_addr_wr_done		= (t2_addr_wr == bram_addr_last)     ? 1'b1 : 1'b0;
	wire	t2_addr_wr_done_crt	= (t2_addr_wr == bram_addr_last_crt) ? 1'b1 : 1'b0;
	wire	p_addr_wr_done			= (p_addr_wr  == bram_addr_last)     ? 1'b1 : 1'b0;
	wire	pp_addr_rd_done		= (pp_addr_rd == bram_addr_last)     ? 1'b1 : 1'b0;
	wire	tp_addr_rd_done		= (tp_addr_rd == bram_addr_last)     ? 1'b1 : 1'b0;
				
		/* map registers to top-level ports */
	assign m_bram_addr = m_addr;
	assign d_bram_addr = d_addr;
	assign f_bram_addr = f_addr;
	assign r_bram_addr = r_addr;
	
		//
		// Internal Memories
		//

		/* memory inputs */
	reg	[31: 0]	r_data_in;
	reg	[31: 0]	t0_data_in;
	reg	[31: 0]	t1_data_in;
	reg	[31: 0]	t2_data_in;
	reg	[31: 0]	p_data_in;
	wire	[31: 0]	pp_data_in;
	wire	[31: 0]	tp_data_in;

		/* memory outputs */
	wire	[31: 0]	t0_data_out;
	wire	[31: 0]	t1_data_out;
	wire	[31: 0]	t2_data_out;
	wire	[31: 0]	p1_data_out;
	wire	[31: 0]	p2_data_out;
	wire	[31: 0]	p3_data_out;
	wire	[31: 0]	pp_data_out;
	wire	[31: 0]	tp_data_out;

		/* write enables */
	reg	r_wren;
	reg	t0_wren;
	reg	t1_wren;
	reg	t2_wren;
	reg	p_wren;
	wire	pp_wren;
	wire	tp_wren;
	
		/* map */
	assign r_bram_in = r_data_in;
	assign r_bram_wr = r_wren;

	bram_1rw_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(OPERAND_ADDR_WIDTH))
	bram_t0 (.clk(clk), .a_addr(t0_addr), .a_wr(t0_wren), .a_in(t0_data_in), .a_out(t0_data_out));

	bram_1rw_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(OPERAND_ADDR_WIDTH))
	bram_t1 (.clk(clk), .a_addr(t1_addr), .a_wr(t1_wren), .a_in(t1_data_in), .a_out(t1_data_out));
	
	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(OPERAND_ADDR_WIDTH))
	bram_t2 (.clk(clk),
		.a_addr(t2_addr_wr), .a_wr(t2_wren), .a_in(t2_data_in), .a_out(),
		.b_addr(t2_addr_rd), .b_out(t2_data_out));

	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(OPERAND_ADDR_WIDTH))
	bram_p1 (.clk(clk),
		.a_addr(p_addr_wr), .a_wr(p_wren), .a_in(p_data_in), .a_out(),
		.b_addr(p1_addr_rd), .b_out(p1_data_out));

	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(OPERAND_ADDR_WIDTH))
	bram_p2 (.clk(clk),
		.a_addr(p_addr_wr), .a_wr(p_wren), .a_in(p_data_in), .a_out(),
		.b_addr(p2_addr_rd), .b_out(p2_data_out));

	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(OPERAND_ADDR_WIDTH))
	bram_p3 (.clk(clk),
		.a_addr(p_addr_wr), .a_wr(p_wren), .a_in(p_data_in), .a_out(),
		.b_addr(p3_addr_rd), .b_out(p3_data_out));

	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(OPERAND_ADDR_WIDTH))
	bram_pp (.clk(clk),
		.a_addr(pp_addr_wr), .a_wr(pp_wren), .a_in(pp_data_in), .a_out(),
		.b_addr(pp_addr_rd), .b_out(pp_data_out));

	bram_1rw_1ro_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(OPERAND_ADDR_WIDTH))
	bram_tp (.clk(clk),
		.a_addr(tp_addr_wr), .a_wr(tp_wren), .a_in(tp_data_in), .a_out(),
		.b_addr(tp_addr_rd), .b_out(tp_data_out));


		//
		// Bit Counter
		//
	reg	[OPERAND_ADDR_WIDTH+4:0]	bit_cnt;
		
	wire	[OPERAND_ADDR_WIDTH+4:0]	bit_cnt_zero = {{OPERAND_ADDR_WIDTH{1'b0}}, {5{1'b0}}};
	wire	[OPERAND_ADDR_WIDTH+4:0]	bit_cnt_last = d_num_bits_latch;
	wire	[OPERAND_ADDR_WIDTH+4:0]	bit_cnt_next = bit_cnt + 1'b1;

		/* handy flag */
	wire	bit_cnt_done = (bit_cnt == bit_cnt_last) ? 1'b1 : 1'b0;
	
	always @(posedge clk)
		//
		if (fsm_next_state == FSM_STATE_EXP_LOAD_1)
			//
			case (fsm_state)
				FSM_STATE_EXP_INIT_4: bit_cnt <= bit_cnt_zero;
				FSM_STATE_EXP_NEXT:   bit_cnt <= !bit_cnt_done ? bit_cnt_next : bit_cnt;
			endcase


		//
		// Flags
		//
	reg	flag_update_r;

	always @(posedge clk)
		//
		if (fsm_next_state == FSM_STATE_EXP_CALC_3)
			flag_update_r <= d_bram_out[bit_cnt[4:0]];
			

		//
		// Memory Address Control Logic
		//
	always @(posedge clk) begin
		//
		// m_addr
		//
		case (fsm_next_state)
			FSM_STATE_MUL_INIT_1: 		m_addr <= bram_addr_zero;
			FSM_STATE_MUL_INIT_2,
			FSM_STATE_MUL_INIT_3,
			FSM_STATE_MUL_INIT_4:		m_addr <= !m_addr_done ? m_addr_next : m_addr;
			//
			FSM_STATE_CRT_INIT_A_1: 	m_addr <= bram_addr_zero;
			FSM_STATE_CRT_INIT_A_2,
			FSM_STATE_CRT_INIT_A_3,
			FSM_STATE_CRT_INIT_A_4:		m_addr <= !m_addr_done_crt ? m_addr_next : m_addr;
		endcase
		//
		// d_addr
		//
		case (fsm_next_state)
			FSM_STATE_EXP_CALC_1:		d_addr <= bit_cnt[OPERAND_ADDR_WIDTH+4:5];
		endcase
		//
		// f_addr
		//
		case (fsm_next_state)
			FSM_STATE_MUL_INIT_1: 		f_addr <= bram_addr_zero;
			FSM_STATE_MUL_INIT_2,
			FSM_STATE_MUL_INIT_3,
			FSM_STATE_MUL_INIT_4:		f_addr <= !f_addr_done ? f_addr_next : f_addr;
			//
			FSM_STATE_CRT_INIT_B_1: 	f_addr <= bram_addr_zero;
			FSM_STATE_CRT_INIT_B_2,
			FSM_STATE_CRT_INIT_B_3,
			FSM_STATE_CRT_INIT_B_4:		f_addr <= !f_addr_done ? f_addr_next : f_addr;
			//
		endcase
		//
		// r_addr
		//
		case (fsm_next_state)
			FSM_STATE_EXP_SAVE_3:		r_addr <= bram_addr_zero;
			FSM_STATE_EXP_SAVE_4:		r_addr <= r_addr_next;
		endcase
		//
		// p_addr_wr
		//
		case (fsm_next_state)
			//
			FSM_STATE_MUL_INIT_3:		p_addr_wr <= bram_addr_zero;
			FSM_STATE_MUL_INIT_4:		p_addr_wr <= p_addr_wr_next;
			//
			FSM_STATE_CRT_INIT_B_3:		p_addr_wr <= bram_addr_zero;
			FSM_STATE_CRT_INIT_B_4:		p_addr_wr <= p_addr_wr_next;
			//
			FSM_STATE_EXP_INIT_3:		p_addr_wr <= bram_addr_zero;
			FSM_STATE_EXP_INIT_4:		p_addr_wr <= p_addr_wr_next;
			//
			FSM_STATE_EXP_FILL_3:		p_addr_wr <= bram_addr_zero;
			FSM_STATE_EXP_FILL_4:		p_addr_wr <= p_addr_wr_next;
		endcase
		//
		// t0_addr
		//
		case (fsm_next_state)
			FSM_STATE_EXP_LOAD_3:		t0_addr <= bram_addr_zero;
			FSM_STATE_EXP_LOAD_4:		t0_addr <= t0_addr_next;
			//
			FSM_STATE_EXP_FILL_1:		t0_addr <= bram_addr_zero;
			FSM_STATE_EXP_FILL_2,
			FSM_STATE_EXP_FILL_3,
			FSM_STATE_EXP_FILL_4:		t0_addr <= !t0_addr_done ? t0_addr_next : t0_addr;
		endcase		
		//
		// t1_addr
		//
		case (fsm_next_state)
			FSM_STATE_EXP_INIT_3:		t1_addr <= bram_addr_zero;
			FSM_STATE_EXP_INIT_4:		t1_addr <= t1_addr_next;
			//
			FSM_STATE_EXP_LOAD_1:		t1_addr <= bram_addr_zero;
			FSM_STATE_EXP_LOAD_2,
			FSM_STATE_EXP_LOAD_3,
			FSM_STATE_EXP_LOAD_4:		t1_addr <= !t1_addr_done ? t1_addr_next : t1_addr;
			//
			FSM_STATE_EXP_FILL_3:		t1_addr <= bram_addr_zero;
			FSM_STATE_EXP_FILL_4:		t1_addr <= t1_addr_next;
			//
			FSM_STATE_EXP_SAVE_1:		t1_addr <= bram_addr_zero;
			FSM_STATE_EXP_SAVE_2,
			FSM_STATE_EXP_SAVE_3,
			FSM_STATE_EXP_SAVE_4:		t1_addr <= !t1_addr_done ? t1_addr_next : t1_addr;
		endcase
		//
		// t2_addr_wr
		//
		case (fsm_next_state)
			//
			FSM_STATE_MUL_INIT_3:		t2_addr_wr <= bram_addr_zero;
			FSM_STATE_MUL_INIT_4:		t2_addr_wr <= t2_addr_wr_next;
			//
			FSM_STATE_CRT_INIT_A_3:		t2_addr_wr <= bram_addr_zero;
			FSM_STATE_CRT_INIT_A_4:		t2_addr_wr <= t2_addr_wr_next;
			//
			FSM_STATE_CRT_INIT_B_3:		t2_addr_wr <= bram_addr_zero;
			FSM_STATE_CRT_INIT_B_4:		t2_addr_wr <= t2_addr_wr_next;
			//
			FSM_STATE_CRT_INIT_C_3:		t2_addr_wr <= bram_addr_zero;
			FSM_STATE_CRT_INIT_C_4:		t2_addr_wr <= t2_addr_wr_next;
			//
			FSM_STATE_EXP_INIT_3:		t2_addr_wr <= bram_addr_zero;
			FSM_STATE_EXP_INIT_4:		t2_addr_wr <= t2_addr_wr_next;
			//
			FSM_STATE_EXP_FILL_3:		t2_addr_wr <= bram_addr_zero;
			FSM_STATE_EXP_FILL_4:		t2_addr_wr <= t2_addr_wr_next;
		endcase		
		//
		// pp_addr_rd
		//
		case (fsm_next_state)
			FSM_STATE_EXP_FILL_1:		pp_addr_rd <= bram_addr_zero;
			FSM_STATE_EXP_FILL_2,
			FSM_STATE_EXP_FILL_3,
			FSM_STATE_EXP_FILL_4:		pp_addr_rd <= !pp_addr_rd_done ? pp_addr_rd_next : pp_addr_rd;
		endcase
		//
		// tp_addr_rd
		//
		case (fsm_next_state)
			FSM_STATE_EXP_INIT_1: 		tp_addr_rd <= bram_addr_zero;
			FSM_STATE_EXP_INIT_2,
			FSM_STATE_EXP_INIT_3,
			FSM_STATE_EXP_INIT_4:		tp_addr_rd <= !tp_addr_rd_done ? tp_addr_rd_next : tp_addr_rd;
			//
			FSM_STATE_CRT_INIT_B_1: 	tp_addr_rd <= bram_addr_zero;
			FSM_STATE_CRT_INIT_B_2,
			FSM_STATE_CRT_INIT_B_3,
			FSM_STATE_CRT_INIT_B_4:		tp_addr_rd <= !tp_addr_rd_done ? tp_addr_rd_next : tp_addr_rd;
			//
			FSM_STATE_CRT_INIT_C_1: 	tp_addr_rd <= bram_addr_zero;
			FSM_STATE_CRT_INIT_C_2,
			FSM_STATE_CRT_INIT_C_3,
			FSM_STATE_CRT_INIT_C_4:		tp_addr_rd <= !tp_addr_rd_done ? tp_addr_rd_next : tp_addr_rd;
			//
			FSM_STATE_EXP_FILL_1:		tp_addr_rd <= bram_addr_zero;
			FSM_STATE_EXP_FILL_2,
			FSM_STATE_EXP_FILL_3,
			FSM_STATE_EXP_FILL_4:		tp_addr_rd <= !tp_addr_rd_done ? tp_addr_rd_next : tp_addr_rd;
		endcase
		//
	end


		//
		// Memory Write Enable Logic
		//
	always @(posedge clk) begin
		//
		// r_wren
		//
		case (fsm_next_state)
			FSM_STATE_EXP_SAVE_3,
			FSM_STATE_EXP_SAVE_4:		r_wren <= 1'b1;
			default:							r_wren <= 1'b0;
		endcase
		//
		// p_wren
		//
		case (fsm_next_state)
			FSM_STATE_MUL_INIT_3,
			FSM_STATE_MUL_INIT_4,
			FSM_STATE_CRT_INIT_B_3,
			FSM_STATE_CRT_INIT_B_4,
			FSM_STATE_EXP_INIT_3,
			FSM_STATE_EXP_INIT_4,
			FSM_STATE_EXP_FILL_3,
			FSM_STATE_EXP_FILL_4:		p_wren <= 1'b1;
			default:							p_wren <= 1'b0;
		endcase
		//
		// t0_wren
		//
		case (fsm_next_state)
			FSM_STATE_EXP_LOAD_3,		
			FSM_STATE_EXP_LOAD_4:		t0_wren <= 1'b1;
			default:							t0_wren <= 1'b0;
		endcase
		//
		// t1_wren
		//
		case (fsm_next_state)
			FSM_STATE_EXP_INIT_3,		
			FSM_STATE_EXP_INIT_4,
			FSM_STATE_EXP_FILL_3,
			FSM_STATE_EXP_FILL_4:		t1_wren <= 1'b1;
			default:							t1_wren <= 1'b0;
		endcase
		//
		// t2_wren
		//
		case (fsm_next_state)
			FSM_STATE_MUL_INIT_3,
			FSM_STATE_MUL_INIT_4,
			FSM_STATE_CRT_INIT_A_3,
			FSM_STATE_CRT_INIT_A_4,
			FSM_STATE_CRT_INIT_B_3,
			FSM_STATE_CRT_INIT_B_4,
			FSM_STATE_CRT_INIT_C_3,
			FSM_STATE_CRT_INIT_C_4,
			FSM_STATE_EXP_INIT_3,		
			FSM_STATE_EXP_INIT_4,
			FSM_STATE_EXP_FILL_3,
			FSM_STATE_EXP_FILL_4:		t2_wren <= 1'b1;
			default:							t2_wren <= 1'b0;
		endcase
		//
	end
	
	
		//
		// Memory Input Selector
		//
	always @(posedge clk) begin
		//
		// r_data_in
		//
		case (fsm_next_state)
			FSM_STATE_EXP_SAVE_3,
			FSM_STATE_EXP_SAVE_4:		r_data_in	<= t1_data_out;
			default:							r_data_in	<= 32'dX;
		endcase		
		//
		// p_data_in
		//
		case (fsm_next_state)
			//
			FSM_STATE_MUL_INIT_3,
			FSM_STATE_MUL_INIT_4:		p_data_in	<= f_bram_out;
			//
			FSM_STATE_CRT_INIT_B_3,
			FSM_STATE_CRT_INIT_B_4:		p_data_in	<= f_bram_out;
			//
			FSM_STATE_EXP_INIT_3,
			FSM_STATE_EXP_INIT_4:		p_data_in	<= tp_data_out;
			//
			FSM_STATE_EXP_FILL_3,
			FSM_STATE_EXP_FILL_4:		p_data_in	<= pp_data_out;
			//
			default:							p_data_in	<= 32'dX;
		endcase
		//
		// t0_data_in
		//
		case (fsm_next_state)
			FSM_STATE_EXP_LOAD_3,
			FSM_STATE_EXP_LOAD_4:		t0_data_in <= t1_data_out;
			default:							t0_data_in <= 32'dX;
		endcase		
		//
		// t1_data_in
		//
		case (fsm_next_state)
			FSM_STATE_EXP_INIT_3:		t1_data_in <= 32'd1;
			FSM_STATE_EXP_INIT_4:		t1_data_in <= 32'd0;
			//
			FSM_STATE_EXP_FILL_3,
			FSM_STATE_EXP_FILL_4:		t1_data_in <= flag_update_r ? tp_data_out : t0_data_out;
			default:							t1_data_in <= 32'dX;
		endcase		
		//
		// t2_data_in
		//
		case (fsm_next_state)
			//
			FSM_STATE_MUL_INIT_3,
			FSM_STATE_MUL_INIT_4:		t2_data_in <= m_bram_out;
			//
			FSM_STATE_CRT_INIT_A_3,
			FSM_STATE_CRT_INIT_A_4:		t2_data_in <= m_bram_out;
			//
			FSM_STATE_CRT_INIT_B_3,
			FSM_STATE_CRT_INIT_B_4:		t2_data_in <= tp_data_out;
			//
			FSM_STATE_CRT_INIT_C_3,
			FSM_STATE_CRT_INIT_C_4:		t2_data_in <= tp_data_out;
			//
			FSM_STATE_EXP_INIT_3:		t2_data_in <= 32'd1;
			FSM_STATE_EXP_INIT_4:		t2_data_in <= 32'd0;
			//
			FSM_STATE_EXP_FILL_3,
			FSM_STATE_EXP_FILL_4:		t2_data_in <= flag_update_r ? tp_data_out : t0_data_out;
			default:							t2_data_in <= 32'dX;
		endcase		
		//
	end
	
	
		//
		// Double Multiplier
		//
	reg	mul_ena;
	reg	mul_crt;
	wire	mul_rdy_pp;
	wire	mul_rdy_tp;
	wire	mul_rdy_all = mul_rdy_pp & mul_rdy_tp;

	modexpa7_systolic_multiplier #
	(
		.OPERAND_ADDR_WIDTH		(OPERAND_ADDR_WIDTH),
		.SYSTOLIC_ARRAY_POWER	(SYSTOLIC_ARRAY_POWER)
	)
	mul_pp
	(
		.clk						(clk),
		.rst_n					(rst_n),

		.ena						(mul_ena),
		.rdy						(mul_rdy_pp),

		.reduce_only			(mul_crt),

		.a_bram_addr			(p1_addr_rd),
		.b_bram_addr			(p2_addr_rd),
		.n_bram_addr			(n1_bram_addr),
		.n_coeff_bram_addr	(n_coeff1_bram_addr),
		.r_bram_addr			(pp_addr_wr),

		.a_bram_out				(p1_data_out),
		.b_bram_out				(p2_data_out),
		.n_bram_out				(n1_bram_out),
		.n_coeff_bram_out		(n_coeff1_bram_out),

		.r_bram_in				(pp_data_in),
		.r_bram_wr				(pp_wren),

		.n_num_words			(m_num_words_latch)
	);

	modexpa7_systolic_multiplier #
	(
		.OPERAND_ADDR_WIDTH		(OPERAND_ADDR_WIDTH),
		.SYSTOLIC_ARRAY_POWER	(SYSTOLIC_ARRAY_POWER)
	)
	mul_tp
	(
		.clk						(clk),
		.rst_n					(rst_n),

		.ena						(mul_ena),
		.rdy						(mul_rdy_tp),

		.reduce_only			(mul_crt),

		.a_bram_addr			(t2_addr_rd),
		.b_bram_addr			(p3_addr_rd),
		.n_bram_addr			(n2_bram_addr),
		.n_coeff_bram_addr	(n_coeff2_bram_addr),
		.r_bram_addr			(tp_addr_wr),

		.a_bram_out				(t2_data_out),
		.b_bram_out				(p3_data_out),
		.n_bram_out				(n2_bram_out),
		.n_coeff_bram_out		(n_coeff2_bram_out),

		.r_bram_in				(tp_data_in),
		.r_bram_wr				(tp_wren),

		.n_num_words			(m_num_words_latch)
	);
	
	
	always @(posedge clk)
		//
		case (fsm_next_state)
			FSM_STATE_MUL_CALC_1,
			FSM_STATE_CRT_CALC_A_1,
			FSM_STATE_CRT_CALC_B_1,
			FSM_STATE_CRT_CALC_C_1,
			FSM_STATE_EXP_CALC_1:		mul_ena <= 1'b1;
			default:							mul_ena <= 1'b0;
		endcase

	always @(posedge clk)
		//
		case (fsm_next_state)
			FSM_STATE_CRT_CALC_A_1:		mul_crt <= 1'b1;
			default:							mul_crt <= 1'b0;
		endcase
			

		//
		// FSM Process
		//
	always @(posedge clk or negedge rst_n)
		//
		if (rst_n == 1'b0)	fsm_state <= FSM_STATE_EXP_IDLE;
		else						fsm_state <= fsm_next_state;
	
	
		//
		// FSM Transition Logic
		//
	always @* begin
		//
		fsm_next_state = FSM_STATE_EXP_STOP;
		//
		case (fsm_state)
			//
			//
			FSM_STATE_MUL_INIT_1:										fsm_next_state = FSM_STATE_MUL_INIT_2;
			FSM_STATE_MUL_INIT_2:										fsm_next_state = FSM_STATE_MUL_INIT_3;
			FSM_STATE_MUL_INIT_3:										fsm_next_state = FSM_STATE_MUL_INIT_4;
			FSM_STATE_MUL_INIT_4:	if (t2_addr_wr_done)			fsm_next_state = FSM_STATE_MUL_CALC_1;
											else								fsm_next_state = FSM_STATE_MUL_INIT_4;
			//
			FSM_STATE_MUL_CALC_1:										fsm_next_state = FSM_STATE_MUL_CALC_2;
			FSM_STATE_MUL_CALC_2:	if (mul_rdy_tp)				fsm_next_state = FSM_STATE_MUL_CALC_3;
											else								fsm_next_state = FSM_STATE_MUL_CALC_2;
			FSM_STATE_MUL_CALC_3:										fsm_next_state = FSM_STATE_EXP_INIT_1;
			//
			//
			FSM_STATE_CRT_INIT_A_1:										fsm_next_state = FSM_STATE_CRT_INIT_A_2;
			FSM_STATE_CRT_INIT_A_2:										fsm_next_state = FSM_STATE_CRT_INIT_A_3;
			FSM_STATE_CRT_INIT_A_3:										fsm_next_state = FSM_STATE_CRT_INIT_A_4;
			FSM_STATE_CRT_INIT_A_4:	if (t2_addr_wr_done_crt)	fsm_next_state = FSM_STATE_CRT_CALC_A_1;
											else								fsm_next_state = FSM_STATE_CRT_INIT_A_4;

			//
			FSM_STATE_CRT_CALC_A_1:										fsm_next_state = FSM_STATE_CRT_CALC_A_2;
			FSM_STATE_CRT_CALC_A_2:	if (mul_rdy_tp)				fsm_next_state = FSM_STATE_CRT_CALC_A_3;
											else								fsm_next_state = FSM_STATE_CRT_CALC_A_2;
			FSM_STATE_CRT_CALC_A_3:										fsm_next_state = FSM_STATE_CRT_INIT_B_1;
			//
			FSM_STATE_CRT_INIT_B_1:										fsm_next_state = FSM_STATE_CRT_INIT_B_2;
			FSM_STATE_CRT_INIT_B_2:										fsm_next_state = FSM_STATE_CRT_INIT_B_3;
			FSM_STATE_CRT_INIT_B_3:										fsm_next_state = FSM_STATE_CRT_INIT_B_4;
			FSM_STATE_CRT_INIT_B_4:	if (t2_addr_wr_done)			fsm_next_state = FSM_STATE_CRT_CALC_B_1;
											else								fsm_next_state = FSM_STATE_CRT_INIT_B_4;
			//
			FSM_STATE_CRT_CALC_B_1:										fsm_next_state = FSM_STATE_CRT_CALC_B_2;
			FSM_STATE_CRT_CALC_B_2:	if (mul_rdy_tp)				fsm_next_state = FSM_STATE_CRT_CALC_B_3;
											else								fsm_next_state = FSM_STATE_CRT_CALC_B_2;
			FSM_STATE_CRT_CALC_B_3:										fsm_next_state = FSM_STATE_CRT_INIT_C_1;
			//
			FSM_STATE_CRT_INIT_C_1:										fsm_next_state = FSM_STATE_CRT_INIT_C_2;
			FSM_STATE_CRT_INIT_C_2:										fsm_next_state = FSM_STATE_CRT_INIT_C_3;
			FSM_STATE_CRT_INIT_C_3:										fsm_next_state = FSM_STATE_CRT_INIT_C_4;
			FSM_STATE_CRT_INIT_C_4:	if (t2_addr_wr_done)			fsm_next_state = FSM_STATE_CRT_CALC_C_1;
											else								fsm_next_state = FSM_STATE_CRT_INIT_C_4;
			//
			FSM_STATE_CRT_CALC_C_1:										fsm_next_state = FSM_STATE_CRT_CALC_C_2;
			FSM_STATE_CRT_CALC_C_2:	if (mul_rdy_tp)				fsm_next_state = FSM_STATE_CRT_CALC_C_3;
											else								fsm_next_state = FSM_STATE_CRT_CALC_C_2;
			FSM_STATE_CRT_CALC_C_3:										fsm_next_state = FSM_STATE_EXP_INIT_1;
			//
			//
			FSM_STATE_EXP_IDLE:		if (ena_trig)					fsm_next_state = crt ?
																					FSM_STATE_CRT_INIT_A_1 : FSM_STATE_MUL_INIT_1;
											else								fsm_next_state = FSM_STATE_EXP_IDLE;
			//
			//
			FSM_STATE_EXP_INIT_1:										fsm_next_state = FSM_STATE_EXP_INIT_2;
			FSM_STATE_EXP_INIT_2:										fsm_next_state = FSM_STATE_EXP_INIT_3;
			FSM_STATE_EXP_INIT_3:										fsm_next_state = FSM_STATE_EXP_INIT_4;
			FSM_STATE_EXP_INIT_4:	if (t1_addr_done)				fsm_next_state = FSM_STATE_EXP_LOAD_1;
											else								fsm_next_state = FSM_STATE_EXP_INIT_4;
			//
			FSM_STATE_EXP_LOAD_1:										fsm_next_state = FSM_STATE_EXP_LOAD_2;
			FSM_STATE_EXP_LOAD_2:										fsm_next_state = FSM_STATE_EXP_LOAD_3;
			FSM_STATE_EXP_LOAD_3:										fsm_next_state = FSM_STATE_EXP_LOAD_4;
			FSM_STATE_EXP_LOAD_4:	if (t0_addr_done)				fsm_next_state = FSM_STATE_EXP_CALC_1;
											else								fsm_next_state = FSM_STATE_EXP_LOAD_4;
			//
			FSM_STATE_EXP_CALC_1:										fsm_next_state = FSM_STATE_EXP_CALC_2;
			FSM_STATE_EXP_CALC_2:	if (mul_rdy_all)				fsm_next_state = FSM_STATE_EXP_CALC_3;
											else								fsm_next_state = FSM_STATE_EXP_CALC_2;
			FSM_STATE_EXP_CALC_3:										fsm_next_state = FSM_STATE_EXP_FILL_1;
			//
			FSM_STATE_EXP_FILL_1:										fsm_next_state = FSM_STATE_EXP_FILL_2;
			FSM_STATE_EXP_FILL_2:										fsm_next_state = FSM_STATE_EXP_FILL_3;
			FSM_STATE_EXP_FILL_3:										fsm_next_state = FSM_STATE_EXP_FILL_4;
			FSM_STATE_EXP_FILL_4:	if (p_addr_wr_done)			fsm_next_state = FSM_STATE_EXP_NEXT;
											else								fsm_next_state = FSM_STATE_EXP_FILL_4;			
			//
			FSM_STATE_EXP_NEXT:		if (bit_cnt_done)				fsm_next_state = FSM_STATE_EXP_SAVE_1;
											else								fsm_next_state = FSM_STATE_EXP_LOAD_1;
			//
			FSM_STATE_EXP_SAVE_1:										fsm_next_state = FSM_STATE_EXP_SAVE_2;
			FSM_STATE_EXP_SAVE_2:										fsm_next_state = FSM_STATE_EXP_SAVE_3;
			FSM_STATE_EXP_SAVE_3:										fsm_next_state = FSM_STATE_EXP_SAVE_4;
			FSM_STATE_EXP_SAVE_4:	if (r_addr_done)				fsm_next_state = FSM_STATE_EXP_STOP;
											else								fsm_next_state = FSM_STATE_EXP_SAVE_4;
			//
			FSM_STATE_EXP_STOP:											fsm_next_state = FSM_STATE_EXP_IDLE;
			//
		endcase
		//
	end
			

endmodule
