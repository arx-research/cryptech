`timescale 1ns / 1ps

module streebog_hash_top
	(
		clock,
		block, block_length,
		init, update, final,
		short_mode,
		digest, digest_valid,
		ready
	);
	
	
		//
		// Parameters
		//
	parameter	PS_PIPELINE_STAGES	=  2;	// 2, 4, 8
	parameter	L_PIPELINE_STAGES		=  2;	// 2, 4, 8, 16, 32, 64
	
	
		//
		// Ports
		//
	input		wire				clock;			// core clock
	input		wire	[511:0]	block;			// input message block
	input		wire	[  9:0]	block_length;	// length of input block in bits (0..512)
	input		wire				init;				// flag to start calculation of new message hash
	input		wire				update;			// flag to compress next message block
	input		wire				final;			// flag to run final transformation after last message block 
	input		wire				short_mode;		// 0 = produce 512-bit hash, 1 = produce 256-bit hash
	output	wire	[511:0]	digest;			// message digest output
	output	wire				digest_valid;	// hash is ready (digest output value is valid)
	output	wire				ready;			// core is ready (init/update/final can be asserted)
	
	
		//
		// Initialization Vectors and Round Count
		//
	localparam	STREEBOG_IV_512		= {512{1'b0}};
	localparam	STREEBOG_IV_256		= {64{8'h01}};
	localparam	STREEBOG_NUM_ROUNDS	= 4'd12;
	
	
		//
		// State Registers
		//
	reg	[511:0]	h;			// |
	reg	[511:0]	Sigma;	// | Internal State Registers
	reg	[511:0]	N;			// |
	
	reg	[511:0]	digest_reg;
	reg				digest_valid_reg = 1'b0;
	reg	[  3:0]	round_count = 4'd0;
	
	assign digest			= digest_reg;
	assign digest_valid	= digest_valid_reg;
		
		
		//
		// Handy Internal Flags
		//
	wire	round_count_active	= (round_count > 4'd0)						? 1 : 0;		// transformation has been started
	wire	round_count_not_done = (round_count < STREEBOG_NUM_ROUNDS)	? 1 : 0;		// transformation has not been finished


		/*
		 * Compression procedure includes 13 rounds. To perform every round we need to know
		 * round key. This implementation uses two parallel LPS cores. The first LPS core (key core)
		 * is used to produce round keys, the second LPS core (data core) is used to encrypt message block.
		 *
		 * Data core is not activated during the first round, because round key is not yet known during
		 * the first round. During the second round, key core computes next (second) round key, while data core encrypts
		 * mesage block using first round key and so on. The last compression round doesn't include encryption step.
		 * Instead of it simple XOR operation is used.
		 *
		 * Compression procedure requires 13 key calculations and 12 data encryptions. LPS cores operate according to
		 * the following schedule:
		 *
		 *
		 *             +----------+----------+----------+-   -+----------+
		 * Round Count |        0 |        1 |        2 | ... |       12 |
		 *             +----------+----------+----------+-   -+----------+
		 * Key Core    |  KEY  #0 |  KEY  #1 |  KEY  #2 | ... |  KEY #12 |
		 *             +----------+----------+----------+-   -+----------+
		 * Data Core   |     Idle | DATA  #0 | DATA  #1 | ... | DATA #11 |
		 *             +----------+----------+----------+-   -+----------+
		 *
		 */


		//
		// LPS Core for Round Key Calculation
		//
	reg	[511:0]	lps_key_in;		//
	wire	[511:0]	lps_key_out;	//
	wire				lps_key_ena;	//
	wire				lps_key_last;	//
	wire				lps_key_rdy;	//
	
	wire	lps_key_ena_update		= (fsm_state == FSM_STATE_UPDATE_LPS_TRIG)		? 1 : 0;
	wire	lps_key_ena_final_n		= (fsm_state == FSM_STATE_FINAL_N_LPS_TRIG)		? 1 : 0;
	wire	lps_key_ena_final_sigma	= (fsm_state == FSM_STATE_FINAL_SIGMA_LPS_TRIG) ? 1 : 0;
	
	assign lps_key_ena = lps_key_ena_update || lps_key_ena_final_n || lps_key_ena_final_sigma;
	
	streebog_core_lps #
	(
		.PS_PIPELINE_STAGES	(PS_PIPELINE_STAGES),
		.L_PIPELINE_STAGES	(L_PIPELINE_STAGES)
	)
	lps_key
	(
		.clk		(clock),
		.ena		(lps_key_ena),
		.rdy		(lps_key_rdy),
		.last		(lps_key_last),
		.din		(lps_key_in),
		.dout		(lps_key_out)
	);
	
	
		//
		// LPS Core for Block Compression
		//
	reg	[511:0]	lps_data_in;
	wire	[511:0]	lps_data_out;
	wire				lps_data_ena;
	wire				lps_data_last;
	wire				lps_data_rdy;

	assign lps_data_ena = lps_key_ena & round_count_active;

	streebog_core_lps #
	(
		.PS_PIPELINE_STAGES	(PS_PIPELINE_STAGES),
		.L_PIPELINE_STAGES	(L_PIPELINE_STAGES)
	)	
	lps_data
	(
		.clk		(clock),
		.ena		(lps_data_ena),
		.rdy		(lps_data_rdy),
		.last		(lps_data_last),
		.din		(lps_data_in),
		.dout		(lps_data_out)
	);
		
	
		/*
		 * According to specification, internal state must be updated after compression, this
		 * involves addition of two pairs of 512-bit numbers. This operation is done in two
		 * parallel summation cores. The first core updates N register, the second core updates
		 * Sigma register. Summation is triggered before LPS cores are activated. Actual update
		 * of N and Sigma occurs after completion of compression procedure.
		 *
		 */
	
	
		//
		// Summation Trigger Flag
		//
	wire	adder_trig = (fsm_state == FSM_STATE_UPDATE_ADDER_TRIG) ? 1 : 0;
	
	
		//
		// Block Length Adder (N = N + |M|)
		//
	wire	[511:0]	adder_n_sum;
	wire				adder_n_rdy;
	
	streebog_core_adder_s6 adder_n
	(
		.clk	(clock),
		.ena	(adder_trig),
		.rdy	(adder_n_rdy),
		.x		(N),
		.y		({{502{1'b0}}, block_length}),
		.sum	(adder_n_sum)
	);
	
	
		//
		// Message Adder (Sigma = Sigma + M)
		//
	wire	[511:0]	adder_sigma_sum;
	wire				adder_sigma_rdy;
	
	streebog_core_adder_s6 adder_sigma
	(
		.clk	(clock),
		.ena	(adder_trig),
		.rdy	(adder_sigma_rdy),
		.x		(Sigma),
		.y		(block),
		.sum	(adder_sigma_sum)
	);
	
	
		//
		// Handy Flags
		//
	wire	lps_last_both		= lps_key_last & lps_data_last;
	wire	lps_rdy_both		= lps_key_rdy  & lps_data_rdy;
	wire	adder_rdy_both		= adder_n_rdy  & adder_sigma_rdy;
	
	
		/*
		 * Operation of this core is controlled by FSM logic. Ready flag is embedded in state encoding. FSM goes out of
		 * idle state when init/update/final flags become active. Init flag has priority over update and final flags.
		 * Update flag has priority over final flag.
		 *
		 */
		 
	
		//
		// FSM States
		//
	localparam	FSM_STATE_IDLE									= 4'b1_00_0;	// core is idle
	//
	localparam	FSM_STATE_UPDATE_LPS_TRIG					= 4'b0_00_0;	// core is triggering gN(h,m) transformation
	localparam	FSM_STATE_UPDATE_LPS_WAIT					= 4'b0_00_1;	// core is waiting for transformation to complete
	//
	localparam	FSM_STATE_UPDATE_ADDER_TRIG				= 4'b0_11_0;	// core is triggering summation
	localparam	FSM_STATE_UPDATE_ADDER_WAIT				= 4'b0_11_1;	// core is waiting for summation to complete
	//
	localparam	FSM_STATE_FINAL_N_LPS_TRIG					= 4'b0_01_0;	// core is triggering g0(h,N) transformation
	localparam	FSM_STATE_FINAL_N_LPS_WAIT					= 4'b0_01_1;	// core is waiting for transformation to complete
	//
	localparam	FSM_STATE_FINAL_SIGMA_LPS_TRIG			= 4'b0_10_0;	// core is triggering g0(h,Sigma) transformation
	localparam	FSM_STATE_FINAL_SIGMA_LPS_WAIT			= 4'b0_10_1;	// core is waiting for transformation of complete
	
	
		//
		// FSM State Register and Core Ready Flag
		//
	reg	[ 3: 0]	fsm_state = FSM_STATE_IDLE;
	assign ready = fsm_state[3];

	
		//
		// FSM Transition Logic
		//
	always @(posedge clock) begin
		//
		case (fsm_state)
			//
			// init
			//
			FSM_STATE_IDLE: begin
				if (!init && update)					fsm_state	<= FSM_STATE_UPDATE_ADDER_TRIG;
				if (!init && !update && final)	fsm_state	<= FSM_STATE_FINAL_N_LPS_TRIG;
			end
			//
			// update -> gN(h,m)
			//
			FSM_STATE_UPDATE_ADDER_TRIG:			fsm_state	<= FSM_STATE_UPDATE_LPS_TRIG;
			FSM_STATE_UPDATE_LPS_TRIG:				fsm_state	<= FSM_STATE_UPDATE_LPS_WAIT;
			FSM_STATE_UPDATE_LPS_WAIT:
				if (lps_rdy_both)						fsm_state	<= round_count_not_done ? FSM_STATE_UPDATE_LPS_TRIG : FSM_STATE_UPDATE_ADDER_WAIT;
			FSM_STATE_UPDATE_ADDER_WAIT:
				if (adder_rdy_both)					fsm_state	<= FSM_STATE_IDLE;
			//
			// final -> g0(h,N)
			//
			FSM_STATE_FINAL_N_LPS_TRIG:			fsm_state	<= FSM_STATE_FINAL_N_LPS_WAIT;
			FSM_STATE_FINAL_N_LPS_WAIT:
				if (lps_rdy_both)						fsm_state	<= round_count_not_done ? FSM_STATE_FINAL_N_LPS_TRIG : FSM_STATE_FINAL_SIGMA_LPS_TRIG;
			//
			// final -> g0(h,Sigma)
			//
			FSM_STATE_FINAL_SIGMA_LPS_TRIG:		fsm_state	<= FSM_STATE_FINAL_SIGMA_LPS_WAIT;
			FSM_STATE_FINAL_SIGMA_LPS_WAIT:
				if (lps_rdy_both)						fsm_state	<= round_count_not_done ? FSM_STATE_FINAL_SIGMA_LPS_TRIG : FSM_STATE_IDLE;
			//
			// default
			//
			default:										fsm_state	<= FSM_STATE_IDLE;
			//
		endcase
		//
	end
	
	
		/*
		 * Key calculation involves 12 round constants. These constants are stored in an array. The first key
		 * (calculated during the first round) does not require a constant. New constant is preloaded during the last
		 * cycle of LPS transformation. LPS cores have dedicated output flag indicating that operation is about to complete.
		 * This flag is used as Clock Enable. Constants are preloaded during rounds 1-12 and are used during rounds 2-13.
		 *
		 */
	
		//
		// Round Constants
		//
	wire	[511:0]	c_array_out;
	
	wire	c_array_ena_update		= (fsm_state == FSM_STATE_UPDATE_LPS_WAIT)		? 1 : 0;
	wire	c_array_ena_final_n		= (fsm_state == FSM_STATE_FINAL_N_LPS_WAIT)		? 1 : 0;
	wire	c_array_ena_final_sigma	= (fsm_state == FSM_STATE_FINAL_SIGMA_LPS_WAIT)	? 1 : 0;
	
	wire	c_array_ena = lps_key_last && round_count_not_done && (c_array_ena_update || c_array_ena_final_n || c_array_ena_final_sigma);
	
	//
	(* ROM_STYLE="BLOCK" *)
	//	
	streebog_rom_c_array c_array
	(
		.clk		(clock),
		.ena		(c_array_ena),
		.din		(round_count),
		.dout		(c_array_out)
	);
	
		/*
		 * The following pieces of code take care of LPS and summation inputs and outputs, they also take care
		 * of output digest register and corresponding valid flag.
		 *
		 */


		//
		// Internal State Control Logic
		//
	always @(posedge clock)
		//
		case (fsm_state)

			FSM_STATE_IDLE: if (init) begin
				h			<= (short_mode == 1'b1) ? STREEBOG_IV_256 : STREEBOG_IV_512;
				N			<= {512{1'b0}};
				Sigma		<= {512{1'b0}};
			end
			
			FSM_STATE_UPDATE_ADDER_WAIT: if (adder_rdy_both) begin
				N			<= adder_n_sum;
				Sigma		<= adder_sigma_sum;
			end

			FSM_STATE_UPDATE_LPS_WAIT:
				if (lps_key_rdy && !round_count_not_done)
					h			<= lps_key_out ^ lps_data_out ^ h ^ block;

			FSM_STATE_FINAL_N_LPS_WAIT:
				if (lps_key_rdy && !round_count_not_done)
					h			<= lps_key_out ^ lps_data_out ^ h ^ N;
					
		endcase
	
	
		//
		// Output Register Control Logic
		//
	always @(posedge clock)
		//
		case (fsm_state)
		
			FSM_STATE_IDLE: if (init) begin
				digest_reg			<= {512{1'bX}};
				digest_valid_reg	<= 1'b0;
			end

			FSM_STATE_FINAL_SIGMA_LPS_WAIT:
				if (lps_key_rdy && !round_count_not_done) begin
					digest_reg			<= lps_key_out ^ lps_data_out ^ h ^ Sigma;
					digest_valid_reg	<= 1'b1;
				end

		endcase
		
		
		//
		// Round Count Logic
		//
	always @(posedge clock)
		//
		case (fsm_state)
		
			FSM_STATE_IDLE:
				if (update || final) round_count <= 4'd0;
			
			FSM_STATE_UPDATE_LPS_WAIT,
			FSM_STATE_FINAL_N_LPS_WAIT,
			FSM_STATE_FINAL_SIGMA_LPS_WAIT:
				if (lps_key_rdy) round_count <= round_count_not_done ? round_count + 1'b1 : 4'd0;
			
		endcase	
		
		
		//
		// Key and Data LPS Cores Logic
		//
	always @(posedge clock)
		//
		case (fsm_state)
		
			FSM_STATE_IDLE: if (!init) begin
				if (update)					lps_key_in	<= h ^ N;
				if (!update && final)	lps_key_in	<= h;
			end

			FSM_STATE_UPDATE_LPS_WAIT:
				if (lps_key_rdy && round_count_not_done) begin
					lps_key_in			<= lps_key_out ^ c_array_out;
					lps_data_in 		<= lps_key_out ^ (round_count_active ? lps_data_out : block);
				end

			FSM_STATE_FINAL_N_LPS_WAIT: if (lps_key_rdy) begin
				lps_key_in			<= lps_key_out ^ (round_count_not_done ? c_array_out : lps_data_out ^ h ^ N);
				lps_data_in 		<= round_count_not_done ? lps_key_out ^ (round_count_active ? lps_data_out : N) : {512{1'bX}};
			end

			FSM_STATE_FINAL_SIGMA_LPS_WAIT:
				if (lps_key_rdy && round_count_not_done) begin
					lps_key_in			<= lps_key_out ^ c_array_out;
					lps_data_in 		<= round_count_active ? lps_key_out ^ lps_data_out : lps_key_out ^ Sigma;
				end 

		endcase
		

endmodule
