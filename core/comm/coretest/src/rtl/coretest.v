//======================================================================
//
// coretest.v
// ----------
// The Cryptech coretest testing module. Combined with an external
// interface that sends and receives bytes using a SYN-ACK
// handshake and a core to be tested, coretest can parse read
// and write commands needed to test the connected core.
//
//
// Author: Joachim Strombergson
// Copyright (c) 2014, NORDUnet A/S
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

module coretest(
                input wire           clk,
                input wire           reset_n,

                // Interface to communication core
                input wire           rx_syn,
                input wire [7 : 0]   rx_data,
                output wire          rx_ack,

                output wire          tx_syn,
                output wire [7 : 0]  tx_data,
                input wire           tx_ack,

                // Interface to the core being tested.
                output wire          core_reset_n,
                output wire          core_cs,
                output wire          core_we,
                output wire [15 : 0] core_address,
                output wire [31 : 0] core_write_data,
                input wire  [31 : 0] core_read_data,
                input wire           core_error
               );


  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  // Max elements in read and write buffers.
  parameter BUFFER_MAX = 4'hf;

  // Command constants.
  parameter SOC       = 8'h55;
  parameter EOC       = 8'haa;
  parameter RESET_CMD = 8'h01;
  parameter READ_CMD  = 8'h10;
  parameter WRITE_CMD = 8'h11;

  // Response constants.
  parameter SOR      = 8'haa;
  parameter EOR      = 8'h55;
  parameter UNKNOWN  = 8'hfe;
  parameter ERROR    = 8'hfd;
  parameter READ_OK  = 8'h7f;
  parameter WRITE_OK = 8'h7e;
  parameter RESET_OK = 8'h7d;

  // rx_engine states.
  parameter RX_IDLE  = 3'h0;
  parameter RX_ACK   = 3'h1;
  parameter RX_NSYN  = 3'h2;

  // tx_engine states.
  parameter TX_IDLE  = 3'h0;
  parameter TX_SYN   = 3'h1;
  parameter TX_NOACK = 3'h2;
  parameter TX_NEXT  = 3'h3;
  parameter TX_SENT  = 3'h4;
  parameter TX_DONE  = 3'h5;

  // test_engine states.
  parameter TEST_IDLE          = 8'h00;
  parameter TEST_GET_CMD       = 8'h10;
  parameter TEST_PARSE_CMD     = 8'h11;
  parameter TEST_GET_ADDR0     = 8'h20;
  parameter TEST_GET_ADDR1     = 8'h21;
  parameter TEST_GET_DATA0     = 8'h24;
  parameter TEST_GET_DATA1     = 8'h25;
  parameter TEST_GET_DATA2     = 8'h26;
  parameter TEST_GET_DATA3     = 8'h27;
  parameter TEST_GET_EOC       = 8'h28;
  parameter TEST_RST_START     = 8'h30;
  parameter TEST_RST_WAIT      = 8'h31;
  parameter TEST_RST_END       = 8'h32;
  parameter TEST_RD_START      = 8'h50;
  parameter TEST_RD_WAIT       = 8'h51;
  parameter TEST_RD_END        = 8'h52;
  parameter TEST_WR_START      = 8'h60;
  parameter TEST_WR_WAIT       = 8'h61;
  parameter TEST_WR_END        = 8'h62;
  parameter TEST_CMD_UNKNOWN   = 8'h80;
  parameter TEST_CMD_ERROR     = 8'h81;
  parameter TEST_SEND_RESPONSE = 8'hc0;


  //----------------------------------------------------------------
  // Registers including update variables and write enable.
  //----------------------------------------------------------------
  reg          rx_syn_reg;

  reg          rx_ack_reg;
  reg          rx_ack_new;
  reg          rx_ack_we;

  reg          tx_syn_reg;
  reg          tx_syn_new;
  reg          tx_syn_we;

  reg          tx_ack_reg;

  reg          core_reset_n_reg;
  reg          core_reset_n_new;
  reg          core_reset_n_we;

  reg          core_cs_reg;
  reg          core_cs_new;
  reg          core_cs_we;

  reg          core_we_reg;
  reg          core_we_new;
  reg          core_we_we;

  reg          send_response_reg;
  reg          send_response_new;
  reg          send_response_we;

  reg          response_sent_reg;
  reg          response_sent_new;
  reg          response_sent_we;

  reg [7 : 0]  cmd_reg;
  reg          cmd_we;

  reg [7 : 0]  core_addr_byte0_reg;
  reg          core_addr_byte0_we;
  reg [7 : 0]  core_addr_byte1_reg;
  reg          core_addr_byte1_we;

  reg [7 : 0]  core_wr_data_byte0_reg;
  reg          core_wr_data_byte0_we;
  reg [7 : 0]  core_wr_data_byte1_reg;
  reg          core_wr_data_byte1_we;
  reg [7 : 0]  core_wr_data_byte2_reg;
  reg          core_wr_data_byte2_we;
  reg [7 : 0]  core_wr_data_byte3_reg;
  reg          core_wr_data_byte3_we;

  reg [31 : 0] core_read_data_reg;
  reg          core_error_reg;
  reg          sample_core_output;

  reg [3 : 0]  rx_buffer_rd_ptr_reg;
  reg [3 : 0]  rx_buffer_rd_ptr_new;
  reg          rx_buffer_rd_ptr_we;
  reg          rx_buffer_rd_ptr_inc;

  reg [3 : 0]  rx_buffer_wr_ptr_reg;
  reg [3 : 0]  rx_buffer_wr_ptr_new;
  reg          rx_buffer_wr_ptr_we;
  reg          rx_buffer_wr_ptr_inc;

  reg [3 : 0]  rx_buffer_ctr_reg;
  reg [3 : 0]  rx_buffer_ctr_new;
  reg          rx_buffer_ctr_we;
  reg          rx_buffer_ctr_inc;
  reg          rx_buffer_ctr_dec;
  reg          rx_buffer_full;
  reg          rx_buffer_empty;

  reg [7 : 0]  rx_buffer [0 : 15];
  reg          rx_buffer_we;

  reg [3 : 0]  tx_buffer_ptr_reg;
  reg [3 : 0]  tx_buffer_ptr_new;
  reg          tx_buffer_ptr_we;
  reg          tx_buffer_ptr_inc;
  reg          tx_buffer_ptr_rst;

  reg [7 : 0]  tx_buffer [0 : 8];
  reg          tx_buffer_we;

  reg [3 : 0]  tx_msg_len_reg;
  reg [3 : 0]  tx_msg_len_new;
  reg          tx_msg_len_we;

  reg [2 : 0]  rx_engine_reg;
  reg [2 : 0]  rx_engine_new;
  reg          rx_engine_we;

  reg [2 : 0]  tx_engine_reg;
  reg [2 : 0]  tx_engine_new;
  reg          tx_engine_we;

  reg [7 : 0]  test_engine_reg;
  reg [7 : 0]  test_engine_new;
  reg          test_engine_we;


  //----------------------------------------------------------------
  // Wires.
  //----------------------------------------------------------------
  reg [7 : 0] tx_buffer_muxed0;
  reg [7 : 0] tx_buffer_muxed1;
  reg [7 : 0] tx_buffer_muxed2;
  reg [7 : 0] tx_buffer_muxed3;
  reg [7 : 0] tx_buffer_muxed4;
  reg [7 : 0] tx_buffer_muxed5;
  reg [7 : 0] tx_buffer_muxed6;
  reg [7 : 0] tx_buffer_muxed7;
  reg [7 : 0] tx_buffer_muxed8;

  reg         update_tx_buffer;
  reg [7 : 0] response_type;

  reg [7 : 0] rx_byte;


  //----------------------------------------------------------------
  // Concurrent connectivity for ports etc.
  //----------------------------------------------------------------
  assign rx_ack          = rx_ack_reg;

  assign tx_syn          = tx_syn_reg;
  assign tx_data         = tx_buffer[tx_buffer_ptr_reg];

  assign core_reset_n    = core_reset_n_reg & reset_n;
  assign core_cs         = core_cs_reg;
  assign core_we         = core_we_reg;
  assign core_address    = {core_addr_byte0_reg, core_addr_byte1_reg};
  assign core_write_data = {core_wr_data_byte0_reg, core_wr_data_byte1_reg,
                            core_wr_data_byte2_reg, core_wr_data_byte3_reg};


  //----------------------------------------------------------------
  // reg_update
  // Update functionality for all registers in the core.
  // All registers are positive edge triggered with synchronous
  // active low reset. All registers have write enable.
  //----------------------------------------------------------------
  always @ (posedge clk)
    begin: reg_update
      if (!reset_n)
        begin
          rx_buffer[0]           <= 8'h00;
          rx_buffer[1]           <= 8'h00;
          rx_buffer[2]           <= 8'h00;
          rx_buffer[3]           <= 8'h00;
          rx_buffer[4]           <= 8'h00;
          rx_buffer[5]           <= 8'h00;
          rx_buffer[6]           <= 8'h00;
          rx_buffer[7]           <= 8'h00;
          rx_buffer[8]           <= 8'h00;
          rx_buffer[9]           <= 8'h00;
          rx_buffer[10]          <= 8'h00;
          rx_buffer[11]          <= 8'h00;
          rx_buffer[12]          <= 8'h00;
          rx_buffer[13]          <= 8'h00;
          rx_buffer[14]          <= 8'h00;
          rx_buffer[15]          <= 8'h00;

          tx_buffer[0]           <= 8'h00;
          tx_buffer[1]           <= 8'h00;
          tx_buffer[2]           <= 8'h00;
          tx_buffer[3]           <= 8'h00;
          tx_buffer[4]           <= 8'h00;
          tx_buffer[5]           <= 8'h00;
          tx_buffer[6]           <= 8'h00;
          tx_buffer[7]           <= 8'h00;
          tx_buffer[8]           <= 8'h00;

          rx_syn_reg             <= 0;
          rx_ack_reg             <= 0;
          tx_ack_reg             <= 0;
          tx_syn_reg             <= 0;

          rx_buffer_rd_ptr_reg   <= 4'h0;
          rx_buffer_wr_ptr_reg   <= 4'h0;
          rx_buffer_ctr_reg      <= 4'h0;

          tx_buffer_ptr_reg      <= 4'h0;
          tx_msg_len_reg         <= 4'h0;

          send_response_reg      <= 0;
          response_sent_reg      <= 0;

          cmd_reg                <= 8'h00;
          core_addr_byte0_reg    <= 8'h00;
          core_addr_byte1_reg    <= 8'h00;
          core_wr_data_byte0_reg <= 8'h00;
          core_wr_data_byte1_reg <= 8'h00;
          core_wr_data_byte2_reg <= 8'h00;
          core_wr_data_byte3_reg <= 8'h00;

          core_reset_n_reg       <= 1;
          core_cs_reg            <= 0;
          core_we_reg            <= 0;
          core_error_reg         <= 0;
          core_read_data_reg     <= 32'h00000000;

          rx_engine_reg          <= RX_IDLE;
          tx_engine_reg          <= TX_IDLE;
          test_engine_reg        <= TEST_IDLE;
        end
      else
        begin
          rx_syn_reg <= rx_syn;
          tx_ack_reg <= tx_ack;

          if (rx_ack_we)
            begin
              rx_ack_reg <= rx_ack_new;
            end

          if (tx_syn_we)
            begin
              tx_syn_reg <= tx_syn_new;
            end

          if (rx_buffer_we)
            begin
              rx_buffer[rx_buffer_wr_ptr_reg] <= rx_data;
            end

          if (tx_buffer_we)
            begin
              tx_buffer[0] <= tx_buffer_muxed0;
              tx_buffer[1] <= tx_buffer_muxed1;
              tx_buffer[2] <= tx_buffer_muxed2;
              tx_buffer[3] <= tx_buffer_muxed3;
              tx_buffer[4] <= tx_buffer_muxed4;
              tx_buffer[5] <= tx_buffer_muxed5;
              tx_buffer[6] <= tx_buffer_muxed6;
              tx_buffer[7] <= tx_buffer_muxed7;
              tx_buffer[8] <= tx_buffer_muxed8;
            end

          if (cmd_we)
            begin
              cmd_reg <= rx_byte;
            end

          if (core_addr_byte0_we)
            begin
              core_addr_byte0_reg <= rx_byte;
            end

          if (core_addr_byte1_we)
            begin
              core_addr_byte1_reg <= rx_byte;
            end

          if (core_wr_data_byte0_we)
            begin
              core_wr_data_byte0_reg <= rx_byte;
            end

          if (core_wr_data_byte1_we)
            begin
              core_wr_data_byte1_reg <= rx_byte;
            end

          if (core_wr_data_byte2_we)
            begin
              core_wr_data_byte2_reg <= rx_byte;
            end

          if (core_wr_data_byte3_we)
            begin
              core_wr_data_byte3_reg <= rx_byte;
            end

          if (rx_buffer_rd_ptr_we)
            begin
              rx_buffer_rd_ptr_reg <= rx_buffer_rd_ptr_new;
            end

          if (rx_buffer_wr_ptr_we)
            begin
              rx_buffer_wr_ptr_reg <= rx_buffer_wr_ptr_new;
            end

          if (rx_buffer_ctr_we)
            begin
              rx_buffer_ctr_reg <= rx_buffer_ctr_new;
            end

          if (tx_buffer_ptr_we)
            begin
              tx_buffer_ptr_reg <= tx_buffer_ptr_new;
            end

          if (tx_msg_len_we)
            begin
              tx_msg_len_reg <= tx_msg_len_new;
            end

          if (core_reset_n_we)
            begin
              core_reset_n_reg <= core_reset_n_new;
            end

          if (core_cs_we)
            begin
              core_cs_reg <= core_cs_new;
            end

          if (core_we_we)
            begin
              core_we_reg <= core_we_new;
            end

          if (send_response_we)
            begin
              send_response_reg <= send_response_new;
            end

          if (response_sent_we)
            begin
              response_sent_reg <= response_sent_new;
            end

          if (sample_core_output)
            begin
              core_error_reg     <= core_error;
              core_read_data_reg <= core_read_data;
            end

          if (rx_engine_we)
            begin
              rx_engine_reg <= rx_engine_new;
            end

          if (tx_engine_we)
            begin
              tx_engine_reg <= tx_engine_new;
            end

          if (test_engine_we)
            begin
              test_engine_reg <= test_engine_new;
            end
        end
    end // reg_update


  //---------------------------------------------------------------
  // read_rx_buffer
  // Combinatinal read mux for the rx_buffer.
  //---------------------------------------------------------------
  always @*
    begin : read_rx_buffer
      rx_byte = rx_buffer[rx_buffer_rd_ptr_reg];
    end // read_rx_buffer


  //---------------------------------------------------------------
  // tx_buffer_logic
  //
  // Update logic for the tx-buffer. Given the response type and
  // the correct contents of the tx_buffer is assembled when
  // and update is signalled by the test engine.
  //---------------------------------------------------------------
  always @*
    begin: tx_buffer_logic
      // Defafult assignments
      tx_buffer_muxed0 = 8'h00;
      tx_buffer_muxed1 = 8'h00;
      tx_buffer_muxed2 = 8'h00;
      tx_buffer_muxed3 = 8'h00;
      tx_buffer_muxed4 = 8'h00;
      tx_buffer_muxed5 = 8'h00;
      tx_buffer_muxed6 = 8'h00;
      tx_buffer_muxed7 = 8'h00;
      tx_buffer_muxed8 = 8'h00;
      tx_msg_len_new   = 4'h0;
      tx_msg_len_we    = 0;
      tx_buffer_we     = 0;

      if (update_tx_buffer)
        begin
          tx_buffer_we = 1;
          tx_buffer_muxed0 = SOR;

          case (response_type)
            READ_OK:
              begin
                tx_buffer_muxed1 = READ_OK;
                tx_buffer_muxed2 = core_addr_byte0_reg;
                tx_buffer_muxed3 = core_addr_byte1_reg;
                tx_buffer_muxed4 = core_read_data_reg[31 : 24];
                tx_buffer_muxed5 = core_read_data_reg[23 : 16];
                tx_buffer_muxed6 = core_read_data_reg[15 :  8];
                tx_buffer_muxed7 = core_read_data_reg[7  :  0];
                tx_buffer_muxed8 = EOR;
                tx_msg_len_new   = 4'h8;
                tx_msg_len_we    = 1;
              end

            WRITE_OK:
              begin
                tx_buffer_muxed1 = WRITE_OK;
                tx_buffer_muxed2 = core_addr_byte0_reg;
                tx_buffer_muxed3 = core_addr_byte1_reg;
                tx_buffer_muxed4 = EOR;
                tx_msg_len_new   = 4'h4;
                tx_msg_len_we    = 1;
              end

            RESET_OK:
              begin
                tx_buffer_muxed1 = RESET_OK;
                tx_buffer_muxed2 = EOR;
                tx_msg_len_new   = 4'h2;
                tx_msg_len_we    = 1;
              end

            ERROR:
              begin
                tx_buffer_muxed1 = ERROR;
                tx_buffer_muxed2 = cmd_reg;
                tx_buffer_muxed3 = EOR;
                tx_msg_len_new   = 4'h3;
                tx_msg_len_we    = 1;
              end

            default:
              begin
                // Any response type not explicitly defined is treated as UNKNOWN.
                tx_buffer_muxed1 = UNKNOWN;
                tx_buffer_muxed2 = cmd_reg;
                tx_buffer_muxed3 = EOR;
                tx_msg_len_new   = 4'h3;
                tx_msg_len_we    = 1;
              end
          endcase // case (response_type)
        end
    end // tx_buffer_logic


  //----------------------------------------------------------------
  // rx_buffer_rd_ptr
  //
  // Logic for the rx buffer read pointer.
  //----------------------------------------------------------------
  always @*
    begin: rx_buffer_rd_ptr
      // Default assignments
      rx_buffer_rd_ptr_new = 4'h0;
      rx_buffer_rd_ptr_we  = 1'b0;
      rx_buffer_ctr_dec    = 0;

      if (rx_buffer_rd_ptr_inc)
        begin
          rx_buffer_ctr_dec    = 1;
          rx_buffer_rd_ptr_new = rx_buffer_rd_ptr_reg + 1'b1;
          rx_buffer_rd_ptr_we  = 1'b1;
        end
    end // rx_buffer_rd_ptr


  //----------------------------------------------------------------
  // rx_buffer_wr_ptr
  //
  // Logic for the rx buffer write pointer.
  //----------------------------------------------------------------
  always @*
    begin: rx_buffer_wr_ptr
      // Default assignments
      rx_buffer_wr_ptr_new = 4'h0;
      rx_buffer_wr_ptr_we  = 1'b0;
      rx_buffer_ctr_inc    = 0;

      if (rx_buffer_wr_ptr_inc)
        begin
          rx_buffer_ctr_inc    = 1;
          rx_buffer_wr_ptr_new = rx_buffer_wr_ptr_reg + 1'b1;
          rx_buffer_wr_ptr_we  = 1'b1;
        end
    end // rx_buffer_wr_ptr


  //----------------------------------------------------------------
  // rx_buffer_ctr
  //
  // Logic for the rx buffer element counter.
  //----------------------------------------------------------------
  always @*
    begin: rx_buffer_ctr
      // Default assignments
      rx_buffer_ctr_new = 4'h0;
      rx_buffer_ctr_we  = 1'b0;
      rx_buffer_empty   = 1'b0;
      rx_buffer_full    = 1'b0;

      if (rx_buffer_ctr_inc)
        begin
          rx_buffer_ctr_new = rx_buffer_ctr_reg + 1'b1;
          rx_buffer_ctr_we  = 1'b1;
        end
      else if (rx_buffer_ctr_dec)
        begin
          rx_buffer_ctr_new = rx_buffer_ctr_reg - 1'b1;
          rx_buffer_ctr_we  = 1'b1;
        end

      if (rx_buffer_ctr_reg == 4'h0)
        begin
          rx_buffer_empty = 1'b1;
        end

      if (rx_buffer_ctr_reg == BUFFER_MAX)
        begin
          rx_buffer_full = 1'b1;
        end
    end // rx_buffer_ctr


  //----------------------------------------------------------------
  // tx_buffer_ptr
  //
  // Logic for the tx buffer pointer. Supports reset and
  // incremental updates.
  //----------------------------------------------------------------
  always @*
    begin: tx_buffer_ptr
      // Default assignments
      tx_buffer_ptr_new = 4'h0;
      tx_buffer_ptr_we  = 1'b0;

      if (tx_buffer_ptr_inc)
        begin
          tx_buffer_ptr_new = tx_buffer_ptr_reg + 1'b1;
          tx_buffer_ptr_we  = 1'b1;
        end

      else if (tx_buffer_ptr_rst)
        begin
          tx_buffer_ptr_new = 4'h0;
          tx_buffer_ptr_we  = 1'b1;
        end
    end // tx_buffer_ptr


  //----------------------------------------------------------------
  // rx_engine
  //
  // FSM responsible for handling receiving message bytes from the
  // host interface and storing them in the receive buffer.
  //----------------------------------------------------------------
  always @*
    begin: rx_engine
      // Default assignments
      rx_ack_new           = 1'b0;
      rx_ack_we            = 1'b0;
      rx_buffer_we         = 1'b0;
      rx_buffer_wr_ptr_inc = 1'b0;
      rx_engine_new        = RX_IDLE;
      rx_engine_we         = 1'b0;

      case (rx_engine_reg)
        RX_IDLE:
          begin
            if (rx_syn_reg)
              begin
                if (!rx_buffer_full)
                  begin
                    rx_buffer_we  = 1'b1;
                    rx_engine_new = RX_ACK;
                    rx_engine_we  = 1'b1;
                  end
              end
          end

        RX_ACK:
          begin
            rx_ack_new           = 1'b1;
            rx_ack_we            = 1'b1;
            rx_buffer_wr_ptr_inc = 1'b1;
            rx_engine_new        = RX_NSYN;
            rx_engine_we         = 1'b1;
          end

        RX_NSYN:
          begin
            if (!rx_syn_reg)
              begin
                rx_ack_new    = 1'b0;
                rx_ack_we     = 1'b1;
                rx_engine_new = RX_IDLE;
                rx_engine_we  = 1;
              end
          end

        default:
          begin

          end
      endcase // case (rx_engine_reg)
    end // rx_engine


  //----------------------------------------------------------------
  // tx_engine
  //
  // FSM responsible for handling transmitting message bytes
  // to the host interface.
  //----------------------------------------------------------------
  always @*
    begin: tx_engine
      // Default assignments
      tx_buffer_ptr_inc = 0;
      tx_buffer_ptr_rst = 0;
      response_sent_new = 0;
      response_sent_we  = 0;
      tx_syn_new        = 0;
      tx_syn_we         = 0;

      tx_engine_new     = TX_IDLE;
      tx_engine_we      = 0;

      case (tx_engine_reg)
        TX_IDLE:
          begin
            if (send_response_reg)
              begin
                tx_syn_new    = 1;
                tx_syn_we     = 1;
                tx_engine_new = TX_SYN;
                tx_engine_we  = 1;
              end
          end

        TX_SYN:
          begin
            if (tx_ack_reg)
              begin
                tx_syn_new    = 0;
                tx_syn_we     = 1;
                tx_engine_new = TX_NOACK;
                tx_engine_we  = 1;
              end
          end

        TX_NOACK:
          begin
            if (!tx_ack_reg)
              begin
                tx_engine_new = TX_NEXT;
                tx_engine_we  = 1;
              end
          end

        TX_NEXT:
          begin
            if (tx_buffer_ptr_reg == tx_msg_len_reg)
              begin
                tx_engine_new = TX_SENT;
                tx_engine_we  = 1;
              end
            else
              begin
                tx_buffer_ptr_inc = 1;
                tx_syn_new        = 1;
                tx_syn_we         = 1;
                tx_engine_new     = TX_SYN;
                tx_engine_we      = 1;
              end
          end

        TX_SENT:
          begin
            response_sent_new = 1;
            response_sent_we  = 1;
            tx_engine_new     = TX_DONE;
            tx_engine_we      = 1;
          end

        TX_DONE:
          begin
            response_sent_new = 0;
            response_sent_we  = 1;
            tx_buffer_ptr_rst = 1;
            tx_engine_new     = TX_IDLE;
            tx_engine_we      = 1;
          end

        default:
          begin
            tx_engine_new = TX_IDLE;
            tx_engine_we  = 1;
          end
      endcase // case (tx_engine_reg)
    end // tx_engine


  //----------------------------------------------------------------
  // test_engine
  //
  // Test engine FSM logic. Parses received commands, tries to
  // execute the commands and assmbles the response to the
  // given commands.
  //----------------------------------------------------------------
  always @*
    begin: test_engine
      // Default assignments.
      core_reset_n_new      = 1;
      core_reset_n_we       = 0;
      core_cs_new           = 0;
      core_cs_we            = 0;
      core_we_new           = 0;
      core_we_we            = 0;
      sample_core_output    = 0;
      update_tx_buffer      = 0;
      response_type         = 8'h00;
      rx_buffer_rd_ptr_inc  = 0;
      cmd_we                = 0;
      core_addr_byte0_we    = 0;
      core_addr_byte1_we    = 0;
      core_wr_data_byte0_we = 0;
      core_wr_data_byte1_we = 0;
      core_wr_data_byte2_we = 0;
      core_wr_data_byte3_we = 0;
      send_response_new     = 0;
      send_response_we      = 0;
      test_engine_new       = TEST_IDLE;
      test_engine_we        = 0;


      case (test_engine_reg)
        TEST_IDLE:
          begin
            if (!rx_buffer_empty)
              begin
                rx_buffer_rd_ptr_inc = 1;
                if (rx_byte == SOC)
                  begin
                    test_engine_new = TEST_GET_CMD;
                    test_engine_we  = 1;
                  end
              end
          end

        TEST_GET_CMD:
          begin
            if (!rx_buffer_empty)
              begin
                rx_buffer_rd_ptr_inc = 1;
                cmd_we               = 1;
                test_engine_new      = TEST_PARSE_CMD;
                test_engine_we       = 1;
              end
          end

        TEST_PARSE_CMD:
          begin
            case (cmd_reg)
              RESET_CMD:
                begin
                  test_engine_new = TEST_GET_EOC;
                  test_engine_we  = 1;
                end

              READ_CMD:
                begin
                  test_engine_new = TEST_GET_ADDR0;
                  test_engine_we  = 1;
                end

              WRITE_CMD:
                begin
                  test_engine_new = TEST_GET_ADDR0;
                  test_engine_we  = 1;
                end

              default:
                begin
                  test_engine_new = TEST_CMD_UNKNOWN;
                  test_engine_we  = 1;
                end
            endcase // case (cmd_reg)
          end


        TEST_GET_ADDR0:
          begin
            if (!rx_buffer_empty)
              begin
                rx_buffer_rd_ptr_inc = 1;
                core_addr_byte0_we   = 1;
                test_engine_new      = TEST_GET_ADDR1;
                test_engine_we       = 1;
              end
          end


        TEST_GET_ADDR1:
          begin
            if (!rx_buffer_empty)
              begin
                rx_buffer_rd_ptr_inc = 1;
                core_addr_byte1_we   = 1;

                case (cmd_reg)
                  READ_CMD:
                    begin
                      test_engine_new = TEST_GET_EOC;
                      test_engine_we  = 1;
                    end

                  WRITE_CMD:
                    begin
                      test_engine_new = TEST_GET_DATA0;
                      test_engine_we  = 1;
                    end

                  default:
                    begin
                      test_engine_new = TEST_CMD_UNKNOWN;
                      test_engine_we  = 1;
                    end
                endcase // case (cmd_reg)
              end
          end


        TEST_GET_DATA0:
          begin
            if (!rx_buffer_empty)
              begin
                rx_buffer_rd_ptr_inc  = 1;
                core_wr_data_byte0_we = 1;
                test_engine_new       = TEST_GET_DATA1;
                test_engine_we        = 1;
              end
          end


        TEST_GET_DATA1:
          begin
            if (!rx_buffer_empty)
              begin
                rx_buffer_rd_ptr_inc  = 1;
                core_wr_data_byte1_we = 1;
                test_engine_new       = TEST_GET_DATA2;
                test_engine_we        = 1;
              end
          end


        TEST_GET_DATA2:
          begin
            if (!rx_buffer_empty)
              begin
                rx_buffer_rd_ptr_inc  = 1;
                core_wr_data_byte2_we = 1;
                test_engine_new       = TEST_GET_DATA3;
                test_engine_we        = 1;
              end
          end


        TEST_GET_DATA3:
          begin
            if (!rx_buffer_empty)
              begin
                rx_buffer_rd_ptr_inc  = 1;
                core_wr_data_byte3_we = 1;
                test_engine_new       = TEST_GET_EOC;
                test_engine_we        = 1;
              end
          end


        TEST_GET_EOC:
          begin
            if (!rx_buffer_empty)
              begin
                rx_buffer_rd_ptr_inc = 1;
                if (rx_byte == EOC)
                  begin
                    case (cmd_reg)
                      RESET_CMD:
                        begin
                          test_engine_new = TEST_RST_START;
                          test_engine_we  = 1;
                        end

                      READ_CMD:
                        begin
                          test_engine_new = TEST_RD_START;
                          test_engine_we  = 1;
                        end

                      WRITE_CMD:
                        begin
                          test_engine_new = TEST_WR_START;
                          test_engine_we  = 1;
                        end

                      default:
                        begin
                          test_engine_new = TEST_CMD_UNKNOWN;
                          test_engine_we  = 1;
                        end
                    endcase // case (cmd_reg)
                  end
                else
                  begin
                    test_engine_new  = TEST_CMD_ERROR;
                    test_engine_we   = 1;
                  end
              end
          end


        TEST_RST_START:
          begin
            core_reset_n_new = 0;
            core_reset_n_we  = 1;
            test_engine_new  = TEST_RST_WAIT;
            test_engine_we   = 1;
          end


        TEST_RST_WAIT:
          begin
            test_engine_new = TEST_RST_END;
            test_engine_we  = 1;
          end


        TEST_RST_END:
          begin
            core_reset_n_new = 1;
            core_reset_n_we  = 1;
            update_tx_buffer = 1;
            response_type    = RESET_OK;
            test_engine_new  = TEST_SEND_RESPONSE;
            test_engine_we   = 1;
          end


        TEST_RD_START:
          begin
            core_cs_new     = 1;
            core_cs_we      = 1;
            test_engine_new = TEST_RD_WAIT;
            test_engine_we  = 1;
          end


        TEST_RD_WAIT:
          begin
            sample_core_output = 1;
            test_engine_new    = TEST_RD_END;
            test_engine_we     = 1;
          end


        TEST_RD_END:
          begin
            core_cs_new        = 0;
            core_cs_we         = 1;
            sample_core_output = 0;

            if (core_error_reg)
              begin
                update_tx_buffer = 1;
                response_type    = ERROR;
              end
            else
              begin
                update_tx_buffer = 1;
                response_type    = READ_OK;
              end

            test_engine_new = TEST_SEND_RESPONSE;
            test_engine_we  = 1;
          end


        TEST_WR_START:
          begin
            core_cs_new = 1;
            core_cs_we  = 1;
            core_we_new = 1;
            core_we_we  = 1;

            test_engine_new = TEST_WR_WAIT;
            test_engine_we  = 1;
          end


        TEST_WR_WAIT:
          begin
            sample_core_output = 1;
            test_engine_new  = TEST_WR_END;
            test_engine_we   = 1;
          end


        TEST_WR_END:
          begin
            core_cs_new        = 0;
            core_cs_we         = 1;
            core_we_new        = 0;
            core_we_we         = 1;
            sample_core_output = 0;

            if (core_error_reg)
              begin
                update_tx_buffer = 1;
                response_type    = ERROR;
              end
            else
              begin
                update_tx_buffer = 1;
                response_type    = WRITE_OK;
              end

            test_engine_new  = TEST_SEND_RESPONSE;
            test_engine_we   = 1;
          end


        TEST_CMD_UNKNOWN:
          begin
            update_tx_buffer = 1;
            response_type    = UNKNOWN;
            test_engine_new  = TEST_SEND_RESPONSE;
            test_engine_we   = 1;
          end


        TEST_CMD_ERROR:
          begin
            update_tx_buffer = 1;
            response_type    = ERROR;
            test_engine_new  = TEST_SEND_RESPONSE;
            test_engine_we   = 1;
          end


        TEST_SEND_RESPONSE:
          begin
            send_response_new = 1;
            send_response_we  = 1;
            if (response_sent_reg)
              begin
                send_response_new = 0;
                send_response_we  = 1;
                test_engine_new   = TEST_IDLE;
                test_engine_we    = 1;
              end
          end


        default:
          begin
            // If we encounter an unknown state we move
            // back to idle.
            test_engine_new = TEST_IDLE;
            test_engine_we  = 1;
          end
      endcase // case (test_engine_reg)
    end // test_engine

endmodule // coretest

//======================================================================
// EOF coretest.v
//======================================================================
