////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1995-2013 Xilinx, Inc.  All rights reserved.
////////////////////////////////////////////////////////////////////////////////
//   ____  ____
//  /   /\/   /
// /___/  \  /    Vendor: Xilinx
// \   \   \/     Version: P.20131013
//  \   \         Application: netgen
//  /   /         Filename: subtractor_s6.v
// /___/   /\     Timestamp: Fri Jul 10 17:50:53 2015
// \   \  /  \ 
//  \___\/\___\
//             
// Command	: -w -sim -ofmt verilog E:/FPGA/ModExpS6_Novena/src/modexps6/ipcore/tmp/_cg/subtractor_s6.ngc E:/FPGA/ModExpS6_Novena/src/modexps6/ipcore/tmp/_cg/subtractor_s6.v 
// Device	: 6slx45csg324-3
// Input file	: E:/FPGA/ModExpS6_Novena/src/modexps6/ipcore/tmp/_cg/subtractor_s6.ngc
// Output file	: E:/FPGA/ModExpS6_Novena/src/modexps6/ipcore/tmp/_cg/subtractor_s6.v
// # of Modules	: 1
// Design Name	: subtractor_s6
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

module subtractor_s6 (
  c_in, c_out, a, b, s
);
  input c_in;
  output c_out;
  input [31 : 0] a;
  input [31 : 0] b;
  output [31 : 0] s;
  
  wire \blk00000001/sig00000064 ;
  wire \blk00000001/sig00000063 ;
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
  wire \NLW_blk00000001/blk00000004_PCOUT<47>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<46>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<45>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<44>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<43>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<42>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<41>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<40>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<39>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<38>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<37>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<36>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<35>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<34>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<33>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<32>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<31>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<30>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<29>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<28>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<27>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<26>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<25>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<24>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<23>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<22>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<21>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<20>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<19>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<18>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<17>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<16>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<15>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<14>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<13>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<12>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<11>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<10>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<9>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<8>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<7>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<6>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<5>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<4>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<3>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<2>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<1>_UNCONNECTED ;
  wire \NLW_blk00000001/blk00000004_PCOUT<0>_UNCONNECTED ;
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
  DSP48A1 #(
    .A0REG ( 0 ),
    .A1REG ( 0 ),
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
  \blk00000001/blk00000004  (
    .CECARRYIN(\blk00000001/sig00000064 ),
    .RSTC(\blk00000001/sig00000064 ),
    .RSTCARRYIN(\blk00000001/sig00000064 ),
    .CED(\blk00000001/sig00000064 ),
    .RSTD(\blk00000001/sig00000064 ),
    .CEOPMODE(\blk00000001/sig00000064 ),
    .CEC(\blk00000001/sig00000064 ),
    .CARRYOUTF(\NLW_blk00000001/blk00000004_CARRYOUTF_UNCONNECTED ),
    .RSTOPMODE(\blk00000001/sig00000064 ),
    .RSTM(\blk00000001/sig00000064 ),
    .CLK(\blk00000001/sig00000064 ),
    .RSTB(\blk00000001/sig00000064 ),
    .CEM(\blk00000001/sig00000064 ),
    .CEB(\blk00000001/sig00000064 ),
    .CARRYIN(\blk00000001/sig00000064 ),
    .CEP(\blk00000001/sig00000064 ),
    .CEA(\blk00000001/sig00000064 ),
    .CARRYOUT(\NLW_blk00000001/blk00000004_CARRYOUT_UNCONNECTED ),
    .RSTA(\blk00000001/sig00000064 ),
    .RSTP(\blk00000001/sig00000064 ),
    .B({b[17], b[16], b[15], b[14], b[13], b[12], b[11], b[10], b[9], b[8], b[7], b[6], b[5], b[4], b[3], b[2], b[1], b[0]}),
    .BCOUT({\NLW_blk00000001/blk00000004_BCOUT<17>_UNCONNECTED , \NLW_blk00000001/blk00000004_BCOUT<16>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_BCOUT<15>_UNCONNECTED , \NLW_blk00000001/blk00000004_BCOUT<14>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_BCOUT<13>_UNCONNECTED , \NLW_blk00000001/blk00000004_BCOUT<12>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_BCOUT<11>_UNCONNECTED , \NLW_blk00000001/blk00000004_BCOUT<10>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_BCOUT<9>_UNCONNECTED , \NLW_blk00000001/blk00000004_BCOUT<8>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_BCOUT<7>_UNCONNECTED , \NLW_blk00000001/blk00000004_BCOUT<6>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_BCOUT<5>_UNCONNECTED , \NLW_blk00000001/blk00000004_BCOUT<4>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_BCOUT<3>_UNCONNECTED , \NLW_blk00000001/blk00000004_BCOUT<2>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_BCOUT<1>_UNCONNECTED , \NLW_blk00000001/blk00000004_BCOUT<0>_UNCONNECTED }),
    .PCIN({\blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , 
\blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , 
\blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , 
\blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , 
\blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , 
\blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , 
\blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , 
\blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , 
\blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , 
\blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 }),
    .C({\blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , 
\blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , 
\blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , 
\blk00000001/sig00000064 , a[31], a[30], a[29], a[28], a[27], a[26], a[25], a[24], a[23], a[22], a[21], a[20], a[19], a[18], a[17], a[16], a[15], 
a[14], a[13], a[12], a[11], a[10], a[9], a[8], a[7], a[6], a[5], a[4], a[3], a[2], a[1], a[0]}),
    .P({\NLW_blk00000001/blk00000004_P<47>_UNCONNECTED , \NLW_blk00000001/blk00000004_P<46>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_P<45>_UNCONNECTED , \NLW_blk00000001/blk00000004_P<44>_UNCONNECTED , \NLW_blk00000001/blk00000004_P<43>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_P<42>_UNCONNECTED , \NLW_blk00000001/blk00000004_P<41>_UNCONNECTED , \NLW_blk00000001/blk00000004_P<40>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_P<39>_UNCONNECTED , \NLW_blk00000001/blk00000004_P<38>_UNCONNECTED , \NLW_blk00000001/blk00000004_P<37>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_P<36>_UNCONNECTED , \NLW_blk00000001/blk00000004_P<35>_UNCONNECTED , \NLW_blk00000001/blk00000004_P<34>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_P<33>_UNCONNECTED , c_out, s[31], s[30], s[29], s[28], s[27], s[26], s[25], s[24], s[23], s[22], s[21], s[20], s[19], 
s[18], s[17], s[16], s[15], s[14], s[13], s[12], s[11], s[10], s[9], s[8], s[7], s[6], s[5], s[4], s[3], s[2], s[1], s[0]}),
    .OPMODE({\blk00000001/sig00000063 , \blk00000001/sig00000064 , c_in, \blk00000001/sig00000064 , \blk00000001/sig00000063 , 
\blk00000001/sig00000063 , \blk00000001/sig00000063 , \blk00000001/sig00000063 }),
    .D({\blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , 
\blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , 
\blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , 
\blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 }),
    .PCOUT({\NLW_blk00000001/blk00000004_PCOUT<47>_UNCONNECTED , \NLW_blk00000001/blk00000004_PCOUT<46>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_PCOUT<45>_UNCONNECTED , \NLW_blk00000001/blk00000004_PCOUT<44>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_PCOUT<43>_UNCONNECTED , \NLW_blk00000001/blk00000004_PCOUT<42>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_PCOUT<41>_UNCONNECTED , \NLW_blk00000001/blk00000004_PCOUT<40>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_PCOUT<39>_UNCONNECTED , \NLW_blk00000001/blk00000004_PCOUT<38>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_PCOUT<37>_UNCONNECTED , \NLW_blk00000001/blk00000004_PCOUT<36>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_PCOUT<35>_UNCONNECTED , \NLW_blk00000001/blk00000004_PCOUT<34>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_PCOUT<33>_UNCONNECTED , \NLW_blk00000001/blk00000004_PCOUT<32>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_PCOUT<31>_UNCONNECTED , \NLW_blk00000001/blk00000004_PCOUT<30>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_PCOUT<29>_UNCONNECTED , \NLW_blk00000001/blk00000004_PCOUT<28>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_PCOUT<27>_UNCONNECTED , \NLW_blk00000001/blk00000004_PCOUT<26>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_PCOUT<25>_UNCONNECTED , \NLW_blk00000001/blk00000004_PCOUT<24>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_PCOUT<23>_UNCONNECTED , \NLW_blk00000001/blk00000004_PCOUT<22>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_PCOUT<21>_UNCONNECTED , \NLW_blk00000001/blk00000004_PCOUT<20>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_PCOUT<19>_UNCONNECTED , \NLW_blk00000001/blk00000004_PCOUT<18>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_PCOUT<17>_UNCONNECTED , \NLW_blk00000001/blk00000004_PCOUT<16>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_PCOUT<15>_UNCONNECTED , \NLW_blk00000001/blk00000004_PCOUT<14>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_PCOUT<13>_UNCONNECTED , \NLW_blk00000001/blk00000004_PCOUT<12>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_PCOUT<11>_UNCONNECTED , \NLW_blk00000001/blk00000004_PCOUT<10>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_PCOUT<9>_UNCONNECTED , \NLW_blk00000001/blk00000004_PCOUT<8>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_PCOUT<7>_UNCONNECTED , \NLW_blk00000001/blk00000004_PCOUT<6>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_PCOUT<5>_UNCONNECTED , \NLW_blk00000001/blk00000004_PCOUT<4>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_PCOUT<3>_UNCONNECTED , \NLW_blk00000001/blk00000004_PCOUT<2>_UNCONNECTED , 
\NLW_blk00000001/blk00000004_PCOUT<1>_UNCONNECTED , \NLW_blk00000001/blk00000004_PCOUT<0>_UNCONNECTED }),
    .A({\blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , \blk00000001/sig00000064 , b[31], b[30], b[29], b[28], b[27]
, b[26], b[25], b[24], b[23], b[22], b[21], b[20], b[19], b[18]}),
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
    .G(\blk00000001/sig00000064 )
  );
  VCC   \blk00000001/blk00000002  (
    .P(\blk00000001/sig00000063 )
  );


endmodule