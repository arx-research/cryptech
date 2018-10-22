////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1995-2013 Xilinx, Inc.  All rights reserved.
////////////////////////////////////////////////////////////////////////////////
//   ____  ____
//  /   /\/   /
// /___/  \  /    Vendor: Xilinx
// \   \   \/     Version: P.20131013
//  \   \         Application: netgen
//  /   /         Filename: multiplier_s6.v
// /___/   /\     Timestamp: Fri Jul 10 17:52:08 2015
// \   \  /  \ 
//  \___\/\___\
//             
// Command	: -w -sim -ofmt verilog E:/FPGA/ModExpS6_Novena/src/modexps6/ipcore/tmp/_cg/multiplier_s6.ngc E:/FPGA/ModExpS6_Novena/src/modexps6/ipcore/tmp/_cg/multiplier_s6.v 
// Device	: 6slx45csg324-3
// Input file	: E:/FPGA/ModExpS6_Novena/src/modexps6/ipcore/tmp/_cg/multiplier_s6.ngc
// Output file	: E:/FPGA/ModExpS6_Novena/src/modexps6/ipcore/tmp/_cg/multiplier_s6.v
// # of Modules	: 1
// Design Name	: multiplier_s6
// Xilinx        : e:\Xilinx\14.7\ISE_DS\ISE\
//             
// Purpose:    
//     This verilog netlist is a verification model and uses simulation 
//     primitives which may not represent the true implementation of the 
//     device, however the netlist is functionally correct and should not 
//     be modified. This file cannot be synthesized and should only be used 
//     with supported simulation tools.
//             
// Reference:  
//     Command Line Tools User Guide, Chapter 23 and Synthesis and Simulation Design Guide, Chapter 6
//             
////////////////////////////////////////////////////////////////////////////////

/*verilator lint_off UNDRIVEN*/
/*verilator lint_off UNUSED*/

