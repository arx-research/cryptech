//======================================================================
//
// montprod_wrapper
// ----------------
// Simple wrapper to allow us to build versions of montprod with
// different operand widths for performance testing.
//
//
// Author: Joachim Strombergson
// Copyright (c) 2015, NORDUnet A/S All rights reserved.
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

module montprod_wrapper(
                        input wire                  clk,
                        input wire                  reset_n,

                        input wire                  calculate,
                        output wire                 ready,
                        input wire [(ADW - 1) : 0]  length,

                        input wire [1 : 0]          operand_select,
                        input wire [(OPW - 1) : 0]  operand_data,
                        output wire [(ADW - 1) : 0] operand_addr,

                        output wire [(ADW - 1) : 0] result_addr,
                        input wire                  result_words,
                        output wire [31 : 0]        result_data,
                        output wire                 result_we
                        );

  //----------------------------------------------------------------
  // Defines
  //----------------------------------------------------------------
  localparam OPW = 64;
  localparam ADW = 7;


  //----------------------------------------------------------------
  // Wires.
  //----------------------------------------------------------------
  wire [(ADW - 1) : 0] opa_addr;
  reg [(OPW - 1) : 0]  opa_data;
  wire [(ADW - 1) : 0] opb_addr;
  reg [(OPW - 1) : 0]  opb_data;
  wire [(ADW - 1) : 0] opm_addr;
  reg [(OPW - 1) : 0]  opm_data;

  reg [(ADW - 1) : 0]  tmp_operand_addr;
  wire [(OPW - 1) : 0] internal_result_data;


  //----------------------------------------------------------------
  // Assignments.
  //----------------------------------------------------------------
  assign result_data = internal_result_data[result_words * 32 +: 32];
  assign operand_addr = tmp_operand_addr;


  //----------------------------------------------------------------
  // core instantiations.
  //----------------------------------------------------------------
  montprod #(.OPW(OPW), .ADW(ADW))
  montprod_inst(
                .clk(clk),
                .reset_n(reset_n),

                .calculate(calculate),
                .ready(ready),

                .length(length),

                .opa_addr(opa_addr),
                .opa_data(opa_data),

                .opb_addr(opb_addr),
                .opb_data(opb_data),

                .opm_addr(opm_addr),
                .opm_data(opm_data),

                .result_addr(result_addr),
                .result_data(internal_result_data),
                .result_we(result_we)
               );


  //----------------------------------------------------------------
  // Operand select mux.
  //----------------------------------------------------------------
  always @*
    begin : operand_mux
      tmp_operand_addr = {ADW{1'b0}};
      opa_data         = {OPW{1'b1}};
      opb_data         = {OPW{1'b1}};
      opm_data         = {OPW{1'b1}};

      case (operand_select)
        0:
          begin
            tmp_operand_addr = opa_addr;
            opa_data         = operand_data;
          end

        1:
          begin
            tmp_operand_addr = opb_addr;
            opb_data         = operand_data;
          end

        2:
          begin
            tmp_operand_addr = opm_addr;
            opm_data         = operand_data;
          end

        default:
          begin
          end
      endcase // case (operand_select)
    end

endmodule // montprod_wrapper

//======================================================================
// EOF montprod_wrapper
//======================================================================
