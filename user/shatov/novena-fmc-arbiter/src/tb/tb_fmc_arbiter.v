//======================================================================
//
// tb_fmc_arbiter.v
// -------------
// Test bench for FMC Arbiter module, read and write transactions
// from STM32 are simulated in this file.
//
// Author: Pavel Shatov
// Copyright (c) 2015, NORDUnet A/S All rights reserved.
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

module tb_fmc_arbiter;


		//
		// FMC Side
		//
	reg				fmc_clk;
	reg	[21: 0]	fmc_a;
	wire	[31: 0]	fmc_d;
	reg	[31: 0]	fmc_d_reg;
	reg				fmc_d_drive;
	reg				fmc_ne1;
	reg				fmc_nl;
	reg				fmc_nwe;
	reg				fmc_noe;
	wire				fmc_nwait;
	

		//
		// Data Bus
		//
	assign fmc_d = fmc_d_drive ? fmc_d_reg : {32{1'bZ}};
	
	
		//
		// System Side
		//
	reg				sys_clk = 1'b0;
	wire	[21: 0]	sys_addr;
	wire				sys_wr_en;
	reg	[31: 0]	sys_data_in = 32'hCA53CA53;
	wire				sys_rd_en;
	wire	[31: 0]	sys_data_out;


	reg	[31: 0]	test_reg;
	always @(posedge sys_clk)
		//
		if (sys_wr_en) begin
			//
			test_reg <= (sys_addr == {22{1'b0}}) ? sys_data_out : {{10{1'b0}}, sys_addr};
			//
		end else if (sys_rd_en) begin
			//
			sys_data_in <= test_reg;
			//
		end
	

		//
		// UUT
		//
	fmc_arbiter #
	(
		.NUM_ADDR_BITS(22)
	)
	uut
	(
		.fmc_clk			(fmc_clk), 
		.fmc_a			(fmc_a), 
		.fmc_d			(fmc_d), 
		.fmc_ne1			(fmc_ne1), 
		.fmc_nl			(fmc_nl), 
		.fmc_nwe			(fmc_nwe), 
		.fmc_noe			(fmc_noe), 
		.fmc_nwait		(fmc_nwait), 
		
		.sys_clk			(sys_clk), 
		.sys_addr		(sys_addr), 
		.sys_wr_en		(sys_wr_en), 
		.sys_data_out	(sys_data_out), 
		.sys_rd_en		(sys_rd_en), 
		.sys_data_in	(sys_data_in)
	);
	
	
		//
		// System Clock (50 MHz)
		//
	always #10 sys_clk = ~sys_clk;
	
	
		//
		// FMC Clock (100 MHz)
		//
	always #5.5 fmc_clk = ~fmc_clk;


		//
		// Initial FMC State
		//
	initial begin
		fmc_clk		= 1'b0;
		fmc_a			= {22{1'bX}};
		fmc_d_reg	= {32{1'bX}};
		fmc_d_drive	= 1'b0;
		fmc_ne1		= 1'b1;
		fmc_nl		= 1'b1;
		fmc_nwe		= 1'b1;
		fmc_noe		= 1'b1;
	end
		
		
		//
		// Script
		//
	reg	[31: 0]	rd;
	initial begin
		//
		#500;
		//
		fmc_write(22'h3ABCDE, 32'hFEDCBA98);
		while (fmc_nwait == 1'b0) #0.1;
		//
		#1000;
		//
		fmc_read(22'h3ABCDE, rd);
		while (fmc_nwait == 1'b0) #0.1;
		fmc_read(22'h3ABCDE, rd);
		//
		#1000;
		//
	end
      
		
		
		//
		// Write Transaction
		//
	integer tick;
	task fmc_write;
		input [21: 0] addr;
		input [31: 0] data;
		begin
			//
			fmc_wait_posedge;
			fmc_wait_negedge;
			//
			fmc_ne1		= 1'b0;	// select device
			fmc_nwe		= 1'b0;	// transaction type is write
			fmc_nl		= 1'b0;	// address is valid
			fmc_a			= addr;	// set address
			//
			fmc_wait_posedge;
			fmc_wait_negedge;
			//
			fmc_nl		= 1'b1;	// address is no longer valid
			//
			for (tick=0; tick<2; tick=tick+1) begin
				fmc_wait_posedge;
				fmc_wait_negedge;
			end
			//
			fmc_d_reg	= data;	// set data
			fmc_d_drive	= 1'b1;	// enable driver
			//
			fmc_wait_posedge;
			fmc_wait_negedge;
			//
			fmc_ne1		= 1'b1;	// deselect device
			fmc_nwe		= 1'b1;	// clear
			fmc_d_drive	= 1'b0;	// disable driver
			//
		end
	endtask;
	
	
		//
		// Read Transaction
		//
	task fmc_read;
		input  [21: 0] addr;
		output [31: 0] data;
		begin
			//
			fmc_wait_posedge;
			fmc_wait_negedge;
			//
			fmc_ne1		= 1'b0;	// select device
			fmc_nl		= 1'b0;	// address is valid
			fmc_a			= addr;	// set address
			//
			fmc_wait_posedge;
			fmc_wait_negedge;
			//
			fmc_nl		= 1'b1;	// address is no longer valid
			//
			for (tick=0; tick<2; tick=tick+1) begin
				fmc_wait_posedge;
				fmc_wait_negedge;
			end
			//
			fmc_noe		= 1'b0;	// reverse bus direction
			fmc_wait_posedge;
			data = fmc_d;
			fmc_wait_negedge;
			//
			fmc_ne1		= 1'b1;	// deselect device
			fmc_noe		= 1'b1;	// reverse bus direction
			//		
		/*
					fmc_ne1		= 1'b0;
					fmc_nl		= 1'b0;
					fmc_a			= addr;
			#5		fmc_clk		= 1'b1;
			#5		fmc_nl		= 1'b1;
					fmc_clk		= 1'b0;
			#5		fmc_clk		= 1'b1;
			#5		fmc_clk		= 1'b0;
					fmc_noe		= 1'b0;
			#5		fmc_clk		= 1'b1;
			#5		fmc_clk		= 1'b0;
			#5		
			while (fmc_nwait == 1'b0) begin
					fmc_clk		= 1'b1;
			#5		fmc_clk		= 1'b0;
			#5;
			end
					data			= fmc_d;
					fmc_clk		= 1'b1;
			#5		fmc_clk		= 1'b0;
			#5		fmc_ne1		= 1'b1;
					fmc_noe		= 1'b1;
			#10;
			*/
		end
	endtask;


	task fmc_wait_posedge;
		begin
			while (fmc_clk != 1'b1) #0.1;
			#0.1;
		end
	endtask;
	
	task fmc_wait_negedge;
		begin
			while (fmc_clk != 1'b0) #0.1;
			#0.1;
		end
	endtask;
	
endmodule

//======================================================================
// EOF tb_fmc_arbiter.v
//======================================================================
