//======================================================================
//
// vndecorrelator.v
// ----------------
// von Neumann decorrelator for bits. The module consumes bits
// and for every two bits consume will either emit zero or one bits.
//
//
// Author: Joachim Strombergson
// Copyright (c) 2014, NORDUnet A/S All rights reserved.
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

module vndecorrelator(
                      input wire  clk,
                      input wire  reset_n,

                      input wire  data_in,
                      input wire  syn_in,

                      output wire data_out,
                      output wire syn_out
                     );


  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  parameter CTRL_IDLE = 1'b0;
  parameter CTRL_BITS = 1'b1;


  //----------------------------------------------------------------
  // Registers including update variables and write enable.
  //----------------------------------------------------------------
  reg          data_in_reg;
  reg          data_in_we;

  reg          data_out_reg;
  reg          data_out_we;

  reg          syn_out_reg;
  reg          syn_out_new;

  reg          vndecorr_ctrl_reg;
  reg          vndecorr_ctrl_new;
  reg          vndecorr_ctrl_we;


  //----------------------------------------------------------------
  // Concurrent connectivity for ports etc.
  //----------------------------------------------------------------
  assign data_out = data_out_reg;
  assign syn_out  = syn_out_reg;


  //----------------------------------------------------------------
  // reg_update
  //----------------------------------------------------------------
  always @ (posedge clk or negedge reset_n)
    begin
      if (!reset_n)
        begin
          data_in_reg       <= 1'b0;
          data_out_reg      <= 1'b0;
          syn_out_reg       <= 1'b0;
          vndecorr_ctrl_reg <= CTRL_IDLE;
        end
      else
        begin
          syn_out_reg <= syn_out_new;

          if (data_in_we)
            begin
              data_in_reg <= data_in;
            end

          if (data_out_we)
            begin
              data_out_reg <= data_in;
            end

          if (vndecorr_ctrl_we)
            begin
              vndecorr_ctrl_reg <= vndecorr_ctrl_new;
            end
        end
    end // reg_update


  //----------------------------------------------------------------
  // vndecorr_logic
  //
  // The logic implementing the von Neumann decorrelator by waiting
  // for subsequent bits and comparing them to determine if both
  // bits should just be discarded or one of them also emitted.
  //----------------------------------------------------------------
  always @*
    begin : vndecorr_logic
      data_in_we        = 1'b0;
      data_out_we       = 1'b0;
      syn_out_new       = 1'b0;
      vndecorr_ctrl_new = CTRL_IDLE;
      vndecorr_ctrl_we  = 1'b0;

      case (vndecorr_ctrl_reg)
        CTRL_IDLE:
          begin
            if (syn_in)
              begin
                data_in_we        = 1'b1;
                vndecorr_ctrl_new = CTRL_BITS;
                vndecorr_ctrl_we  = 1'b1;
              end
          end

        CTRL_BITS:
          begin
            if (syn_in)
              begin
                if (data_in != data_in_reg)
                  begin
                    data_out_we = 1'b1;
                    syn_out_new = 1'b1;
                  end

                vndecorr_ctrl_new = CTRL_IDLE;
                vndecorr_ctrl_we  = 1'b1;
              end
          end
      endcase // case (vndecorr_ctrl_reg)
    end //  vndecorr_logic

endmodule // vndecorrelator

//======================================================================
// EOF vndecorrelator.v
//======================================================================
