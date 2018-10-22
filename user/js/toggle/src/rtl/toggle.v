//======================================================================
//
// toggle
// ------
// gpio pin toggler.
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

module toggle

  (
   input wire          sys_clk,
   input wire          reset_n,
   output wire [7 : 0] gpio_b
  );

  localparam TOGGLE_DELAY_CYCLES = 100;

  reg [7 : 0] toggle_ctr_reg;
  reg [7 : 0] toggle_ctr_new;

  reg toggle_reg;
  reg toggle_new;
  reg toggle_we;

  assign gpio_b = {7'b1010101, toggle_reg};

  always @(posedge sys_clk)
    begin: sys_clk_toggle_reg_update
      if (!reset_n)
        begin
          toggle_ctr_reg <= 32'h0;
          toggle_reg     <= 1'b0;
        end
      else
        begin
          toggle_ctr_reg <= toggle_ctr_new;

          if (toggle_we)
            toggle_reg <= toggle_new;
        end
    end

  always @*
    begin : sys_clk_toggle
      toggle_ctr_new = toggle_ctr_reg + 1'b1;
      toggle_new     = ~toggle_reg;
      toggle_we      = 1'b0;

      if (toggle_ctr_reg == TOGGLE_DELAY_CYCLES)
        begin
          toggle_ctr_new = 32'h0;
          toggle_we      = 1'b1;
        end
    end

endmodule // toggle

//======================================================================
// EOF toggle.v
//======================================================================
