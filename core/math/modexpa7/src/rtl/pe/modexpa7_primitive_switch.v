`define USE_VENDOR_PRIMITIVES

`ifdef USE_VENDOR_PRIMITIVES

`define ADDER32_PRIMITIVE			modexpa7_adder32_artix7
`define SUBTRACTOR32_PRIMITIVE	modexpa7_subtractor32_artix7
`define SYSTOLIC_PE_PRIMITIVE	modexpa7_systolic_pe_artix7

`else

`define ADDER32_PRIMITIVE			modexpa7_adder32_generic
`define SUBTRACTOR32_PRIMITIVE	modexpa7_subtractor32_generic
`define SYSTOLIC_PE_PRIMITIVE	modexpa7_systolic_pe_generic


`endif