`timescale 1 ns/1 ps

module multiplier_s6 (
  clk, a, b, p
);
  input clk;
  input [31 : 0] a;
  input [31 : 0] b;
  output [63 : 0] p;
  
  wire \blk00000001/sig000001c7 ;
  wire \blk00000001/sig000001c6 ;
  wire \blk00000001/sig000001c5 ;
  wire \blk00000001/sig000001c4 ;
  wire \blk00000001/sig000001c3 ;
  wire \blk00000001/sig000001c2 ;
  wire \blk00000001/sig000001c1 ;
  wire \blk00000001/sig000001c0 ;
  wire \blk00000001/sig000001bf ;
  wire \blk00000001/sig000001be ;
  wire \blk00000001/sig000001bd ;
  wire \blk00000001/sig000001bc ;
  wire \blk00000001/sig000001bb ;
  wire \blk00000001/sig000001ba ;
  wire \blk00000001/sig000001b9 ;
  wire \blk00000001/sig000001b8 ;
  wire \blk00000001/sig000001b7 ;
  wire \blk00000001/sig000001b6 ;
  wire \blk00000001/sig000001b5 ;
  wire \blk00000001/sig000001b4 ;
  wire \blk00000001/sig000001b3 ;
  wire \blk00000001/sig000001b2 ;
  wire \blk00000001/sig000001b1 ;
  wire \blk00000001/sig000001b0 ;
  wire \blk00000001/sig000001af ;
  wire \blk00000001/sig000001ae ;
  wire \blk00000001/sig000001ad ;
  wire \blk00000001/sig000001ac ;
  wire \blk00000001/sig000001ab ;
  wire \blk00000001/sig000001aa ;
  wire \blk00000001/sig000001a9 ;
  wire \blk00000001/sig000001a8 ;
  wire \blk00000001/sig000001a7 ;
  wire \blk00000001/sig000001a6 ;
  wire \blk00000001/sig000001a5 ;
  wire \blk00000001/sig000001a4 ;
  wire \blk00000001/sig000001a3 ;
  wire \blk00000001/sig000001a2 ;
  wire \blk00000001/sig000001a1 ;
  wire \blk00000001/sig000001a0 ;
  wire \blk00000001/sig0000019f ;
  wire \blk00000001/sig0000019e ;
  wire \blk00000001/sig0000019d ;
  wire \blk00000001/sig0000019c ;
  wire \blk00000001/sig0000019b ;
  wire \blk00000001/sig0000019a ;
  wire \blk00000001/sig00000199 ;
  wire \blk00000001/sig00000198 ;
  wire \blk00000001/sig00000197 ;
  wire \blk00000001/sig00000196 ;
  wire \blk00000001/sig00000195 ;
  wire \blk00000001/sig00000194 ;
  wire \blk00000001/sig00000193 ;
  wire \blk00000001/sig00000192 ;
  wire \blk00000001/sig00000191 ;
  wire \blk00000001/sig00000190 ;
  wire \blk00000001/sig0000018f ;
  wire \blk00000001/sig0000018e ;
  wire \blk00000001/sig0000018d ;
  wire \blk00000001/sig0000018c ;
  wire \blk00000001/sig0000018b ;
  wire \blk00000001/sig0000018a ;
  wire \blk00000001/sig00000189 ;
  wire \blk00000001/sig00000188 ;
  wire \blk00000001/sig00000187 ;
  wire \blk00000001/sig00000186 ;
  wire \blk00000001/sig00000185 ;
  wire \blk00000001/sig00000184 ;
  wire \blk00000001/sig00000183 ;
  wire \blk00000001/sig00000182 ;
  wire \blk00000001/sig00000181 ;
  wire \blk00000001/sig00000180 ;
  wire \blk00000001/sig0000017f ;
  wire \blk00000001/sig0000017e ;
  wire \blk00000001/sig0000017d ;
  wire \blk00000001/sig0000017c ;
  wire \blk00000001/sig0000017b ;
  wire \blk00000001/sig0000017a ;
  wire \blk00000001/sig00000179 ;
  wire \blk00000001/sig00000178 ;
  wire \blk00000001/sig00000177 ;
  wire \blk00000001/sig00000176 ;
  wire \blk00000001/sig00000175 ;
  wire \blk00000001/sig00000174 ;
  wire \blk00000001/sig00000173 ;
  wire \blk00000001/sig00000172 ;
  wire \blk00000001/sig00000171 ;
  wire \blk00000001/sig00000170 ;
  wire \blk00000001/sig0000016f ;
  wire \blk00000001/sig0000016e ;
  wire \blk00000001/sig0000016d ;
  wire \blk00000001/sig0000016c ;
  wire \blk00000001/sig0000016b ;
  wire \blk00000001/sig0000016a ;
  wire \blk00000001/sig00000169 ;
  wire \blk00000001/sig00000168 ;
  wire \blk00000001/sig00000167 ;
  wire \blk00000001/sig00000166 ;
  wire \blk00000001/sig00000165 ;
  wire \blk00000001/sig00000164 ;
  wire \blk00000001/sig00000163 ;
  wire \blk00000001/sig00000162 ;
  wire \blk00000001/sig00000161 ;
  wire \blk00000001/sig00000160 ;
  wire \blk00000001/sig0000015f ;
  wire \blk00000001/sig0000015e ;
  wire \blk00000001/sig0000015d ;
  wire \blk00000001/sig0000015c ;
  wire \blk00000001/sig0000015b ;
  wire \blk00000001/sig0000015a ;
  wire \blk00000001/sig00000159 ;
  wire \blk00000001/sig00000158 ;
  wire \blk00000001/sig00000157 ;
  wire \blk00000001/sig00000156 ;
  wire \blk00000001/sig00000155 ;
  wire \blk00000001/sig00000154 ;
  wire \blk00000001/sig00000153 ;
  wire \blk00000001/sig00000152 ;
  wire \blk00000001/sig00000151 ;
  wire \blk00000001/sig00000150 ;
  wire \blk00000001/sig0000014f ;
  wire \blk00000001/sig0000014e ;
  wire \blk00000001/sig0000014d ;
  wire \blk00000001/sig0000014c ;
  wire \blk00000001/sig0000014b ;
  wire \blk00000001/sig0000014a ;
  wire \blk00000001/sig00000149 ;
  wire \blk00000001/sig00000148 ;
  wire \blk00000001/sig00000147 ;
  wire \blk00000001/sig00000146 ;
  wire \blk00000001/sig00000145 ;
  wire \blk00000001/sig00000144 ;
  wire \blk00000001/sig00000143 ;
  wire \blk00000001/sig00000142 ;
  wire \blk00000001/sig00000141 ;
  wire \blk00000001/sig00000140 ;
  wire \blk00000001/sig0000013f ;
  wire \blk00000001/sig0000013e ;
  wire \blk00000001/sig0000013d ;
  wire \blk00000001/sig0000013c ;
  wire \blk00000001/sig0000013b ;
  wire \blk00000001/sig0000013a ;
  wire \blk00000001/sig00000139 ;
  wire \blk00000001/sig00000138 ;
  wire \blk00000001/sig00000137 ;
  wire \blk00000001/sig00000136 ;
  wire \blk00000001/sig00000135 ;
  wire \blk00000001/sig00000134 ;
  wire \blk00000001/sig00000133 ;
  wire \blk00000001/sig00000132 ;
  wire \blk00000001/sig00000131 ;
  wire \blk00000001/sig00000130 ;
  wire \blk00000001/sig0000012f ;
  wire \blk00000001/sig0000012e ;
  wire \blk00000001/sig0000012d ;
  wire \blk00000001/sig0000012c ;
  wire \blk00000001/sig0000012b ;
  wire \blk00000001/sig0000012a ;
  wire \blk00000001/sig00000129 ;
  wire \blk00000001/sig00000128 ;
  wire \blk00000001/sig00000127 ;
  wire \blk00000001/sig00000126 ;
  wire \blk00000001/sig00000125 ;
  wire \blk00000001/sig00000124 ;
  wire \blk00000001/sig00000123 ;
  wire \blk00000001/sig00000122 ;
  wire \blk00000001/sig00000121 ;
  wire \blk00000001/sig00000120 ;
  wire \blk00000001/sig0000011f ;
  wire \blk00000001/sig0000011e ;
  wire \blk00000001/sig0000011d ;
  wire \blk00000001/sig0000011c ;
  wire \blk00000001/sig0000011b ;
  wire \blk00000001/sig0000011a ;
  wire \blk00000001/sig00000119 ;
  wire \blk00000001/sig00000118 ;
  wire \blk00000001/sig00000117 ;
  wire \blk00000001/sig00000116 ;
  wire \blk00000001/sig00000115 ;
  wire \blk00000001/sig00000114 ;
  wire \blk00000001/sig00000113 ;
  wire \blk00000001/sig00000112 ;
  wire \blk00000001/sig00000111 ;
  wire \blk00000001/sig00000110 ;
  wire \blk00000001/sig0000010f ;
  wire \blk00000001/sig0000010e ;
  wire \blk00000001/sig0000010d ;
  wire \blk00000001/sig0000010c ;
  wire \blk00000001/sig0000010b ;
  wire \blk00000001/sig0000010a ;
  wire \blk00000001/sig00000109 ;
  wire \blk00000001/sig00000108 ;
  wire \blk00000001/sig00000107 ;
  wire \blk00000001/sig00000106 ;
  wire \blk00000001/sig00000105 ;
  wire \blk00000001/sig00000104 ;
  wire \blk00000001/sig00000103 ;
  wire \blk00000001/sig00000102 ;
  wire \blk00000001/sig00000101 ;
  wire \blk00000001/sig00000100 ;
  wire \blk00000001/sig000000ff ;
  wire \blk00000001/sig000000fe ;
  wire \blk00000001/sig000000fd ;
  wire \blk00000001/sig000000fc ;
  wire \blk00000001/sig000000fb ;
  wire \blk00000001/sig000000fa ;
  wire \blk00000001/sig000000f9 ;
  wire \blk00000001/sig000000f8 ;
  wire \blk00000001/sig000000f7 ;
  wire \blk00000001/sig000000f6 ;
  wire \blk00000001/sig000000f5 ;
  wire \blk00000001/sig000000f4 ;
  wire \blk00000001/sig000000f3 ;
  wire \blk00000001/sig000000f2 ;
  wire \blk00000001/sig000000f1 ;
  wire \blk00000001/sig000000f0 ;
  wire \blk00000001/sig000000ef ;
  wire \blk00000001/sig000000ee ;
  wire \blk00000001/sig000000ed ;
  wire \blk00000001/sig000000ec ;
  wire \blk00000001/sig000000eb ;
  wire \blk00000001/sig000000ea ;
  wire \blk00000001/sig000000e9 ;
  wire \blk00000001/sig000000e8 ;
  wire \blk00000001/sig000000e7 ;
  wire \blk00000001/sig000000e6 ;
  wire \blk00000001/sig000000e5 ;
  wire \blk00000001/sig000000e4 ;
  wire \blk00000001/sig000000e3 ;
  wire \blk00000001/sig000000e2 ;
  wire \blk00000001/sig000000e1 ;
  wire \blk00000001/sig000000e0 ;
  wire \blk00000001/sig000000df ;
  wire \blk00000001/sig000000de ;
  wire \blk00000001/sig000000dd ;
  wire \blk00000001/sig000000dc ;
  wire \blk00000001/sig000000db ;
  wire \blk00000001/sig000000da ;
  wire \blk00000001/sig000000d9 ;
  wire \blk00000001/sig000000d8 ;
  wire \blk00000001/sig000000d7 ;
  wire \blk00000001/sig000000d6 ;
  wire \blk00000001/sig000000d5 ;
  wire \blk00000001/sig000000d4 ;
  wire \blk00000001/sig000000d3 ;
  wire \blk00000001/sig000000d2 ;
  wire \blk00000001/sig000000d1 ;
  wire \blk00000001/sig000000d0 ;
  wire \blk00000001/sig000000cf ;
  wire \blk00000001/sig000000ce ;
  wire \blk00000001/sig000000cd ;
  wire \blk00000001/sig000000cc ;
  wire \blk00000001/sig000000cb ;
  wire \blk00000001/sig000000ca ;
  wire \blk00000001/sig000000c9 ;
  wire \blk00000001/sig000000c8 ;
  wire \blk00000001/sig000000c7 ;
  wire \blk00000001/sig000000c6 ;
  wire \blk00000001/sig000000c5 ;
  wire \blk00000001/sig000000c4 ;
  wire \blk00000001/sig000000c3 ;
  wire \blk00000001/sig000000c2 ;
  wire \blk00000001/sig000000c1 ;
  wire \blk00000001/sig000000c0 ;
  wire \blk00000001/sig000000bf ;
  wire \blk00000001/sig000000be ;
  wire \blk00000001/sig000000bd ;
  wire \blk00000001/sig000000bc ;
  wire \blk00000001/sig000000bb ;
  wire \blk00000001/sig000000ba ;
  wire \blk00000001/sig000000b9 ;
  wire \blk00000001/sig000000b8 ;
  wire \blk00000001/sig000000b7 ;
  wire \blk00000001/sig000000b6 ;
  wire \blk00000001/sig000000b5 ;
  wire \blk00000001/sig000000b4 ;
  wire \blk00000001/sig000000b3 ;
  wire \blk00000001/sig000000b2 ;
  wire \blk00000001/sig000000b1 ;
  wire \blk00000001/sig000000b0 ;
  wire \blk00000001/sig000000af ;
  wire \blk00000001/sig000000ae ;
  wire \blk00000001/sig000000ad ;
  wire \blk00000001/sig000000ac ;
  wire \blk00000001/sig000000ab ;
  wire \blk00000001/sig000000aa ;
  wire \blk00000001/sig000000a9 ;
  wire \blk00000001/sig000000a8 ;
  wire \blk00000001/sig000000a7 ;
  wire \blk00000001/sig000000a6 ;
  wire \blk00000001/sig000000a5 ;
  wire \blk00000001/sig000000a4 ;
  wire \blk00000001/sig000000a3 ;
  wire \blk00000001/sig000000a2 ;
  wire \blk00000001/sig000000a1 ;
  wire \blk00000001/sig000000a0 ;
  wire \blk00000001/sig0000009f ;
  wire \blk00000001/sig0000009e ;
  wire \blk00000001/sig0000009d ;
  wire \blk00000001/sig0000009c ;
  wire \blk00000001/sig0000009b ;
  wire \blk00000001/sig0000009a ;
  wire \blk00000001/sig00000099 ;
  wire \blk00000001/sig00000098 ;
  wire \blk00000001/sig00000097 ;
  wire \blk00000001/sig00000096 ;
  wire \blk00000001/sig00000095 ;
  wire \blk00000001/sig00000094 ;
  wire \blk00000001/sig00000093 ;
  wire \blk00000001/sig00000092 ;
  wire \blk00000001/sig00000091 ;
  wire \blk00000001/sig00000090 ;
  wire \blk00000001/sig0000008f ;
  wire \blk00000001/sig0000008e ;
  wire \blk00000001/sig0000008d ;
  wire \blk00000001/sig0000008c ;
  wire \blk00000001/sig0000008b ;
  wire \blk00000001/sig0000008a ;
  wire \blk00000001/sig00000089 ;
  wire \blk00000001/sig00000088 ;
  wire \blk00000001/sig00000087 ;
  wire \blk00000001/sig00000086 ;
  wire \blk00000001/sig00000085 ;
  wire \blk00000001/sig00000084 ;
  wire \blk00000001/sig00000083 ;
  wire \blk00000001/sig00000082 ;
  wire \NLW_blk00000001/blk00000007_CARRYOUTF_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_CARRYOUT_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<47>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<46>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<45>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<44>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<43>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<42>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<41>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<40>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<39>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<38>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<37>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<36>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<35>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<34>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<33>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<32>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<31>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<30>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<29>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<28>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<27>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<26>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<25>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<24>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<23>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<22>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<21>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<20>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<19>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<18>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<17>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<16>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<15>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<14>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<13>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<12>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<11>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<10>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<9>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<8>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<7>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<6>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<5>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<4>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<3>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<2>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<1>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_C<0>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<35>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<34>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<33>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<32>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<31>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<30>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<29>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<28>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<27>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<26>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<25>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<24>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<23>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<22>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<21>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<20>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<19>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<18>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<17>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<16>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<15>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<14>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<13>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<12>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<11>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<10>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<9>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<8>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<7>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<6>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<5>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<4>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<3>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<2>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<1>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000007_M<0>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_CARRYOUTF_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_CARRYOUT_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<47>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<46>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<45>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<44>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<43>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<42>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<41>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<40>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<39>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<38>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<37>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<36>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<35>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<34>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<33>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<32>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<31>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<30>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<29>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<28>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<27>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<26>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<25>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<24>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<23>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<22>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<21>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<20>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<19>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<18>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<17>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<16>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<15>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<14>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<13>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<12>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<11>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<10>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<9>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<8>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<7>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<6>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<5>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<4>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<3>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<2>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<1>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_C<0>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<35>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<34>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<33>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<32>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<31>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<30>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<29>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<28>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<27>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<26>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<25>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<24>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<23>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<22>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<21>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<20>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<19>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<18>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<17>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<16>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<15>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<14>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<13>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<12>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<11>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<10>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<9>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<8>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<7>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<6>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<5>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<4>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<3>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<2>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<1>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000006_M<0>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_CARRYOUTF_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_CARRYOUT_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_BCOUT<17>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_BCOUT<16>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_BCOUT<15>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_BCOUT<14>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_BCOUT<13>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_BCOUT<12>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_BCOUT<11>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_BCOUT<10>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_BCOUT<9>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_BCOUT<8>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_BCOUT<7>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_BCOUT<6>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_BCOUT<5>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_BCOUT<4>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_BCOUT<3>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_BCOUT<2>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_BCOUT<1>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_BCOUT<0>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<47>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<46>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<45>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<44>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<43>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<42>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<41>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<40>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<39>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<38>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<37>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<36>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<35>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<34>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<33>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<32>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<31>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<30>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<29>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<28>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<27>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<26>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<25>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<24>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<23>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<22>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<21>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<20>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<19>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<18>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<17>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<16>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<15>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<14>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<13>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<12>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<11>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<10>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<9>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<8>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<7>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<6>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<5>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<4>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<3>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<2>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<1>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_P<0>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<35>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<34>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<33>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<32>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<31>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<30>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<29>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<28>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<27>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<26>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<25>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<24>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<23>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<22>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<21>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<20>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<19>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<18>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<17>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<16>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<15>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<14>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<13>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<12>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<11>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<10>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<9>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<8>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<7>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<6>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<5>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<4>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<3>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<2>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<1>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000005_M<0>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_CARRYOUTF_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_CARRYOUT_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_BCOUT<17>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_BCOUT<16>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_BCOUT<15>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_BCOUT<14>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_BCOUT<13>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_BCOUT<12>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_BCOUT<11>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_BCOUT<10>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_BCOUT<9>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_BCOUT<8>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_BCOUT<7>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_BCOUT<6>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_BCOUT<5>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_BCOUT<4>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_BCOUT<3>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_BCOUT<2>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_BCOUT<1>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_BCOUT<0>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_P<47>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_P<46>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_P<45>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_P<44>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_P<43>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_P<42>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_P<41>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_P<40>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_P<39>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_P<38>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_P<37>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_P<36>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_P<35>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_P<34>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_P<33>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_P<32>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_P<31>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_P<30>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<35>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<34>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<33>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<32>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<31>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<30>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<29>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<28>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<27>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<26>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<25>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<24>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<23>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<22>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<21>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<20>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<19>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<18>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<17>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<16>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<15>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<14>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<13>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<12>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<11>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<10>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<9>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<8>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<7>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<6>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<5>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<4>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<3>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<2>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<1>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_M<0>_UNCONNECTED ;
  FD #(
    .INIT ( 1'b0 ))
  \blk00000001/blk00000029  (
    .C(clk),
    .D(\blk00000001/sig000001a5 ),
    .Q(p[0])
  );
  FD #(
    .INIT ( 1'b0 ))
  \blk00000001/blk00000028  (
    .C(clk),
    .D(\blk00000001/sig000001a6 ),
    .Q(p[1])
  );
  FD #(
    .INIT ( 1'b0 ))
  \blk00000001/blk00000027  (
    .C(clk),
    .D(\blk00000001/sig000001a7 ),
    .Q(p[2])
  );
  FD #(
    .INIT ( 1'b0 ))
  \blk00000001/blk00000026  (
    .C(clk),
    .D(\blk00000001/sig000001a8 ),
    .Q(p[3])
  );
  FD #(
    .INIT ( 1'b0 ))
  \blk00000001/blk00000025  (
    .C(clk),
    .D(\blk00000001/sig000001a9 ),
    .Q(p[4])
  );
  FD #(
    .INIT ( 1'b0 ))
  \blk00000001/blk00000024  (
    .C(clk),
    .D(\blk00000001/sig000001aa ),
    .Q(p[5])
  );
  FD #(
    .INIT ( 1'b0 ))
  \blk00000001/blk00000023  (
    .C(clk),
    .D(\blk00000001/sig000001ab ),
    .Q(p[6])
  );
  FD #(
    .INIT ( 1'b0 ))
  \blk00000001/blk00000022  (
    .C(clk),
    .D(\blk00000001/sig000001ac ),
    .Q(p[7])
  );
  FD #(
    .INIT ( 1'b0 ))
  \blk00000001/blk00000021  (
    .C(clk),
    .D(\blk00000001/sig000001ad ),
    .Q(p[8])
  );
  FD #(
    .INIT ( 1'b0 ))
  \blk00000001/blk00000020  (
    .C(clk),
    .D(\blk00000001/sig000001ae ),
    .Q(p[9])
  );
  FD #(
    .INIT ( 1'b0 ))
  \blk00000001/blk0000001f  (
    .C(clk),
    .D(\blk00000001/sig000001af ),
    .Q(p[10])
  );
  FD #(
    .INIT ( 1'b0 ))
  \blk00000001/blk0000001e  (
    .C(clk),
    .D(\blk00000001/sig000001b0 ),
    .Q(p[11])
  );
  FD #(
    .INIT ( 1'b0 ))
  \blk00000001/blk0000001d  (
    .C(clk),
    .D(\blk00000001/sig000001b1 ),
    .Q(p[12])
  );
  FD #(
    .INIT ( 1'b0 ))
  \blk00000001/blk0000001c  (
    .C(clk),
    .D(\blk00000001/sig000001b2 ),
    .Q(p[13])
  );
  FD #(
    .INIT ( 1'b0 ))
  \blk00000001/blk0000001b  (
    .C(clk),
    .D(\blk00000001/sig000001b3 ),
    .Q(p[14])
  );
  FD #(
    .INIT ( 1'b0 ))
  \blk00000001/blk0000001a  (
    .C(clk),
    .D(\blk00000001/sig000001b4 ),
    .Q(p[15])
  );
  FD #(
    .INIT ( 1'b0 ))
  \blk00000001/blk00000019  (
    .C(clk),
    .D(\blk00000001/sig000001b5 ),
    .Q(p[16])
  );
  FD #(
    .INIT ( 1'b0 ))
  \blk00000001/blk00000018  (
    .C(clk),
    .D(\blk00000001/sig00000133 ),
    .Q(p[17])
  );
  FD #(
    .INIT ( 1'b0 ))
  \blk00000001/blk00000017  (
    .C(clk),
    .D(\blk00000001/sig00000134 ),
    .Q(p[18])
  );
  FD #(
    .INIT ( 1'b0 ))
  \blk00000001/blk00000016  (
    .C(clk),
    .D(\blk00000001/sig00000135 ),
    .Q(p[19])
  );
  FD #(
    .INIT ( 1'b0 ))
  \blk00000001/blk00000015  (
    .C(clk),
    .D(\blk00000001/sig00000136 ),
    .Q(p[20])
  );
  FD #(
    .INIT ( 1'b0 ))
  \blk00000001/blk00000014  (
    .C(clk),
    .D(\blk00000001/sig00000137 ),
    .Q(p[21])
  );
  FD #(
    .INIT ( 1'b0 ))
  \blk00000001/blk00000013  (
    .C(clk),
    .D(\blk00000001/sig00000138 ),
    .Q(p[22])
  );
  FD #(
    .INIT ( 1'b0 ))
  \blk00000001/blk00000012  (
    .C(clk),
    .D(\blk00000001/sig00000139 ),
    .Q(p[23])
  );
  FD #(
    .INIT ( 1'b0 ))
  \blk00000001/blk00000011  (
    .C(clk),
    .D(\blk00000001/sig0000013a ),
    .Q(p[24])
  );
  FD #(
    .INIT ( 1'b0 ))
  \blk00000001/blk00000010  (
    .C(clk),
    .D(\blk00000001/sig0000013b ),
    .Q(p[25])
  );
  FD #(
    .INIT ( 1'b0 ))
  \blk00000001/blk0000000f  (
    .C(clk),
    .D(\blk00000001/sig0000013c ),
    .Q(p[26])
  );
  FD #(
    .INIT ( 1'b0 ))
  \blk00000001/blk0000000e  (
    .C(clk),
    .D(\blk00000001/sig0000013d ),
    .Q(p[27])
  );
  FD #(
    .INIT ( 1'b0 ))
  \blk00000001/blk0000000d  (
    .C(clk),
    .D(\blk00000001/sig0000013e ),
    .Q(p[28])
  );
  FD #(
    .INIT ( 1'b0 ))
  \blk00000001/blk0000000c  (
    .C(clk),
    .D(\blk00000001/sig0000013f ),
    .Q(p[29])
  );
  FD #(
    .INIT ( 1'b0 ))
  \blk00000001/blk0000000b  (
    .C(clk),
    .D(\blk00000001/sig00000140 ),
    .Q(p[30])
  );
  FD #(
    .INIT ( 1'b0 ))
  \blk00000001/blk0000000a  (
    .C(clk),
    .D(\blk00000001/sig00000141 ),
    .Q(p[31])
  );
  FD #(
    .INIT ( 1'b0 ))
  \blk00000001/blk00000009  (
    .C(clk),
    .D(\blk00000001/sig00000142 ),
    .Q(p[32])
  );
  FD #(
    .INIT ( 1'b0 ))
  \blk00000001/blk00000008  (
    .C(clk),
    .D(\blk00000001/sig00000143 ),
    .Q(p[33])
  );
  DSP48A1 #(
    .A0REG ( 0 ),
    .A1REG ( 1 ),
    .B0REG ( 0 ),
    .B1REG ( 1 ),
    .CARRYINREG ( 0 ),
    .CARRYINSEL ( "OPMODE5" ),
    .CREG ( 0 ),
    .DREG ( 0 ),
    .MREG ( 0 ),
    .OPMODEREG ( 0 ),
    .PREG ( 0 ),
    .RSTTYPE ( "SYNC" ),
    .CARRYOUTREG ( 0 ))
  \blk00000001/blk00000007  (
    .CECARRYIN(\blk00000001/sig000000b3 ),
    .RSTC(\blk00000001/sig000000b3 ),
    .RSTCARRYIN(\blk00000001/sig000000b3 ),
    .CED(\blk00000001/sig000000b3 ),
    .RSTD(\blk00000001/sig000000b3 ),
    .CEOPMODE(\blk00000001/sig000000b3 ),
    .CEC(\blk00000001/sig000000b3 ),
    .CARRYOUTF(\NLW_blk00000001/blk00000007_CARRYOUTF_UNCONNECTED ),
    .RSTOPMODE(\blk00000001/sig000000b3 ),
    .RSTM(\blk00000001/sig000000b3 ),
    .CLK(clk),
    .RSTB(\blk00000001/sig000000b3 ),
    .CEM(\blk00000001/sig000000b3 ),
    .CEB(\blk00000001/sig000000b2 ),
    .CARRYIN(\blk00000001/sig000000b3 ),
    .CEP(\blk00000001/sig000000b3 ),
    .CEA(\blk00000001/sig000000b2 ),
    .CARRYOUT(\NLW_blk00000001/blk00000007_CARRYOUT_UNCONNECTED ),
    .RSTA(\blk00000001/sig000000b3 ),
    .RSTP(\blk00000001/sig000000b3 ),
    .B({\blk00000001/sig000000b3 , b[16], b[15], b[14], b[13], b[12], b[11], b[10], b[9], b[8], b[7], b[6], b[5], b[4], b[3], b[2], b[1], b[0]}),
    .BCOUT({\blk00000001/sig000001c7 , \blk00000001/sig000001c6 , \blk00000001/sig000001c5 , \blk00000001/sig000001c4 , \blk00000001/sig000001c3 , 
\blk00000001/sig000001c2 , \blk00000001/sig000001c1 , \blk00000001/sig000001c0 , \blk00000001/sig000001bf , \blk00000001/sig000001be , 
\blk00000001/sig000001bd , \blk00000001/sig000001bc , \blk00000001/sig000001bb , \blk00000001/sig000001ba , \blk00000001/sig000001b9 , 
\blk00000001/sig000001b8 , \blk00000001/sig000001b7 , \blk00000001/sig000001b6 }),
    .PCIN({\blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , 
\blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , 
\blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , 
\blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , 
\blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , 
\blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , 
\blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , 
\blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , 
\blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , 
\blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 }),
    .C({\NLW_blk00000001/blk00000007_C<47>_UNCONNECTED , \NLW_blk00000001/blk00000007_C<46>_UNCONNECTED , 
\NLW_blk00000001/blk00000007_C<45>_UNCONNECTED , \NLW_blk00000001/blk00000007_C<44>_UNCONNECTED , \NLW_blk00000001/blk00000007_C<43>_UNCONNECTED , 
\NLW_blk00000001/blk00000007_C<42>_UNCONNECTED , \NLW_blk00000001/blk00000007_C<41>_UNCONNECTED , \NLW_blk00000001/blk00000007_C<40>_UNCONNECTED , 
\NLW_blk00000001/blk00000007_C<39>_UNCONNECTED , \NLW_blk00000001/blk00000007_C<38>_UNCONNECTED , \NLW_blk00000001/blk00000007_C<37>_UNCONNECTED , 
\NLW_blk00000001/blk00000007_C<36>_UNCONNECTED , \NLW_blk00000001/blk00000007_C<35>_UNCONNECTED , \NLW_blk00000001/blk00000007_C<34>_UNCONNECTED , 
\NLW_blk00000001/blk00000007_C<33>_UNCONNECTED , \NLW_blk00000001/blk00000007_C<32>_UNCONNECTED , \NLW_blk00000001/blk00000007_C<31>_UNCONNECTED , 
\NLW_blk00000001/blk00000007_C<30>_UNCONNECTED , \NLW_blk00000001/blk00000007_C<29>_UNCONNECTED , \NLW_blk00000001/blk00000007_C<28>_UNCONNECTED , 
\NLW_blk00000001/blk00000007_C<27>_UNCONNECTED , \NLW_blk00000001/blk00000007_C<26>_UNCONNECTED , \NLW_blk00000001/blk00000007_C<25>_UNCONNECTED , 
\NLW_blk00000001/blk00000007_C<24>_UNCONNECTED , \NLW_blk00000001/blk00000007_C<23>_UNCONNECTED , \NLW_blk00000001/blk00000007_C<22>_UNCONNECTED , 
\NLW_blk00000001/blk00000007_C<21>_UNCONNECTED , \NLW_blk00000001/blk00000007_C<20>_UNCONNECTED , \NLW_blk00000001/blk00000007_C<19>_UNCONNECTED , 
\NLW_blk00000001/blk00000007_C<18>_UNCONNECTED , \NLW_blk00000001/blk00000007_C<17>_UNCONNECTED , \NLW_blk00000001/blk00000007_C<16>_UNCONNECTED , 
\NLW_blk00000001/blk00000007_C<15>_UNCONNECTED , \NLW_blk00000001/blk00000007_C<14>_UNCONNECTED , \NLW_blk00000001/blk00000007_C<13>_UNCONNECTED , 
\NLW_blk00000001/blk00000007_C<12>_UNCONNECTED , \NLW_blk00000001/blk00000007_C<11>_UNCONNECTED , \NLW_blk00000001/blk00000007_C<10>_UNCONNECTED , 
\NLW_blk00000001/blk00000007_C<9>_UNCONNECTED , \NLW_blk00000001/blk00000007_C<8>_UNCONNECTED , \NLW_blk00000001/blk00000007_C<7>_UNCONNECTED , 
\NLW_blk00000001/blk00000007_C<6>_UNCONNECTED , \NLW_blk00000001/blk00000007_C<5>_UNCONNECTED , \NLW_blk00000001/blk00000007_C<4>_UNCONNECTED , 
\NLW_blk00000001/blk00000007_C<3>_UNCONNECTED , \NLW_blk00000001/blk00000007_C<2>_UNCONNECTED , \NLW_blk00000001/blk00000007_C<1>_UNCONNECTED , 
\NLW_blk00000001/blk00000007_C<0>_UNCONNECTED }),
    .P({\blk00000001/sig000001a4 , \blk00000001/sig000001a3 , \blk00000001/sig000001a2 , \blk00000001/sig000001a1 , \blk00000001/sig000001a0 , 
\blk00000001/sig0000019f , \blk00000001/sig0000019e , \blk00000001/sig0000019d , \blk00000001/sig0000019c , \blk00000001/sig0000019b , 
\blk00000001/sig0000019a , \blk00000001/sig00000199 , \blk00000001/sig00000198 , \blk00000001/sig00000197 , \blk00000001/sig00000196 , 
\blk00000001/sig00000195 , \blk00000001/sig00000194 , \blk00000001/sig00000193 , \blk00000001/sig00000192 , \blk00000001/sig00000191 , 
\blk00000001/sig00000190 , \blk00000001/sig0000018f , \blk00000001/sig0000018e , \blk00000001/sig0000018d , \blk00000001/sig0000018c , 
\blk00000001/sig0000018b , \blk00000001/sig0000018a , \blk00000001/sig00000189 , \blk00000001/sig00000188 , \blk00000001/sig00000187 , 
\blk00000001/sig00000186 , \blk00000001/sig000001b5 , \blk00000001/sig000001b4 , \blk00000001/sig000001b3 , \blk00000001/sig000001b2 , 
\blk00000001/sig000001b1 , \blk00000001/sig000001b0 , \blk00000001/sig000001af , \blk00000001/sig000001ae , \blk00000001/sig000001ad , 
\blk00000001/sig000001ac , \blk00000001/sig000001ab , \blk00000001/sig000001aa , \blk00000001/sig000001a9 , \blk00000001/sig000001a8 , 
\blk00000001/sig000001a7 , \blk00000001/sig000001a6 , \blk00000001/sig000001a5 }),
    .OPMODE({\blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , 
\blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b2 }),
    .D({\blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , 
\blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , 
\blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , 
\blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 }),
    .PCOUT({\blk00000001/sig00000185 , \blk00000001/sig00000184 , \blk00000001/sig00000183 , \blk00000001/sig00000182 , \blk00000001/sig00000181 , 
\blk00000001/sig00000180 , \blk00000001/sig0000017f , \blk00000001/sig0000017e , \blk00000001/sig0000017d , \blk00000001/sig0000017c , 
\blk00000001/sig0000017b , \blk00000001/sig0000017a , \blk00000001/sig00000179 , \blk00000001/sig00000178 , \blk00000001/sig00000177 , 
\blk00000001/sig00000176 , \blk00000001/sig00000175 , \blk00000001/sig00000174 , \blk00000001/sig00000173 , \blk00000001/sig00000172 , 
\blk00000001/sig00000171 , \blk00000001/sig00000170 , \blk00000001/sig0000016f , \blk00000001/sig0000016e , \blk00000001/sig0000016d , 
\blk00000001/sig0000016c , \blk00000001/sig0000016b , \blk00000001/sig0000016a , \blk00000001/sig00000169 , \blk00000001/sig00000168 , 
\blk00000001/sig00000167 , \blk00000001/sig00000166 , \blk00000001/sig00000165 , \blk00000001/sig00000164 , \blk00000001/sig00000163 , 
\blk00000001/sig00000162 , \blk00000001/sig00000161 , \blk00000001/sig00000160 , \blk00000001/sig0000015f , \blk00000001/sig0000015e , 
\blk00000001/sig0000015d , \blk00000001/sig0000015c , \blk00000001/sig0000015b , \blk00000001/sig0000015a , \blk00000001/sig00000159 , 
\blk00000001/sig00000158 , \blk00000001/sig00000157 , \blk00000001/sig00000156 }),
    .A({\blk00000001/sig000000b3 , a[16], a[15], a[14], a[13], a[12], a[11], a[10], a[9], a[8], a[7], a[6], a[5], a[4], a[3], a[2], a[1], a[0]}),
    .M({\NLW_blk00000001/blk00000007_M<35>_UNCONNECTED , \NLW_blk00000001/blk00000007_M<34>_UNCONNECTED , 
\NLW_blk00000001/blk00000007_M<33>_UNCONNECTED , \NLW_blk00000001/blk00000007_M<32>_UNCONNECTED , \NLW_blk00000001/blk00000007_M<31>_UNCONNECTED , 
\NLW_blk00000001/blk00000007_M<30>_UNCONNECTED , \NLW_blk00000001/blk00000007_M<29>_UNCONNECTED , \NLW_blk00000001/blk00000007_M<28>_UNCONNECTED , 
\NLW_blk00000001/blk00000007_M<27>_UNCONNECTED , \NLW_blk00000001/blk00000007_M<26>_UNCONNECTED , \NLW_blk00000001/blk00000007_M<25>_UNCONNECTED , 
\NLW_blk00000001/blk00000007_M<24>_UNCONNECTED , \NLW_blk00000001/blk00000007_M<23>_UNCONNECTED , \NLW_blk00000001/blk00000007_M<22>_UNCONNECTED , 
\NLW_blk00000001/blk00000007_M<21>_UNCONNECTED , \NLW_blk00000001/blk00000007_M<20>_UNCONNECTED , \NLW_blk00000001/blk00000007_M<19>_UNCONNECTED , 
\NLW_blk00000001/blk00000007_M<18>_UNCONNECTED , \NLW_blk00000001/blk00000007_M<17>_UNCONNECTED , \NLW_blk00000001/blk00000007_M<16>_UNCONNECTED , 
\NLW_blk00000001/blk00000007_M<15>_UNCONNECTED , \NLW_blk00000001/blk00000007_M<14>_UNCONNECTED , \NLW_blk00000001/blk00000007_M<13>_UNCONNECTED , 
\NLW_blk00000001/blk00000007_M<12>_UNCONNECTED , \NLW_blk00000001/blk00000007_M<11>_UNCONNECTED , \NLW_blk00000001/blk00000007_M<10>_UNCONNECTED , 
\NLW_blk00000001/blk00000007_M<9>_UNCONNECTED , \NLW_blk00000001/blk00000007_M<8>_UNCONNECTED , \NLW_blk00000001/blk00000007_M<7>_UNCONNECTED , 
\NLW_blk00000001/blk00000007_M<6>_UNCONNECTED , \NLW_blk00000001/blk00000007_M<5>_UNCONNECTED , \NLW_blk00000001/blk00000007_M<4>_UNCONNECTED , 
\NLW_blk00000001/blk00000007_M<3>_UNCONNECTED , \NLW_blk00000001/blk00000007_M<2>_UNCONNECTED , \NLW_blk00000001/blk00000007_M<1>_UNCONNECTED , 
\NLW_blk00000001/blk00000007_M<0>_UNCONNECTED })
  );
  DSP48A1 #(
    .A0REG ( 0 ),
    .A1REG ( 1 ),
    .B0REG ( 0 ),
    .B1REG ( 1 ),
    .CARRYINREG ( 0 ),
    .CARRYINSEL ( "OPMODE5" ),
    .CREG ( 0 ),
    .DREG ( 0 ),
    .MREG ( 0 ),
    .OPMODEREG ( 0 ),
    .PREG ( 0 ),
    .RSTTYPE ( "SYNC" ),
    .CARRYOUTREG ( 0 ))
  \blk00000001/blk00000006  (
    .CECARRYIN(\blk00000001/sig000000b3 ),
    .RSTC(\blk00000001/sig000000b3 ),
    .RSTCARRYIN(\blk00000001/sig000000b3 ),
    .CED(\blk00000001/sig000000b3 ),
    .RSTD(\blk00000001/sig000000b3 ),
    .CEOPMODE(\blk00000001/sig000000b3 ),
    .CEC(\blk00000001/sig000000b3 ),
    .CARRYOUTF(\NLW_blk00000001/blk00000006_CARRYOUTF_UNCONNECTED ),
    .RSTOPMODE(\blk00000001/sig000000b3 ),
    .RSTM(\blk00000001/sig000000b3 ),
    .CLK(clk),
    .RSTB(\blk00000001/sig000000b3 ),
    .CEM(\blk00000001/sig000000b3 ),
    .CEB(\blk00000001/sig000000b2 ),
    .CARRYIN(\blk00000001/sig000000b3 ),
    .CEP(\blk00000001/sig000000b3 ),
    .CEA(\blk00000001/sig000000b2 ),
    .CARRYOUT(\NLW_blk00000001/blk00000006_CARRYOUT_UNCONNECTED ),
    .RSTA(\blk00000001/sig000000b3 ),
    .RSTP(\blk00000001/sig000000b3 ),
    .B({\blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , b[31], b[30], b[29], b[28], b[27], b[26], b[25], b[24], b[23]
, b[22], b[21], b[20], b[19], b[18], b[17]}),
    .BCOUT({\blk00000001/sig00000155 , \blk00000001/sig00000154 , \blk00000001/sig00000153 , \blk00000001/sig00000152 , \blk00000001/sig00000151 , 
\blk00000001/sig00000150 , \blk00000001/sig0000014f , \blk00000001/sig0000014e , \blk00000001/sig0000014d , \blk00000001/sig0000014c , 
\blk00000001/sig0000014b , \blk00000001/sig0000014a , \blk00000001/sig00000149 , \blk00000001/sig00000148 , \blk00000001/sig00000147 , 
\blk00000001/sig00000146 , \blk00000001/sig00000145 , \blk00000001/sig00000144 }),
    .PCIN({\blk00000001/sig000000e3 , \blk00000001/sig000000e2 , \blk00000001/sig000000e1 , \blk00000001/sig000000e0 , \blk00000001/sig000000df , 
\blk00000001/sig000000de , \blk00000001/sig000000dd , \blk00000001/sig000000dc , \blk00000001/sig000000db , \blk00000001/sig000000da , 
\blk00000001/sig000000d9 , \blk00000001/sig000000d8 , \blk00000001/sig000000d7 , \blk00000001/sig000000d6 , \blk00000001/sig000000d5 , 
\blk00000001/sig000000d4 , \blk00000001/sig000000d3 , \blk00000001/sig000000d2 , \blk00000001/sig000000d1 , \blk00000001/sig000000d0 , 
\blk00000001/sig000000cf , \blk00000001/sig000000ce , \blk00000001/sig000000cd , \blk00000001/sig000000cc , \blk00000001/sig000000cb , 
\blk00000001/sig000000ca , \blk00000001/sig000000c9 , \blk00000001/sig000000c8 , \blk00000001/sig000000c7 , \blk00000001/sig000000c6 , 
\blk00000001/sig000000c5 , \blk00000001/sig000000c4 , \blk00000001/sig000000c3 , \blk00000001/sig000000c2 , \blk00000001/sig000000c1 , 
\blk00000001/sig000000c0 , \blk00000001/sig000000bf , \blk00000001/sig000000be , \blk00000001/sig000000bd , \blk00000001/sig000000bc , 
\blk00000001/sig000000bb , \blk00000001/sig000000ba , \blk00000001/sig000000b9 , \blk00000001/sig000000b8 , \blk00000001/sig000000b7 , 
\blk00000001/sig000000b6 , \blk00000001/sig000000b5 , \blk00000001/sig000000b4 }),
    .C({\NLW_blk00000001/blk00000006_C<47>_UNCONNECTED , \NLW_blk00000001/blk00000006_C<46>_UNCONNECTED , 
\NLW_blk00000001/blk00000006_C<45>_UNCONNECTED , \NLW_blk00000001/blk00000006_C<44>_UNCONNECTED , \NLW_blk00000001/blk00000006_C<43>_UNCONNECTED , 
\NLW_blk00000001/blk00000006_C<42>_UNCONNECTED , \NLW_blk00000001/blk00000006_C<41>_UNCONNECTED , \NLW_blk00000001/blk00000006_C<40>_UNCONNECTED , 
\NLW_blk00000001/blk00000006_C<39>_UNCONNECTED , \NLW_blk00000001/blk00000006_C<38>_UNCONNECTED , \NLW_blk00000001/blk00000006_C<37>_UNCONNECTED , 
\NLW_blk00000001/blk00000006_C<36>_UNCONNECTED , \NLW_blk00000001/blk00000006_C<35>_UNCONNECTED , \NLW_blk00000001/blk00000006_C<34>_UNCONNECTED , 
\NLW_blk00000001/blk00000006_C<33>_UNCONNECTED , \NLW_blk00000001/blk00000006_C<32>_UNCONNECTED , \NLW_blk00000001/blk00000006_C<31>_UNCONNECTED , 
\NLW_blk00000001/blk00000006_C<30>_UNCONNECTED , \NLW_blk00000001/blk00000006_C<29>_UNCONNECTED , \NLW_blk00000001/blk00000006_C<28>_UNCONNECTED , 
\NLW_blk00000001/blk00000006_C<27>_UNCONNECTED , \NLW_blk00000001/blk00000006_C<26>_UNCONNECTED , \NLW_blk00000001/blk00000006_C<25>_UNCONNECTED , 
\NLW_blk00000001/blk00000006_C<24>_UNCONNECTED , \NLW_blk00000001/blk00000006_C<23>_UNCONNECTED , \NLW_blk00000001/blk00000006_C<22>_UNCONNECTED , 
\NLW_blk00000001/blk00000006_C<21>_UNCONNECTED , \NLW_blk00000001/blk00000006_C<20>_UNCONNECTED , \NLW_blk00000001/blk00000006_C<19>_UNCONNECTED , 
\NLW_blk00000001/blk00000006_C<18>_UNCONNECTED , \NLW_blk00000001/blk00000006_C<17>_UNCONNECTED , \NLW_blk00000001/blk00000006_C<16>_UNCONNECTED , 
\NLW_blk00000001/blk00000006_C<15>_UNCONNECTED , \NLW_blk00000001/blk00000006_C<14>_UNCONNECTED , \NLW_blk00000001/blk00000006_C<13>_UNCONNECTED , 
\NLW_blk00000001/blk00000006_C<12>_UNCONNECTED , \NLW_blk00000001/blk00000006_C<11>_UNCONNECTED , \NLW_blk00000001/blk00000006_C<10>_UNCONNECTED , 
\NLW_blk00000001/blk00000006_C<9>_UNCONNECTED , \NLW_blk00000001/blk00000006_C<8>_UNCONNECTED , \NLW_blk00000001/blk00000006_C<7>_UNCONNECTED , 
\NLW_blk00000001/blk00000006_C<6>_UNCONNECTED , \NLW_blk00000001/blk00000006_C<5>_UNCONNECTED , \NLW_blk00000001/blk00000006_C<4>_UNCONNECTED , 
\NLW_blk00000001/blk00000006_C<3>_UNCONNECTED , \NLW_blk00000001/blk00000006_C<2>_UNCONNECTED , \NLW_blk00000001/blk00000006_C<1>_UNCONNECTED , 
\NLW_blk00000001/blk00000006_C<0>_UNCONNECTED }),
    .P({\blk00000001/sig00000132 , \blk00000001/sig00000131 , \blk00000001/sig00000130 , \blk00000001/sig0000012f , \blk00000001/sig0000012e , 
\blk00000001/sig0000012d , \blk00000001/sig0000012c , \blk00000001/sig0000012b , \blk00000001/sig0000012a , \blk00000001/sig00000129 , 
\blk00000001/sig00000128 , \blk00000001/sig00000127 , \blk00000001/sig00000126 , \blk00000001/sig00000125 , \blk00000001/sig00000124 , 
\blk00000001/sig00000123 , \blk00000001/sig00000122 , \blk00000001/sig00000121 , \blk00000001/sig00000120 , \blk00000001/sig0000011f , 
\blk00000001/sig0000011e , \blk00000001/sig0000011d , \blk00000001/sig0000011c , \blk00000001/sig0000011b , \blk00000001/sig0000011a , 
\blk00000001/sig00000119 , \blk00000001/sig00000118 , \blk00000001/sig00000117 , \blk00000001/sig00000116 , \blk00000001/sig00000115 , 
\blk00000001/sig00000114 , \blk00000001/sig00000143 , \blk00000001/sig00000142 , \blk00000001/sig00000141 , \blk00000001/sig00000140 , 
\blk00000001/sig0000013f , \blk00000001/sig0000013e , \blk00000001/sig0000013d , \blk00000001/sig0000013c , \blk00000001/sig0000013b , 
\blk00000001/sig0000013a , \blk00000001/sig00000139 , \blk00000001/sig00000138 , \blk00000001/sig00000137 , \blk00000001/sig00000136 , 
\blk00000001/sig00000135 , \blk00000001/sig00000134 , \blk00000001/sig00000133 }),
    .OPMODE({\blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , 
\blk00000001/sig000000b2 , \blk00000001/sig000000b3 , \blk00000001/sig000000b2 }),
    .D({\blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , 
\blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , 
\blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , 
\blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 }),
    .PCOUT({\blk00000001/sig00000113 , \blk00000001/sig00000112 , \blk00000001/sig00000111 , \blk00000001/sig00000110 , \blk00000001/sig0000010f , 
\blk00000001/sig0000010e , \blk00000001/sig0000010d , \blk00000001/sig0000010c , \blk00000001/sig0000010b , \blk00000001/sig0000010a , 
\blk00000001/sig00000109 , \blk00000001/sig00000108 , \blk00000001/sig00000107 , \blk00000001/sig00000106 , \blk00000001/sig00000105 , 
\blk00000001/sig00000104 , \blk00000001/sig00000103 , \blk00000001/sig00000102 , \blk00000001/sig00000101 , \blk00000001/sig00000100 , 
\blk00000001/sig000000ff , \blk00000001/sig000000fe , \blk00000001/sig000000fd , \blk00000001/sig000000fc , \blk00000001/sig000000fb , 
\blk00000001/sig000000fa , \blk00000001/sig000000f9 , \blk00000001/sig000000f8 , \blk00000001/sig000000f7 , \blk00000001/sig000000f6 , 
\blk00000001/sig000000f5 , \blk00000001/sig000000f4 , \blk00000001/sig000000f3 , \blk00000001/sig000000f2 , \blk00000001/sig000000f1 , 
\blk00000001/sig000000f0 , \blk00000001/sig000000ef , \blk00000001/sig000000ee , \blk00000001/sig000000ed , \blk00000001/sig000000ec , 
\blk00000001/sig000000eb , \blk00000001/sig000000ea , \blk00000001/sig000000e9 , \blk00000001/sig000000e8 , \blk00000001/sig000000e7 , 
\blk00000001/sig000000e6 , \blk00000001/sig000000e5 , \blk00000001/sig000000e4 }),
    .A({\blk00000001/sig000000b3 , a[16], a[15], a[14], a[13], a[12], a[11], a[10], a[9], a[8], a[7], a[6], a[5], a[4], a[3], a[2], a[1], a[0]}),
    .M({\NLW_blk00000001/blk00000006_M<35>_UNCONNECTED , \NLW_blk00000001/blk00000006_M<34>_UNCONNECTED , 
\NLW_blk00000001/blk00000006_M<33>_UNCONNECTED , \NLW_blk00000001/blk00000006_M<32>_UNCONNECTED , \NLW_blk00000001/blk00000006_M<31>_UNCONNECTED , 
\NLW_blk00000001/blk00000006_M<30>_UNCONNECTED , \NLW_blk00000001/blk00000006_M<29>_UNCONNECTED , \NLW_blk00000001/blk00000006_M<28>_UNCONNECTED , 
\NLW_blk00000001/blk00000006_M<27>_UNCONNECTED , \NLW_blk00000001/blk00000006_M<26>_UNCONNECTED , \NLW_blk00000001/blk00000006_M<25>_UNCONNECTED , 
\NLW_blk00000001/blk00000006_M<24>_UNCONNECTED , \NLW_blk00000001/blk00000006_M<23>_UNCONNECTED , \NLW_blk00000001/blk00000006_M<22>_UNCONNECTED , 
\NLW_blk00000001/blk00000006_M<21>_UNCONNECTED , \NLW_blk00000001/blk00000006_M<20>_UNCONNECTED , \NLW_blk00000001/blk00000006_M<19>_UNCONNECTED , 
\NLW_blk00000001/blk00000006_M<18>_UNCONNECTED , \NLW_blk00000001/blk00000006_M<17>_UNCONNECTED , \NLW_blk00000001/blk00000006_M<16>_UNCONNECTED , 
\NLW_blk00000001/blk00000006_M<15>_UNCONNECTED , \NLW_blk00000001/blk00000006_M<14>_UNCONNECTED , \NLW_blk00000001/blk00000006_M<13>_UNCONNECTED , 
\NLW_blk00000001/blk00000006_M<12>_UNCONNECTED , \NLW_blk00000001/blk00000006_M<11>_UNCONNECTED , \NLW_blk00000001/blk00000006_M<10>_UNCONNECTED , 
\NLW_blk00000001/blk00000006_M<9>_UNCONNECTED , \NLW_blk00000001/blk00000006_M<8>_UNCONNECTED , \NLW_blk00000001/blk00000006_M<7>_UNCONNECTED , 
\NLW_blk00000001/blk00000006_M<6>_UNCONNECTED , \NLW_blk00000001/blk00000006_M<5>_UNCONNECTED , \NLW_blk00000001/blk00000006_M<4>_UNCONNECTED , 
\NLW_blk00000001/blk00000006_M<3>_UNCONNECTED , \NLW_blk00000001/blk00000006_M<2>_UNCONNECTED , \NLW_blk00000001/blk00000006_M<1>_UNCONNECTED , 
\NLW_blk00000001/blk00000006_M<0>_UNCONNECTED })
  );
  DSP48A1 #(
    .A0REG ( 0 ),
    .A1REG ( 1 ),
    .B0REG ( 0 ),
    .B1REG ( 0 ),
    .CARRYINREG ( 0 ),
    .CARRYINSEL ( "OPMODE5" ),
    .CREG ( 0 ),
    .DREG ( 0 ),
    .MREG ( 0 ),
    .OPMODEREG ( 0 ),
    .PREG ( 0 ),
    .RSTTYPE ( "SYNC" ),
    .CARRYOUTREG ( 0 ))
  \blk00000001/blk00000005  (
    .CECARRYIN(\blk00000001/sig000000b3 ),
    .RSTC(\blk00000001/sig000000b3 ),
    .RSTCARRYIN(\blk00000001/sig000000b3 ),
    .CED(\blk00000001/sig000000b3 ),
    .RSTD(\blk00000001/sig000000b3 ),
    .CEOPMODE(\blk00000001/sig000000b3 ),
    .CEC(\blk00000001/sig000000b3 ),
    .CARRYOUTF(\NLW_blk00000001/blk00000005_CARRYOUTF_UNCONNECTED ),
    .RSTOPMODE(\blk00000001/sig000000b3 ),
    .RSTM(\blk00000001/sig000000b3 ),
    .CLK(clk),
    .RSTB(\blk00000001/sig000000b3 ),
    .CEM(\blk00000001/sig000000b3 ),
    .CEB(\blk00000001/sig000000b3 ),
    .CARRYIN(\blk00000001/sig000000b3 ),
    .CEP(\blk00000001/sig000000b3 ),
    .CEA(\blk00000001/sig000000b2 ),
    .CARRYOUT(\NLW_blk00000001/blk00000005_CARRYOUT_UNCONNECTED ),
    .RSTA(\blk00000001/sig000000b3 ),
    .RSTP(\blk00000001/sig000000b3 ),
    .B({\blk00000001/sig000001c7 , \blk00000001/sig000001c6 , \blk00000001/sig000001c5 , \blk00000001/sig000001c4 , \blk00000001/sig000001c3 , 
\blk00000001/sig000001c2 , \blk00000001/sig000001c1 , \blk00000001/sig000001c0 , \blk00000001/sig000001bf , \blk00000001/sig000001be , 
\blk00000001/sig000001bd , \blk00000001/sig000001bc , \blk00000001/sig000001bb , \blk00000001/sig000001ba , \blk00000001/sig000001b9 , 
\blk00000001/sig000001b8 , \blk00000001/sig000001b7 , \blk00000001/sig000001b6 }),
    .BCOUT({\NLW_blk00000001/blk00000005_BCOUT<17>_UNCONNECTED , \NLW_blk00000001/blk00000005_BCOUT<16>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_BCOUT<15>_UNCONNECTED , \NLW_blk00000001/blk00000005_BCOUT<14>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_BCOUT<13>_UNCONNECTED , \NLW_blk00000001/blk00000005_BCOUT<12>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_BCOUT<11>_UNCONNECTED , \NLW_blk00000001/blk00000005_BCOUT<10>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_BCOUT<9>_UNCONNECTED , \NLW_blk00000001/blk00000005_BCOUT<8>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_BCOUT<7>_UNCONNECTED , \NLW_blk00000001/blk00000005_BCOUT<6>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_BCOUT<5>_UNCONNECTED , \NLW_blk00000001/blk00000005_BCOUT<4>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_BCOUT<3>_UNCONNECTED , \NLW_blk00000001/blk00000005_BCOUT<2>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_BCOUT<1>_UNCONNECTED , \NLW_blk00000001/blk00000005_BCOUT<0>_UNCONNECTED }),
    .PCIN({\blk00000001/sig00000185 , \blk00000001/sig00000184 , \blk00000001/sig00000183 , \blk00000001/sig00000182 , \blk00000001/sig00000181 , 
\blk00000001/sig00000180 , \blk00000001/sig0000017f , \blk00000001/sig0000017e , \blk00000001/sig0000017d , \blk00000001/sig0000017c , 
\blk00000001/sig0000017b , \blk00000001/sig0000017a , \blk00000001/sig00000179 , \blk00000001/sig00000178 , \blk00000001/sig00000177 , 
\blk00000001/sig00000176 , \blk00000001/sig00000175 , \blk00000001/sig00000174 , \blk00000001/sig00000173 , \blk00000001/sig00000172 , 
\blk00000001/sig00000171 , \blk00000001/sig00000170 , \blk00000001/sig0000016f , \blk00000001/sig0000016e , \blk00000001/sig0000016d , 
\blk00000001/sig0000016c , \blk00000001/sig0000016b , \blk00000001/sig0000016a , \blk00000001/sig00000169 , \blk00000001/sig00000168 , 
\blk00000001/sig00000167 , \blk00000001/sig00000166 , \blk00000001/sig00000165 , \blk00000001/sig00000164 , \blk00000001/sig00000163 , 
\blk00000001/sig00000162 , \blk00000001/sig00000161 , \blk00000001/sig00000160 , \blk00000001/sig0000015f , \blk00000001/sig0000015e , 
\blk00000001/sig0000015d , \blk00000001/sig0000015c , \blk00000001/sig0000015b , \blk00000001/sig0000015a , \blk00000001/sig00000159 , 
\blk00000001/sig00000158 , \blk00000001/sig00000157 , \blk00000001/sig00000156 }),
    .C({\blk00000001/sig000001a4 , \blk00000001/sig000001a4 , \blk00000001/sig000001a4 , \blk00000001/sig000001a4 , \blk00000001/sig000001a4 , 
\blk00000001/sig000001a4 , \blk00000001/sig000001a4 , \blk00000001/sig000001a4 , \blk00000001/sig000001a4 , \blk00000001/sig000001a4 , 
\blk00000001/sig000001a4 , \blk00000001/sig000001a4 , \blk00000001/sig000001a4 , \blk00000001/sig000001a4 , \blk00000001/sig000001a4 , 
\blk00000001/sig000001a4 , \blk00000001/sig000001a4 , \blk00000001/sig000001a4 , \blk00000001/sig000001a3 , \blk00000001/sig000001a2 , 
\blk00000001/sig000001a1 , \blk00000001/sig000001a0 , \blk00000001/sig0000019f , \blk00000001/sig0000019e , \blk00000001/sig0000019d , 
\blk00000001/sig0000019c , \blk00000001/sig0000019b , \blk00000001/sig0000019a , \blk00000001/sig00000199 , \blk00000001/sig00000198 , 
\blk00000001/sig00000197 , \blk00000001/sig00000196 , \blk00000001/sig00000195 , \blk00000001/sig00000194 , \blk00000001/sig00000193 , 
\blk00000001/sig00000192 , \blk00000001/sig00000191 , \blk00000001/sig00000190 , \blk00000001/sig0000018f , \blk00000001/sig0000018e , 
\blk00000001/sig0000018d , \blk00000001/sig0000018c , \blk00000001/sig0000018b , \blk00000001/sig0000018a , \blk00000001/sig00000189 , 
\blk00000001/sig00000188 , \blk00000001/sig00000187 , \blk00000001/sig00000186 }),
    .P({\NLW_blk00000001/blk00000005_P<47>_UNCONNECTED , \NLW_blk00000001/blk00000005_P<46>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_P<45>_UNCONNECTED , \NLW_blk00000001/blk00000005_P<44>_UNCONNECTED , \NLW_blk00000001/blk00000005_P<43>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_P<42>_UNCONNECTED , \NLW_blk00000001/blk00000005_P<41>_UNCONNECTED , \NLW_blk00000001/blk00000005_P<40>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_P<39>_UNCONNECTED , \NLW_blk00000001/blk00000005_P<38>_UNCONNECTED , \NLW_blk00000001/blk00000005_P<37>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_P<36>_UNCONNECTED , \NLW_blk00000001/blk00000005_P<35>_UNCONNECTED , \NLW_blk00000001/blk00000005_P<34>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_P<33>_UNCONNECTED , \NLW_blk00000001/blk00000005_P<32>_UNCONNECTED , \NLW_blk00000001/blk00000005_P<31>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_P<30>_UNCONNECTED , \NLW_blk00000001/blk00000005_P<29>_UNCONNECTED , \NLW_blk00000001/blk00000005_P<28>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_P<27>_UNCONNECTED , \NLW_blk00000001/blk00000005_P<26>_UNCONNECTED , \NLW_blk00000001/blk00000005_P<25>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_P<24>_UNCONNECTED , \NLW_blk00000001/blk00000005_P<23>_UNCONNECTED , \NLW_blk00000001/blk00000005_P<22>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_P<21>_UNCONNECTED , \NLW_blk00000001/blk00000005_P<20>_UNCONNECTED , \NLW_blk00000001/blk00000005_P<19>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_P<18>_UNCONNECTED , \NLW_blk00000001/blk00000005_P<17>_UNCONNECTED , \NLW_blk00000001/blk00000005_P<16>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_P<15>_UNCONNECTED , \NLW_blk00000001/blk00000005_P<14>_UNCONNECTED , \NLW_blk00000001/blk00000005_P<13>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_P<12>_UNCONNECTED , \NLW_blk00000001/blk00000005_P<11>_UNCONNECTED , \NLW_blk00000001/blk00000005_P<10>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_P<9>_UNCONNECTED , \NLW_blk00000001/blk00000005_P<8>_UNCONNECTED , \NLW_blk00000001/blk00000005_P<7>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_P<6>_UNCONNECTED , \NLW_blk00000001/blk00000005_P<5>_UNCONNECTED , \NLW_blk00000001/blk00000005_P<4>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_P<3>_UNCONNECTED , \NLW_blk00000001/blk00000005_P<2>_UNCONNECTED , \NLW_blk00000001/blk00000005_P<1>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_P<0>_UNCONNECTED }),
    .OPMODE({\blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b2 , 
\blk00000001/sig000000b2 , \blk00000001/sig000000b3 , \blk00000001/sig000000b2 }),
    .D({\blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , 
\blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , 
\blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , 
\blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 }),
    .PCOUT({\blk00000001/sig000000e3 , \blk00000001/sig000000e2 , \blk00000001/sig000000e1 , \blk00000001/sig000000e0 , \blk00000001/sig000000df , 
\blk00000001/sig000000de , \blk00000001/sig000000dd , \blk00000001/sig000000dc , \blk00000001/sig000000db , \blk00000001/sig000000da , 
\blk00000001/sig000000d9 , \blk00000001/sig000000d8 , \blk00000001/sig000000d7 , \blk00000001/sig000000d6 , \blk00000001/sig000000d5 , 
\blk00000001/sig000000d4 , \blk00000001/sig000000d3 , \blk00000001/sig000000d2 , \blk00000001/sig000000d1 , \blk00000001/sig000000d0 , 
\blk00000001/sig000000cf , \blk00000001/sig000000ce , \blk00000001/sig000000cd , \blk00000001/sig000000cc , \blk00000001/sig000000cb , 
\blk00000001/sig000000ca , \blk00000001/sig000000c9 , \blk00000001/sig000000c8 , \blk00000001/sig000000c7 , \blk00000001/sig000000c6 , 
\blk00000001/sig000000c5 , \blk00000001/sig000000c4 , \blk00000001/sig000000c3 , \blk00000001/sig000000c2 , \blk00000001/sig000000c1 , 
\blk00000001/sig000000c0 , \blk00000001/sig000000bf , \blk00000001/sig000000be , \blk00000001/sig000000bd , \blk00000001/sig000000bc , 
\blk00000001/sig000000bb , \blk00000001/sig000000ba , \blk00000001/sig000000b9 , \blk00000001/sig000000b8 , \blk00000001/sig000000b7 , 
\blk00000001/sig000000b6 , \blk00000001/sig000000b5 , \blk00000001/sig000000b4 }),
    .A({\blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , a[31], a[30], a[29], a[28], a[27], a[26], a[25], a[24], a[23]
, a[22], a[21], a[20], a[19], a[18], a[17]}),
    .M({\NLW_blk00000001/blk00000005_M<35>_UNCONNECTED , \NLW_blk00000001/blk00000005_M<34>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_M<33>_UNCONNECTED , \NLW_blk00000001/blk00000005_M<32>_UNCONNECTED , \NLW_blk00000001/blk00000005_M<31>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_M<30>_UNCONNECTED , \NLW_blk00000001/blk00000005_M<29>_UNCONNECTED , \NLW_blk00000001/blk00000005_M<28>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_M<27>_UNCONNECTED , \NLW_blk00000001/blk00000005_M<26>_UNCONNECTED , \NLW_blk00000001/blk00000005_M<25>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_M<24>_UNCONNECTED , \NLW_blk00000001/blk00000005_M<23>_UNCONNECTED , \NLW_blk00000001/blk00000005_M<22>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_M<21>_UNCONNECTED , \NLW_blk00000001/blk00000005_M<20>_UNCONNECTED , \NLW_blk00000001/blk00000005_M<19>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_M<18>_UNCONNECTED , \NLW_blk00000001/blk00000005_M<17>_UNCONNECTED , \NLW_blk00000001/blk00000005_M<16>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_M<15>_UNCONNECTED , \NLW_blk00000001/blk00000005_M<14>_UNCONNECTED , \NLW_blk00000001/blk00000005_M<13>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_M<12>_UNCONNECTED , \NLW_blk00000001/blk00000005_M<11>_UNCONNECTED , \NLW_blk00000001/blk00000005_M<10>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_M<9>_UNCONNECTED , \NLW_blk00000001/blk00000005_M<8>_UNCONNECTED , \NLW_blk00000001/blk00000005_M<7>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_M<6>_UNCONNECTED , \NLW_blk00000001/blk00000005_M<5>_UNCONNECTED , \NLW_blk00000001/blk00000005_M<4>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_M<3>_UNCONNECTED , \NLW_blk00000001/blk00000005_M<2>_UNCONNECTED , \NLW_blk00000001/blk00000005_M<1>_UNCONNECTED , 
\NLW_blk00000001/blk00000005_M<0>_UNCONNECTED })
  );
  DSP48A1 #(
    .A0REG ( 0 ),
    .A1REG ( 1 ),
    .B0REG ( 0 ),
    .B1REG ( 0 ),
    .CARRYINREG ( 0 ),
    .CARRYINSEL ( "OPMODE5" ),
    .CREG ( 0 ),
    .DREG ( 0 ),
    .MREG ( 0 ),
    .OPMODEREG ( 0 ),
    .PREG ( 1 ),
    .RSTTYPE ( "SYNC" ),
    .CARRYOUTREG ( 0 ))
  \blk00000001/blk00000004  (
    .CECARRYIN(\blk00000001/sig000000b3 ),
    .RSTC(\blk00000001/sig000000b3 ),
    .RSTCARRYIN(\blk00000001/sig000000b3 ),
    .CED(\blk00000001/sig000000b3 ),
    .RSTD(\blk00000001/sig000000b3 ),
    .CEOPMODE(\blk00000001/sig000000b3 ),
    .CEC(\blk00000001/sig000000b3 ),
    .CARRYOUTF(\NLW_blk00000001/blk00000004_CARRYOUTF_UNCONNECTED ),
    .RSTOPMODE(\blk00000001/sig000000b3 ),
    .RSTM(\blk00000001/sig000000b3 ),
    .CLK(clk),
    .RSTB(\blk00000001/sig000000b3 ),
    .CEM(\blk00000001/sig000000b3 ),
    .CEB(\blk00000001/sig000000b3 ),
    .CARRYIN(\blk00000001/sig000000b3 ),
    .CEP(\blk00000001/sig000000b2 ),
    .CEA(\blk00000001/sig000000b2 ),
    .CARRYOUT(\NLW_blk00000001/blk00000004_CARRYOUT_UNCONNECTED ),
    .RSTA(\blk00000001/sig000000b3 ),
    .RSTP(\blk00000001/sig000000b3 ),
    .B({\blk00000001/sig00000155 , \blk00000001/sig00000154 , \blk00000001/sig00000153 , \blk00000001/sig00000152 , \blk00000001/sig00000151 , 
\blk00000001/sig00000150 , \blk00000001/sig0000014f , \blk00000001/sig0000014e , \blk00000001/sig0000014d , \blk00000001/sig0000014c , 
\blk00000001/sig0000014b , \blk00000001/sig0000014a , \blk00000001/sig00000149 , \blk00000001/sig00000148 , \blk00000001/sig00000147 , 
\blk00000001/sig00000146 , \blk00000001/sig00000145 , \blk00000001/sig00000144 }),
    .BCOUT({\NLW_blk00000001/blk00000004_BCOUT<17>_UNCONNECTED , \NLW_blk00000001/blk00000004_BCOUT<16>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_BCOUT<15>_UNCONNECTED , \NLW_blk00000001/blk00000004_BCOUT<14>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_BCOUT<13>_UNCONNECTED , \NLW_blk00000001/blk00000004_BCOUT<12>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_BCOUT<11>_UNCONNECTED , \NLW_blk00000001/blk00000004_BCOUT<10>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_BCOUT<9>_UNCONNECTED , \NLW_blk00000001/blk00000004_BCOUT<8>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_BCOUT<7>_UNCONNECTED , \NLW_blk00000001/blk00000004_BCOUT<6>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_BCOUT<5>_UNCONNECTED , \NLW_blk00000001/blk00000004_BCOUT<4>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_BCOUT<3>_UNCONNECTED , \NLW_blk00000001/blk00000004_BCOUT<2>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_BCOUT<1>_UNCONNECTED , \NLW_blk00000001/blk00000004_BCOUT<0>_UNCONNECTED }),
    .PCIN({\blk00000001/sig00000113 , \blk00000001/sig00000112 , \blk00000001/sig00000111 , \blk00000001/sig00000110 , \blk00000001/sig0000010f , 
\blk00000001/sig0000010e , \blk00000001/sig0000010d , \blk00000001/sig0000010c , \blk00000001/sig0000010b , \blk00000001/sig0000010a , 
\blk00000001/sig00000109 , \blk00000001/sig00000108 , \blk00000001/sig00000107 , \blk00000001/sig00000106 , \blk00000001/sig00000105 , 
\blk00000001/sig00000104 , \blk00000001/sig00000103 , \blk00000001/sig00000102 , \blk00000001/sig00000101 , \blk00000001/sig00000100 , 
\blk00000001/sig000000ff , \blk00000001/sig000000fe , \blk00000001/sig000000fd , \blk00000001/sig000000fc , \blk00000001/sig000000fb , 
\blk00000001/sig000000fa , \blk00000001/sig000000f9 , \blk00000001/sig000000f8 , \blk00000001/sig000000f7 , \blk00000001/sig000000f6 , 
\blk00000001/sig000000f5 , \blk00000001/sig000000f4 , \blk00000001/sig000000f3 , \blk00000001/sig000000f2 , \blk00000001/sig000000f1 , 
\blk00000001/sig000000f0 , \blk00000001/sig000000ef , \blk00000001/sig000000ee , \blk00000001/sig000000ed , \blk00000001/sig000000ec , 
\blk00000001/sig000000eb , \blk00000001/sig000000ea , \blk00000001/sig000000e9 , \blk00000001/sig000000e8 , \blk00000001/sig000000e7 , 
\blk00000001/sig000000e6 , \blk00000001/sig000000e5 , \blk00000001/sig000000e4 }),
    .C({\blk00000001/sig00000132 , \blk00000001/sig00000132 , \blk00000001/sig00000132 , \blk00000001/sig00000132 , \blk00000001/sig00000132 , 
\blk00000001/sig00000132 , \blk00000001/sig00000132 , \blk00000001/sig00000132 , \blk00000001/sig00000132 , \blk00000001/sig00000132 , 
\blk00000001/sig00000132 , \blk00000001/sig00000132 , \blk00000001/sig00000132 , \blk00000001/sig00000132 , \blk00000001/sig00000132 , 
\blk00000001/sig00000132 , \blk00000001/sig00000132 , \blk00000001/sig00000132 , \blk00000001/sig00000131 , \blk00000001/sig00000130 , 
\blk00000001/sig0000012f , \blk00000001/sig0000012e , \blk00000001/sig0000012d , \blk00000001/sig0000012c , \blk00000001/sig0000012b , 
\blk00000001/sig0000012a , \blk00000001/sig00000129 , \blk00000001/sig00000128 , \blk00000001/sig00000127 , \blk00000001/sig00000126 , 
\blk00000001/sig00000125 , \blk00000001/sig00000124 , \blk00000001/sig00000123 , \blk00000001/sig00000122 , \blk00000001/sig00000121 , 
\blk00000001/sig00000120 , \blk00000001/sig0000011f , \blk00000001/sig0000011e , \blk00000001/sig0000011d , \blk00000001/sig0000011c , 
\blk00000001/sig0000011b , \blk00000001/sig0000011a , \blk00000001/sig00000119 , \blk00000001/sig00000118 , \blk00000001/sig00000117 , 
\blk00000001/sig00000116 , \blk00000001/sig00000115 , \blk00000001/sig00000114 }),
    .P({\NLW_blk00000001/blk00000004_P<47>_UNCONNECTED , \NLW_blk00000001/blk00000004_P<46>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_P<45>_UNCONNECTED , \NLW_blk00000001/blk00000004_P<44>_UNCONNECTED , \NLW_blk00000001/blk00000004_P<43>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_P<42>_UNCONNECTED , \NLW_blk00000001/blk00000004_P<41>_UNCONNECTED , \NLW_blk00000001/blk00000004_P<40>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_P<39>_UNCONNECTED , \NLW_blk00000001/blk00000004_P<38>_UNCONNECTED , \NLW_blk00000001/blk00000004_P<37>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_P<36>_UNCONNECTED , \NLW_blk00000001/blk00000004_P<35>_UNCONNECTED , \NLW_blk00000001/blk00000004_P<34>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_P<33>_UNCONNECTED , \NLW_blk00000001/blk00000004_P<32>_UNCONNECTED , \NLW_blk00000001/blk00000004_P<31>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_P<30>_UNCONNECTED , p[63], p[62], p[61], p[60], p[59], p[58], p[57], p[56], p[55], p[54], p[53], p[52], p[51], p[50], 
p[49], p[48], p[47], p[46], p[45], p[44], p[43], p[42], p[41], p[40], p[39], p[38], p[37], p[36], p[35], p[34]}),
    .OPMODE({\blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b2 , 
\blk00000001/sig000000b2 , \blk00000001/sig000000b3 , \blk00000001/sig000000b2 }),
    .D({\blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , 
\blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , 
\blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , 
\blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 }),
    .PCOUT({\blk00000001/sig00000082 , \blk00000001/sig00000083 , \blk00000001/sig00000084 , \blk00000001/sig00000085 , \blk00000001/sig00000086 , 
\blk00000001/sig00000087 , \blk00000001/sig00000088 , \blk00000001/sig00000089 , \blk00000001/sig0000008a , \blk00000001/sig0000008b , 
\blk00000001/sig0000008c , \blk00000001/sig0000008d , \blk00000001/sig0000008e , \blk00000001/sig0000008f , \blk00000001/sig00000090 , 
\blk00000001/sig00000091 , \blk00000001/sig00000092 , \blk00000001/sig00000093 , \blk00000001/sig00000094 , \blk00000001/sig00000095 , 
\blk00000001/sig00000096 , \blk00000001/sig00000097 , \blk00000001/sig00000098 , \blk00000001/sig00000099 , \blk00000001/sig0000009a , 
\blk00000001/sig0000009b , \blk00000001/sig0000009c , \blk00000001/sig0000009d , \blk00000001/sig0000009e , \blk00000001/sig0000009f , 
\blk00000001/sig000000a0 , \blk00000001/sig000000a1 , \blk00000001/sig000000a2 , \blk00000001/sig000000a3 , \blk00000001/sig000000a4 , 
\blk00000001/sig000000a5 , \blk00000001/sig000000a6 , \blk00000001/sig000000a7 , \blk00000001/sig000000a8 , \blk00000001/sig000000a9 , 
\blk00000001/sig000000aa , \blk00000001/sig000000ab , \blk00000001/sig000000ac , \blk00000001/sig000000ad , \blk00000001/sig000000ae , 
\blk00000001/sig000000af , \blk00000001/sig000000b0 , \blk00000001/sig000000b1 }),
    .A({\blk00000001/sig000000b3 , \blk00000001/sig000000b3 , \blk00000001/sig000000b3 , a[31], a[30], a[29], a[28], a[27], a[26], a[25], a[24], a[23]
, a[22], a[21], a[20], a[19], a[18], a[17]}),
    .M({\NLW_blk00000001/blk00000004_M<35>_UNCONNECTED , \NLW_blk00000001/blk00000004_M<34>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_M<33>_UNCONNECTED , \NLW_blk00000001/blk00000004_M<32>_UNCONNECTED , \NLW_blk00000001/blk00000004_M<31>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_M<30>_UNCONNECTED , \NLW_blk00000001/blk00000004_M<29>_UNCONNECTED , \NLW_blk00000001/blk00000004_M<28>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_M<27>_UNCONNECTED , \NLW_blk00000001/blk00000004_M<26>_UNCONNECTED , \NLW_blk00000001/blk00000004_M<25>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_M<24>_UNCONNECTED , \NLW_blk00000001/blk00000004_M<23>_UNCONNECTED , \NLW_blk00000001/blk00000004_M<22>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_M<21>_UNCONNECTED , \NLW_blk00000001/blk00000004_M<20>_UNCONNECTED , \NLW_blk00000001/blk00000004_M<19>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_M<18>_UNCONNECTED , \NLW_blk00000001/blk00000004_M<17>_UNCONNECTED , \NLW_blk00000001/blk00000004_M<16>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_M<15>_UNCONNECTED , \NLW_blk00000001/blk00000004_M<14>_UNCONNECTED , \NLW_blk00000001/blk00000004_M<13>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_M<12>_UNCONNECTED , \NLW_blk00000001/blk00000004_M<11>_UNCONNECTED , \NLW_blk00000001/blk00000004_M<10>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_M<9>_UNCONNECTED , \NLW_blk00000001/blk00000004_M<8>_UNCONNECTED , \NLW_blk00000001/blk00000004_M<7>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_M<6>_UNCONNECTED , \NLW_blk00000001/blk00000004_M<5>_UNCONNECTED , \NLW_blk00000001/blk00000004_M<4>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_M<3>_UNCONNECTED , \NLW_blk00000001/blk00000004_M<2>_UNCONNECTED , \NLW_blk00000001/blk00000004_M<1>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_M<0>_UNCONNECTED })
  );
  GND   \blk00000001/blk00000003  (
    .G(\blk00000001/sig000000b3 )
  );
  VCC   \blk00000001/blk00000002  (
    .P(\blk00000001/sig000000b2 )
  );

endmodule
