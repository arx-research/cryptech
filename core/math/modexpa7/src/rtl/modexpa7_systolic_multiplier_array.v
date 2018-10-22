//======================================================================
//
// modexpa7_systolic_multiplier_array.v
// -----------------------------------------------------------------------------
// Systolic Montgomery multiplier Processing Element Array
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

module modexpa7_systolic_multiplier_array #
	(
		parameter	OPERAND_ADDR_WIDTH		= 4,
		parameter	SYSTOLIC_ARRAY_POWER		= 2
	)
	(
		input																				clk,
		input																				rst_n,

		input																				ena,
		output																			rdy,

		input																				crt,

		output	[OPERAND_ADDR_WIDTH - SYSTOLIC_ARRAY_POWER - 1 : 0]	loader_addr_rd,

		input		[         32 * (2 ** SYSTOLIC_ARRAY_POWER) - 1 : 0]	pe_a_wide,
		input		[         32 * (2 ** SYSTOLIC_ARRAY_POWER) - 1 : 0]	pe_b_wide,

		output	[                       OPERAND_ADDR_WIDTH - 1 : 0]	a_bram_addr,
		
		output	[                       OPERAND_ADDR_WIDTH     : 0]	p_bram_addr,
		output	[                                       32 - 1 : 0]	p_bram_in,
		output																			p_bram_wr,

		input		[                       OPERAND_ADDR_WIDTH - 1 : 0]	n_num_words,
		input		[                       OPERAND_ADDR_WIDTH     : 0]	p_num_words
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
	
	localparam	[ 7: 0]	FSM_STATE_MULT_START		= 8'h11;
	localparam	[ 7: 0]	FSM_STATE_MULT_CRUNCH	= 8'h12;
	localparam	[ 7: 0]	FSM_STATE_MULT_RELOAD	= 8'h13;
	localparam	[ 7: 0]	FSM_STATE_MULT_FINAL		= 8'h14;
	
	localparam	[ 7: 0]	FSM_STATE_STOP				= 8'hFF;
	
	
		/*
		 * FSM State / Next State
		 */
	reg	[ 7: 0]	fsm_state = FSM_STATE_IDLE;
	reg	[ 7: 0]	fsm_next_state;


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

		// save number of words in n when new operation starts
	always @(posedge clk)
		//
		if ((fsm_state == FSM_STATE_IDLE) && ena_trig) begin
			n_num_words_latch <= n_num_words;
			p_num_words_latch <= p_num_words;
		end
			
			
		/*
		 * Systolic Cycle Counters
		 */
		
		// handy values 
	wire	[SYSTOLIC_CNTR_WIDTH-1:0]	syst_cnt_zero = {SYSTOLIC_CNTR_WIDTH{1'b0}};
	wire	[SYSTOLIC_CNTR_WIDTH-1:0]	syst_cnt_last = n_num_words_latch[OPERAND_ADDR_WIDTH-1:SYSTOLIC_ARRAY_POWER];
	
		// counters
	reg	[SYSTOLIC_CNTR_WIDTH-1:0]	syst_cnt_load;
	reg	[SYSTOLIC_CNTR_WIDTH-1:0]	syst_cnt_unload;
		
		// handy increment values
	wire	[SYSTOLIC_CNTR_WIDTH-1:0]	syst_cnt_load_next		= syst_cnt_load   + 1'b1;
	wire	[SYSTOLIC_CNTR_WIDTH-1:0]	syst_cnt_unload_next		= syst_cnt_unload + 1'b1;

		// handy stop flags
	wire										syst_cnt_load_done		= (syst_cnt_load   == syst_cnt_last) ? 1'b1 : 1'b0;
	wire										syst_cnt_unload_done		= (syst_cnt_unload == syst_cnt_last) ? 1'b1 : 1'b0;

	always @(posedge clk)
		//
		case (fsm_next_state)
			FSM_STATE_MULT_START,
			FSM_STATE_MULT_RELOAD:
				//
				syst_cnt_load <= syst_cnt_zero;
			
			FSM_STATE_MULT_CRUNCH:
				//
				syst_cnt_load <= !syst_cnt_load_done ? syst_cnt_load_next : syst_cnt_load;
				
		endcase
		
	always @(posedge clk)
		//
		if (fsm_state == FSM_STATE_MULT_CRUNCH) begin
			//	
			if (shreg_done_latency)
				syst_cnt_unload <= syst_cnt_zero;
			else if (shreg_now_unloading)
				syst_cnt_unload <= !syst_cnt_unload_done ? syst_cnt_unload_next : syst_cnt_unload;
			//
		end

			
		/*
		 * Timing Shift Registers
		 */
		 
	reg	[SYSTOLIC_NUM_CYCLES-1:0]	shreg_load;
	reg	[SYSTOLIC_PE_LATENCY  :0]	shreg_latency;
	reg	[SYSTOLIC_NUM_CYCLES-1:0]	shreg_unload;

	wire	shreg_done_load		= shreg_load[syst_cnt_last];
	wire	shreg_done_latency	= shreg_latency[SYSTOLIC_PE_LATENCY];
	wire	shreg_done_unload		= shreg_unload[syst_cnt_last];
	
	reg	shreg_now_loading;
	//reg	shreg_now_latency;
	reg	shreg_now_unloading;

	reg	shreg_done_latency_dly;
	always @(posedge clk)
		shreg_done_latency_dly <= shreg_done_latency;
	
	always @(posedge clk)
		//
		case (fsm_state)
			//
			FSM_STATE_MULT_START,
			FSM_STATE_MULT_RELOAD: begin
				//
				shreg_now_loading		<= 1'b1;
				//shreg_now_latency		<= 1'b1;
				shreg_now_unloading	<= 1'b0;
				//
				shreg_load		<= {{SYSTOLIC_NUM_CYCLES-1{1'b0}}, 1'b1};
				shreg_latency	<= {{SYSTOLIC_PE_LATENCY  {1'b0}}, 1'b1};
				shreg_unload	<= {{SYSTOLIC_NUM_CYCLES-1{1'b0}}, 1'b0};
				//
			end
			//
			FSM_STATE_MULT_CRUNCH: begin
				//
				shreg_load		<= {shreg_load   [SYSTOLIC_NUM_CYCLES-2:0], 1'b0};
				shreg_latency	<= {shreg_latency[SYSTOLIC_PE_LATENCY-1:0], 1'b0};
				shreg_unload	<= {shreg_unload [SYSTOLIC_NUM_CYCLES-2:0], shreg_latency[SYSTOLIC_PE_LATENCY]};
				//
				if (shreg_done_load)				shreg_now_loading <= 1'b0;
				//if (shreg_done_latency)			shreg_now_latency <= 1'b0;
				if (shreg_done_latency)			shreg_now_unloading <= 1'b1;
				else if (shreg_done_unload)	shreg_now_unloading <= 1'b0;
				
			end
			//
			default: begin
				shreg_now_loading		<= 1'b0;
				//shreg_now_latency		<= 1'b0;
				shreg_now_unloading	<= 1'b0;
			end
			//
		endcase
			
			
		/*
		 * Systolic Array of Processing Elements
		 */
	reg	[31: 0]	pe_a        [0:SYSTOLIC_ARRAY_LENGTH-1];
	reg	[31: 0]	pe_b        [0:SYSTOLIC_ARRAY_LENGTH-1];
	wire	[31: 0]	pe_t        [0:SYSTOLIC_ARRAY_LENGTH-1];
	wire	[31: 0]	pe_c_in     [0:SYSTOLIC_ARRAY_LENGTH-1];
	wire	[31: 0]	pe_p        [0:SYSTOLIC_ARRAY_LENGTH-1];
	wire	[31: 0]	pe_c_out    [0:SYSTOLIC_ARRAY_LENGTH-1];
	

		/*
		 * FIFOs
		 */
	reg	fifo_c_rst;
	reg	fifo_t_rst;

	reg	fifo_c_wren;
	wire	fifo_c_rden;
	
	reg	fifo_t_wren;
	wire	fifo_t_rden;
		
	reg	[32 * SYSTOLIC_ARRAY_LENGTH - 1 : 0]	fifo_c_din;
	wire	[32 * SYSTOLIC_ARRAY_LENGTH - 1 : 0]	fifo_c_dout;
	
	wire	[32 * SYSTOLIC_ARRAY_LENGTH - 1 : 0]	fifo_t_din;
	wire	[32 * SYSTOLIC_ARRAY_LENGTH - 1 : 0]	fifo_t_dout;
	
	wire	[32 *                          1  - 1 : 0]	fifo_t_din_msb;
	reg	[32 * (SYSTOLIC_ARRAY_LENGTH - 1) - 1 : 0]	fifo_t_din_lsb;
	
	assign fifo_t_din = {fifo_t_din_msb, fifo_t_din_lsb};
	
	modexpa7_simple_fifo #
	(
		.BUS_WIDTH	(32 * SYSTOLIC_ARRAY_LENGTH),
		.DEPTH_BITS	(SYSTOLIC_CNTR_WIDTH)
	)
	fifo_c
	(
		.clk			(clk),
		.rst			(fifo_c_rst),
		.wr_en		(fifo_c_wren),
		.d_in			(fifo_c_din),
		.rd_en		(fifo_c_rden),
		.d_out		(fifo_c_dout)
	);
	
	modexpa7_simple_fifo #
	(
		.BUS_WIDTH	(32 * SYSTOLIC_ARRAY_LENGTH),
		.DEPTH_BITS	(SYSTOLIC_CNTR_WIDTH)
	)
	fifo_t
	(
		.clk			(clk),
		.rst			(fifo_t_rst),
		.wr_en		(fifo_t_wren),
		.d_in			(fifo_t_din),
		.rd_en		(fifo_t_rden),
		.d_out		(fifo_t_dout)
	);

	genvar i;
	generate for (i=0; i<SYSTOLIC_ARRAY_LENGTH; i=i+1)
		//
		begin : gen_modexpa7_systolic_pe
			//
			modexpa7_systolic_pe systolic_pe_inst
			(
				.clk		(clk),
				.a			(pe_a[i]),
				.b			(pe_b[i]),
				.t			(pe_t[i]),
				.c_in		(pe_c_in[i]),
				.p			(pe_p[i]),
				.c_out	(pe_c_out[i])
			);
			//
			assign pe_c_in[i] = fifo_c_dout[32 * (i + 1) - 1 -: 32];
			assign pe_t[i]    = fifo_t_dout[32 * (i + 1) - 1 -: 32];
			//
			always @(posedge clk)
				fifo_c_din[32 * (i + 1) - 1 -: 32] <= pe_c_out[i];
			//
		end
		//
	endgenerate
	
	generate for (i=1; i<SYSTOLIC_ARRAY_LENGTH; i=i+1)
		//
		begin : gen_modexpa7_fifo_t_lsb
			//
			always @(posedge clk)
				fifo_t_din_lsb[32 * i - 1 -: 32] <= pe_p[i];
			//
		end
		//
	endgenerate
	
	assign fifo_t_din_msb = shreg_now_unloading ? pe_p[0] : 32'd0;


		/*
		 * FIFO Reset Logic
		 */
	always @(posedge clk)
		//
		case (fsm_state)
			FSM_STATE_MULT_START:									fifo_c_rst <= 1'b1;
			FSM_STATE_MULT_CRUNCH:	if (shreg_done_load)		fifo_c_rst <= 1'b0;
		endcase

	always @(posedge clk)
		//
		case (fsm_state)
			FSM_STATE_MULT_START:									fifo_t_rst <= 1'b1;
			FSM_STATE_MULT_CRUNCH:	if (shreg_done_load)		fifo_t_rst <= 1'b0;
		endcase

		/*
		 *
		 */
	assign fifo_c_rden = shreg_now_loading;
	assign fifo_t_rden = shreg_now_loading;

	always @(posedge clk) fifo_c_wren <= shreg_now_unloading;
	always @(posedge clk) fifo_t_wren <= shreg_now_unloading;


		/*
		 * Block Memory Interface
		 */

		// the very first address
	wire	[OPERAND_ADDR_WIDTH - 1 : 0]	bram_addr_zero     = {OPERAND_ADDR_WIDTH  {1'b0}};
	wire	[OPERAND_ADDR_WIDTH     : 0]	bram_addr_ext_zero = {OPERAND_ADDR_WIDTH+1{1'b0}};
	
		// the very last address
	wire	[OPERAND_ADDR_WIDTH - 1 : 0]	bram_addr_last     = n_num_words_latch;
	wire	[OPERAND_ADDR_WIDTH - 1 : 0]	bram_addr_last_crt =
		{n_num_words_latch[OPERAND_ADDR_WIDTH-2:0], 1'b1};
	wire	[OPERAND_ADDR_WIDTH     : 0]	bram_addr_ext_last = p_num_words_latch;
		
		// registers
	reg	[OPERAND_ADDR_WIDTH - 1 : 0]	a_addr;
	reg	[OPERAND_ADDR_WIDTH     : 0]	p_addr;
	reg	[                32 - 1 : 0]	p_data_in;
	reg											p_wren;

		// handy values 
	wire	[OPERAND_ADDR_WIDTH - 1 : 0]	a_addr_next = a_addr + 1'b1;
	wire	[OPERAND_ADDR_WIDTH     : 0]	p_addr_next = p_addr + 1'b1;
	
		// handy flags
	wire	a_addr_done     = (a_addr == bram_addr_last)     ? 1'b1 : 1'b0;
	wire	a_addr_done_crt = (a_addr == bram_addr_last_crt) ? 1'b1 : 1'b0;
	wire	p_addr_done     = (p_addr == bram_addr_ext_last) ? 1'b1 : 1'b0;
	
		// map top-level ports to internal registers
	assign a_bram_addr	= a_addr;
	assign p_bram_addr	= p_addr;
	assign p_bram_in		= p_data_in;
	assign p_bram_wr		= p_wren;

	integer j;
	always @(posedge clk)
		//
		if (fsm_state == FSM_STATE_MULT_CRUNCH)
			//
			for (j=0; j<SYSTOLIC_ARRAY_LENGTH; j=j+1)
				//
				if (shreg_now_loading) begin
					pe_a[j]		<= (p_addr > {1'b0, a_addr}) ? 32'd0 : pe_a_wide[32 * (j + 1) - 1 -: 32];
					pe_b[j]		<= pe_b_wide[32 * (j + 1) - 1 -: 32];
				end else begin
					pe_a[j]		<= 32'hXXXXXXXX;				
					pe_b[j]		<= 32'hXXXXXXXX;
				end



		/*
		 *
		 */
	always @(posedge clk)
		//
		case (fsm_next_state)
			FSM_STATE_MULT_RELOAD:	p_wren <= 1'b1;
			default:						p_wren <= 1'b0;
		endcase


	always @(posedge clk)
		//
		if ((fsm_state == FSM_STATE_MULT_CRUNCH) && shreg_done_latency_dly)
			p_data_in <= crt ? pe_a_wide[31:0] : pe_p[0];

		/*
		 * Block Memory Address Control
		 */
	always @(posedge clk) begin
		//
		case (fsm_state)
			FSM_STATE_MULT_START:	p_addr <= bram_addr_zero;
			FSM_STATE_MULT_RELOAD:	p_addr <= p_addr_next;
		endcase
		//
		case (fsm_next_state)
			FSM_STATE_MULT_START:	a_addr <= bram_addr_zero;
			FSM_STATE_MULT_RELOAD:	if (crt)		a_addr <= !a_addr_done_crt ? a_addr_next : a_addr;
											else			a_addr <= !a_addr_done     ? a_addr_next : a_addr;
		endcase
		//
	end

		
		/*
		 * Loader Address Control
		 */
	reg	[SYSTOLIC_CNTR_WIDTH-1:0]	loader_addr;

	assign loader_addr_rd = loader_addr;

	always @(posedge clk)
		//
		case (fsm_next_state)
			//
			FSM_STATE_MULT_START,
			FSM_STATE_MULT_RELOAD:
				loader_addr <= syst_cnt_zero;
			//									
			FSM_STATE_MULT_CRUNCH:
				//
				loader_addr <= !syst_cnt_load_done ? syst_cnt_load_next : syst_cnt_load;
			//	
		endcase



		
			
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
	always @* begin
		//
		fsm_next_state = FSM_STATE_STOP;
		//
		case (fsm_state)
			//
			FSM_STATE_IDLE:			if (ena_trig)				fsm_next_state = FSM_STATE_MULT_START;
											else							fsm_next_state = FSM_STATE_IDLE;
			//
			FSM_STATE_MULT_START:									fsm_next_state = FSM_STATE_MULT_CRUNCH;
			FSM_STATE_MULT_CRUNCH:	if (shreg_done_unload)	fsm_next_state = FSM_STATE_MULT_RELOAD;
											else							fsm_next_state = FSM_STATE_MULT_CRUNCH;
			FSM_STATE_MULT_RELOAD:	if (p_addr_done)			fsm_next_state = FSM_STATE_MULT_FINAL;
											else							fsm_next_state = FSM_STATE_MULT_CRUNCH;
			FSM_STATE_MULT_FINAL:									fsm_next_state = FSM_STATE_STOP;
			//
			FSM_STATE_STOP:											fsm_next_state = FSM_STATE_IDLE;
			//
		endcase
		//
	end


endmodule

//======================================================================
// End of file
//======================================================================
