//======================================================================
//
// fmc_arbiter.v
// -------------
// Port arbiter for the FMC interface for the Cryptech
// Novena FPGA + STM32 Bridge Board framework.
//
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

module fmc_arbiter
  (
		// fmc bus
		fmc_clk,
		fmc_a, fmc_d,
		fmc_ne1, fmc_nl, fmc_nwe, fmc_noe, fmc_nwait,
		
		// system clock
		sys_clk,

		// user bus
		sys_addr, 
		sys_wr_en,
		sys_data_out,
		sys_rd_en,
		sys_data_in
   );
	
	
	//
	// Parameters
	//
	parameter NUM_ADDR_BITS = 22;
	
	
	//
	// Ports
	//
	input		wire								fmc_clk;
	input		wire	[NUM_ADDR_BITS-1:0]	fmc_a;
	inout		wire	[             31:0]	fmc_d;
	input		wire								fmc_ne1;
	input		wire								fmc_nl;
	input		wire								fmc_nwe;
	input		wire								fmc_noe;
	output	wire								fmc_nwait;
	
	input		wire								sys_clk;
	
	output	wire	[NUM_ADDR_BITS-1:0]	sys_addr;
	output	wire								sys_wr_en;
	output	wire	[             31:0]	sys_data_out;
	output	wire								sys_rd_en;
	input		wire	[             31:0]	sys_data_in;
		

   //
   // Data Bus PHY
   //

   /* PHY is needed to control bi-directional data bus. */

   wire	[31: 0]  d_ro;   // value read from pins (receiver output)
   reg	[31: 0]	d_di;   // value drives onto pins (driver input)

   fmc_d_phy #
	(
		.BUS_WIDTH(32)
	)
	d_phy
	(
		.buf_io(fmc_d),          // <-- connect directly to top-level bi-dir port
		.buf_di(d_di),
		.buf_ro(d_ro),
		.buf_t(fmc_noe)          // <-- bus direction is controlled by STM32
	);
	

   //
   // FSM
   //
   localparam   FMC_FSM_STATE_INIT      			= 5'b0_0_000; // arbiter is idle

   localparam   FMC_FSM_STATE_WRITE_START 		= 5'b1_1_000; // got address to write at
   localparam   FMC_FSM_STATE_WRITE_LATENCY_1	= 5'b1_1_001; // dummy state to compensate STM32's latency
	localparam   FMC_FSM_STATE_WRITE_LATENCY_2	= 5'b1_1_010; // dummy state to compensate STM32's latency
	localparam   FMC_FSM_STATE_WRITE_LATENCY_3	= 5'b1_1_011; // dummy state to compensate STM32's latency
	localparam   FMC_FSM_STATE_WRITE_DATABEAT		= 5'b1_1_100; // got data to write
   localparam   FMC_FSM_STATE_WRITE_WAIT			= 5'b1_1_101; // request to user-side logic sent
   localparam   FMC_FSM_STATE_WRITE_DONE			= 5'b1_1_111; // user-side logic acknowledged transaction

   localparam   FMC_FSM_STATE_READ_START  		= 5'b1_0_000; // got address to read from
	localparam   FMC_FSM_STATE_READ_LATENCY_1		= 5'b1_0_001; // dummy state to compensate STM32's latency
	localparam   FMC_FSM_STATE_READ_LATENCY_2		= 5'b1_0_010; // dummy state to compensate STM32's latency
	localparam   FMC_FSM_STATE_READ_LATENCY_3		= 5'b1_0_011; // dummy state to compensate STM32's latency
   localparam   FMC_FSM_STATE_READ_WAIT   		= 5'b1_0_101; // request to user-side logic sent
   localparam   FMC_FSM_STATE_READ_READY  		= 5'b1_0_110; // got acknowledge from user logic
   localparam   FMC_FSM_STATE_READ_DATABEAT		= 5'b1_0_100; // returned data to master
   localparam   FMC_FSM_STATE_READ_DONE   		= 5'b1_0_111; // transaction complete

   reg [              4:0]  fmc_fsm_state   = FMC_FSM_STATE_INIT;   		// fsm state
   reg [NUM_ADDR_BITS-1:0]  fmc_addr_latch  = {NUM_ADDR_BITS{1'bX}};		// transaction address
	reg [             31:0]  fmc_data_latch = {32{1'bX}};						// write data latch

   /* These flags are used to wake up from INIT state. */
   wire         fmc_write_start_flag = (fmc_ne1 == 1'b0) && (fmc_nwe == 1'b0) && (fmc_nl == 1'b0);
   wire         fmc_read_start_flag  = (fmc_ne1 == 1'b0) && (fmc_nwe == 1'b1) && (fmc_nl == 1'b0);

   /* These are transaction response flag and data from user-side logic. */
   wire         fmc_user_ack;
   wire [31: 0] fmc_user_data;

   //
   // FSM Transition Logic
   //
   always @(posedge fmc_clk)
		//
		case (fmc_fsm_state)
			//
			// INIT -> WRITE, INIT -> READ
			//
			FMC_FSM_STATE_INIT: begin
				//
				if (fmc_write_start_flag)	fmc_fsm_state	<= FMC_FSM_STATE_WRITE_START;
				if (fmc_read_start_flag)	fmc_fsm_state	<= FMC_FSM_STATE_READ_START;
				//
			end
			//
			// WRITE
			//
			FMC_FSM_STATE_WRITE_START:										fmc_fsm_state  <= FMC_FSM_STATE_WRITE_LATENCY_1;
			FMC_FSM_STATE_WRITE_LATENCY_1:								fmc_fsm_state	<= FMC_FSM_STATE_WRITE_LATENCY_2;
			FMC_FSM_STATE_WRITE_LATENCY_2:								fmc_fsm_state	<= FMC_FSM_STATE_WRITE_LATENCY_3;
			FMC_FSM_STATE_WRITE_LATENCY_3:								fmc_fsm_state	<= FMC_FSM_STATE_WRITE_DATABEAT;
			FMC_FSM_STATE_WRITE_DATABEAT:									fmc_fsm_state  <= FMC_FSM_STATE_WRITE_WAIT;
			FMC_FSM_STATE_WRITE_WAIT:			if (fmc_user_ack)		fmc_fsm_state	<= FMC_FSM_STATE_WRITE_DONE;
			FMC_FSM_STATE_WRITE_DONE:										fmc_fsm_state  <= FMC_FSM_STATE_INIT;
			//
			// READ
			//
			FMC_FSM_STATE_READ_START:									fmc_fsm_state	<= FMC_FSM_STATE_READ_LATENCY_1;
			FMC_FSM_STATE_READ_LATENCY_1:								fmc_fsm_state	<= FMC_FSM_STATE_READ_LATENCY_2;
			FMC_FSM_STATE_READ_LATENCY_2:								fmc_fsm_state	<= FMC_FSM_STATE_READ_LATENCY_3;
			FMC_FSM_STATE_READ_LATENCY_3:								fmc_fsm_state	<= FMC_FSM_STATE_READ_WAIT;
			FMC_FSM_STATE_READ_WAIT:			if (fmc_user_ack)	fmc_fsm_state	<= FMC_FSM_STATE_READ_READY;
			FMC_FSM_STATE_READ_READY:									fmc_fsm_state	<= FMC_FSM_STATE_READ_DATABEAT;
			FMC_FSM_STATE_READ_DATABEAT:								fmc_fsm_state	<= FMC_FSM_STATE_READ_DONE;
			FMC_FSM_STATE_READ_DONE:									fmc_fsm_state	<= FMC_FSM_STATE_INIT;
			//
			default:															fmc_fsm_state	<= FMC_FSM_STATE_INIT;
			//
		endcase


   //
   // Address Latch
   //
   always @(posedge fmc_clk)
     //
     if ((fmc_fsm_state == FMC_FSM_STATE_INIT) && (fmc_write_start_flag || fmc_read_start_flag))
		//
		fmc_addr_latch <= fmc_a;


	//
   // Additional Write Logic (Data Latch)
   //
   always @(posedge fmc_clk)
     //
     if (fmc_fsm_state == FMC_FSM_STATE_WRITE_LATENCY_3)
		//
       fmc_data_latch <= d_ro;


   //
   // Additional Read Logic (Read Latch)
   //
	
	/* Note that this register is updated on the falling edge of FMC_CLK, because
	 * STM32 samples bi-directional data bus on the rising edge.
	 */
	
   always @(negedge fmc_clk)
     //
     if (fmc_fsm_state == FMC_FSM_STATE_READ_DATABEAT)
		//
		d_di <= fmc_user_data;
	
        

   //
   // Wait Logic
   //
   reg  fmc_wait_reg    = 1'b0;

   always @(posedge fmc_clk)
     //
	  begin
        //
        if ( (fmc_fsm_state == FMC_FSM_STATE_WRITE_START) ||
				 (fmc_fsm_state == FMC_FSM_STATE_READ_START) )
          fmc_wait_reg  <= 1'b1;                // start waiting for read/write to complete
        /*
        if ( (fmc_fsm_state == FMC_FSM_STATE_WRITE_DONE) ||
             (fmc_fsm_state == FMC_FSM_STATE_READ_READY) )
          fmc_wait_reg  <= 1'b0;
        */
        if (fmc_fsm_state == FMC_FSM_STATE_INIT)
          fmc_wait_reg  <= 1'b0;                // fsm is idle, no need to wait any more
        //
     end

   assign fmc_nwait = ~fmc_wait_reg;


   /* These flags are used to generate 1-cycle pulses to trigger CDC
    * transaction.  Note that FSM goes from WRITE_DATABEAT to WRITE_WAIT and from
    * READ_LATENCY_3 to READ_WAIT unconditionally, so these flags will always be
    * active for 1 cycle only, which is exactly what we need.
    */

   wire arbiter_write_req_pulse = (fmc_fsm_state == FMC_FSM_STATE_WRITE_DATABEAT)  ? 1'b1 : 1'b0;
   wire arbiter_read_req_pulse  = (fmc_fsm_state == FMC_FSM_STATE_READ_LATENCY_3)  ? 1'b1 : 1'b0;

   //
   // CDC Block
   //

   /* This block is used to transfer request data from FMC_CLK clock domain to
    * SYS_CLK clock domain and then transfer acknowledge from SYS_CLK to FMC_CLK
    * clock domain in return. Af first 1+1+22+32 = 56 bits are transfered,
    * these are: write flag, read flag, address, write data. During read transaction
	 * some bogus write data is passed, which is not used later anyway.
	 * During read requests 32 bits of data are returned, during write requests
	 * 32 bits of bogus data are returned, that are never used later.
    */

   fmc_arbiter_cdc #
	(
		.NUM_ADDR_BITS(NUM_ADDR_BITS)
	)
	fmc_cdc
	(
      .fmc_clk(fmc_clk),

      .fmc_req(arbiter_write_req_pulse | arbiter_read_req_pulse),
      .fmc_ack(fmc_user_ack),

      .fmc_din({arbiter_write_req_pulse, arbiter_read_req_pulse, fmc_addr_latch, fmc_data_latch}),
      .fmc_dout(fmc_user_data),

      .sys_clk(sys_clk),
      .sys_addr(sys_addr),
      .sys_wren(sys_wr_en),
      .sys_data_out(sys_data_out),
      .sys_rden(sys_rd_en),
      .sys_data_in(sys_data_in)
	);


endmodule


//======================================================================
// EOF fmc_arbiter.v
//======================================================================
