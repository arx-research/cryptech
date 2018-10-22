// dummy modules for Xilinx IP for verilator linting

// The module definitions are ganked from
// /opt/Xilinx/14.7/ISE_DS/ISE/verilog/src/unisims. It would be easier
// to run verilator with -I for that directory, but verilator really
// doesn't like the Xilinx code.

// Copyright (c) 1995/2004 Xilinx, Inc.
// All Right Reserved.

/*verilator lint_off UNDRIVEN*/
/*verilator lint_off UNUSED*/

module DCM_SP (
	CLK0, CLK180, CLK270, CLK2X, CLK2X180, CLK90,
	CLKDV, CLKFX, CLKFX180, LOCKED, PSDONE, STATUS,
	CLKFB, CLKIN, DSSEN, PSCLK, PSEN, PSINCDEC, RST);
parameter real CLKDV_DIVIDE = 2.0;
parameter integer CLKFX_DIVIDE = 1;
parameter integer CLKFX_MULTIPLY = 4;
parameter CLKIN_DIVIDE_BY_2 = "FALSE";
parameter real CLKIN_PERIOD = 10.0;
parameter CLKOUT_PHASE_SHIFT = "NONE";
parameter CLK_FEEDBACK = "1X";
parameter DESKEW_ADJUST = "SYSTEM_SYNCHRONOUS";
parameter DFS_FREQUENCY_MODE = "LOW";
parameter DLL_FREQUENCY_MODE = "LOW";
parameter DSS_MODE = "NONE";
parameter DUTY_CYCLE_CORRECTION = "TRUE";
parameter FACTORY_JF = 16'hC080;
parameter integer PHASE_SHIFT = 0;
parameter STARTUP_WAIT = "FALSE";
output CLK0, CLK180, CLK270, CLK2X, CLK2X180, CLK90;
output CLKDV, CLKFX, CLKFX180, LOCKED, PSDONE;
output [7:0] STATUS;
input CLKFB, CLKIN, DSSEN;
input PSCLK, PSEN, PSINCDEC, RST;
endmodule

module BUFG (O, I);
    output O;
    input  I;
endmodule

module IBUFGDS (O, I, IB);
    output O;
    input  I, IB;
endmodule

module IOBUF (O, IO, I, T);
    parameter CAPACITANCE = "DONT_CARE";
    parameter integer DRIVE = 12;
    parameter IBUF_DELAY_VALUE = "0";
    parameter IBUF_LOW_PWR = "TRUE";
    parameter IFD_DELAY_VALUE = "AUTO";
    parameter IOSTANDARD = "DEFAULT";
    parameter SLEW = "SLOW";
    output O;
    inout  IO;
    input  I, T;
endmodule

module FDCE (Q, C, CE, CLR, D);
    parameter INIT = 1'b0;
    output Q;
    input  C, CE, CLR, D;
endmodule

module FD (Q, C, D);
    parameter INIT = 1'b0;
    output Q;
    input  C, D;
endmodule

module DSP48A1 (BCOUT, CARRYOUT, CARRYOUTF, M, P, PCOUT, A, B, C, CARRYIN, CEA, CEB, CEC, CECARRYIN, CED, CEM, CEOPMODE, CEP, CLK, D, OPMODE, PCIN, RSTA, RSTB, RSTC, RSTCARRYIN, RSTD, RSTM, RSTOPMODE, RSTP);
    parameter integer A0REG = 0;
    parameter integer A1REG = 1;
    parameter integer B0REG = 0;
    parameter integer B1REG = 1;
    parameter integer CARRYINREG = 1;
    parameter integer CARRYOUTREG = 1;
    parameter CARRYINSEL = "OPMODE5";
    parameter integer CREG = 1;
    parameter integer DREG = 1;
    parameter integer MREG = 1;
    parameter integer OPMODEREG = 1;
    parameter integer PREG = 1;
    parameter RSTTYPE = "SYNC";
    output [17:0] BCOUT;
    output CARRYOUT;
    output CARRYOUTF;
    output [35:0] M;
    output [47:0] P;
    output [47:0] PCOUT;
    input [17:0] A;
    input [17:0] B;
    input [47:0] C;
    input CARRYIN;
    input CEA;
    input CEB;
    input CEC;
    input CECARRYIN;
    input CED;
    input CEM;
    input CEOPMODE;
    input CEP;
    input CLK;
    input [17:0] D;
    input [7:0] OPMODE;
    input [47:0] PCIN;
    input RSTA;
    input RSTB;
    input RSTC;
    input RSTCARRYIN;
    input RSTD;
    input RSTM;
    input RSTOPMODE;
    input RSTP;
endmodule

module GND(G);
    output G;
endmodule

module VCC(P);
    output P;
endmodule

