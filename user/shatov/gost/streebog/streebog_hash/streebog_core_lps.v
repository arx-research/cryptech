`timescale 1ns / 1ps

module streebog_core_lps
	(
		clk,
		ena, rdy, last,
		din, dout
	);
	
	
		//
		// Parameters
		//
	parameter	PS_PIPELINE_STAGES	=  8;	// 2, 4, 8
	parameter	L_PIPELINE_STAGES		=  8;	// 2, 4, 8, 16, 32, 64


		//
		// Ports
		//
	input		wire				clk;		// core clock
	input		wire				ena;		// start transformation flag
	output	wire				rdy;		// transformation done flag (dout is valid)
	output	wire				last;		// transformation about to complete (rdy flag will be asserted during the next cycle)
	input		wire	[511:0]	din;		// input data to transform
	output	wire	[511:0]	dout;		// output data (result of transformation)
	
				
		/*
		 * This LPS core has parametrized internal pipeline. P and S transformations are combined into one PS transformation and
		 * have common pipeline. L transformation has its own separate pipeline. The total latency of this core is thus
		 * PS_PIPELINE_STAGES*L_PIPELINE_STAGES. The fastest version completes the tranformation in 2*2=4 cycles, the slowest
		 * version requires 8*64=512 cycles. S transformation substitutes bytes according to a lookup table. P transformation does
		 * permutation of input bytes. L transformation multiplies input data by a special predefined matrix. If you don't understand
		 * how matrices are multiplied, you should not try to understand how the following code works. This may damage your brain.
		 * You've been warned. Seriously.
		 *
		 */


		//
		// Constants
		//
		
		/*
		 * PS transformation operates on 64-bit words. Input data contains 512/64=8 such words.
		 * Depending on PS pipeline stage count we can transform 1, 2 or 4 words at a time.
		 *
		 * L transformation operates on 64-bit words. Depending on L pipeline stage count we
		 * can transform 1, 2, 4, 8, 16 or 32 bits of a word at a time.
		 *
		 */

	localparam	PS_WORDS_AT_ONCE	=  8 / PS_PIPELINE_STAGES;
	localparam	L_BITS_AT_ONCE		= 64 / L_PIPELINE_STAGES;
	
		/*
		 * These functions return number of bytes needed to store pipeline stage counters. They will
		 * also prevent users from specifying illegal pipeline widths . This module will not synthesize
		 * with invalid pipeline stage count, because counter width will not be explicitely defined.
		 *
		 */
	
	function	integer	PS_NUM_COUNT_BITS;
		input	integer	x;
		begin
			case (x)
				2:	PS_NUM_COUNT_BITS = 1;
				4:	PS_NUM_COUNT_BITS = 2;
				8:	PS_NUM_COUNT_BITS = 3;
			endcase
		end
	endfunction
	
	function	integer	L_NUM_COUNT_BITS;
		input	integer	y;
		begin
			case (y)
				 2:	L_NUM_COUNT_BITS = 1;
				 4:	L_NUM_COUNT_BITS = 2;
				 8:	L_NUM_COUNT_BITS = 3;
				16:	L_NUM_COUNT_BITS = 4;
				32:	L_NUM_COUNT_BITS = 5;
				64:	L_NUM_COUNT_BITS = 6;
			endcase
		end
	endfunction
	
	
		//
		// Counter Widths
		//
	localparam	L_CNT_BITS	= L_NUM_COUNT_BITS(L_PIPELINE_STAGES);		// width of L counter
	localparam	PS_CNT_BITS	= PS_NUM_COUNT_BITS(PS_PIPELINE_STAGES);	// width of PS counter
	
	
		//
		// Input Multiplexor
		//
	wire	[63: 0]	din_mux[0:7];		// eight 64-bit words
	
		/*
		 * This multiplexor does the P transformation. P transformation is effectively a matrix
		 * transposition. Input 512-bit word is treated as a 8x8 byte matrix. Multiplexor outputs
		 * a set of 8 64-bit words. These words are columns of the original matrix (transposition
		 * turns rows into colums).
		 *
		 */
	
	genvar i, j;
	generate for (i=0; i<8; i=i+1)
		begin: gen_din_mux_i
			for (j=0; j<8; j=j+1) begin: gen_din_mux_j
				assign din_mux[i][8*j + 7 : 8*j] = din[64*j + 8*i + 7 : 64*j + 8*i];
			end
		end
	endgenerate
	
	
		//
		// Output Multiplexor
		//
	reg	[63: 0]	dout_mux[0:7];		// eight 64-bit words
	
		/*
		 * Output 64-bit subwords are concatenated to form output 512-bit word.
		 *
		 */
		 
	genvar k;
	generate for (k=0; k<8; k=k+1)
		begin: gen_dout_mux
			assign dout[64*k+63:64*k] = dout_mux[k];
		end
	endgenerate
	
	
		//
		// PS and L Counters
		//
		
		/*
		 * These counters control internal data flow of this core. For example, if PS has 2 stages and
		 * L has 4 stages, then the count will look like this:
		 *     ____
		 * ENA     \\\________________________________
		 *     _____                                 _
		 * RDY  ^   \_______________________________/
		 *      |   |   |   |   |   |   |   |   |   |
		 * +----+---+---+---+---+---+---+---+---+---+-
		 * | PS | 0 | 0 | 0 | 0 | 1 | 1 | 1 | 1 | 0 |
		 * +----+---+---+---+---+---+---+---+---+---+-
		 * |  L | 0 | 1 | 2 | 3 | 0 | 1 | 2 | 3 | 0 |
		 * +----+---+---+---+---+---+---+---+---+---+-
		 *        ^               ^               |
		 *        |               |               +--> both counters will be zero during the last cycle
		 *        |               |
		 *        +---------------+------------------> preloading of new word(s) into S lookup table(s)
		 *
		 */
		 
	reg	[ L_CNT_BITS-1:0]	l_count	= { L_CNT_BITS{1'b0}};
	reg	[PS_CNT_BITS-1:0]	ps_count	= {PS_CNT_BITS{1'b0}};


		//
		// Handy Flags
		//
		
		/*
		 * These flags are used instead of lengthy (z_count == {Z_CNT_BITS{1'bZ}}) comparisons.
		 *
		 */

	wire	 l_count_done	= ( l_count == { L_CNT_BITS{1'b1}}) ? 1 : 0;
	wire	ps_count_done	= (ps_count == {PS_CNT_BITS{1'b1}}) ? 1 : 0;
	
	wire	 l_count_zero	= ( l_count == { L_CNT_BITS{1'b0}}) ? 1 : 0;
	wire	ps_count_zero	= (ps_count == {PS_CNT_BITS{1'b0}}) ? 1 : 0;
	
	
		//
		// Preload Flags
		//
		
		/*
		 * These flags are used as clock enables for S lookup table.
		 *
		 */
		
	wire	ps_preload_first	= (rdy && ena);
	wire	ps_preload_next	= (!rdy && !ps_count_zero && l_count_zero);
	
	
		//
		// Last Flag
		//
		
		/*
		 * This flag indicates that core operation is about to complete.
		 *
		 */
	assign last = !rdy && ps_count_zero && l_count_zero;

	
		//
		// Counter Logic
		//
	always @(posedge clk) begin
		//
		if (!rdy && l_count_done)								ps_count	<= ps_count + 1'b1;	// next word(s)
		//
		if (rdy && ena)				 							 l_count	<=  l_count + 1'b1;	// start of transformation
		//
		if (!rdy && !(ps_count_zero && l_count_zero))	 l_count	<=  l_count + 1'b1;	// next part of word(s)
		//
	end
	
	
		//
		// Ready Output Register
		//
	reg rdy_reg = 1'b1;
	assign rdy = rdy_reg;
	
	
		//
		// Ready Set and Clear Logic
		//
	always @(posedge clk) begin
		//
		if (rdy && ena)										rdy_reg <= 0;	// start of transformation
		//
		if (!rdy && l_count_zero && ps_count_zero)	rdy_reg <= 1;	// end of transformation
		//
	end
		
		
		//
		// S Table Indices
		//
		
		/*
		 * To transform several words at once a set of indices is required.
		 *
		 */
		
	wire	[ 2: 0]	s_in_offset	[0:PS_WORDS_AT_ONCE-1];		// indices of words being transformed
	wire	[63: 0]	s_out			[0:PS_WORDS_AT_ONCE-1];		// output words of S transformation
	
	assign s_in_offset[0] = ps_count * PS_WORDS_AT_ONCE;	// the first index is defined by PS counter,
																			// following indices are linearly increasing
	
	genvar sw, sb;														// word and byte counter
	generate for (sw=1; sw<PS_WORDS_AT_ONCE; sw=sw+1)
		begin: gen_s_in_offset
			assign s_in_offset[sw] = s_in_offset[sw-1] + 1'b1;
		end
	endgenerate
	
	
		//
		// S Lookup Table
		//
	generate for (sw=0; sw<PS_WORDS_AT_ONCE; sw=sw+1)
		begin: gen_s_out_word
			for (sb=0; sb<8; sb=sb+1) begin: gen_s_out_byte
				//
				(* ROM_STYLE="BLOCK" *)
				//
				streebog_rom_s_table s_table
				(
					.clk		(clk),
					.ena		(ps_preload_first | ps_preload_next),
					.din		(din_mux[s_in_offset[sw]][8*sb + 7 : 8*sb]),
					.dout		(s_out[sw][8*sb + 7 : 8*sb])
				);
				//
			end
		end
	endgenerate
	
	
	
		//
		// A Matrix Indices
		//
		
		/*
		 * To transform several bits at once a set of indices is required.
		 *
		 */		
		 
	wire	[ 5: 0]	l_in_offset	[0:L_BITS_AT_ONCE-1];	// indices of bits being transformed
	wire	[63: 0]	l_out			[0:L_BITS_AT_ONCE-1];	// output bits of L transformation

	assign l_in_offset[0] = l_count * L_BITS_AT_ONCE;	// the first index is defined by L counter,
																		// following indices are linearly increasing
	
	genvar l;
	generate for (l=1; l<L_BITS_AT_ONCE; l=l+1)
		begin: gen_l_in_offset
			assign l_in_offset[l] = l_in_offset[l-1] + 1'b1;
		end
	endgenerate
	
	
		//
		// A Matrix
		//
	generate for (l=0; l<L_BITS_AT_ONCE; l=l+1)
		begin: gen_l_out		
			//
			(* ROM_STYLE="BLOCK" *)
			//
			streebog_rom_a_matrix a_matrix
			(
				.clk		(clk),
				.din		(l_in_offset[l]),
				.dout		(l_out[l])
			);
			//
		end
	endgenerate
	
	
		//
		// Multiplication Logic
		//
		
		/*
		 * Original specification describes multiplication method that effectively adds
		 * matrix rows based on source vector items. Instead of that multiplication is
		 * done column-by-column.
		 *
		 */
		 
	wire	[L_BITS_AT_ONCE-1:0]	l_out_part[0:PS_WORDS_AT_ONCE-1];
	
	genvar lw, lb;
	generate for (lw=0; lw<PS_WORDS_AT_ONCE; lw=lw+1)
		begin: gen_l_out_part
			for (lb=0; lb<L_BITS_AT_ONCE; lb=lb+1) begin: gen_l_out_bit
				//
				assign l_out_part[lw][lb] = ^(l_out[lb] & s_out[lw]);
				//
			end
		end
	endgenerate
	
	
		/*
		 * PS and L transformations have 1-cycle latency, so delayed versions
		 * of offsets are needed to update output registers accordingly.
		 *
		 */
		 
	reg	[PS_CNT_BITS-1:0]	ps_count_dly	= 0;	// delayed PS counter
	reg	[ L_CNT_BITS-1:0]	 l_count_dly	= 0;	// delayed L counter
	
	always @(posedge clk) ps_count_dly <= ps_count;
	always @(posedge clk)  l_count_dly <=  l_count;
	
	
		//
		// Output Offset Tables
		//
	wire	[ 2: 0]	dout_offset_word	[0:PS_WORDS_AT_ONCE-1];
	wire	[ 5: 0]	dout_offset_bit	[0:L_BITS_AT_ONCE  -1];

	assign dout_offset_word[0] = ps_count_dly * PS_WORDS_AT_ONCE;
	assign dout_offset_bit[0]  =  l_count_dly * L_BITS_AT_ONCE;
	
	genvar z;
	
	generate for (z=1; z<PS_WORDS_AT_ONCE; z=z+1)
		begin: gen_dout_offset_word
			assign dout_offset_word[z] = dout_offset_word[z-1] + 1'b1;
		end
	endgenerate
	
	generate for (z=1; z<L_BITS_AT_ONCE; z=z+1)
		begin: gen_dout_offset_bit
			assign dout_offset_bit[z] = dout_offset_bit[z-1] + 1'b1;
		end
	endgenerate
	
	
	
		//
		// Output Logic
		//
	integer lps_w, lps_b;
	
	always @(posedge clk)
		//
		if (! rdy)
			//
			for (lps_w=0; lps_w<PS_WORDS_AT_ONCE; lps_w=lps_w+1)
				for (lps_b=0; lps_b<L_BITS_AT_ONCE; lps_b=lps_b+1)
					dout_mux[dout_offset_word[lps_w]][dout_offset_bit[lps_b]] <= l_out_part[lps_w][lps_b];
					//dout_mux[dout_offset_word[lps_w]][L_BITS_AT_ONCE*l_count_dly+lps_b] <= l_out_part[lps_w][lps_b];
	
	
endmodule
