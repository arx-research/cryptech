//======================================================================
//
// rosc_entropy.v
// ---------------
// Fake rosc entropy source. Basically a wrapper around the
// pseudo entropy core. Used during simulation of the TRNG.
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

module rosc_entropy(
                    input wire           clk,
                    input wire           reset_n,

                    input wire           cs,
                    input wire           we,
                    input wire  [7 : 0]  address,
                    input wire  [31 : 0] write_data,
                    output wire [31 : 0] read_data,
                    output wire          error,

                    input wire           discard,
                    input wire           test_mode,
                    output wire          security_error,

                    output wire          entropy_enabled,
                    output wire [31 : 0] entropy_data,
                    output wire          entropy_valid,
                    input wire           entropy_ack,

                    output wire [7 : 0]  debug,
                    input wire           debug_update
                   );


  //----------------------------------------------------------------
  // Internal constant and parameter definitions.
  //----------------------------------------------------------------
  assign read_data      = 32'h00000000;
  assign error          = 0;
  assign security_error = 0;
  assign debug          = 8'h00;


  //----------------------------------------------------------------
  // Core instantiation.
  //----------------------------------------------------------------
  pseudo_entropy core(
                      .clk(clk),
                      .reset_n(reset_n),

                      .enable(1'b1),

                      .raw_entropy(),
                      .stats(),

                      .enabled(entropy_enabled),
                      .entropy_syn(entropy_valid),
                      .entropy_data(entropy_data),
                      .entropy_ack(entropy_ack)
                     );

endmodule // rosc_entropy

//======================================================================
// EOF rosc_entropy.v
//======================================================================
