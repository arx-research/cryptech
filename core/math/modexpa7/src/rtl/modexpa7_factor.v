//======================================================================
//
// modexpa7_factor.v
// -----------------------------------------------------------------------------
// Montgomery factor calculation block.
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

module modexpa7_factor #
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
		input											clk,
		input											rst_n,

		input											ena,
		output										rdy,

		output	[OPERAND_ADDR_WIDTH-1:0]	n_bram_addr,
		output	[OPERAND_ADDR_WIDTH-1:0]	f_bram_addr,

		input		[                32-1:0]	n_bram_out,

		output	[                32-1:0]	f_bram_in,
		output										f_bram_wr,

		input		[OPERAND_ADDR_WIDTH-1:0]	n_num_words
	);

	
		//
		// FSM Declaration
		//
	localparam	[ 7: 0]	FSM_STATE_IDLE		= 8'h00;
	
	localparam	[ 7: 0]	FSM_STATE_INIT_1	= 8'hA1;
	localparam	[ 7: 0]	FSM_STATE_INIT_2	= 8'hA2;
		
	localparam	[ 7: 0]	FSM_STATE_CALC_1	= 8'hB1;
	localparam	[ 7: 0]	FSM_STATE_CALC_2	= 8'hB2;
	localparam	[ 7: 0]	FSM_STATE_CALC_3	= 8'hB3;
	localparam	[ 7: 0]	FSM_STATE_CALC_4	= 8'hB4;
	localparam	[ 7: 0]	FSM_STATE_CALC_5	= 8'hB5;
	localparam	[ 7: 0]	FSM_STATE_CALC_6	= 8'hB6;
	localparam	[ 7: 0]	FSM_STATE_CALC_7	= 8'hB7;
	localparam	[ 7: 0]	FSM_STATE_CALC_8	= 8'hB8;
	
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
		// Enable Delay (Trigger)
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
	reg	[OPERAND_ADDR_WIDTH+5:0]	cyc_cnt;		// cycle counter
		
	wire	[OPERAND_ADDR_WIDTH+5:0]	cyc_cnt_zero = {1'b0, {OPERAND_ADDR_WIDTH{1'b0}}, {5{1'b0}}};
	wire	[OPERAND_ADDR_WIDTH+5:0]	cyc_cnt_last = {n_num_words, 1'b1, {5{1'b1}}};
	wire	[OPERAND_ADDR_WIDTH+5:0]	cyc_cnt_next = cyc_cnt + 1'b1;

		/* handy flag */
	wire	cyc_cnt_done = (cyc_cnt == cyc_cnt_last) ? 1'b1 : 1'b0;
	
	always @(posedge clk)
		//
		if (fsm_next_state == FSM_STATE_CALC_1)
			//
			case (fsm_state)
				FSM_STATE_INIT_2:	cyc_cnt <= cyc_cnt_zero;
				FSM_STATE_SAVE_5:	cyc_cnt <= cyc_cnt_done ? cyc_cnt : cyc_cnt_next;
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
		 * This module uses 5 block memories:
		 * N - external input, stores modulus
		 * F - external output, stores Montgomery factor
		 * F0 - internal, stores intermediate result
		 * F1 - internal, stores quantity F0 << 1
		 * F2 - internal, stores quantity F1 - N
		 *
		 */
		 
	reg	[OPERAND_ADDR_WIDTH-1:0]	f_addr;
	reg	[OPERAND_ADDR_WIDTH-1:0]	f0_addr;	
	reg	[OPERAND_ADDR_WIDTH-1:0]	f1_addr;
	reg	[OPERAND_ADDR_WIDTH-1:0]	f2_addr;
	
	reg	[31: 0]	f_data_in;
	reg	[31: 0]	f0_data_in;
	reg	[31: 0]	f1_data_in;
	reg	[31: 0]	f2_data_in;
	
	wire	[31: 0]	f0_data_out;
	wire	[31: 0]	f1_data_out;
	wire	[31: 0]	f2_data_out;
	
	reg				f_wren;
	reg				f0_wren;
	reg				f1_wren;
	reg				f2_wren;
	
		/* map top-level ports to internal registers */
	assign n_bram_addr	= f0_addr;
	assign f_bram_addr	= f_addr;
	assign f_bram_in		= f_data_in;
	assign f_bram_wr		= f_wren;
	
	bram_1rw_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(OPERAND_ADDR_WIDTH))
	bram_f0 (.clk(clk), .a_addr(f0_addr), .a_wr(f0_wren), .a_in(f0_data_in), .a_out(f0_data_out));

	bram_1rw_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(OPERAND_ADDR_WIDTH))
	bram_f1 (.clk(clk), .a_addr(f1_addr), .a_wr(f1_wren), .a_in(f1_data_in), .a_out(f1_data_out));

	bram_1rw_readfirst #(.MEM_WIDTH(32), .MEM_ADDR_BITS(OPERAND_ADDR_WIDTH))
	bram_f2 (.clk(clk), .a_addr(f2_addr), .a_wr(f2_wren), .a_in(f2_data_in), .a_out(f2_data_out));		
		
		/* handy values */
	wire	[OPERAND_ADDR_WIDTH-1:0]	f_addr_next = f_addr + 1'b1;
	wire	[OPERAND_ADDR_WIDTH-1:0]	f0_addr_next = f0_addr + 1'b1;
	wire	[OPERAND_ADDR_WIDTH-1:0]	f1_addr_next = f1_addr + 1'b1;
	wire	[OPERAND_ADDR_WIDTH-1:0]	f2_addr_next = f2_addr + 1'b1;
	
		/* handy flags */
	wire										f_addr_done =  (f_addr == bram_addr_last) ? 1'b1 : 1'b0;
	wire										f0_addr_done =  (f0_addr == bram_addr_last) ? 1'b1 : 1'b0;
	wire										f1_addr_done =  (f1_addr == bram_addr_last) ? 1'b1 : 1'b0;
	wire										f2_addr_done =  (f2_addr == bram_addr_last) ? 1'b1 : 1'b0;
	
		//
		// Delayed Flags
		//
	reg f12_addr_done_dly;
	
	always @(posedge clk)
		//
		f12_addr_done_dly <= f1_addr_done & f2_addr_done;
	
	
		//
		// Modulus Delay Line
		//
	reg	[31: 0]	n_bram_out_dly;
	
		/* delay block memory output by 1 clock cycle */
	always @(posedge clk) n_bram_out_dly <= n_bram_out;
	
	
		//
		// Subtractor
		//
		
		/*
		 * This subtractor calculated quantity F2 = F1 - N
		 *
		 */
	
	wire	[31: 0]	sub_d;
	wire				sub_b_in;
	reg				sub_b_in_mask;
	wire				sub_b_out;
	
		/* add masking into borrow feedback chain */
	assign sub_b_in = sub_b_out & ~sub_b_in_mask;

	always @(posedge clk)
		
			/* mask borrow into the very first word */
		sub_b_in_mask <= (fsm_next_state == FSM_STATE_CALC_3) ? 1'b1 : 1'b0;
		
	modexpa7_subtractor32 sub_inst
	(
		.clk		(clk),
		.ce		(1'b1),
		.a			(f1_data_in),
		.b			(n_bram_out_dly),
		.b_in		(sub_b_in),
		.d			(sub_d),
		.b_out	(sub_b_out)
	);


		//
		// F0 Shift Carry Logic
		//
		
		/*
		 * F0 value is repeatedly shifted to the left, so we need carry logic
		 * to save the MSB of the current output word and feed into the LSB
		 * of the next input word.
		 *
		 */
	
	reg	f0_data_out_carry;

		/* shifted output */
	wire	[31: 0]	f0_data_out_shifted = {f0_data_out[30:0], f0_data_out_carry};

	always @(posedge clk)
		
			/* mask carry into the very first word, propagate carry otherwise */
		case (fsm_next_state)
			FSM_STATE_CALC_2:		f0_data_out_carry <= 1'b0;
			FSM_STATE_CALC_3,
			FSM_STATE_CALC_4,
			FSM_STATE_CALC_5,
			FSM_STATE_CALC_6:		f0_data_out_carry <= f0_data_out[31];
			default:					f0_data_out_carry <= 1'bX;
		endcase


		//
		// Delay Lines
		//
	reg	sub_b_out_dly1;
	reg	f0_data_out_carry_dly1;
	reg	f0_data_out_carry_dly2;
	
	always @(posedge clk) begin
		sub_b_out_dly1				<= sub_b_out;
		f0_data_out_carry_dly1	<= f0_data_out_carry;
		f0_data_out_carry_dly2	<= f0_data_out_carry_dly1;
	end
	
	
		//
		// F Update Flag
		//
	reg	flag_keep_f;
	
	always @(posedge clk)
		
			/* update flag when new word of F2 is obtained */
		if (fsm_next_state == FSM_STATE_SAVE_1)
			flag_keep_f <= sub_b_out_dly1 & ~f0_data_out_carry_dly2;

	
		//
		// Block Memory Address Update Logic
		//
	always @(posedge clk) begin
		//
		// F0
		//
		case (fsm_next_state)
			FSM_STATE_INIT_1,
			FSM_STATE_CALC_1,
			FSM_STATE_SAVE_3:		f0_addr <= bram_addr_zero;
			//
			FSM_STATE_INIT_2,
			FSM_STATE_CALC_2,
			FSM_STATE_CALC_3,
			FSM_STATE_CALC_4,
			FSM_STATE_CALC_5,
			FSM_STATE_CALC_6,
			FSM_STATE_SAVE_4,
			FSM_STATE_SAVE_5:		f0_addr <= !f0_addr_done ? f0_addr_next : f0_addr;
		endcase
		//
		// F1
		//
		case (fsm_next_state)
			FSM_STATE_CALC_3,
			FSM_STATE_SAVE_1:		f1_addr <= bram_addr_zero;
			//
			FSM_STATE_CALC_4,
			FSM_STATE_CALC_5,
			FSM_STATE_CALC_6,
			FSM_STATE_SAVE_2,
			FSM_STATE_SAVE_3,
			FSM_STATE_SAVE_4:		f1_addr <= !f1_addr_done ? f1_addr_next : f1_addr;
		endcase
		//
		// F2
		//
		case (fsm_next_state)
			FSM_STATE_CALC_5,
			FSM_STATE_SAVE_1:		f2_addr <= bram_addr_zero;
			//
			FSM_STATE_CALC_6,
			FSM_STATE_CALC_7,
			FSM_STATE_CALC_8,
			FSM_STATE_SAVE_2,
			FSM_STATE_SAVE_3,
			FSM_STATE_SAVE_4:		f2_addr <= !f2_addr_done ? f2_addr_next : f2_addr;
		endcase
		//
		// F
		//
		case (fsm_next_state)
			FSM_STATE_SAVE_3:		f_addr <= bram_addr_zero;
			//
			FSM_STATE_SAVE_4,
			FSM_STATE_SAVE_5:		f_addr <= !f_addr_done ? f_addr_next : f_addr;
		endcase
		//
	end
	
	
		//
		// Block Memory Write Enable Logic
		//
	always @(posedge clk) begin
		//
		// F0
		//
		case (fsm_next_state)			
			FSM_STATE_INIT_1,
			FSM_STATE_INIT_2,
			FSM_STATE_SAVE_3,
			FSM_STATE_SAVE_4,
			FSM_STATE_SAVE_5:		f0_wren <= 1'b1;
			default:					f0_wren <= 1'b0;
		endcase
		//
		// F1
		//
		case (fsm_next_state)
			FSM_STATE_CALC_3,
			FSM_STATE_CALC_4,
			FSM_STATE_CALC_5,
			FSM_STATE_CALC_6:		f1_wren <= 1'b1;
			default:					f1_wren <= 1'b0;
		endcase
		//
		// F2
		//
		case (fsm_next_state)
			FSM_STATE_CALC_5,
			FSM_STATE_CALC_6,
			FSM_STATE_CALC_7,
			FSM_STATE_CALC_8:		f2_wren <= 1'b1;
			default:					f2_wren <= 1'b0;
		endcase
		//
		// F
		//
		case (fsm_next_state)			
			FSM_STATE_SAVE_3,
			FSM_STATE_SAVE_4,
			FSM_STATE_SAVE_5:		f_wren <= cyc_cnt_done;
			default:					f_wren <= 1'b0;
		endcase
		//
	end


		//
		// Block Memory Input Logic
		//
	always @(posedge clk) begin
		//
		// F0
		//
		case (fsm_next_state)
			FSM_STATE_INIT_1:		f0_data_in <= 32'd1;
			FSM_STATE_INIT_2:		f0_data_in <= 32'd0;
			//
			FSM_STATE_SAVE_3,
			FSM_STATE_SAVE_4,
			FSM_STATE_SAVE_5:		f0_data_in <= flag_keep_f ? f1_data_out : f2_data_out;
			default:					f0_data_in <= {32{1'bX}};
		endcase
		//
		// F1
		//
		case (fsm_next_state)
			FSM_STATE_CALC_3,
			FSM_STATE_CALC_4,
			FSM_STATE_CALC_5,
			FSM_STATE_CALC_6:		f1_data_in <= f0_data_out_shifted;
			default:					f1_data_in <= {32{1'bX}};
		endcase
		//
		// F2
		//
		case (fsm_next_state)
			FSM_STATE_CALC_5,
			FSM_STATE_CALC_6,
			FSM_STATE_CALC_7,
			FSM_STATE_CALC_8:		f2_data_in <= sub_d;
			default:					f2_data_in <= {32{1'bX}};
		endcase
		//
		// F
		//
		case (fsm_next_state)
			FSM_STATE_SAVE_3,
			FSM_STATE_SAVE_4,
			FSM_STATE_SAVE_5:		f_data_in <= flag_keep_f ? f1_data_out : f2_data_out;
			default:					f_data_in <= {32{1'bX}};
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
			FSM_STATE_INIT_2:		if (f0_addr_done)			fsm_next_state = FSM_STATE_CALC_1;
										else							fsm_next_state = FSM_STATE_INIT_2;
												
			FSM_STATE_CALC_1:										fsm_next_state = FSM_STATE_CALC_2;
			FSM_STATE_CALC_2:										fsm_next_state = FSM_STATE_CALC_3;
			FSM_STATE_CALC_3:										fsm_next_state = FSM_STATE_CALC_4;
			FSM_STATE_CALC_4:										fsm_next_state = FSM_STATE_CALC_5;
			FSM_STATE_CALC_5:										fsm_next_state = FSM_STATE_CALC_6;
			FSM_STATE_CALC_6:		if (f1_addr_done)			fsm_next_state = FSM_STATE_CALC_7;
										else							fsm_next_state = FSM_STATE_CALC_6;
			FSM_STATE_CALC_7:										fsm_next_state = FSM_STATE_CALC_8;
			FSM_STATE_CALC_8:										fsm_next_state = FSM_STATE_SAVE_1;

			FSM_STATE_SAVE_1:										fsm_next_state = FSM_STATE_SAVE_2;
			FSM_STATE_SAVE_2:										fsm_next_state = FSM_STATE_SAVE_3;
			FSM_STATE_SAVE_3:										fsm_next_state = FSM_STATE_SAVE_4;
			FSM_STATE_SAVE_4:		if (f12_addr_done_dly)	fsm_next_state = FSM_STATE_SAVE_5;
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
