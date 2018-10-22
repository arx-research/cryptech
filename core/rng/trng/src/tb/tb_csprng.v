//======================================================================
//
// tb_csprng.v
// -----------
// Testbench for the csprng module in the trng.
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

//------------------------------------------------------------------
// Test module.
//------------------------------------------------------------------
module tb_csprng();

  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  parameter DEBUG = 0;

  parameter CLK_HALF_PERIOD = 1;
  parameter CLK_PERIOD      = 2 * CLK_HALF_PERIOD;

  localparam ADDR_NAME0            = 8'h00;
  localparam ADDR_NAME1            = 8'h01;
  localparam ADDR_VERSION          = 8'h02;

  localparam ADDR_CTRL             = 8'h10;
  localparam CTRL_ENABLE_BIT       = 0;
  localparam CTRL_SEED_BIT         = 1;

  localparam ADDR_STATUS           = 8'h11;
  localparam STATUS_RND_VALID_BIT  = 0;

  localparam ADDR_STAT_BLOCKS_LOW  = 8'h14;
  localparam ADDR_STAT_BLOCKS_HIGH = 8'h15;
  localparam ADDR_STAT_RESEEDS     = 8'h16;

  localparam ADDR_RND_DATA         = 8'h20;

  localparam ADDR_NUM_ROUNDS       = 8'h40;
  localparam ADDR_NUM_BLOCKS_LOW   = 8'h41;
  localparam ADDR_NUM_BLOCKS_HIGH  = 8'h42;

  //----------------------------------------------------------------
  // Register and Wire declarations.
  //----------------------------------------------------------------
  reg [31 : 0] cycle_ctr;
  reg [31 : 0] error_ctr;
  reg [31 : 0] tc_ctr;

  reg           tb_clk;
  reg           tb_reset_n;

  reg           tb_cs;
  reg           tb_we;
  reg [7 : 0]   tb_address;
  reg [31 : 0]  tb_write_data;
  wire [31 : 0] tb_read_data;
  wire          tb_error;

  reg           tb_discard;
  reg           tb_test_mode;

  wire          tb_ready;
  wire          tb_more_seed;
  wire          tb_security_error;
  reg           tb_seed_syn;
  reg [511 : 0] tb_seed_data;
  wire          tb_seed_ack;
  wire [31: 0]  tb_rnd_data;
  wire          tb_rnd_syn;
  reg           tb_rnd_ack;

  wire [7 : 0]  tb_debug;
  reg           tb_debug_update;

  reg [31 : 0]  read_data;
  reg [7 : 0]   pbyte;


  //----------------------------------------------------------------
  // Device Under Test.
  //----------------------------------------------------------------
  trng_csprng dut(
                  .clk(tb_clk),
                  .reset_n(tb_reset_n),

                  .cs(tb_cs),
                  .we(tb_we),
                  .address(tb_address),
                  .write_data(tb_write_data),
                  .read_data(tb_read_data),
                  .error(tb_error),

                  .discard(tb_discard),
                  .test_mode(tb_test_mode),
                  .security_error(tb_security_error),

                  .more_seed(tb_more_seed),
                  .seed_data(tb_seed_data),
                  .seed_syn(tb_seed_syn),
                  .seed_ack(tb_seed_ack),

                  .debug(tb_debug),
                  .debug_update(tb_debug_update)
                 );


  //----------------------------------------------------------------
  // clk_gen
  //
  // Always running clock generator process.
  //----------------------------------------------------------------
  always
    begin : clk_gen
      #CLK_HALF_PERIOD;
      tb_clk = !tb_clk;
    end // clk_gen


  //----------------------------------------------------------------
  // sys_monitor()
  //
  // An always running process that creates a cycle counter and
  // conditionally displays information about the DUT.
  //----------------------------------------------------------------
  always
    begin : sys_monitor
      cycle_ctr = cycle_ctr + 1;

      #(CLK_PERIOD);

      if (DEBUG)
        begin
          dump_dut_state();
        end
    end


  //----------------------------------------------------------------
  // dump_dut_state()
  //
  // Dump the state of the dump when needed.
  //----------------------------------------------------------------
  task dump_dut_state;
    begin
      $display("cycle: 0x%016x", cycle_ctr);
      $display("State of DUT");
      $display("------------");
      $display("Inputs:");
      $display("test_mode = 0x%01x, seed = 0x%01x, enable = 0x%01x",
               dut.test_mode, dut.seed_reg, dut.enable_reg);
      $display("num_rounds = 0x%02x, num_blocks = 0x%016x",
               dut.num_rounds_reg, {dut.num_blocks_high_reg,
                                    dut.num_blocks_low_reg});
      $display("seed_syn = 0x%01x, seed_ack = 0x%01x, seed_data = 0x%064x",
               dut.seed_syn, dut.seed_ack, dut.seed_data);
      $display("");

      $display("Internal states:");
      $display("cipher_key    = 0x%032x", dut.cipher_key_reg);
      $display("cipher_iv     = 0x%08x, cipher_ctr = 0x%08x",
               dut.cipher_iv_reg, dut.cipher_ctr_reg);
      $display("cipher_block  = 0x%064x", dut.cipher_block_reg);
      $display("csprng_blocks = 0x%016x", dut.block_stat_ctr_reg);
      $display("csprng_ctrl   = 0x%02x", dut.csprng_ctrl_reg);
      $display("");

      $display("Cipher states:");
      $display("cipher init: 0x%01x, cipher next: 0x%01x",
               dut.cipher_inst.init, dut.cipher_inst.next);
      $display("cipher ctrl: 0x%01x, qr ctr: 0x%01x, dr ctr: 0x%02x",
               dut.cipher_inst.chacha_ctrl_reg, dut.cipher_inst.qr_ctr_reg,
               dut.cipher_inst.dr_ctr_reg);
      $display("cipher ready: 0x%01x, cipher data out valid: 0x%01x",
               dut.cipher_inst.ready, dut.cipher_inst.data_out_valid);
      $display("cipher data out: 0x%064x", dut.cipher_inst.data_out);
      $display("");
    end
  endtask // dump_dut_state


  //----------------------------------------------------------------
  // write_word()
  //
  // Write the given word to the DUT using the DUT interface.
  //----------------------------------------------------------------
  task write_word(input [11 : 0] address,
                  input [31 : 0] word);
    begin
      if (DEBUG)
        begin
          $display("*** Writing 0x%08x to 0x%02x.", word, address);
          $display("");
        end

      tb_address = address;
      tb_write_data = word;
      tb_cs = 1;
      tb_we = 1;

      #(2 * CLK_PERIOD);
      tb_cs = 0;
      tb_we = 0;
    end
  endtask // write_word


  //----------------------------------------------------------------
  // read_word()
  //
  // Read a data word from the given address in the DUT.
  // the word read will be available in the global variable
  // read_data.
  //----------------------------------------------------------------
  task read_word(input [11 : 0]  address);
    begin
      tb_address = address;
      tb_cs = 1;
      tb_we = 0;
      #(CLK_PERIOD);
      read_data = tb_read_data;
      tb_cs = 0;

      if (DEBUG)
        begin
          $display("*** Reading 0x%08x from 0x%02x.", read_data, address);
          $display("");
        end
    end
  endtask // read_word


  //----------------------------------------------------------------
  // reset_dut()
  //
  // Toggle reset to put the DUT into a well known state.
  //----------------------------------------------------------------
  task reset_dut;
    begin
      $display("*** Toggle reset.");
      tb_reset_n = 0;

      #(2 * CLK_PERIOD);
      tb_reset_n = 1;
      $display("");
    end
  endtask // reset_dut


  //----------------------------------------------------------------
  // display_test_results()
  //
  // Display the accumulated test results.
  //----------------------------------------------------------------
  task display_test_results;
    begin
      if (error_ctr == 0)
        begin
          $display("*** All %02d test cases completed successfully", tc_ctr);
        end
      else
        begin
          $display("*** %02d tests completed - %02d test cases did not complete successfully.",
                   tc_ctr, error_ctr);
        end
    end
  endtask // display_test_results


  //----------------------------------------------------------------
  // init_sim()
  //
  // Initialize all counters and testbed functionality as well
  // as setting the DUT inputs to defined values.
  //----------------------------------------------------------------
  task init_sim;
    begin
      cycle_ctr       = 0;
      error_ctr       = 0;
      tc_ctr          = 0;

      tb_clk          = 0;
      tb_reset_n      = 1;

      tb_cs           = 0;
      tb_we           = 0;
      tb_address      = 8'h00;
      tb_write_data   = 32'h00000000;

      tb_discard      = 0;
      tb_test_mode    = 0;

      tb_seed_syn     = 0;
      tb_seed_data    = {8{64'h0000000000000000}};
      tb_rnd_ack      = 0;
      tb_debug_update = 0;

      pbyte           = 8'h00;
    end
  endtask // init_sim


  //----------------------------------------------------------------
  // seed_generator
  //
  // When a seed_syn is observed this process will provide a new
  // seed to the DUT, assert SYN, wait for ACK and then update
  // the seed pattern state.
  //----------------------------------------------------------------
  always @ (posedge tb_more_seed)
    begin : seed_generator
      #(CLK_PERIOD);
      tb_seed_data = {64{pbyte}};
      tb_seed_syn  = 1'b1;

      while (!tb_seed_ack)
        #(CLK_PERIOD);

      tb_seed_syn  = 1'b0;
      pbyte = pbyte + 8'h01;
    end


  //----------------------------------------------------------------
  // read_rng_data()
  //
  // Support task that reads a given number of data words
  // from the DUT via the API.
  //----------------------------------------------------------------
  task read_rng_data(input [31 : 0] num_words);
    reg [31 : 0] i;
    begin
      i = 32'h00000000;

      $display("*** Trying to read 0x%08x RNG data words", num_words);
      while (i < num_words)
        begin
          tb_cs      = 1'b1;
          tb_we      = 1'b0;
          tb_address = ADDR_RND_DATA;
          i = i + 1;

          #CLK_PERIOD;

          if (DEBUG)
            $display("*** RNG data word 0x%08x: 0x%08x", i, tb_read_data);
        end

      tb_cs = 1'b0;
      $display("*** Reading of RNG data words completed.");
    end
  endtask // read_rng_data


  //----------------------------------------------------------------
  // tc1_init_csprng()
  //
  // TC1: Test that the DUT automatically starts initialize when
  // enable is set. We also starts pulling random data from the
  // csprng to see that it actually emits data as expected.
  //----------------------------------------------------------------
  task tc1_init_csprng;
    begin
      tc_ctr = tc_ctr + 1;

      $display("*** TC1: Test automatic init of csprng started.");

      tb_seed_data = {8{64'haaaaaaaa55555555}};
      tb_seed_syn  = 1'b1;

      // Start pulling data.
      read_rng_data(32'h100);

      $display("*** TC1: Test automatic init of csprng done.");
    end
  endtask // tc1_init_csprng


  //----------------------------------------------------------------
  // tc2_reseed_csprng()
  //
  // TC2: Test that the CSPRNG is reseeded as expected.
  // We set the max block size to a small value and pull data.
  //----------------------------------------------------------------
  task tc2_reseed_csprng;
    begin
      tc_ctr = tc_ctr + 1;

      $display("*** TC2: Test reseed of CSPRNG started.");

      tb_seed_data = {8{64'h0102030405060708}};
      tb_seed_syn  = 1'b1;

      // Set the max number of blocks to a low value
      write_word(ADDR_NUM_BLOCKS_HIGH, 32'h00000000);
      write_word(ADDR_NUM_BLOCKS_LOW, 32'h00000007);

      read_rng_data(32'h10000);

      $display("*** TC2 done..");
    end
  endtask // tc2_reseed_csprng


  //----------------------------------------------------------------
  // csprng_test
  //
  // The main test functionality.
  //----------------------------------------------------------------
  initial
    begin : csprng_test

      $display("   -= Testbench for csprng started =-");
      $display("    ================================");
      $display("");

      init_sim();
      dump_dut_state();
      reset_dut();
      dump_dut_state();

//      tc1_init_csprng();
      tc2_reseed_csprng();

      display_test_results();

      $display("");
      $display("*** csprng simulation done. ***");
      $finish;
    end // csprng_test
endmodule // tb_csprng

//======================================================================
// EOF tb_csprng.v
//======================================================================
