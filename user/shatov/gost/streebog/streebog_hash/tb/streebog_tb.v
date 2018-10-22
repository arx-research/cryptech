`timescale 1ns / 1ps

module streebog_tb;


	localparam	STREEBOG_MODE_SHORT	= 1;
	localparam	STREEBOG_MODE_LONG	= 0;
	
		// short message that fits into one block
	localparam	[511:0]	MSG_SINGLE						= 512'h01323130393837363534333231303938373635343332313039383736353433323130393837363534333231303938373635343332313039383736353433323130;
	
		// length of short message in bits
	localparam	[  9:0]	MSG_SINGLE_LENGTH				= 10'd504;
	
		// correct 512-bit digest of short message
	localparam	[511:0]	MSG_SINGLE_DIGEST_LONG		= 512'h486f64c1917879417fef082b3381a4e211c324f074654c38823a7b76f830ad00fa1fbae42b1285c0352f227524bc9ab16254288dd6863dccd5b9f54a1ad0541b;
	
		// correct 256-bit digest of short message
	localparam	[255:0]	MSG_SINGLE_DIGEST_SHORT		= 256'h00557be5e584fd52a449b16b0251d05d27f94ab76cbaa6da890b59d8ef1e159d;
	
	
		// first block of long message
	localparam	[511:0]	MSG_DOUBLE_FIRST				= 512'hfbeafaebef20fffbf0e1e0f0f520e0ed20e8ece0ebe5f0f2f120fff0eeec20f120faf2fee5e2202ce8f6f3ede220e8e6eee1e8f0f2d1202ce8f0f2e5e220e5d1;
	
		// second block of long message
	localparam	[511:0]	MSG_DOUBLE_SECOND				= 512'h0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001fbe2e5f0eee3c820;
	
		// length of first part of long message in bits
	localparam	[  9:0]	MSG_DOUBLE_FIRST_LENGTH		= 10'd512;
	
		// length of second part of long message in bits
	localparam	[  9:0]	MSG_DOUBLE_SECOND_LENGTH	= 10'd64;
	
		// correct 512-bit digest of long message
	localparam	[511:0]	MSG_DOUBLE_DIGEST_LONG		= 512'h28fbc9bada033b1460642bdcddb90c3fb3e56c497ccd0f62b8a2ad4935e85f037613966de4ee00531ae60f3b5a47f8dae06915d5f2f194996fcabf2622e6881e;
	
		// correct 256-bit digest of short message
	localparam	[511:0]	MSG_DOUBLE_DIGEST_SHORT		= 256'h508f7e553c06501d749a66fc28c6cac0b005746d97537fa85d9e40904efed29d;
	
	
		//
		// Inputs
		//
	reg				clock;
	reg	[511:0]	block;
	reg	[  9:0]	block_length;
	reg				init		= 0;
	reg				update	= 0;
	reg				final		= 0;
	reg				short_mode;


		//
		// Outputs
		//
	wire	[511:0]	digest;
	wire				digest_valid;
	wire				ready;


		//
		// UUT
		//
	streebog_hash_top uut
	(
		.clock			(clock), 
		
		.block			(block), 
		.block_length	(block_length), 
		.init				(init), 
		.update			(update), 
		.final			(final), 
		.short_mode		(short_mode), 
		.digest			(digest), 
		.digest_valid	(digest_valid), 
		.ready			(ready)
	);
	
		//
		// Clock
		//
	initial clock = 1'b0;
	always #5 clock = ~clock;
	
	reg	[511:0]	hash;
	wire	[255:0]	hash_short = hash[511:256];

	initial begin
		//
		#100;
		//
		$display("Checking 512-bit mode on short message...");
		//
		streebog_init(STREEBOG_MODE_LONG);
		streebog_set_block(MSG_SINGLE, MSG_SINGLE_LENGTH);
		streebog_update();
		streebog_final();
		//
		if (hash == MSG_SINGLE_DIGEST_LONG) $display("OK");
		else $display("ERROR: hash == %0128h", hash);
		//
		#100;
		//
		$display("Checking 256-bit mode on short message...");
		//
		streebog_init(STREEBOG_MODE_SHORT);
		streebog_set_block(MSG_SINGLE, MSG_SINGLE_LENGTH);
		streebog_update();
		streebog_final();
		//
		if (hash_short == MSG_SINGLE_DIGEST_SHORT) $display("OK");
		else $display("ERROR: hash_short == %064h", hash_short);
		//
		#100;
		//
		$display("Checking 512-bit mode on long message...");
		//
		streebog_init(STREEBOG_MODE_LONG);
		streebog_set_block(MSG_DOUBLE_FIRST, MSG_DOUBLE_FIRST_LENGTH);
		streebog_update();
		streebog_set_block(MSG_DOUBLE_SECOND, MSG_DOUBLE_SECOND_LENGTH);
		streebog_update();		
		streebog_final();
		//
		if (hash == MSG_DOUBLE_DIGEST_LONG) $display("OK");
		else $display("ERROR: hash == %0128h", hash);		
		//
		#100;
		//
		$display("Checking 256-bit mode on long message...");
		//
		streebog_init(STREEBOG_MODE_SHORT);
		streebog_set_block(MSG_DOUBLE_FIRST, MSG_DOUBLE_FIRST_LENGTH);
		streebog_update();
		streebog_set_block(MSG_DOUBLE_SECOND, MSG_DOUBLE_SECOND_LENGTH);
		streebog_update();		
		streebog_final();
		//
		if (hash_short == MSG_DOUBLE_DIGEST_SHORT) $display("OK");
		else $display("ERROR: hash_short == %064h", hash_short);
		//
		#100;
		//
		$finish;
	end
      
		
	task streebog_init;
		input	use_short_mode;
		begin
			short_mode	= use_short_mode;
			init			= 1;
			#10;
			init			= 0;
			#10;
		end
	endtask
	
	
	task streebog_set_block;
		input		[511:0]	new_block;
		input		[  9:0]	new_block_length;
		begin
			block				= new_block;
			block_length	= new_block_length;
			
		end
	endtask;
	
	
	task streebog_update;
		begin
			update	= 1;
			#10;
			update	= 0;
			#10
			while (!ready) #10;
			#10;
		end
	endtask
	
	
	task streebog_final;
		begin
			final		= 1;
			#10;
			final		= 0;
			#10
			while (!digest_valid) #10;
			hash = digest;
			#10;
			while (!ready) #10;
			#10;
		end
	endtask
	
endmodule

