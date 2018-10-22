//======================================================================
//
// tb_keywrap.v
// ------------
// Testbench for the keywrap top level wrapper (and core).
//
//
// Author: Joachim Strombergson
// Copyright (c) 2018, NORDUnet A/S
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

module tb_keywrap();

  parameter DEBUG     = 0;
  parameter DUMP_TOP  = 0;
  parameter DUMP_CORE = 0;

  parameter CLK_HALF_PERIOD = 1;
  parameter CLK_PERIOD      = 2 * CLK_HALF_PERIOD;

  parameter ADDR_BITS = 13;
  parameter MEM_BASE  = {1'h1, {(ADDR_BITS - 1){1'h0}}};


  // API for the core.
  localparam ADDR_NAME0       = 8'h00;
  localparam ADDR_NAME1       = 8'h01;
  localparam ADDR_VERSION     = 8'h02;

  localparam ADDR_CTRL        = 8'h08;
  localparam CTRL_INIT_BIT    = 0;
  localparam CTRL_NEXT_BIT    = 1;

  localparam ADDR_STATUS      = 8'h09;
  localparam STATUS_READY_BIT = 0;
  localparam STATUS_VALID_BIT = 1;

  localparam ADDR_CONFIG      = 8'h0a;
  localparam CTRL_ENCDEC_BIT  = 0;
  localparam CTRL_KEYLEN_BIT  = 1;

  localparam ADDR_RLEN        = 8'h0c;
  localparam ADDR_R_BANK      = 8'h0d;
  localparam ADDR_A0          = 8'h0e;
  localparam ADDR_A1          = 8'h0f;

  localparam ADDR_KEY0        = 8'h10;
  localparam ADDR_KEY1        = 8'h11;
  localparam ADDR_KEY2        = 8'h12;
  localparam ADDR_KEY3        = 8'h13;
  localparam ADDR_KEY4        = 8'h14;
  localparam ADDR_KEY5        = 8'h15;
  localparam ADDR_KEY6        = 8'h16;
  localparam ADDR_KEY7        = 8'h17;

  localparam ADDR_R_DATA0     = 8'h80;
  localparam ADDR_R_DATA127   = 8'hff;


  //----------------------------------------------------------------
  // Register and Wire declarations.
  //----------------------------------------------------------------
  reg [31 : 0]  cycle_ctr;
  reg [31 : 0]  error_ctr;
  reg [31 : 0]  tc_ctr;

  reg [31 : 0]  read_data;
  reg [127 : 0] result_data;

  reg                       tb_clk;
  reg                       tb_reset_n;
  reg                       tb_cs;
  reg                       tb_we;
  reg [(ADDR_BITS -1 ) : 0] tb_address;
  reg [31 : 0]              tb_write_data;
  wire [31 : 0]             tb_read_data;
  wire                      tb_error;


  //----------------------------------------------------------------
  // Device Under Test.
  //----------------------------------------------------------------
  keywrap dut(
              .clk(tb_clk),
              .reset_n(tb_reset_n),
              .cs(tb_cs),
              .we(tb_we),
              .address(tb_address),
              .write_data(tb_write_data),
              .read_data(tb_read_data),
              .error(tb_error)
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
      if (DEBUG)
        dump_dut_state();
      #(CLK_PERIOD);
    end



  //----------------------------------------------------------------
  // read_word()
  //
  // Read a data word from the given address in the DUT.
  // the word read will be available in the global variable
  // read_data.
  //----------------------------------------------------------------
  task read_word(input [(ADDR_BITS - 1) : 0]  address);
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
  // write_word()
  //
  // Write the given word to the DUT using the DUT interface.
  //----------------------------------------------------------------
  task write_word(input [(ADDR_BITS - 1) : 0] address,
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

      #(1 * CLK_PERIOD);
      tb_cs = 0;
      tb_we = 0;
    end
  endtask // write_word


  //----------------------------------------------------------------
  // wait_ready
  //
  // Wait for the DUT to signal that the result is ready
  //----------------------------------------------------------------
  task wait_ready;
    begin : wait_ready
      reg rdy;
      rdy = 1'b0;

      while (rdy != 1'b1)
        begin
          read_word(ADDR_STATUS);
          rdy = tb_read_data[STATUS_READY_BIT];
        end
    end
  endtask // wait_ready


  //----------------------------------------------------------------
  // dump_mem()
  //
  // Dump the n first memory positions in the dut internal memory.
  //----------------------------------------------------------------
  task dump_mem(input integer n);
    begin : dump_mem
      integer i;
      for (i  = 0 ; i < n ; i = i + 1)
        $display("mem0[0x%06x] = 0x%08x  mem1[0x%06x] = 0x%08x",
                 i, dut.core.mem.mem0[i], i, dut.core.mem.mem1[i]);
      $display("");
    end
  endtask // dump_mem


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

      if (DUMP_TOP)
        begin
          $display("top level state:");
          $display("init_reg  = 0x%x    next_reg   = 0x%x", dut.init_reg, dut.next_reg);
          $display("endec_reg = 0x%x    keylen_reg = 0x%x", dut.encdec_reg, dut.keylen_reg);
          $display("rlen_reg  = 0x%08x", dut.rlen_reg);
          $display("a0_reg    = 0x%08x  a1_reg     = 0x%08x", dut.a0_reg, dut.a1_reg);
          $display("");
        end

      if (DUMP_CORE)
        begin
          $display("core level state:");
          $display("init   = 0x%0x  next = 0x%0x  ready = 0x%0x  valid = 0x%0x",
                   dut.core.init, dut.core.next, dut.core.ready, dut.core.valid);
          $display("api_we = 0x%0x  api_addr = 0x%0x  api_wr_data = 0x%0x  api_rd_data = 0x%0x",
                   dut.core.api_we, dut.core.api_addr, dut.core.api_wr_data, dut.core.api_rd_data);
          $display("rlen   = 0x%0x", dut.core.rlen);
          $display("key    = 0x%0x", dut.core.key);
          $display("a_init = 0x%0x  a_result = 0x%0x", dut.core.a_init, dut.core.a_result);
          $display("");


          $display("update_state = 0x%0x", dut.core.update_state);
          $display("a_reg  = 0x%0x  a_new = 0x%0x  a_we = 0x%0x",
                   dut.core.a_reg, dut.core.a_new, dut.core.a_we);
          $display("core_we = 0x%0x  core_addr = 0x%0x",
                   dut.core.core_we, dut.core.core_addr);
          $display("core_rd_data = 0x%0x  core_wr_data = 0x%0x ",
                   dut.core.core_rd_data, dut.core.core_wr_data);
          $display("xor_val = 0x%0x", dut.core.keywrap_logic.xor_val);
          $display("");


          $display("aes_ready = 0x%0x  aes_valid = 0x%0x",
                   dut.core.aes_ready, dut.core.aes_valid);
          $display("aes_init = 0x%0x  aes_next = 0x%0x",
                   dut.core.aes_init, dut.core.aes_next);
          $display("aes_block = 0x%0x  aes_result = 0x%0x",
                   dut.core.aes_block, dut.core.aes_result);
          $display("");


          $display("block_ctr_reg = 0x%0x  iteration_ctr_reg = 0x%0x",
                   dut.core.block_ctr_reg, dut.core.iteration_ctr_reg);
          $display("keywrap_core_ctrl_reg = 0x%0x", dut.core.keywrap_core_ctrl_reg);
          $display("keywrap_core_ctrl_new = 0x%0x", dut.core.keywrap_core_ctrl_new);
          $display("keywrap_core_ctrl_we  = 0x%0x", dut.core.keywrap_core_ctrl_we);
        end

      $display("");
      $display("");
    end
  endtask // dump_dut_state


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
      cycle_ctr     = 0;
      error_ctr     = 0;
      tc_ctr        = 0;

      tb_clk        = 0;
      tb_reset_n    = 1;

      tb_cs         = 0;
      tb_we         = 0;
      tb_address    = 8'h0;
      tb_write_data = 32'h0;
    end
  endtask // init_sim


  //----------------------------------------------------------------
  // reset_dut()
  //
  // Toggle reset to put the DUT into a well known state.
  //----------------------------------------------------------------
  task reset_dut;
    begin
      $display("** Toggling reset.");
      tb_reset_n = 0;

      #(2 * CLK_PERIOD);
      tb_reset_n = 1;
      $display("");
    end
  endtask // reset_dut


  //----------------------------------------------------------------
  // test_core_access
  // Simple test that we can perform read access to regs
  // in the core.
  //----------------------------------------------------------------
  task test_core_access;
    begin : test_core_access
      $display("** TC test_core_access START.");

      read_word(ADDR_NAME0);
      $display("NAME0: %s", read_data);
      read_word(ADDR_NAME1);
      $display("NAME1: %s", read_data);
      read_word(ADDR_VERSION);
      $display("version: %s", read_data);

      $display("** TC test_core_access END.");
      $display("");
    end
  endtask // test_core_access


  //----------------------------------------------------------------
  // test_kwp_ae_128_1
  // Implements wrap test based on NIST KWP_AE 128 bit key
  // with 248 bit plaintext.
  //----------------------------------------------------------------
  task test_kwp_ae_128_1;
    begin : kwp_ae_128_1
      integer i;
      integer err;

      err = 0;

      tc_ctr = tc_ctr + 1;

      $display("** TC kwp_ae_128_1 START.");

      // Write key and keylength, we also want to encrypt/wrap.
      write_word(ADDR_KEY0,   32'hc03db3cc);
      write_word(ADDR_KEY1,   32'h1416dcd1);
      write_word(ADDR_KEY2,   32'hc069a195);
      write_word(ADDR_KEY3,   32'ha8d77e3d);
      write_word(ADDR_CONFIG, 32'h00000001);

      // Initialize the AES engine (to expand the key).
      // Wait for init to complete.
      // Note, not actually needed to wait. We can write R data during init.
      $display("* Trying to initialize.");
      write_word(ADDR_CTRL, 32'h00000001);
      #(2 * CLK_PERIOD);
      wait_ready();
      $display("* Init should be done.");


      // Set the length or R in blocks.
      // Write the R bank to be written to.
      // Write the R blocks to be processed.
      write_word(ADDR_RLEN,  32'h00000004);


      // Write the data to be wrapped.
      write_word(MEM_BASE + 0, 32'h46f87f58);
      write_word(MEM_BASE + 1, 32'hcdda4200);
      write_word(MEM_BASE + 2, 32'hf53d99ce);
      write_word(MEM_BASE + 3, 32'h2e49bdb7);
      write_word(MEM_BASE + 4, 32'h6212511f);
      write_word(MEM_BASE + 5, 32'he0cd4d0b);
      write_word(MEM_BASE + 6, 32'h5f37a27d);
      write_word(MEM_BASE + 7, 32'h45a28800);

      // Write magic words to A.
      write_word(ADDR_A0, 32'ha65959a6);
      write_word(ADDR_A1, 32'h0000001f);


      $display("* Dumping state and mem after data write and A words.");
      dump_dut_state();
      dump_mem(6);



      $display("* Contents of memory and dut before wrap processing:");
      dump_mem(6);


      // Start wrapping and wait for wrap to complete.
      $display("* Trying to start processing.");
      write_word(ADDR_CTRL, 32'h00000002);
      #(2 * CLK_PERIOD);
      wait_ready();
      $display("* Processing should be done.");


      $display("Contents of memory and dut after wrap processing:");
      dump_mem(6);
      dump_dut_state();

      // Read and check the A registers.
      read_word(ADDR_A0);
      if (read_data != 32'h57e3b669)
        begin
          $display("Error A0 after wrap: 0x%08x, expected 0x57e3b669", read_data);
          err = 1;
        end

      read_word(ADDR_A1);
      if (read_data != 32'h9c6e8177)
        begin
          $display("Error A1 after wrap: 0x%08x, expected 0x9c6e8177", read_data);
          err = 1;
        end

      if (err)
        begin
          $display("kwp_ae_128_1 completed with errors.");
          error_ctr = error_ctr + 1;
        end
      else
        $display("kwp_ae_128_1 completed successfully.");

      $display("** TC kwp_ae_128_1 END.\n");
    end
  endtask // test_kwp_ae_128_1



  //----------------------------------------------------------------
  // test_kwp_ad_128_1
  // Implements unwrap test based on NIST KWP_AE 128 bit key
  // with 248 bit plaintext.
  //----------------------------------------------------------------
  task test_kwp_ad_128_1;
    begin : kwp_ad_128_1
      integer i;
      integer err;

      err = 0;

      tc_ctr = tc_ctr + 1;

      $display("** TC kwp_ad_128_1 START.");

      // Write key and keylength, we also want to decrypt/unwrap.
      write_word(ADDR_KEY0,   32'hc03db3cc);
      write_word(ADDR_KEY1,   32'h1416dcd1);
      write_word(ADDR_KEY2,   32'hc069a195);
      write_word(ADDR_KEY3,   32'ha8d77e3d);
      write_word(ADDR_CONFIG, 32'h00000000);


      // Initialize the AES engine (to expand the key).
      // Wait for init to complete.
      // Note, not actually needed to wait. We can write R data during init.
      $display("* Trying to initialize.");
      write_word(ADDR_CTRL, 32'h00000001);
      #(2 * CLK_PERIOD);
      wait_ready();
      $display("* Init should be done.");


      // Set the length or R in blocks.
      // Write the R bank to be written to.
      // Write the R blocks to be processed.
      write_word(ADDR_RLEN,  32'h00000004);

      write_word(MEM_BASE + 0, 32'h59a69492);
      write_word(MEM_BASE + 1, 32'hbb7e2cd0);
      write_word(MEM_BASE + 2, 32'h0160d2eb);
      write_word(MEM_BASE + 3, 32'hef9bf4d4);
      write_word(MEM_BASE + 4, 32'heb16fbf7);
      write_word(MEM_BASE + 5, 32'h98f1340f);
      write_word(MEM_BASE + 6, 32'h6df6558a);
      write_word(MEM_BASE + 7, 32'h4fb84cd0);

      // Write magic words to A.
      write_word(ADDR_A0, 32'h57e3b669);
      write_word(ADDR_A1, 32'h9c6e8177);


      $display("* Contents of memory and dut before unwrap processing:");
      dump_mem(6);


      // Start unwrapping and wait for unwrap to complete.
      $display("* Trying to start processing.");
      write_word(ADDR_CTRL, 32'h00000002);
      #(2 * CLK_PERIOD);
      wait_ready();
      $display("* Processing should be done.");


      $display("Contents of memory and dut after unwrap processing:");
      dump_mem(6);
      dump_dut_state();


      // Read and display the A registers.
      read_word(ADDR_A0);
      $display("A0 after unwrap: 0x%08x", read_data);
      read_word(ADDR_A1);
      $display("A1 after unwrap: 0x%08x", read_data);

      // Read and display the R blocks that has been processed.
      for (i = 0 ; i < 8 ; i = i + 1)
        begin
          read_word(MEM_BASE + i);
          $display("mem[0x%08x] = 0x%08x", i, read_data);
        end

      // Read and check the A registers.
      read_word(ADDR_A0);
      if (read_data != 32'ha65959a6)
        begin
          $display("Error A0 after wrap: 0x%08x, expected 0xa65959a6", read_data);
          err = 1;
        end

      read_word(ADDR_A1);
      if (read_data != 32'h0000001f)
        begin
          $display("Error A1 after wrap: 0x%08x, expected 0x0000001f", read_data);
          err = 1;
        end

      if (err)
        begin
          $display("kwp_ad_128_1 completed with errors.");
          error_ctr = error_ctr + 1;
        end
      else
        $display("kwp_ad_128_1 completed successfully.");

      $display("** TC kwp_ad_128_1 END.\n");
    end
  endtask // test_kwp_ad_128_1


  //----------------------------------------------------------------
  // test_kwp_ae_128_2
  // Implements wrap test based on NIST KWP_AE 128 bit key with
  // 4096 bit plaintext.
  //----------------------------------------------------------------
  task test_kwp_ae_128_2;
    begin : kwp_ae_128_2
      integer i;
      integer err;

      err = 0;
      tc_ctr = tc_ctr + 1;

      $display("** TC kwp_ae_128_2 START.");

      // Write key and keylength, we also want to encrypt/wrap.
      write_word(ADDR_KEY0,   32'h6b8ba9cc);
      write_word(ADDR_KEY1,   32'h9b31068b);
      write_word(ADDR_KEY2,   32'ha175abfc);
      write_word(ADDR_KEY3,   32'hc60c1338);
      write_word(ADDR_CONFIG, 32'h00000001);


      // Initialize the AES engine (to expand the key).
      // Wait for init to complete.
      $display("* Trying to initialize.");
      write_word(ADDR_CTRL, 32'h00000001);
      #(2 * CLK_PERIOD);
      wait_ready();
      $display("* Init should be done.");


      // Set the length or R in blocks.
      // Write the R bank to be written to.
      // Write the R blocks to be processed.
      write_word(ADDR_RLEN,  32'h00000040);

      write_word(MEM_BASE + 0, 32'h8af887c5);
      write_word(MEM_BASE + 1, 32'h8dfbc38e);
      write_word(MEM_BASE + 2, 32'he0423eef);
      write_word(MEM_BASE + 3, 32'hcc0e032d);
      write_word(MEM_BASE + 4, 32'hcc79dd11);
      write_word(MEM_BASE + 5, 32'h6638ca65);
      write_word(MEM_BASE + 6, 32'had75dca2);
      write_word(MEM_BASE + 7, 32'ha2459f13);
      write_word(MEM_BASE + 8, 32'h934dbe61);
      write_word(MEM_BASE + 9, 32'ha62cb26d);
      write_word(MEM_BASE + 10, 32'h8bbddbab);
      write_word(MEM_BASE + 11, 32'hf9bf52bb);
      write_word(MEM_BASE + 12, 32'he137ef1d);
      write_word(MEM_BASE + 13, 32'h3e30eacf);
      write_word(MEM_BASE + 14, 32'h0fe456ec);
      write_word(MEM_BASE + 15, 32'h808d6798);
      write_word(MEM_BASE + 16, 32'hdc29fe54);
      write_word(MEM_BASE + 17, 32'hfa1f784a);
      write_word(MEM_BASE + 18, 32'ha3c11cf3);
      write_word(MEM_BASE + 19, 32'h94050095);
      write_word(MEM_BASE + 20, 32'h81d3f1d5);
      write_word(MEM_BASE + 21, 32'h96843813);
      write_word(MEM_BASE + 22, 32'ha6685e50);
      write_word(MEM_BASE + 23, 32'h3fac8535);
      write_word(MEM_BASE + 24, 32'he0c06ecc);
      write_word(MEM_BASE + 25, 32'ha8561b6a);
      write_word(MEM_BASE + 26, 32'h1f22c578);
      write_word(MEM_BASE + 27, 32'heefb6919);
      write_word(MEM_BASE + 28, 32'h12be2e16);
      write_word(MEM_BASE + 29, 32'h67946101);
      write_word(MEM_BASE + 30, 32'hae8c3501);
      write_word(MEM_BASE + 31, 32'he6c66eb1);
      write_word(MEM_BASE + 32, 32'h7e14f260);
      write_word(MEM_BASE + 33, 32'h8c9ce6fb);
      write_word(MEM_BASE + 34, 32'hab4a1597);
      write_word(MEM_BASE + 35, 32'hed49ccb3);
      write_word(MEM_BASE + 36, 32'h930b1060);
      write_word(MEM_BASE + 37, 32'hf98c97d8);
      write_word(MEM_BASE + 38, 32'hdc4ce81e);
      write_word(MEM_BASE + 39, 32'h35279c4d);
      write_word(MEM_BASE + 40, 32'h30d1bf86);
      write_word(MEM_BASE + 41, 32'hc9b919a3);
      write_word(MEM_BASE + 42, 32'hce4f0109);
      write_word(MEM_BASE + 43, 32'he77929e5);
      write_word(MEM_BASE + 44, 32'h8c4c3aeb);
      write_word(MEM_BASE + 45, 32'h5de1ec5e);
      write_word(MEM_BASE + 46, 32'h0afa38ae);
      write_word(MEM_BASE + 47, 32'h896df912);
      write_word(MEM_BASE + 48, 32'h1c72c255);
      write_word(MEM_BASE + 49, 32'h141f2f5c);
      write_word(MEM_BASE + 50, 32'h9a51be50);
      write_word(MEM_BASE + 51, 32'h72547cf8);
      write_word(MEM_BASE + 52, 32'ha3b06740);
      write_word(MEM_BASE + 53, 32'h4e62f961);
      write_word(MEM_BASE + 54, 32'h5a02479c);
      write_word(MEM_BASE + 55, 32'hf8c202e7);
      write_word(MEM_BASE + 56, 32'hfeb2e258);
      write_word(MEM_BASE + 57, 32'h314e0ebe);
      write_word(MEM_BASE + 58, 32'h62878a5c);
      write_word(MEM_BASE + 59, 32'h4ecd4e9d);
      write_word(MEM_BASE + 60, 32'hf7dab2e1);
      write_word(MEM_BASE + 61, 32'hfa9a7b53);
      write_word(MEM_BASE + 62, 32'h2c2169ac);
      write_word(MEM_BASE + 63, 32'hedb7998d);
      write_word(MEM_BASE + 64, 32'h5cd8a711);
      write_word(MEM_BASE + 65, 32'h8848ce7e);
      write_word(MEM_BASE + 66, 32'he9fb2f68);
      write_word(MEM_BASE + 67, 32'he28c2b27);
      write_word(MEM_BASE + 68, 32'h9ddc064d);
      write_word(MEM_BASE + 69, 32'hb70ad73c);
      write_word(MEM_BASE + 70, 32'h6dbe10c5);
      write_word(MEM_BASE + 71, 32'he1c56a70);
      write_word(MEM_BASE + 72, 32'h9c1407f9);
      write_word(MEM_BASE + 73, 32'h3a727cce);
      write_word(MEM_BASE + 74, 32'h1075103a);
      write_word(MEM_BASE + 75, 32'h4009ae2f);
      write_word(MEM_BASE + 76, 32'h7731b7d7);
      write_word(MEM_BASE + 77, 32'h1756eee1);
      write_word(MEM_BASE + 78, 32'h19b828ef);
      write_word(MEM_BASE + 79, 32'h4ed61eff);
      write_word(MEM_BASE + 80, 32'h16493553);
      write_word(MEM_BASE + 81, 32'h2a94fa8f);
      write_word(MEM_BASE + 82, 32'he62dc2e2);
      write_word(MEM_BASE + 83, 32'h2cf20f16);
      write_word(MEM_BASE + 84, 32'h8ae65f4b);
      write_word(MEM_BASE + 85, 32'h6785286c);
      write_word(MEM_BASE + 86, 32'h253f365f);
      write_word(MEM_BASE + 87, 32'h29453a47);
      write_word(MEM_BASE + 88, 32'h9dc2824b);
      write_word(MEM_BASE + 89, 32'h8bdabd96);
      write_word(MEM_BASE + 90, 32'h2da3b76a);
      write_word(MEM_BASE + 91, 32'he9c8a720);
      write_word(MEM_BASE + 92, 32'h155e158f);
      write_word(MEM_BASE + 93, 32'he389c8cc);
      write_word(MEM_BASE + 94, 32'h7fa6ad52);
      write_word(MEM_BASE + 95, 32'h2c951b5c);
      write_word(MEM_BASE + 96, 32'h236bf964);
      write_word(MEM_BASE + 97, 32'hb5b1bfb0);
      write_word(MEM_BASE + 98, 32'h98a39835);
      write_word(MEM_BASE + 99, 32'h759b9540);
      write_word(MEM_BASE + 100, 32'h4b72b17f);
      write_word(MEM_BASE + 101, 32'h7dbcda93);
      write_word(MEM_BASE + 102, 32'h6177ae05);
      write_word(MEM_BASE + 103, 32'h9269f41e);
      write_word(MEM_BASE + 104, 32'hcdac81a4);
      write_word(MEM_BASE + 105, 32'h9f5bbfd2);
      write_word(MEM_BASE + 106, 32'he801392a);
      write_word(MEM_BASE + 107, 32'h043ef068);
      write_word(MEM_BASE + 108, 32'h73550a67);
      write_word(MEM_BASE + 109, 32'hfcbc039f);
      write_word(MEM_BASE + 110, 32'h0b5d30ce);
      write_word(MEM_BASE + 111, 32'h490baa97);
      write_word(MEM_BASE + 112, 32'h9dbbaf9e);
      write_word(MEM_BASE + 113, 32'h53d45d7e);
      write_word(MEM_BASE + 114, 32'h2dff26b2);
      write_word(MEM_BASE + 115, 32'hf7e6628d);
      write_word(MEM_BASE + 116, 32'hed694217);
      write_word(MEM_BASE + 117, 32'ha39f454b);
      write_word(MEM_BASE + 118, 32'h288e7906);
      write_word(MEM_BASE + 119, 32'hb79faf4a);
      write_word(MEM_BASE + 120, 32'h407a7d20);
      write_word(MEM_BASE + 121, 32'h7646f930);
      write_word(MEM_BASE + 122, 32'h96a157f0);
      write_word(MEM_BASE + 123, 32'hd1dca05a);
      write_word(MEM_BASE + 124, 32'h7f92e318);
      write_word(MEM_BASE + 125, 32'hfc1ff62c);
      write_word(MEM_BASE + 126, 32'he2de7f12);
      write_word(MEM_BASE + 127, 32'h9b187053);

      // Write magic words to A.
      write_word(ADDR_A0, 32'ha65959a6);
      write_word(ADDR_A1, 32'h00000200);

      $display("* Contents of memory and dut before wrap processing:");
      dump_mem(65);

      // Start wrapping and wait for wrap to complete.
      $display("* Trying to start processing.");
      write_word(ADDR_CTRL, 32'h00000002);
      #(2 * CLK_PERIOD);
      wait_ready();
      $display("* Processing should be done.");


      $display("Contents of memory and dut after wrap processing:");
      dump_mem(65);
      dump_dut_state();


      // Read and display the A registers.
      read_word(ADDR_A0);
      $display("A0 after wrap: 0x%08x", read_data);
      read_word(ADDR_A1);
      $display("A1 after wrap: 0x%08x", read_data);

      // Read and display the R blocks that has been processed.
      for (i = 0 ; i < 128 ; i = i + 1)
        begin
          read_word(MEM_BASE + i);
          $display("mem[0x%08x] = 0x%08x", i, read_data);
        end

      // Read and check the A registers.
      read_word(ADDR_A0);
      if (read_data != 32'haea19443)
        begin
          $display("Error A0 after wrap: 0x%08x, expected 0xaea19443", read_data);
          err = 1;
        end

      read_word(ADDR_A1);
      if (read_data != 32'hd7f8ad7d)
        begin
          $display("Error A1 after wrap: 0x%08x, expected 0xd7f8ad7d", read_data);
          err = 1;
        end

      if (err)
        begin
          $display("kwp_ae_128_2 completed with errors.");
          error_ctr = error_ctr + 1;
        end
      else
        $display("kwp_ae_128_2 completed successfully.");

      $display("** TC kwp_ae_128_2 END.\n");
    end
  endtask // test_kwp_ae_128_2


  //----------------------------------------------------------------
  // test_kwp_ad_128_2
  // Implements unwrap test based on NIST KWP_AD 128 bit key with
  // 4096 bit plaintext.
  //----------------------------------------------------------------
  task test_kwp_ad_128_2;
    begin : kwp_ad_128_2
      integer i;
      integer err;

      err = 0;
      tc_ctr = tc_ctr + 1;

      $display("** TC kwp_ad_128_2 START.");

      // Write key and keylength, we also want to unwrap/decrypt.
      write_word(ADDR_KEY0,   32'h6b8ba9cc);
      write_word(ADDR_KEY1,   32'h9b31068b);
      write_word(ADDR_KEY2,   32'ha175abfc);
      write_word(ADDR_KEY3,   32'hc60c1338);
      write_word(ADDR_CONFIG, 32'h00000000);


      // Initialize the AES engine (to expand the key).
      // Wait for init to complete.
      $display("* Trying to initialize.");
      write_word(ADDR_CTRL, 32'h00000001);
      #(2 * CLK_PERIOD);
      wait_ready();
      $display("* Init should be done.");


      // Set the length or R in blocks.
      // Write the R bank to be written to.
      // Write the R blocks to be processed.
      write_word(ADDR_RLEN,  32'h00000040);

      write_word(MEM_BASE + 0, 32'h4501c1ec);
      write_word(MEM_BASE + 1, 32'hadc6b5e3);
      write_word(MEM_BASE + 2, 32'hf1c23c29);
      write_word(MEM_BASE + 3, 32'heca60890);
      write_word(MEM_BASE + 4, 32'h5f9cabdd);
      write_word(MEM_BASE + 5, 32'h46e34a55);
      write_word(MEM_BASE + 6, 32'he1f7ac83);
      write_word(MEM_BASE + 7, 32'h08e75c90);
      write_word(MEM_BASE + 8, 32'h3675982b);
      write_word(MEM_BASE + 9, 32'hda99173a);
      write_word(MEM_BASE + 10, 32'h2ba57d2c);
      write_word(MEM_BASE + 11, 32'hcf2e01a0);
      write_word(MEM_BASE + 12, 32'h2589f89d);
      write_word(MEM_BASE + 13, 32'hfd4b3c7f);
      write_word(MEM_BASE + 14, 32'hd229ec91);
      write_word(MEM_BASE + 15, 32'hc9d0c46e);
      write_word(MEM_BASE + 16, 32'ha5dee3c0);
      write_word(MEM_BASE + 17, 32'h48cd4611);
      write_word(MEM_BASE + 18, 32'hbfeadc9b);
      write_word(MEM_BASE + 19, 32'hf26daa1e);
      write_word(MEM_BASE + 20, 32'h02cb72e2);
      write_word(MEM_BASE + 21, 32'h22cf3dab);
      write_word(MEM_BASE + 22, 32'h120dd1e8);
      write_word(MEM_BASE + 23, 32'hc2dd9bd5);
      write_word(MEM_BASE + 24, 32'h8bbefa5d);
      write_word(MEM_BASE + 25, 32'h14526abd);
      write_word(MEM_BASE + 26, 32'h1e8d2170);
      write_word(MEM_BASE + 27, 32'ha6ba8283);
      write_word(MEM_BASE + 28, 32'hc243ec2f);
      write_word(MEM_BASE + 29, 32'hd5ef0703);
      write_word(MEM_BASE + 30, 32'h0b1ef5f6);
      write_word(MEM_BASE + 31, 32'h9f9620e4);
      write_word(MEM_BASE + 32, 32'hb17a3639);
      write_word(MEM_BASE + 33, 32'h34100588);
      write_word(MEM_BASE + 34, 32'h7b9ffc79);
      write_word(MEM_BASE + 35, 32'h35335947);
      write_word(MEM_BASE + 36, 32'h03e5dcae);
      write_word(MEM_BASE + 37, 32'h67bd0ce7);
      write_word(MEM_BASE + 38, 32'ha3c98ca6);
      write_word(MEM_BASE + 39, 32'h5815a4d0);
      write_word(MEM_BASE + 40, 32'h67f27e6e);
      write_word(MEM_BASE + 41, 32'h66d6636c);
      write_word(MEM_BASE + 42, 32'hebb78973);
      write_word(MEM_BASE + 43, 32'h2566a52a);
      write_word(MEM_BASE + 44, 32'hc3970e14);
      write_word(MEM_BASE + 45, 32'hc37310dc);
      write_word(MEM_BASE + 46, 32'h2fcee0e7);
      write_word(MEM_BASE + 47, 32'h39a16291);
      write_word(MEM_BASE + 48, 32'h029fd2b4);
      write_word(MEM_BASE + 49, 32'hd534e304);
      write_word(MEM_BASE + 50, 32'h45474b26);
      write_word(MEM_BASE + 51, 32'h711a8b3e);
      write_word(MEM_BASE + 52, 32'h1ee3cc88);
      write_word(MEM_BASE + 53, 32'hb09e8b17);
      write_word(MEM_BASE + 54, 32'h45b6cc0f);
      write_word(MEM_BASE + 55, 32'h067624ec);
      write_word(MEM_BASE + 56, 32'hb232db75);
      write_word(MEM_BASE + 57, 32'h0b01fe54);
      write_word(MEM_BASE + 58, 32'h57fdea77);
      write_word(MEM_BASE + 59, 32'hb251b10f);
      write_word(MEM_BASE + 60, 32'he95d3eee);
      write_word(MEM_BASE + 61, 32'hdb083bdf);
      write_word(MEM_BASE + 62, 32'h109c41db);
      write_word(MEM_BASE + 63, 32'ha26cc965);
      write_word(MEM_BASE + 64, 32'h4f787bf9);
      write_word(MEM_BASE + 65, 32'h5735ff07);
      write_word(MEM_BASE + 66, 32'h070b175c);
      write_word(MEM_BASE + 67, 32'hea8b6230);
      write_word(MEM_BASE + 68, 32'h2e6087b9);
      write_word(MEM_BASE + 69, 32'h1a041547);
      write_word(MEM_BASE + 70, 32'h46056910);
      write_word(MEM_BASE + 71, 32'h99f1a9e2);
      write_word(MEM_BASE + 72, 32'hb626c4b3);
      write_word(MEM_BASE + 73, 32'hbb7aeb8e);
      write_word(MEM_BASE + 74, 32'had9922bc);
      write_word(MEM_BASE + 75, 32'h3617cb42);
      write_word(MEM_BASE + 76, 32'h7c669b88);
      write_word(MEM_BASE + 77, 32'hbe5f98ae);
      write_word(MEM_BASE + 78, 32'ha7edb8b0);
      write_word(MEM_BASE + 79, 32'h063bec80);
      write_word(MEM_BASE + 80, 32'haf4c081f);
      write_word(MEM_BASE + 81, 32'h89778d7c);
      write_word(MEM_BASE + 82, 32'h7242ddae);
      write_word(MEM_BASE + 83, 32'h88e8d3af);
      write_word(MEM_BASE + 84, 32'hf1f80e57);
      write_word(MEM_BASE + 85, 32'h5e1aab4a);
      write_word(MEM_BASE + 86, 32'h5d115bc2);
      write_word(MEM_BASE + 87, 32'h7636fd14);
      write_word(MEM_BASE + 88, 32'hd19bc594);
      write_word(MEM_BASE + 89, 32'h33f69763);
      write_word(MEM_BASE + 90, 32'h5ecd870d);
      write_word(MEM_BASE + 91, 32'h17e7f5b0);
      write_word(MEM_BASE + 92, 32'h04dee400);
      write_word(MEM_BASE + 93, 32'h1cddc34a);
      write_word(MEM_BASE + 94, 32'hb6e377ee);
      write_word(MEM_BASE + 95, 32'hb3fb08e9);
      write_word(MEM_BASE + 96, 32'h47697076);
      write_word(MEM_BASE + 97, 32'h5105d93e);
      write_word(MEM_BASE + 98, 32'h4558fe3d);
      write_word(MEM_BASE + 99, 32'h4fc6fe05);
      write_word(MEM_BASE + 100, 32'h3aab9c6c);
      write_word(MEM_BASE + 101, 32'hf032f111);
      write_word(MEM_BASE + 102, 32'h6e70c2d6);
      write_word(MEM_BASE + 103, 32'h5f7c8cde);
      write_word(MEM_BASE + 104, 32'hb6ad63ac);
      write_word(MEM_BASE + 105, 32'h4291f93d);
      write_word(MEM_BASE + 106, 32'h467ebbb2);
      write_word(MEM_BASE + 107, 32'h9ead265c);
      write_word(MEM_BASE + 108, 32'h05ac684d);
      write_word(MEM_BASE + 109, 32'h20a6bef0);
      write_word(MEM_BASE + 110, 32'h9b71830f);
      write_word(MEM_BASE + 111, 32'h717e08bc);
      write_word(MEM_BASE + 112, 32'hb4f9d377);
      write_word(MEM_BASE + 113, 32'h3bec928f);
      write_word(MEM_BASE + 114, 32'h66eeb64d);
      write_word(MEM_BASE + 115, 32'hc451e958);
      write_word(MEM_BASE + 116, 32'he357ebbf);
      write_word(MEM_BASE + 117, 32'hef5a342d);
      write_word(MEM_BASE + 118, 32'hf28707ac);
      write_word(MEM_BASE + 119, 32'h4b8e3e8c);
      write_word(MEM_BASE + 120, 32'h854e8d69);
      write_word(MEM_BASE + 121, 32'h1cb92e87);
      write_word(MEM_BASE + 122, 32'hc0d57558);
      write_word(MEM_BASE + 123, 32'he44cd754);
      write_word(MEM_BASE + 124, 32'h424865c2);
      write_word(MEM_BASE + 125, 32'h29c9e1ab);
      write_word(MEM_BASE + 126, 32'hb28e003b);
      write_word(MEM_BASE + 127, 32'h6819400b);

      // Write magic words to A.
      write_word(ADDR_A0, 32'haea19443);
      write_word(ADDR_A1, 32'hd7f8ad7d);


      $display("* Contents of memory and dut before unwrap processing:");
      dump_mem(65);

      // Start unwrapping and wait for unwrap to complete.
      $display("* Trying to start processing.");
      write_word(ADDR_CTRL, 32'h00000002);
      #(2 * CLK_PERIOD);
      wait_ready();
      $display("* Processing should be done.");


      $display("Contents of memory and dut after unwrap processing:");
      dump_mem(65);
      dump_dut_state();


      // Read and display the A registers.
      read_word(ADDR_A0);
      $display("A0 after unwrap: 0x%08x", read_data);
      read_word(ADDR_A1);
      $display("A1 after unwrap: 0x%08x", read_data);

      // Read and display the R blocks that has been processed.
      for (i = 0 ; i < 128 ; i = i + 1)
        begin
          read_word(MEM_BASE + i);
          $display("mem[0x%08x] = 0x%08x", i, read_data);
        end

      // Read and check the A registers.
      read_word(ADDR_A0);
      if (read_data != 32'ha65959a6)
        begin
          $display("Error A0 after wrap: 0x%08x, expected 0xa65959a6", read_data);
          err = 1;
        end

      read_word(ADDR_A1);
      if (read_data != 32'h00000200)
        begin
          $display("Error A1 after wrap: 0x%08x, expected 0x00000200", read_data);
          err = 1;
        end

      if (err)
        begin
          $display("kwp_ad_128_2 completed with errors.");
          error_ctr = error_ctr + 1;
        end
      else
        $display("kwp_ad_128_2 completed successfully.");

      $display("** TC kwp_ad_128_2 END.\n");
    end
  endtask // test_kwp_ad_128_2



  //----------------------------------------------------------------
  // test_big_wrap_256
  // Implements wrap test with a huge (16 kB+) data object
  // and 256 bit key.
  //----------------------------------------------------------------
  task test_big_wrap_256;
    begin : test_big_wrap_256
      integer i;
      integer err;

      err = 0;

      tc_ctr = tc_ctr + 1;

      $display("** TC test_big_wrap_256 START.");

      // Write key and keylength, we also want to encrypt/wrap.
      write_word(ADDR_KEY0,   32'h55aa55aa);
      write_word(ADDR_KEY1,   32'h55aa55aa);
      write_word(ADDR_KEY2,   32'h55aa55aa);
      write_word(ADDR_KEY3,   32'h55aa55aa);
      write_word(ADDR_KEY4,   32'h55aa55aa);
      write_word(ADDR_KEY5,   32'h55aa55aa);
      write_word(ADDR_KEY6,   32'h55aa55aa);
      write_word(ADDR_KEY7,   32'h55aa55aa);
      write_word(ADDR_CONFIG, 32'h00000003);

      // Initialize the AES engine (to expand the key).
      // Wait for init to complete.
      // Note, not actually needed to wait. We can write R data during init.
      $display("* Trying to initialize.");
      write_word(ADDR_CTRL, 32'h00000001);
      #(2 * CLK_PERIOD);
      wait_ready();
      $display("* Init should be done.");


      // Set the length or R in blocks.
      // Write the R bank to be written to.
      // Write the R blocks to be processed.
      write_word(ADDR_RLEN,  32'h000007f8);


      // Write the data to be wrapped.
      for (i = 0 ; i < 4080 ; i = i + 1)
        write_word(MEM_BASE + i, 32'h01010101);

      // Write magic words to A.
      write_word(ADDR_A0, 32'ha65959a6);
      write_word(ADDR_A1, 32'h00003fc0);

      // Start wrapping and wait for wrap to complete.
      $display("* Trying to start processing.");
      write_word(ADDR_CTRL, 32'h00000002);
      #(2 * CLK_PERIOD);
      wait_ready();
      $display("* Processing should be done.");


      // Read and check the A registers.
      read_word(ADDR_A0);
      if (read_data != 32'h53179eb9)
        begin
          $display("Error A0 after wrap: 0x%08x, expected 0x53179eb9", read_data);
          err = 1;
        end

      read_word(ADDR_A1);
      if (read_data != 32'ha90ce632)
        begin
          $display("Error A1 after wrap: 0x%08x, expected 0xa90ce632", read_data);
          err = 1;
        end

      if (err)
        begin
          $display("test_big_wrap_256 completed with errors.");
          error_ctr = error_ctr + 1;
        end
      else
        $display("test_big_wrap_256 completed successfully.");

      $display("** TC test_big_wrap_256 END.\n");
    end
  endtask // test_big_wrap_256


  //----------------------------------------------------------------
  // main
  //----------------------------------------------------------------
  initial
    begin : main
      $display("   -= Testbench for Keywrap started =-");
      $display("    ==================================");
      $display("");

      $display("Address bits: %d", ADDR_BITS);
      $display("MEM_BASE:     0x%08x", MEM_BASE);
      $display("");

      init_sim();

      dump_dut_state();
      reset_dut();
      dump_dut_state();

      test_core_access();

      test_kwp_ae_128_1();
      test_kwp_ad_128_1();
      test_kwp_ae_128_2();
      test_kwp_ad_128_2();

      test_big_wrap_256();

      display_test_results();

      $display("");
      $display("   -= Testbench for Keywrap completed =-");
      $display("    ====================================");
      $finish;
    end // main

endmodule // tb_keywrap

//======================================================================
// EOF tb_keywrap.v
//======================================================================
