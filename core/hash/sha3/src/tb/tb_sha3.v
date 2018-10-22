//======================================================================
//
// tb_sha3.v
// -----------------------------
// Testbench for the SHA-3 core.
//
// Authors: Pavel Shatov, Joachim Strombergson, Pernd Paysan
// Copyright (c) 2015, 2017 NORDUnet A/S
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
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

module tb_sha3();


	//----------------------------------------------------------------
	// Hashes of empty messages
	//----------------------------------------------------------------
	localparam	[511:0]	SHA3_224_EMPTY_MSG =
		{	224'h6b4e03423667dbb73b6e15454f0eb1abd4597f9a1b078e3f5b5a6bc7,
			{512-224{1'bX}}};

	localparam	[511:0]	SHA3_256_EMPTY_MSG =
		{	256'ha7ffc6f8bf1ed76651c14756a061d662f580ff4de43b49fa82d80a4b80f8434a,
			{512-256{1'bX}}};
			
	localparam	[511:0]	SHA3_384_EMPTY_MSG =
		{	384'h0c63a75b845e4f7d01107d852e4c2485c51a50aaaa94fc61995e71bbee983a2ac3713831264adb47fb6bd1e058d5f004,
			{512-384{1'bX}}};

	localparam	[511:0]	SHA3_512_EMPTY_MSG =
		{	512'ha69f73cca23a9ac5c8b567dc185a756e97c982164fe25859e0d1dcc1475c80a615b2123af1f5f94c11e3e9402c3ac558f500199d95b6d3e301758586281dcd26};

		/*
		 * The "short" message is the "abc" string (0x616263):
		 *
		 * https://www.di-mgt.com.au/sha_testvectors.html
		 *
		 */
	localparam	[511:0]	SHA3_224_SHORT_MSG =
		{	224'he642824c3f8cf24ad09234ee7d3c766fc9a3a5168d0c94ad73b46fdf,
			{512-224{1'bX}}};

	localparam	[511:0]	SHA3_256_SHORT_MSG =
		{	256'h3a985da74fe225b2045c172d6bd390bd855f086e3e9d525b46bfe24511431532,
			{512-256{1'bX}}};

	localparam	[511:0]	SHA3_384_SHORT_MSG =
		{	384'hec01498288516fc926459f58e2c6ad8df9b473cb0fc08c2596da7cf0e49be4b298d88cea927ac7f539f1edf228376d25,
			{512-384{1'bX}}};

	localparam	[511:0]	SHA3_512_SHORT_MSG =
		{	512'hb751850b1a57168a5693cd924b6b096e08f621827444f70d884f5d0240d2712e10e116e9192af3c91a7ec57647e3934057340b4cf408d5a56592f8274eec53f0};

		/*
		 * The "long" message is the 1600-bit string 0xA3...A3 taken from:
		 * https://csrc.nist.gov/Projects/Cryptographic-Standards-and-Guidelines/example-values
		 *
		 */

	localparam	[511:0]	SHA3_224_LONG_MSG =
		{	224'h9376816ABA503F72F96CE7EB65AC095DEEE3BE4BF9BBC2A1CB7E11E0,
			{512-224{1'bX}}};

	localparam	[511:0]	SHA3_256_LONG_MSG =
		{	256'h79F38ADEC5C20307A98EF76E8324AFBFD46CFD81B22E3973C65FA1BD9DE31787,
			{512-256{1'bX}}};

	localparam	[511:0]	SHA3_384_LONG_MSG =
		{	384'h1881DE2CA7E41EF95DC4732B8F5F002B189CC1E42B74168ED1732649CE1DBCDD76197A31FD55EE989F2D7050DD473E8F,
			{512-384{1'bX}}};

	localparam	[511:0]	SHA3_512_LONG_MSG =
		{	512'hE76DFAD22084A8B1467FCF2FFA58361BEC7628EDF5F3FDC0E4805DC48CAEECA81B7C13C30ADF52A3659584739A2DF46BE589C51CA1A4A8416DF6545A1CE8BA00};

		/*
		 * Sponge Parameters
		 *
		 */
	`define SHA3_224_BLOCK_BITS	1152
	`define SHA3_256_BLOCK_BITS	1088
	`define SHA3_384_BLOCK_BITS	 832
	`define SHA3_512_BLOCK_BITS	 576

	`define SHA3_224_OUTPUT_BITS	 224
	`define SHA3_256_OUTPUT_BITS	 256
	`define SHA3_384_OUTPUT_BITS	 384
	`define SHA3_512_OUTPUT_BITS	 512
			

	//----------------------------------------------------------------
	// Internal constant and parameter definitions.
	//----------------------------------------------------------------
	parameter CLK_HALF_PERIOD = 2;
	parameter CLK_PERIOD = 2 * CLK_HALF_PERIOD;


	//----------------------------------------------------------------
	// Register and Wire declarations.
	//----------------------------------------------------------------
	reg				tb_clk;
	reg				tb_rst_n;
	reg				tb_we;
	reg	[ 6:0]	tb_addr;
	reg	[31:0]	tb_wr_data;
	reg				tb_dut_init;
	reg				tb_dut_next;
	wire	[31:0]	tb_rd_data;
	wire				tb_rdy;
	
	integer i, j;
	integer num_err;
	integer total_bits;
	integer block_words, output_words;
	
	reg	[511:0]	hash_shreg;		// output
	reg	[31: 0]	hash_word;		// current output word
	
	reg				mismatch;		// error flag


  //----------------------------------------------------------------
  // Device Under Test.
  //----------------------------------------------------------------
  sha3 dut
  (
		.clk		(tb_clk),
		.nreset	(tb_rst_n),
		.w			(tb_we),
		.addr		(tb_addr),
		.din		(tb_wr_data),
		.dout		(tb_rd_data),
		.init		(tb_dut_init),
		.next		(tb_dut_next),
		.ready	(tb_rdy)
	);


	//----------------------------------------------------------------
	// clk_gen
	//
	// Clock generator process.
	//----------------------------------------------------------------
	always
		begin : clk_gen
			#CLK_HALF_PERIOD tb_clk = ~tb_clk;
		end // clk_gen
		

	//----------------------------------------------------------------
	// reset_dut()
	//
	// Toggles reset to force the DUT into a well defined state.
	//----------------------------------------------------------------
	task reset_dut;
		begin
			$display("*** Toggling reset...");
			tb_rst_n = 0;
			#(4 * CLK_HALF_PERIOD);
			tb_rst_n = 1;
		end
	endtask // reset_dut()


	//----------------------------------------------------------------
	// init_sim()
	//
	// Initialize all counters and testbed functionality as well
	// as setting the DUT inputs to defined values.
	//----------------------------------------------------------------
	task init_sim;
		begin
			tb_clk		= 0;
			tb_rst_n		= 0;
			tb_we			= 0;
			tb_addr		= 7'd0;
			tb_wr_data	= 32'h0;
			tb_dut_init = 0;
			tb_dut_next = 0;
		end
	endtask // init_sim()


	//----------------------------------------------------------------
	// test_empty_message()
	//
	// The first pritimive test that verifies what happens with an
	// empty input message.
	//----------------------------------------------------------------
	task test_empty_message;
	
		input	[511:0]	correct_hash;	// known-good hash
		input integer	block_size;		// block length (bits)
		input integer	output_size;	// output length (bits)
		
		begin

			$display("*** Testing %0d-bit hash (%0d-bit block)...", output_size, block_size);

				// number of words in block and output
			block_words = block_size >> 5;	// block_words = block_length / 32
			output_words = output_size >> 5;	// output_words = output_length / 32
			
				// wait for some time
			#(4*CLK_PERIOD);

				/*
				 * Note, that we must wipe entire block buffer, because
				 * it may contain leftovers from previous calculations!
				 * We wipe the registers and apply padding at the same time.
				 *
				 */
				 
			for (i=0; i<50; i=i+1) begin
				case (i)
					0:					tb_wr_data = 32'h00000006;		// ...0001 | 10
					block_words-1:	tb_wr_data = 32'h80000000;		//	1000...
					default:			tb_wr_data = 32'h00000000;		
				endcase
				tb_addr = {1'b0, i[5:0]};	// increment address
				tb_we = 1;						// enable writing
				#(CLK_PERIOD);
			end

			tb_addr = 'bX;
			tb_we = 0;

				/* run one absord-squeeze cycle */
			sha3_sponge_pump_init();
			
				// reset error counter
			mismatch = 0;
				
				// save the expected result
			hash_shreg = correct_hash;
			
				// now read and compare
			for (i=0; i<output_words; i=i+1) begin
			
				tb_addr = {1'b1, i[5:0]};
				#(CLK_PERIOD);
				
					// swap bytes in the read value
				for (j=31; j>0; j=j-8)
					hash_word[j-:8] = tb_rd_data[31-j+:8];
				
				$display("    *** hash_word[%0d]: reference = %08x, calculated = %08x",
					i, hash_shreg[511-:32], hash_word);
				
					// compare
				if (hash_shreg[511-:32] !== hash_word) begin
					mismatch = 1;
					num_err = num_err + 1;
				end
				
				hash_shreg = {hash_shreg[511-32:0], {32{1'bX}}};
			end			
			
			if (mismatch) $display("    *** Test failed.");
			else          $display("    *** Test passed.");
			
		end

	endtask // test_empty_message


	//----------------------------------------------------------------
	// test_short_message()
	//
	// The second simple test that verifies what happens with an
	// input message, that fits into just one block.
	//----------------------------------------------------------------
	task test_short_message;
	
		input	[511:0]	correct_hash;	// known-good hash
		input integer	block_size;		// block length (bits)
		input integer	output_size;	// output length (bits)
		
		begin

			$display("*** Testing %0d-bit hash (%0d-bit block)...", output_size, block_size);

				// number of words in block and output
			block_words = block_size >> 5;	// block_words = block_length / 32
			output_words = output_size >> 5;	// output_words = output_length / 32
			
				// wait for some time
			#(4*CLK_PERIOD);

				/*
				 * Note, that we must wipe entire internal state, because
				 * it may contain leftovers from previous calculations!
				 * We wipe the internal state, fill in the message (0x616263)
				 * and apply padding all at once.
				 *
				 */
				 
			for (i=0; i<50; i=i+1) begin
				case (i)
					0:					tb_wr_data = 32'h06636261;		// ...0001 | 10 | 'cba'
					block_words-1:	tb_wr_data = 32'h80000000;		//	1000...
					default:			tb_wr_data = 32'h00000000;							
				endcase
				tb_addr = {1'b0, i[5:0]};	// increment address
				tb_we = 1;						// enable writing
				#(CLK_PERIOD);
			end

			tb_addr = 'bX;
			tb_we = 0;

				/* run one absord-squeeze cycle */
			sha3_sponge_pump_init();
			
				// reset error counter
			mismatch = 0;
				
				// save the expected result
			hash_shreg = correct_hash;
			
				// now read and compare
			for (i=0; i<output_words; i=i+1) begin
			
				tb_addr = {1'b1, i[5:0]};
				#(CLK_PERIOD);
				
					// swap bytes in the read value
				for (j=31; j>0; j=j-8)
					hash_word[j-:8] = tb_rd_data[31-j+:8];
				
				$display("    *** hash_word[%0d]: reference = %08x, calculated = %08x",
					i, hash_shreg[511-:32], hash_word);
				
					// compare
				if (hash_shreg[511-:32] !== hash_word) begin
					mismatch = 1;
					num_err = num_err + 1;
				end
				
				hash_shreg = {hash_shreg[511-32:0], {32{1'bX}}};
			end			
			
			if (mismatch) $display("    *** Test failed.");
			else          $display("    *** Test passed.");
			
		end

	endtask // test_short_message


	//----------------------------------------------------------------
	// test_long_message()
	//
	// The second not so pritimive test, that verifies what happens
	// when an input message spans more than one block.
	//----------------------------------------------------------------
	task test_long_message;
	
		input	[511:0]	correct_hash;
		input integer	block_size;
		input integer	output_size;
				
		begin

			$display("*** Testing %0d-bit hash (%0d-bit block)...", output_size, block_size);

				// number of words in block and output
			block_words = block_size >> 5;		// block_words = block_length / 32
			output_words = output_size >> 5;	// output_words = output_length / 32
		
				// wait for some time
			#(4*CLK_PERIOD);

				/* start filling the first block */
			total_bits = 1600;

				/*
				 * note, that we must wipe entire internal state, because
				 * it may contain leftovers from previous calculations!
				 */

			for (i=0; i<50; i=i+1) begin
				tb_wr_data = (i < block_words) ? 32'hA3A3A3A3 : 32'h00000000;
				tb_addr = {1'b0, i[5:0]};
				tb_we = 1;
				#(CLK_PERIOD);
				if (i < block_words)
					total_bits = total_bits - 32;
			end

			tb_addr = 'bX;
			tb_we = 0;

				/* run one absord-squeeze cycle */
			sha3_sponge_pump_init();

				// process remaining bits
			while (total_bits > -64) begin

					/*
					 * note, that there's no need to fill entire bank now,
					 * because x ^ 0 == x
					 */

				for (i=0; i<block_words; i=i+1) begin
				
						
					if (total_bits > 0) begin
							// message
						tb_wr_data = 32'hA3A3A3A3;
						tb_addr = {1'b0, i[5:0]};
						tb_we = 1;
						#(CLK_PERIOD);
						total_bits = total_bits - 32;
					end else if (total_bits == 0) begin
							// padding
						tb_wr_data = 32'h00000006;
						tb_addr = {1'b0, i[5:0]};
						tb_we = 1;
						#(CLK_PERIOD);
						total_bits = total_bits - 32;
					end else if (i == (block_words-1)) begin
							// more padding
						tb_wr_data = 32'h80000000;
						tb_addr = {1'b0, i[5:0]};
						tb_we = 1;
						#(CLK_PERIOD);
						total_bits = total_bits - 32;
					end else begin
							// intermediate bytes
						tb_wr_data = 32'h00000000;
						tb_addr = {1'b0, i[5:0]};
						tb_we = 1;
						#(CLK_PERIOD);						
					end
					
				end

				tb_addr = 'bX;
				tb_we = 0;

					/* run one absord-squeeze cycle */
				sha3_sponge_pump_next();

			end
			
				// reset error counter
			mismatch = 0;
				
				// save the expected result
			hash_shreg = correct_hash;
			
				// now read and compare
			for (i=0; i<output_words; i=i+1) begin
			
				tb_addr = {1'b1, i[5:0]};
				#(CLK_PERIOD);
				
					// swap bytes in the read value
				for (j=31; j>0; j=j-8)
					hash_word[j-:8] = tb_rd_data[31-j+:8];
				
				$display("    *** hash_word[%0d]: reference = %08x, calculated = %08x",
					i, hash_shreg[511-:32], hash_word);
				
					// compare
				if (hash_shreg[511-:32] !== hash_word) begin
					mismatch = 1;
					num_err = num_err + 1;
				end
				
				hash_shreg = {hash_shreg[511-32:0], {32{1'bX}}};
			end			
			
			if (mismatch) $display("    *** Test failed.");
			else          $display("    *** Test passed.");
			
		end

	endtask // test_long_message


	//----------------------------------------------------------------
	// sha3_sponge_pump_init;
	//
	// Make the "sponge" absord input data and then wait for output
	// data to be "squeezed out".
	//----------------------------------------------------------------
	task sha3_sponge_pump_init;
	
		begin : sha3_sponge_pump_init_task
		
			reg	poll_rdy;		// flag to keep waiting while the DUT is busy

				// start operation
			tb_dut_init = 1;
			#(CLK_PERIOD);
			tb_dut_init = 0;
				
				// poll ready output until the operation is finished
			poll_rdy = 1;
			while (poll_rdy) begin
				#(CLK_PERIOD);
				poll_rdy = !tb_rdy;
			end
			
		end
		
	endtask // sha3_sponge_pump


	//----------------------------------------------------------------
	// sha3_sponge_pump_next;
	//
	// Make the "sponge" absord input data and then wait for output
	// data to be "squeezed out".
	//----------------------------------------------------------------
	task sha3_sponge_pump_next;
	
		begin : sha3_sponge_pump_next_task
		
			reg	poll_rdy;		// flag to keep waiting while the DUT is busy

				// start operation
			tb_dut_next = 1;
			#(CLK_PERIOD);
			tb_dut_next = 0;
				
				// poll ready output until the operation is finished
			poll_rdy = 1;
			while (poll_rdy) begin
				#(CLK_PERIOD);
				poll_rdy = !tb_rdy;
			end
			
		end
		
	endtask // sha3_sponge_pump


  //----------------------------------------------------------------
  // sha3_test
  // The main test functionality.
  //----------------------------------------------------------------
  initial
  
    begin : sha3_test
      $display("*** Testbench for sha3.v started.");

		num_err = 0;

      init_sim();
      reset_dut();
		
      test_empty_message(SHA3_224_EMPTY_MSG, `SHA3_224_BLOCK_BITS, `SHA3_224_OUTPUT_BITS);
      test_empty_message(SHA3_256_EMPTY_MSG, `SHA3_256_BLOCK_BITS, `SHA3_256_OUTPUT_BITS);
      test_empty_message(SHA3_384_EMPTY_MSG, `SHA3_384_BLOCK_BITS, `SHA3_384_OUTPUT_BITS);
      test_empty_message(SHA3_512_EMPTY_MSG, `SHA3_512_BLOCK_BITS, `SHA3_512_OUTPUT_BITS);
		
		test_short_message(SHA3_224_SHORT_MSG, `SHA3_224_BLOCK_BITS, `SHA3_224_OUTPUT_BITS);
		test_short_message(SHA3_256_SHORT_MSG, `SHA3_256_BLOCK_BITS, `SHA3_256_OUTPUT_BITS);
		test_short_message(SHA3_384_SHORT_MSG, `SHA3_384_BLOCK_BITS, `SHA3_384_OUTPUT_BITS);
		test_short_message(SHA3_512_SHORT_MSG, `SHA3_512_BLOCK_BITS, `SHA3_512_OUTPUT_BITS);

		test_long_message(SHA3_224_LONG_MSG, `SHA3_224_BLOCK_BITS, `SHA3_224_OUTPUT_BITS);
		test_long_message(SHA3_256_LONG_MSG, `SHA3_256_BLOCK_BITS, `SHA3_256_OUTPUT_BITS);
		test_long_message(SHA3_384_LONG_MSG, `SHA3_384_BLOCK_BITS, `SHA3_384_OUTPUT_BITS);
		test_long_message(SHA3_512_LONG_MSG, `SHA3_512_BLOCK_BITS, `SHA3_512_OUTPUT_BITS);

      $display("*** Testbench for sha3.v done.");
		
		if (num_err == 0)
			$display("    *** All tests passed.");
		else
			$display("    *** %0d tests not passed.", num_err);
		
      $finish;
		
    end // sha3_test
	 
endmodule // tb_sha3

//======================================================================
// EOF tb_sha3.v
//======================================================================
