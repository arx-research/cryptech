//======================================================================
//
// eim_arbiter.v
// -------------
// Port arbiter for the EIM interface for the Cryptech
// Novena FPGA framework.
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

module eim_arbiter
  (
   // eim bus
   input wire          eim_bclk, 
   input wire          eim_cs0_n,
   inout wire [15: 0]  eim_da,
   input wire [18:16]  eim_a,
   input wire          eim_lba_n,
   input wire          eim_wr_n,
   input wire          eim_oe_n,
   output wire         eim_wait_n,

   // system clock
   input wire          sys_clk,

   // user bus
   output wire [16: 0] sys_addr, 
   output wire         sys_wren,
   output wire [31: 0] sys_data_out,
   output wire         sys_rden,
   input wire [31: 0]  sys_data_in
   );


   //
   // Data/Address PHY
   //

   /* PHY is needed to control bi-directional address/data bus. */

   wire [15: 0]        da_ro;   // value read from pins
   reg [15: 0]         da_di;   // value drives onto pins

   eim_da_phy da_phy
     (
      .buf_io(eim_da),          // <-- connect directly top-level port
      .buf_di(da_di),
      .buf_ro(da_ro),
      .buf_t(eim_oe_n)          // <-- driven by EIM directly
      );


   //
   // FSM
   //
   localparam   EIM_FSM_STATE_INIT        = 5'b0_0_000; // arbiter is idle

   localparam   EIM_FSM_STATE_WRITE_START = 5'b1_1_000; // got address to write at
   localparam   EIM_FSM_STATE_WRITE_LSB   = 5'b1_1_001; // got lower 16 bits of data to write
   localparam   EIM_FSM_STATE_WRITE_MSB   = 5'b1_1_010; // got upper 16 bits of data to write
   localparam   EIM_FSM_STATE_WRITE_WAIT  = 5'b1_1_100; // request to user-side logic sent
   localparam   EIM_FSM_STATE_WRITE_DONE  = 5'b1_1_111; // user-side logic acknowledged transaction

   localparam   EIM_FSM_STATE_READ_START  = 5'b1_0_000; // got address to read from
   localparam   EIM_FSM_STATE_READ_WAIT   = 5'b1_0_100; // request to user-side logic sent
   localparam   EIM_FSM_STATE_READ_READY  = 5'b1_0_011; // got acknowledge from user logic
   localparam   EIM_FSM_STATE_READ_LSB    = 5'b1_0_001; // returned lower 16 bits to master
   localparam   EIM_FSM_STATE_READ_MSB    = 5'b1_0_010; // returned upper 16 bits to master
   localparam   EIM_FSM_STATE_READ_DONE   = 5'b1_0_111; // transaction complete

   reg [ 4: 0]  eim_fsm_state   = EIM_FSM_STATE_INIT;   // fsm state
   reg [16: 0]  eim_addr_latch  = {17{1'bX}};           // transaction address
   reg [15: 0]  eim_write_lsb_latch = {16{1'bX}};       // lower 16 bits of data to write

   /* These flags are used to wake up from INIT state. */
   wire         eim_write_start_flag = (eim_lba_n == 1'b0) && (eim_wr_n == 1'b0) && (da_ro[1:0] == 2'b00);
   wire         eim_read_start_flag  = (eim_lba_n == 1'b0) && (eim_wr_n == 1'b1) && (da_ro[1:0] == 2'b00);

   /* These are transaction response flag and data from user-side logic. */
   wire         eim_user_ack;
   wire [31: 0] eim_user_data;

   /* FSM is reset whenever Chip Select is de-asserted. */

   //
   // FSM Transition Logic
   //
   always @(posedge eim_bclk or posedge eim_cs0_n)
     begin
        //
        if (eim_cs0_n == 1'b1)
          eim_fsm_state <= EIM_FSM_STATE_INIT;
        //
        else
          begin
             //
             case (eim_fsm_state)
               //
               // INIT -> WRITE, INIT -> READ
               //
               EIM_FSM_STATE_INIT:
                 begin
                    if (eim_write_start_flag)
                      eim_fsm_state     <= EIM_FSM_STATE_WRITE_START;
                    if (eim_read_start_flag)
                      eim_fsm_state     <= EIM_FSM_STATE_READ_START;
                 end
               //
               // WRITE
               //
               EIM_FSM_STATE_WRITE_START:
                 eim_fsm_state  <= EIM_FSM_STATE_WRITE_LSB;
               //
               EIM_FSM_STATE_WRITE_LSB:
                 eim_fsm_state  <= EIM_FSM_STATE_WRITE_MSB;
               //
               EIM_FSM_STATE_WRITE_MSB:
                 eim_fsm_state  <= EIM_FSM_STATE_WRITE_WAIT;
               //
               EIM_FSM_STATE_WRITE_WAIT:
                 if (eim_user_ack)
                   eim_fsm_state <= EIM_FSM_STATE_WRITE_DONE;
               //
               EIM_FSM_STATE_WRITE_DONE:
                 eim_fsm_state  <= EIM_FSM_STATE_INIT;
               //
               // READ
               //
               EIM_FSM_STATE_READ_START:
                 eim_fsm_state  <= EIM_FSM_STATE_READ_WAIT;
               //
               EIM_FSM_STATE_READ_WAIT:
                 if (eim_user_ack)
                   eim_fsm_state <= EIM_FSM_STATE_READ_READY;
               //
               EIM_FSM_STATE_READ_READY:
                 eim_fsm_state <= EIM_FSM_STATE_READ_LSB;
               //
               EIM_FSM_STATE_READ_LSB:
                 eim_fsm_state  <= EIM_FSM_STATE_READ_MSB;
               //
               EIM_FSM_STATE_READ_MSB:
                 eim_fsm_state  <= EIM_FSM_STATE_READ_DONE;
               //
               EIM_FSM_STATE_READ_DONE:
                 eim_fsm_state  <= EIM_FSM_STATE_INIT;
               //
               //
               //
               default:
                 eim_fsm_state  <= EIM_FSM_STATE_INIT;
               //
             endcase
             //
          end
        //
     end


   //
   // Address Latch
   //
   always @(posedge eim_bclk)
     //
     if ((eim_fsm_state == EIM_FSM_STATE_INIT) && (eim_write_start_flag || eim_read_start_flag))
       eim_addr_latch <= {eim_a[18:16], da_ro[15:2]};


   //
   // Additional Write Logic
   //
   always @(posedge eim_bclk)
     //
     if (eim_fsm_state == EIM_FSM_STATE_WRITE_START)
       eim_write_lsb_latch <= da_ro;


   //
   // Additional Read Logic
   //

   /* Note that this stuff operates on falling clock edge, because the cpu
    * samples our bi-directional data bus on rising clock edge.
    */

   always @(negedge eim_bclk or posedge eim_cs0_n)
     //
     if (eim_cs0_n == 1'b1)                                                                             da_di <= {16{1'bX}};                    // don't care what to drive
     else begin
        //
        if (eim_fsm_state == EIM_FSM_STATE_READ_LSB)
          da_di <= eim_user_data[15: 0];        // drive lower 16 bits at first...
        if (eim_fsm_state == EIM_FSM_STATE_READ_MSB)
          da_di <= eim_user_data[31:16];        // ...then drive upper 16 bits
        //
     end


   //
   // Wait Logic
   //

   /* Note that this stuff operates on falling clock edge, because the cpu
    *  samples our WAIT_N flag on rising clock edge.
    */

   reg  eim_wait_reg    = 1'b0;

   always @(negedge eim_bclk or posedge eim_cs0_n)
     //
     if (eim_cs0_n == 1'b1)
       eim_wait_reg     <= 1'b0;                // clear wait
     else begin
        //
        if (eim_fsm_state == EIM_FSM_STATE_WRITE_START)
          eim_wait_reg  <= 1'b1;                // start waiting for write to complete
        if (eim_fsm_state == EIM_FSM_STATE_READ_START)
          eim_wait_reg  <= 1'b1;                // start waiting for read to complete
        //
        if (eim_fsm_state == EIM_FSM_STATE_WRITE_DONE)
          eim_wait_reg  <= 1'b0;                // write transaction done
        if (eim_fsm_state == EIM_FSM_STATE_READ_READY)
          eim_wait_reg  <= 1'b0;                // read transaction done
        //
        if (eim_fsm_state == EIM_FSM_STATE_INIT)
          eim_wait_reg  <= 1'b0;                // fsm is idle, no need to wait any more
        //
     end

   assign eim_wait_n = ~eim_wait_reg;


   /* These flags are used to generate 1-cycle pulses to trigger CDC
    * transaction.  Note that FSM goes from WRITE_LSB to WRITE_MSB and from
    * READ_START to READ_WAIT unconditionally, so these flags will always be
    * active for 1 cycle only, which is exactly what we need.
    */

   wire arbiter_write_req_pulse = (eim_fsm_state == EIM_FSM_STATE_WRITE_LSB)  ? 1'b1 : 1'b0;
   wire arbiter_read_req_pulse  = (eim_fsm_state == EIM_FSM_STATE_READ_START) ? 1'b1 : 1'b0;

   //
   // CDC Block
   //

   /* This block is used to transfer request data from BCLK clock domain to
    * SYS_CLK clock domain and then transfer acknowledge from SYS_CLK to BCLK
    * clock domain in return. Af first 1+1+3+14+32 = 51 bits are transfered,
    * these are: write flag, read flag, msb part of address, lsb part of address,
    * write data. During read transaction some bogus write data is passed,
    * which is not used later anyway. During read requests 32 bits of data are
    * returned, during write requests 32 bits of bogus data are returned, that
    * are never used later.
    */

   eim_arbiter_cdc eim_cdc
     (
      .eim_clk(eim_bclk),

      .eim_req(arbiter_write_req_pulse | arbiter_read_req_pulse),
      .eim_ack(eim_user_ack),

      .eim_din({arbiter_write_req_pulse, arbiter_read_req_pulse,
                eim_addr_latch, da_ro, eim_write_lsb_latch}),
      .eim_dout(eim_user_data),

      .sys_clk(sys_clk),
      .sys_addr(sys_addr),
      .sys_wren(sys_wren),
      .sys_data_out(sys_data_out),
      .sys_rden(sys_rden),
      .sys_data_in(sys_data_in)
      );


endmodule

//======================================================================
// EOF eim_arbiter.v
//======================================================================
