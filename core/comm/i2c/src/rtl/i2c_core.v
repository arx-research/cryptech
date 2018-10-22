//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2011, Andrew "bunnie" Huang
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, 
// are permitted provided that the following conditions are met:
//
//  * Redistributions of source code must retain the above copyright notice, 
//    this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright notice, 
//    this list of conditions and the following disclaimer in the documentation and/or 
//    other materials provided with the distribution.
//
//    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY 
//    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
//    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
//    SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
//    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
//    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
//    PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
//    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
//    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
//    POSSIBILITY OF SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////////////////////
// A simple I2C slave implementation. Oversampled for robustness.
// The slave is extended into the snoop & surpress version for the DDC bus;
// this is just a starting point for basic testing and also simple comms
// with the CPU.
//
// i2c slave module requires the top level module to implement the IOBs
// This is just to keep the tri-state easy to implemen across the hierarchy
//
// The code required on the top level is:
//   IOBUF #(.DRIVE(12), .SLEW("SLOW")) IOBUF_sda (.IO(SDA), .I(1'b0), .T(!SDA_pd));
//
///////////
`timescale 1 ns / 1 ps

// This file is based on https://github.com/bunnie/novena-gpbb-fpga/blob/master/novena-gpbb.srcs/sources_1/imports/imports/i2c_slave.v
//
// For Cryptech, we replaced the register interface with the rxd/txd
// interface to coretest, and changed i2c_device_addr from an 8-bit
// input to a 7-bit output.

module i2c_core (
                 input wire 	    clk,
                 input wire 	    reset,

                 // External data interface
		 input wire 	    SCL,
		 input wire 	    SDA,
		 output reg 	    SDA_pd,
		 output wire [6:0]  i2c_device_addr,

                 // Internal receive interface.
                 output wire 	    rxd_syn,
                 output [7 : 0]     rxd_data,
                 input wire 	    rxd_ack,

                 // Internal transmit interface.
                 input wire 	    txd_syn,
                 input wire [7 : 0] txd_data,
                 output wire 	    txd_ack
		 );

   /////// I2C physical layer components
   /// SDA is stable when SCL is high.
   /// If SDA moves while SCL is high, this is considered a start or stop condition.
   ///
   /// Otherwise, SDA can move around when SCL is low (this is where we suppress bits or 
   /// overdrive as needed). SDA is a wired-AND bus, so you only "drive" zero.
   ///
   /// In an oversampled implementation, a rising and falling edge de-glitcher is needed
   /// for SCL and SDA.
   ///

   // rise fall time cycles computation:
   // At 400kHz operation, 2.5us is a cycle. "chatter" from transition should be about
   // 5% of total cycle time max (just rule of thumb), so 0.125us should be the equiv
   // number of cycles.
   // For the demo board, a 25 MHz clock is provided, and 0.125us ~ 4 cycles
   // At 100kHz operation, 10us is a cycle, so 0.5us ~ 12 cycles
   parameter TRF_CYCLES = 5'd4;  // number of cycles for rise/fall time
   
   ////////////////
   ///// protocol-level state machine
   ////////////////
   parameter I2C_START     = 16'b1 << 0; // should only pass through this state for one cycle
   parameter I2C_RESTART   = 16'b1 << 1;
   parameter I2C_DADDR     = 16'b1 << 2;
   parameter I2C_ACK_DADDR = 16'b1 << 3;
   parameter I2C_WR_DATA   = 16'b1 << 4;
   parameter I2C_ACK_WR    = 16'b1 << 5;
   parameter I2C_END_WR    = 16'b1 << 6;
   parameter I2C_RD_DATA   = 16'b1 << 7;
   parameter I2C_ACK_RD    = 16'b1 << 8;
   parameter I2C_END_RD    = 16'b1 << 9;
   parameter I2C_END_RD2   = 16'b1 << 10;
   parameter I2C_WAITSTOP  = 16'b1 << 11;
   parameter I2C_RXD_SYN   = 16'b1 << 12;
   parameter I2C_RXD_ACK   = 16'b1 << 13;
   parameter I2C_TXD_SYN   = 16'b1 << 14;
   parameter I2C_TXD_ACK   = 16'b1 << 15;

   parameter I2C_nSTATES = 16;

   reg [(I2C_nSTATES-1):0]     I2C_cstate = {{(I2C_nSTATES-1){1'b0}}, 1'b1};  //current and next states
   reg [(I2C_nSTATES-1):0]     I2C_nstate;

//`define SIMULATION  
`ifdef SIMULATION
   // synthesis translate_off
   reg [8*20:1] 	                    I2C_state_ascii = "I2C_START          ";
   always @(I2C_cstate) begin
      if      (I2C_cstate == I2C_START)     I2C_state_ascii <= "I2C_START          ";
      else if (I2C_cstate == I2C_RESTART)   I2C_state_ascii <= "I2C_RESTART        ";
      else if (I2C_cstate == I2C_DADDR)     I2C_state_ascii <= "I2C_DADDR          ";
      else if (I2C_cstate == I2C_ACK_DADDR) I2C_state_ascii <= "I2C_ACK_DADDR      ";
      else if (I2C_cstate == I2C_WR_DATA)   I2C_state_ascii <= "I2C_WR_DATA        ";
      else if (I2C_cstate == I2C_ACK_WR)    I2C_state_ascii <= "I2C_ACK_WR         ";
      else if (I2C_cstate == I2C_END_WR)    I2C_state_ascii <= "I2C_END_WR         ";
      else if (I2C_cstate == I2C_RD_DATA)   I2C_state_ascii <= "I2C_RD_DATA        ";
      else if (I2C_cstate == I2C_ACK_RD)    I2C_state_ascii <= "I2C_ACK_RD         ";
      else if (I2C_cstate == I2C_END_RD)    I2C_state_ascii <= "I2C_END_RD         ";
      else if (I2C_cstate == I2C_END_RD2)   I2C_state_ascii <= "I2C_END_RD2        ";
      else if (I2C_cstate == I2C_WAITSTOP)  I2C_state_ascii <= "I2C_WAITSTOP       ";
      else if (I2C_cstate == I2C_RXD_SYN)   I2C_state_ascii <= "I2C_RXD_SYN        ";
      else if (I2C_cstate == I2C_RXD_ACK)   I2C_state_ascii <= "I2C_RXD_ACK        ";
      else if (I2C_cstate == I2C_TXD_SYN)   I2C_state_ascii <= "I2C_TXD_SYN        ";
      else if (I2C_cstate == I2C_TXD_ACK)   I2C_state_ascii <= "I2C_TXD_ACK        ";
      else                                  I2C_state_ascii <= "WTF                ";
   end
   // synthesis translate_on
`endif
   
   reg [3:0] 		       I2C_bitcnt;
   reg [7:0] 		       I2C_daddr;
   reg [7:0] 		       I2C_wdata;
   reg [7:0] 		       I2C_rdata;

   reg 			       rxd_syn_reg;
   reg 			       txd_ack_reg;

   assign rxd_data = I2C_wdata;
   assign rxd_syn  = rxd_syn_reg;
   assign txd_ack  = txd_ack_reg;
   assign i2c_device_addr = I2C_daddr[7:1];

   ////////// code begins here
   always @ (posedge clk) begin
      if (reset || ((SCL_cstate == SCL_HIGH) && (SDA_cstate == SDA_RISE))) // stop condition always resets
	I2C_cstate <= I2C_START; 
      else
	I2C_cstate <= I2C_nstate;
   end

   always @ (*) begin
      case (I2C_cstate)
	I2C_START: begin // wait for the start condition
	   I2C_nstate = ((SDA_cstate == SDA_FALL) && (SCL_cstate == SCL_HIGH)) ? I2C_DADDR : I2C_START;
	end
	I2C_RESTART: begin // repeated start moves immediately to DADDR
	   I2C_nstate = I2C_DADDR;
	end

	// device address branch
	I2C_DADDR: begin // 8 bits to get the address
	   I2C_nstate = ((I2C_bitcnt > 4'h7) && (SCL_cstate == SCL_FALL)) ? I2C_ACK_DADDR : I2C_DADDR;
	end
	I2C_ACK_DADDR: begin // depending upon W/R bit state, go to one of two branches
	   I2C_nstate = (SCL_cstate == SCL_FALL) ?
			(I2C_daddr[0] == 1'b0 ? I2C_WR_DATA : I2C_TXD_SYN) :
			I2C_ACK_DADDR; // !SCL_FALL
	end

	// write branch
	I2C_WR_DATA: begin // 8 bits to get the write data
	   I2C_nstate = ((SDA_cstate == SDA_FALL) && (SCL_cstate == SCL_HIGH)) ? I2C_RESTART : // repeated start
			((I2C_bitcnt > 4'h7) && (SCL_cstate == SCL_FALL)) ? I2C_RXD_SYN : I2C_WR_DATA;
	end
	I2C_RXD_SYN: begin // put data on the coretest bus
	   I2C_nstate = I2C_RXD_ACK;
	end
	I2C_RXD_ACK: begin // wait for coretest ack
           I2C_nstate = rxd_ack ? I2C_ACK_WR : I2C_RXD_ACK;
	end
	I2C_ACK_WR: begin // trigger the ack response (pull SDA low until next falling edge)
	   // and stay in this state until the next falling edge of SCL
	   I2C_nstate = (SCL_cstate == SCL_FALL) ? I2C_END_WR : I2C_ACK_WR;
	end
	I2C_END_WR: begin // one-cycle state to update address+1, reset SDA pulldown
	   I2C_nstate = I2C_WR_DATA; // SCL is now low
	end

	// read branch
	I2C_TXD_SYN: begin // get data from the coretest bus 
	   // if data isn't available (txd_syn isn't asserted) by the time we
	   // get to this state, it probably never will be, so skip it
           I2C_nstate = txd_syn ? I2C_TXD_ACK : I2C_RD_DATA;
	end
	I2C_TXD_ACK: begin // send coretest ack
	   // hold ack high until syn is lowered
	   I2C_nstate = txd_syn ? I2C_TXD_ACK : I2C_RD_DATA;
	end
	I2C_RD_DATA: begin // 8 bits to get the read data
	   I2C_nstate = ((SDA_cstate == SDA_FALL) && (SCL_cstate == SCL_HIGH)) ? I2C_RESTART : // repeated start
			((I2C_bitcnt > 4'h7) && (SCL_cstate == SCL_FALL)) ? I2C_ACK_RD : I2C_RD_DATA;
	end
	I2C_ACK_RD: begin // wait for an (n)ack response
	   // need to sample (n)ack on a rising edge
	   I2C_nstate = (SCL_cstate == SCL_RISE) ? I2C_END_RD : I2C_ACK_RD;
	end
	I2C_END_RD: begin // if nack, just go to start state (don't explicitly check stop event)
	   // single cycle state for adr+1 update
	   I2C_nstate = (SDA_cstate == SDA_LOW) ? I2C_END_RD2 : I2C_START;
	end
	I2C_END_RD2: begin // before entering I2C_RD_DATA, we need to have seen a falling edge.
	   I2C_nstate = (SCL_cstate == SCL_FALL) ? I2C_RD_DATA : I2C_END_RD2;
	end

	// we're not the addressed device, so we just idle until we see a stop
	I2C_WAITSTOP: begin
	   I2C_nstate = (((SCL_cstate == SCL_HIGH) && (SDA_cstate == SDA_RISE))) ? // stop
			I2C_START : 
			(((SCL_cstate == SCL_HIGH) && (SDA_cstate == SDA_FALL))) ? // or start
			I2C_RESTART :
			I2C_WAITSTOP;
	end
      endcase // case (cstate)
   end

   always @ (posedge clk) begin
      if( reset ) begin
	 I2C_bitcnt <= 4'b0;
	 I2C_daddr <= 8'b0;
	 I2C_wdata <= 8'b0;
	 SDA_pd <= 1'b0;
	 I2C_rdata <= 8'b0;
      end else begin
	 case (I2C_cstate)
	   I2C_START: begin // everything in reset
	      I2C_bitcnt <= 4'b0;
	      I2C_daddr <= 8'b0;
	      I2C_wdata <= 8'b0;
	      I2C_rdata <= 8'b0;
	      SDA_pd <= 1'b0;
	   end

	   I2C_RESTART: begin
	      I2C_bitcnt <= 4'b0;
	      I2C_daddr <= 8'b0;
	      I2C_wdata <= 8'b0;
	      I2C_rdata <= 8'b0;
	      SDA_pd <= 1'b0;
	   end

	   // get my i2c device address (am I being talked to?)
	   I2C_DADDR: begin // shift in the address on rising edges of clock
	      if( SCL_cstate == SCL_RISE ) begin
		 I2C_bitcnt <= I2C_bitcnt + 4'b1;
		 I2C_daddr[7] <= I2C_daddr[6];
		 I2C_daddr[6] <= I2C_daddr[5];
		 I2C_daddr[5] <= I2C_daddr[4];
		 I2C_daddr[4] <= I2C_daddr[3];
		 I2C_daddr[3] <= I2C_daddr[2];
		 I2C_daddr[2] <= I2C_daddr[1];
		 I2C_daddr[1] <= I2C_daddr[0];
		 I2C_daddr[0] <= (SDA_cstate == SDA_HIGH) ? 1'b1 : 1'b0;
	      end else begin // we're oversampled so we need a hold-state gutter
		 I2C_bitcnt <= I2C_bitcnt;
		 I2C_daddr <= I2C_daddr;
	      end // else: !if( SCL_cstate == SCL_RISE )
	      SDA_pd <= 1'b0;
	      I2C_wdata <= 8'b0;
	      I2C_rdata <= 8'b0;
	   end // case: I2C_DADDR
	   I2C_ACK_DADDR: begin
	      SDA_pd <= 1'b1;  // active pull down ACK
	      I2C_daddr <= I2C_daddr;
	      I2C_bitcnt <= 4'b0;
	      I2C_wdata <= 8'b0;
	      I2C_rdata <= 8'b0;
	   end

	   // write branch
	   I2C_WR_DATA: begin // shift in data on rising edges of clock
	      if( SCL_cstate == SCL_RISE ) begin
		 I2C_bitcnt <= I2C_bitcnt + 4'b1;
		 I2C_wdata[7] <= I2C_wdata[6];
		 I2C_wdata[6] <= I2C_wdata[5];
		 I2C_wdata[5] <= I2C_wdata[4];
		 I2C_wdata[4] <= I2C_wdata[3];
		 I2C_wdata[3] <= I2C_wdata[2];
		 I2C_wdata[2] <= I2C_wdata[1];
		 I2C_wdata[1] <= I2C_wdata[0];
		 I2C_wdata[0] <= (SDA_cstate == SDA_HIGH) ? 1'b1 : 1'b0;
	      end else begin
		 I2C_bitcnt <= I2C_bitcnt; // hold state gutter
		 I2C_wdata <= I2C_wdata;
	      end // else: !if( SCL_cstate == SCL_RISE )
	      SDA_pd <= 1'b0;
	      I2C_daddr <= I2C_daddr;
	      I2C_rdata <= I2C_rdata;
	   end // case: I2C_WR_DATA
	   I2C_RXD_SYN: begin // put data on the coretest bus and raise syn
              rxd_syn_reg <= 1;
	   end
	   I2C_RXD_ACK: begin // wait for coretest ack
              if (rxd_ack)
		 rxd_syn_reg <= 0;
	   end
	   I2C_ACK_WR: begin
	      SDA_pd <= 1'b1;  // active pull down ACK
	      I2C_daddr <= I2C_daddr;
	      I2C_bitcnt <= 4'b0;
	      I2C_wdata <= I2C_wdata;
	      I2C_rdata <= I2C_rdata;
	   end
	   I2C_END_WR: begin
	      SDA_pd <= 1'b0; // let SDA rise (host may look for this to know ack is done
	      I2C_bitcnt <= 4'b0;
	      I2C_wdata <= 8'b0;
	      I2C_rdata <= I2C_rdata;
	      I2C_daddr <= I2C_daddr;
	   end

	   // read branch
	   I2C_TXD_SYN: begin // get data from the coretest bus
              if (txd_syn) begin
		 I2C_rdata <= txd_data;
		 txd_ack_reg <= 1;
	      end
	   end
	   I2C_TXD_ACK: begin // send coretest ack
              if (!txd_syn)
		 txd_ack_reg <= 0;
	   end
	   I2C_RD_DATA: begin // shift out data on falling edges of clock
	      SDA_pd <= I2C_rdata[7] ? 1'b0 : 1'b1;
	      if( SCL_cstate == SCL_RISE ) begin
		 I2C_bitcnt <= I2C_bitcnt + 4'b1;
	      end else begin
		 I2C_bitcnt <= I2C_bitcnt; // hold state gutter
	      end
	      
	      if( SCL_cstate == SCL_FALL ) begin
		 I2C_rdata[7] <= I2C_rdata[6];
		 I2C_rdata[6] <= I2C_rdata[5];
		 I2C_rdata[5] <= I2C_rdata[4];
		 I2C_rdata[4] <= I2C_rdata[3];
		 I2C_rdata[3] <= I2C_rdata[2];
		 I2C_rdata[2] <= I2C_rdata[1];
		 I2C_rdata[1] <= I2C_rdata[0];
		 I2C_rdata[0] <= 1'b0;
	      end else begin
		 I2C_rdata <= I2C_rdata;
	      end // else: !if( SCL_cstate == SCL_RISE )
	      I2C_daddr <= I2C_daddr;
	      I2C_wdata <= I2C_wdata;
	   end // case: I2C_RD_DATA
	   I2C_ACK_RD: begin
	      SDA_pd <= 1'b0;  // in ack state don't pull down, we are listening to host
	      I2C_daddr <= I2C_daddr;
	      I2C_bitcnt <= 4'b0;
	      I2C_rdata <= I2C_rdata;
	      I2C_wdata <= I2C_wdata;
	   end
	   I2C_END_RD: begin
	      SDA_pd <= 1'b0; // let SDA rise (host may look for this to know ack is done
	      I2C_daddr <= I2C_daddr;
	      I2C_bitcnt <= 4'b0;
	      I2C_rdata <= I2C_rdata;
	      I2C_wdata <= I2C_wdata;
	   end
	   I2C_END_RD2: begin
	      SDA_pd <= 1'b0;
	      I2C_daddr <= 8'b0;
	      I2C_bitcnt <= 4'b0;
	      I2C_rdata <= I2C_rdata;
	      I2C_wdata <= I2C_wdata;
	   end

	   I2C_WAITSTOP: begin
	      SDA_pd <= 1'b0;
	      I2C_daddr <= 8'b0;
	      I2C_bitcnt <= 4'b0;
	      I2C_rdata <= I2C_rdata;
	      I2C_wdata <= I2C_wdata;
	   end
	 endcase // case (cstate)
      end // else: !if( reset )
   end // always @ (posedge clk or posedge reset)


   ///////////////////////////////////////////////////////////////
   /////////// low level state machines //////////////////////////
   ///////////////////////////////////////////////////////////////
   
   
   ////////////////
   ///// SCL low-level sampling state machine
   ////////////////
   parameter SCL_HIGH = 4'b1 << 0; // should only pass through this state for one cycle
   parameter SCL_FALL = 4'b1 << 1;
   parameter SCL_LOW  = 4'b1 << 2;
   parameter SCL_RISE = 4'b1 << 3;
   parameter SCL_nSTATES = 4;

   reg [(SCL_nSTATES-1):0]     SCL_cstate = {{(SCL_nSTATES-1){1'b0}}, 1'b1};  //current and next states
   reg [(SCL_nSTATES-1):0]     SCL_nstate;

//`define SIMULATION  
`ifdef SIMULATION
   // synthesis translate_off
   reg [8*20:1] 	                 SCL_state_ascii = "SCL_HIGH           ";

   always @(SCL_cstate) begin
      if      (SCL_cstate == SCL_HIGH)     SCL_state_ascii <= "SCL_HIGH           ";
      else if (SCL_cstate == SCL_FALL)     SCL_state_ascii <= "SCL_FALL           ";
      else if (SCL_cstate == SCL_LOW )     SCL_state_ascii <= "SCL_LOW            ";
      else if (SCL_cstate == SCL_RISE)     SCL_state_ascii <= "SCL_RISE           ";
      else SCL_state_ascii                                 <= "WTF                ";
   end
   // synthesis translate_on
`endif

   reg [4:0] 		       SCL_rfcnt;
   reg 			       SCL_s, SCL_sync;
   reg 			       SDA_s, SDA_sync;

   always @ (posedge clk) begin
      if (reset)
	SCL_cstate <= SCL_HIGH; // always start here even if it's wrong -- easier to test
      else
	SCL_cstate <= SCL_nstate;
   end

   always @ (*) begin
      case (SCL_cstate)
	SCL_HIGH: begin
	   SCL_nstate = ((SCL_rfcnt > TRF_CYCLES) && (SCL_sync == 1'b0)) ? SCL_FALL : SCL_HIGH;
	end
	SCL_FALL: begin
	   SCL_nstate = SCL_LOW;
	end
	SCL_LOW: begin
	   SCL_nstate = ((SCL_rfcnt > TRF_CYCLES) && (SCL_sync == 1'b1)) ? SCL_RISE : SCL_LOW;
	end
	SCL_RISE: begin
	   SCL_nstate = SCL_HIGH;
	end
      endcase // case (cstate)
   end // always @ (*)

   always @ (posedge clk) begin
      if( reset ) begin
	 SCL_rfcnt <= 5'b0;
      end else begin
	 case (SCL_cstate)
	   SCL_HIGH: begin
	      if( SCL_sync == 1'b1 ) begin
		 SCL_rfcnt <= 5'b0;
	      end else begin
		 SCL_rfcnt <= SCL_rfcnt + 5'b1;
	      end
	   end
	   SCL_FALL: begin
	      SCL_rfcnt <= 5'b0;
	   end
	   SCL_LOW: begin
	      if( SCL_sync == 1'b0 ) begin
		 SCL_rfcnt <= 5'b0;
	      end else begin
		 SCL_rfcnt <= SCL_rfcnt + 5'b1;
	      end
	   end
	   SCL_RISE: begin
	      SCL_rfcnt <= 5'b0;
	   end
	 endcase // case (cstate)
      end // else: !if( reset )
   end // always @ (posedge clk or posedge reset)


   ////////////////
   ///// SDA low-level sampling state machine
   ////////////////
   parameter SDA_HIGH = 4'b1 << 0; // should only pass through this state for one cycle
   parameter SDA_FALL = 4'b1 << 1;
   parameter SDA_LOW  = 4'b1 << 2;
   parameter SDA_RISE = 4'b1 << 3;
   parameter SDA_nSTATES = 4;

   reg [(SDA_nSTATES-1):0]     SDA_cstate = {{(SDA_nSTATES-1){1'b0}}, 1'b1};  //current and next states
   reg [(SDA_nSTATES-1):0]     SDA_nstate;

//`define SIMULATION  
`ifdef SIMULATION
   // synthesis translate_off
   reg [8*20:1] 	                 SDA_state_ascii = "SDA_HIGH           ";

   always @(SDA_cstate) begin
      if      (SDA_cstate == SDA_HIGH)     SDA_state_ascii <= "SDA_HIGH           ";
      else if (SDA_cstate == SDA_FALL)     SDA_state_ascii <= "SDA_FALL           ";
      else if (SDA_cstate == SDA_LOW )     SDA_state_ascii <= "SDA_LOW            ";
      else if (SDA_cstate == SDA_RISE)     SDA_state_ascii <= "SDA_RISE           ";
      else SDA_state_ascii                                 <= "WTF                ";
   end
   // synthesis translate_on
`endif

   reg [4:0] 		       SDA_rfcnt;

   always @ (posedge clk) begin
      if (reset)
	SDA_cstate <= SDA_HIGH; // always start here even if it's wrong -- easier to test
      else
	SDA_cstate <= SDA_nstate;
   end

   always @ (*) begin
      case (SDA_cstate)
	SDA_HIGH: begin
	   SDA_nstate = ((SDA_rfcnt > TRF_CYCLES) && (SDA_sync == 1'b0)) ? SDA_FALL : SDA_HIGH;
	end
	SDA_FALL: begin
	   SDA_nstate = SDA_LOW;
	end
	SDA_LOW: begin
	   SDA_nstate = ((SDA_rfcnt > TRF_CYCLES) && (SDA_sync == 1'b1)) ? SDA_RISE : SDA_LOW;
	end
	SDA_RISE: begin
	   SDA_nstate = SDA_HIGH;
	end
      endcase // case (cstate)
   end // always @ (*)

   always @ (posedge clk) begin
      if( reset ) begin
	 SDA_rfcnt <= 5'b0;
      end else begin
	 case (SDA_cstate)
	   SDA_HIGH: begin
	      if( SDA_sync == 1'b1 ) begin
		 SDA_rfcnt <= 5'b0;
	      end else begin
		 SDA_rfcnt <= SDA_rfcnt + 5'b1;
	      end
	   end
	   SDA_FALL: begin
	      SDA_rfcnt <= 5'b0;
	   end
	   SDA_LOW: begin
	      if( SDA_sync == 1'b0 ) begin
		 SDA_rfcnt <= 5'b0;
	      end else begin
		 SDA_rfcnt <= SDA_rfcnt + 5'b1;
	      end
	   end
	   SDA_RISE: begin
	      SDA_rfcnt <= 5'b0;
	   end
	 endcase // case (cstate)
      end // else: !if( reset )
   end // always @ (posedge clk or posedge reset)

   
   
   /////////////////////
   /////// synchronizers
   /////////////////////
   always @ (posedge clk) begin
      SCL_s <= SCL;
      SCL_sync <= SCL_s;
      SDA_s <= SDA;
      SDA_sync <= SDA_s;
   end // always @ (posedge clk or posedge reset)
   
endmodule // i2c_slave
