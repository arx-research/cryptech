module streebog_wrapper
	(
		input		wire           clk,
		input		wire           rst,

		input		wire           cs,
		input		wire           we,

		input		wire  [ 7: 0]	address,
		input		wire  [31: 0]	write_data,
		output	wire	[31: 0]	read_data
	);

	  //----------------------------------------------------------------
	  // Internal constant and parameter definitions.
	  //----------------------------------------------------------------
	localparam ADDR_NAME0		= 8'h00;
	localparam ADDR_NAME1		= 8'h01;
	localparam ADDR_VERSION		= 8'h02;

	localparam ADDR_CTRL			= 8'h08;		// {short, final, update, init}
	localparam ADDR_STATUS		= 8'h09;		// {valid, ready}
	localparam ADDR_BLOCK_BITS	= 8'h0a;		// block length in bits
	localparam ADDR_MODE			= 8'h0b;		// 0=long (512-bit) mode, 1=short (256-bit) mode

	localparam ADDR_BLOCK0		= 8'h10;
	localparam ADDR_BLOCK1		= 8'h11;
	localparam ADDR_BLOCK2		= 8'h12;
	localparam ADDR_BLOCK3		= 8'h13;
	localparam ADDR_BLOCK4		= 8'h14;
	localparam ADDR_BLOCK5		= 8'h15;
	localparam ADDR_BLOCK6		= 8'h16;
	localparam ADDR_BLOCK7		= 8'h17;
	localparam ADDR_BLOCK8		= 8'h18;
	localparam ADDR_BLOCK9		= 8'h19;
	localparam ADDR_BLOCK10		= 8'h1a;
	localparam ADDR_BLOCK11		= 8'h1b;
	localparam ADDR_BLOCK12		= 8'h1c;
	localparam ADDR_BLOCK13		= 8'h1d;
	localparam ADDR_BLOCK14		= 8'h1e;
	localparam ADDR_BLOCK15		= 8'h1f;

	localparam ADDR_DIGEST0		= 8'h20;
	localparam ADDR_DIGEST1		= 8'h21;
	localparam ADDR_DIGEST2		= 8'h22;
	localparam ADDR_DIGEST3		= 8'h23;
	localparam ADDR_DIGEST4		= 8'h24;
	localparam ADDR_DIGEST5		= 8'h25;
	localparam ADDR_DIGEST6		= 8'h26;
	localparam ADDR_DIGEST7		= 8'h27;
	localparam ADDR_DIGEST8		= 8'h28;
	localparam ADDR_DIGEST9		= 8'h29;
	localparam ADDR_DIGEST10	= 8'h2a;
	localparam ADDR_DIGEST11	= 8'h2b;
	localparam ADDR_DIGEST12	= 8'h2c;
	localparam ADDR_DIGEST13	= 8'h2d;
	localparam ADDR_DIGEST14	= 8'h2e;
	localparam ADDR_DIGEST15	= 8'h2f;


	localparam CTRL_INIT_BIT		= 0;
	localparam CTRL_UPDATE_BIT		= 1;
	localparam CTRL_FINAL_BIT		= 2;

	localparam STATUS_READY_BIT	= 0;
	localparam STATUS_VALID_BIT	= 1;

	localparam CORE_NAME0     = 32'h73747265;	// "stre"
	localparam CORE_NAME1     = 32'h65626F67;	// "ebog"
	localparam CORE_VERSION   = 32'h302E3130;	// "0.10"


		//----------------------------------------------------------------
		// Control register
		//----------------------------------------------------------------
	reg	[2:0]	reg_ctrl;			// core input
	reg	[9:0]	reg_block_bits;	// input block length in bits
	reg			reg_mode;			// long/short mode
	

		//----------------------------------------------------------------
		// Init, Update and Final 1-Cycle Pulses
		//----------------------------------------------------------------
	reg	[2:0]	reg_ctrl_dly;
	always @(posedge clk) reg_ctrl_dly <= reg_ctrl;

	wire core_init_pulse		= (reg_ctrl[CTRL_INIT_BIT]   == 1'b1) && (reg_ctrl_dly[CTRL_INIT_BIT]   == 1'b0);
	wire core_update_pulse	= (reg_ctrl[CTRL_UPDATE_BIT] == 1'b1) && (reg_ctrl_dly[CTRL_UPDATE_BIT] == 1'b0);
	wire core_final_pulse	= (reg_ctrl[CTRL_FINAL_BIT]  == 1'b1) && (reg_ctrl_dly[CTRL_FINAL_BIT]  == 1'b0);


		//----------------------------------------------------------------
		// Status register
		//----------------------------------------------------------------
	wire  core_ready;		// core output
	wire  digest_valid;	// core output

	wire [1:0] reg_status = {digest_valid, core_ready};


		//----------------------------------------------------------------
		// Block and Digest
		//----------------------------------------------------------------
	reg  [511 : 0] core_block;		// core input
	wire [511 : 0] core_digest;	// core output


	//----------------------------------------------------------------
	// core instantiation.
	//----------------------------------------------------------------
	streebog_hash_top streebog
	(
		.clock			(clk),
		
		.block			(core_block),
		.block_length	(reg_block_bits),
		
		.init				(core_init_pulse),
		.update			(core_update_pulse),
		.final			(core_final_pulse),
		
		.short_mode		(reg_mode),
		
		.digest			(core_digest),
		.digest_valid	(digest_valid),
		
		.ready			(core_ready)
	);

		//----------------------------------------------------------------
		// Read Latch
		//----------------------------------------------------------------
	reg [31: 0] tmp_read_data;

	assign read_data = tmp_read_data;


	//----------------------------------------------------------------
	// Read/Write Interface
	//----------------------------------------------------------------
	always @(posedge clk)
		//
		if (rst) begin
			//
			reg_ctrl			<= 3'b000;
			reg_block_bits	<= 10'd0;
			reg_mode			<= 1'b0;
			core_block		<= {512{1'b0}};
			tmp_read_data	<= 32'h00000000;
			//
		end else if (cs) begin
			//
			if (we) begin
				//
				// Write Handler
				//
				case (address)
					ADDR_CTRL:			reg_ctrl					<= write_data[2:0];
					ADDR_BLOCK_BITS:	reg_block_bits			<= write_data[9:0];
					ADDR_MODE:			reg_mode					<= write_data[0];
					ADDR_BLOCK0:		core_block[511:480]	<= write_data;
					ADDR_BLOCK1:		core_block[479:448]	<= write_data;
					ADDR_BLOCK2:		core_block[447:416]	<= write_data;
					ADDR_BLOCK3:		core_block[415:384]	<= write_data;
					ADDR_BLOCK4:		core_block[383:352]	<= write_data;
					ADDR_BLOCK5:		core_block[351:320]	<= write_data;
					ADDR_BLOCK6:		core_block[319:288]	<= write_data;
					ADDR_BLOCK7:		core_block[287:256]	<= write_data;
					ADDR_BLOCK8:		core_block[255:224]	<= write_data;
					ADDR_BLOCK9:		core_block[223:192]	<= write_data;
					ADDR_BLOCK10:		core_block[191:160]	<= write_data;
					ADDR_BLOCK11:		core_block[159:128]	<= write_data;
					ADDR_BLOCK12:		core_block[127: 96]	<= write_data;
					ADDR_BLOCK13:		core_block[ 95: 64]	<= write_data;
					ADDR_BLOCK14:		core_block[ 63: 32]	<= write_data;
					ADDR_BLOCK15:		core_block[ 31:  0]	<= write_data;
				endcase
				//	
			end else begin
				//
				// Read Handler
				//
				case (address)
					//
					ADDR_NAME0:			tmp_read_data <= CORE_NAME0;
					ADDR_NAME1:			tmp_read_data <= CORE_NAME1;
					ADDR_VERSION:		tmp_read_data <= CORE_VERSION;
					ADDR_CTRL:			tmp_read_data <= {{28{1'b0}}, reg_ctrl};
					ADDR_STATUS:		tmp_read_data <= {{30{1'b0}}, reg_status};
					ADDR_BLOCK_BITS:	tmp_read_data <= {{22{1'b0}}, reg_block_bits};
					ADDR_MODE:			tmp_read_data <= {{31{1'b0}}, reg_mode};
					//
					ADDR_BLOCK0:		tmp_read_data <= core_block[511:480];
					ADDR_BLOCK1:		tmp_read_data <= core_block[479:448];
					ADDR_BLOCK2:		tmp_read_data <= core_block[447:416];
					ADDR_BLOCK3:		tmp_read_data <= core_block[415:384];
					ADDR_BLOCK4:		tmp_read_data <= core_block[383:352];
					ADDR_BLOCK5:		tmp_read_data <= core_block[351:320];
					ADDR_BLOCK6:		tmp_read_data <= core_block[319:288];
					ADDR_BLOCK7:		tmp_read_data <= core_block[287:256];
					ADDR_BLOCK8:		tmp_read_data <= core_block[255:224];
					ADDR_BLOCK9:		tmp_read_data <= core_block[223:192];
					ADDR_BLOCK10:		tmp_read_data <= core_block[191:160];
					ADDR_BLOCK11:		tmp_read_data <= core_block[159:128];
					ADDR_BLOCK12:		tmp_read_data <= core_block[127: 96];
					ADDR_BLOCK13:		tmp_read_data <= core_block[ 95: 64];
					ADDR_BLOCK14:		tmp_read_data <= core_block[ 63: 32];
					ADDR_BLOCK15:		tmp_read_data <= core_block[ 31:  0];
					//
					ADDR_DIGEST0:		tmp_read_data <= core_digest[511:480];
					ADDR_DIGEST1:		tmp_read_data <= core_digest[479:448];
					ADDR_DIGEST2:		tmp_read_data <= core_digest[447:416];
					ADDR_DIGEST3:		tmp_read_data <= core_digest[415:384];
					ADDR_DIGEST4:		tmp_read_data <= core_digest[383:352];
					ADDR_DIGEST5:		tmp_read_data <= core_digest[351:320];
					ADDR_DIGEST6:		tmp_read_data <= core_digest[319:288];
					ADDR_DIGEST7:		tmp_read_data <= core_digest[287:256];
					ADDR_DIGEST8:		tmp_read_data <= core_digest[255:224];
					ADDR_DIGEST9:		tmp_read_data <= core_digest[223:192];
					ADDR_DIGEST10:		tmp_read_data <= core_digest[191:160];
					ADDR_DIGEST11:		tmp_read_data <= core_digest[159:128];
					ADDR_DIGEST12:		tmp_read_data <= core_digest[127: 96];
					ADDR_DIGEST13:		tmp_read_data <= core_digest[ 95: 64];
					ADDR_DIGEST14:		tmp_read_data <= core_digest[ 63: 32];
					ADDR_DIGEST15:		tmp_read_data <= core_digest[ 31:  0];
					//
					default:				tmp_read_data <= 32'h00000000;
					//
				endcase
				//
			end
			//
		end


endmodule // streebog_wrapper


//======================================================================
// EOF streebog_wrapper.v
//======================================================================
