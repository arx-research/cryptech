`timescale 1ns / 1ps

module streebog_rom_s_table
	(
		clk, ena,
		din, dout
	);
	
	
		//
		// Ports
		//
	input		wire				clk;
	input		wire				ena;
	input		wire	[ 7: 0]	din;
	output	wire	[ 7: 0]	dout;
	
	
		//
		// Output Register
		//
	reg	[ 7: 0]	dout_reg;
	assign dout = dout_reg;
	
	
		//
		// S Transformation Lookup Table
		//
	always @(posedge clk) begin
		//
		if (ena) begin
			//
			case (din)
				//
				8'h00: dout_reg <= 8'hFC;
				8'h01: dout_reg <= 8'hEE;
				8'h02: dout_reg <= 8'hDD;
				8'h03: dout_reg <= 8'h11;
				8'h04: dout_reg <= 8'hCF;
				8'h05: dout_reg <= 8'h6E;
				8'h06: dout_reg <= 8'h31;
				8'h07: dout_reg <= 8'h16;
				8'h08: dout_reg <= 8'hFB;
				8'h09: dout_reg <= 8'hC4;
				8'h0A: dout_reg <= 8'hFA;
				8'h0B: dout_reg <= 8'hDA;
				8'h0C: dout_reg <= 8'h23;
				8'h0D: dout_reg <= 8'hC5;
				8'h0E: dout_reg <= 8'h04;
				8'h0F: dout_reg <= 8'h4D;
				8'h10: dout_reg <= 8'hE9;
				8'h11: dout_reg <= 8'h77;
				8'h12: dout_reg <= 8'hF0;
				8'h13: dout_reg <= 8'hDB;
				8'h14: dout_reg <= 8'h93;
				8'h15: dout_reg <= 8'h2E;
				8'h16: dout_reg <= 8'h99;
				8'h17: dout_reg <= 8'hBA;
				8'h18: dout_reg <= 8'h17;
				8'h19: dout_reg <= 8'h36;
				8'h1A: dout_reg <= 8'hF1;
				8'h1B: dout_reg <= 8'hBB;
				8'h1C: dout_reg <= 8'h14;
				8'h1D: dout_reg <= 8'hCD;
				8'h1E: dout_reg <= 8'h5F;
				8'h1F: dout_reg <= 8'hC1;
				8'h20: dout_reg <= 8'hF9;
				8'h21: dout_reg <= 8'h18;
				8'h22: dout_reg <= 8'h65;
				8'h23: dout_reg <= 8'h5A;
				8'h24: dout_reg <= 8'hE2;
				8'h25: dout_reg <= 8'h5C;
				8'h26: dout_reg <= 8'hEF;
				8'h27: dout_reg <= 8'h21;
				8'h28: dout_reg <= 8'h81;
				8'h29: dout_reg <= 8'h1C;
				8'h2A: dout_reg <= 8'h3C;
				8'h2B: dout_reg <= 8'h42;
				8'h2C: dout_reg <= 8'h8B;
				8'h2D: dout_reg <= 8'h01;
				8'h2E: dout_reg <= 8'h8E;
				8'h2F: dout_reg <= 8'h4F;
				8'h30: dout_reg <= 8'h05;
				8'h31: dout_reg <= 8'h84;
				8'h32: dout_reg <= 8'h02;
				8'h33: dout_reg <= 8'hAE;
				8'h34: dout_reg <= 8'hE3;
				8'h35: dout_reg <= 8'h6A;
				8'h36: dout_reg <= 8'h8F;
				8'h37: dout_reg <= 8'hA0;
				8'h38: dout_reg <= 8'h06;
				8'h39: dout_reg <= 8'h0B;
				8'h3A: dout_reg <= 8'hED;
				8'h3B: dout_reg <= 8'h98;
				8'h3C: dout_reg <= 8'h7F;
				8'h3D: dout_reg <= 8'hD4;
				8'h3E: dout_reg <= 8'hD3;
				8'h3F: dout_reg <= 8'h1F;
				8'h40: dout_reg <= 8'hEB;
				8'h41: dout_reg <= 8'h34;
				8'h42: dout_reg <= 8'h2C;
				8'h43: dout_reg <= 8'h51;
				8'h44: dout_reg <= 8'hEA;
				8'h45: dout_reg <= 8'hC8;
				8'h46: dout_reg <= 8'h48;
				8'h47: dout_reg <= 8'hAB;
				8'h48: dout_reg <= 8'hF2;
				8'h49: dout_reg <= 8'h2A;
				8'h4A: dout_reg <= 8'h68;
				8'h4B: dout_reg <= 8'hA2;
				8'h4C: dout_reg <= 8'hFD;
				8'h4D: dout_reg <= 8'h3A;
				8'h4E: dout_reg <= 8'hCE;
				8'h4F: dout_reg <= 8'hCC;
				8'h50: dout_reg <= 8'hB5;
				8'h51: dout_reg <= 8'h70;
				8'h52: dout_reg <= 8'h0E;
				8'h53: dout_reg <= 8'h56;
				8'h54: dout_reg <= 8'h08;
				8'h55: dout_reg <= 8'h0C;
				8'h56: dout_reg <= 8'h76;
				8'h57: dout_reg <= 8'h12;
				8'h58: dout_reg <= 8'hBF;
				8'h59: dout_reg <= 8'h72;
				8'h5A: dout_reg <= 8'h13;
				8'h5B: dout_reg <= 8'h47;
				8'h5C: dout_reg <= 8'h9C;
				8'h5D: dout_reg <= 8'hB7;
				8'h5E: dout_reg <= 8'h5D;
				8'h5F: dout_reg <= 8'h87;
				8'h60: dout_reg <= 8'h15;
				8'h61: dout_reg <= 8'hA1;
				8'h62: dout_reg <= 8'h96;
				8'h63: dout_reg <= 8'h29;
				8'h64: dout_reg <= 8'h10;
				8'h65: dout_reg <= 8'h7B;
				8'h66: dout_reg <= 8'h9A;
				8'h67: dout_reg <= 8'hC7;
				8'h68: dout_reg <= 8'hF3;
				8'h69: dout_reg <= 8'h91;
				8'h6A: dout_reg <= 8'h78;
				8'h6B: dout_reg <= 8'h6F;
				8'h6C: dout_reg <= 8'h9D;
				8'h6D: dout_reg <= 8'h9E;
				8'h6E: dout_reg <= 8'hB2;
				8'h6F: dout_reg <= 8'hB1;
				8'h70: dout_reg <= 8'h32;
				8'h71: dout_reg <= 8'h75;
				8'h72: dout_reg <= 8'h19;
				8'h73: dout_reg <= 8'h3D;
				8'h74: dout_reg <= 8'hFF;
				8'h75: dout_reg <= 8'h35;
				8'h76: dout_reg <= 8'h8A;
				8'h77: dout_reg <= 8'h7E;
				8'h78: dout_reg <= 8'h6D;
				8'h79: dout_reg <= 8'h54;
				8'h7A: dout_reg <= 8'hC6;
				8'h7B: dout_reg <= 8'h80;
				8'h7C: dout_reg <= 8'hC3;
				8'h7D: dout_reg <= 8'hBD;
				8'h7E: dout_reg <= 8'h0D;
				8'h7F: dout_reg <= 8'h57;
				8'h80: dout_reg <= 8'hDF;
				8'h81: dout_reg <= 8'hF5;
				8'h82: dout_reg <= 8'h24;
				8'h83: dout_reg <= 8'hA9;
				8'h84: dout_reg <= 8'h3E;
				8'h85: dout_reg <= 8'hA8;
				8'h86: dout_reg <= 8'h43;
				8'h87: dout_reg <= 8'hC9;
				8'h88: dout_reg <= 8'hD7;
				8'h89: dout_reg <= 8'h79;
				8'h8A: dout_reg <= 8'hD6;
				8'h8B: dout_reg <= 8'hF6;
				8'h8C: dout_reg <= 8'h7C;
				8'h8D: dout_reg <= 8'h22;
				8'h8E: dout_reg <= 8'hB9;
				8'h8F: dout_reg <= 8'h03;
				8'h90: dout_reg <= 8'hE0;
				8'h91: dout_reg <= 8'h0F;
				8'h92: dout_reg <= 8'hEC;
				8'h93: dout_reg <= 8'hDE;
				8'h94: dout_reg <= 8'h7A;
				8'h95: dout_reg <= 8'h94;
				8'h96: dout_reg <= 8'hB0;
				8'h97: dout_reg <= 8'hBC;
				8'h98: dout_reg <= 8'hDC;
				8'h99: dout_reg <= 8'hE8;
				8'h9A: dout_reg <= 8'h28;
				8'h9B: dout_reg <= 8'h50;
				8'h9C: dout_reg <= 8'h4E;
				8'h9D: dout_reg <= 8'h33;
				8'h9E: dout_reg <= 8'h0A;
				8'h9F: dout_reg <= 8'h4A;
				8'hA0: dout_reg <= 8'hA7;
				8'hA1: dout_reg <= 8'h97;
				8'hA2: dout_reg <= 8'h60;
				8'hA3: dout_reg <= 8'h73;
				8'hA4: dout_reg <= 8'h1E;
				8'hA5: dout_reg <= 8'h00;
				8'hA6: dout_reg <= 8'h62;
				8'hA7: dout_reg <= 8'h44;
				8'hA8: dout_reg <= 8'h1A;
				8'hA9: dout_reg <= 8'hB8;
				8'hAA: dout_reg <= 8'h38;
				8'hAB: dout_reg <= 8'h82;
				8'hAC: dout_reg <= 8'h64;
				8'hAD: dout_reg <= 8'h9F;
				8'hAE: dout_reg <= 8'h26;
				8'hAF: dout_reg <= 8'h41;
				8'hB0: dout_reg <= 8'hAD;
				8'hB1: dout_reg <= 8'h45;
				8'hB2: dout_reg <= 8'h46;
				8'hB3: dout_reg <= 8'h92;
				8'hB4: dout_reg <= 8'h27;
				8'hB5: dout_reg <= 8'h5E;
				8'hB6: dout_reg <= 8'h55;
				8'hB7: dout_reg <= 8'h2F;
				8'hB8: dout_reg <= 8'h8C;
				8'hB9: dout_reg <= 8'hA3;
				8'hBA: dout_reg <= 8'hA5;
				8'hBB: dout_reg <= 8'h7D;
				8'hBC: dout_reg <= 8'h69;
				8'hBD: dout_reg <= 8'hD5;
				8'hBE: dout_reg <= 8'h95;
				8'hBF: dout_reg <= 8'h3B;
				8'hC0: dout_reg <= 8'h07;
				8'hC1: dout_reg <= 8'h58;
				8'hC2: dout_reg <= 8'hB3;
				8'hC3: dout_reg <= 8'h40;
				8'hC4: dout_reg <= 8'h86;
				8'hC5: dout_reg <= 8'hAC;
				8'hC6: dout_reg <= 8'h1D;
				8'hC7: dout_reg <= 8'hF7;
				8'hC8: dout_reg <= 8'h30;
				8'hC9: dout_reg <= 8'h37;
				8'hCA: dout_reg <= 8'h6B;
				8'hCB: dout_reg <= 8'hE4;
				8'hCC: dout_reg <= 8'h88;
				8'hCD: dout_reg <= 8'hD9;
				8'hCE: dout_reg <= 8'hE7;
				8'hCF: dout_reg <= 8'h89;
				8'hD0: dout_reg <= 8'hE1;
				8'hD1: dout_reg <= 8'h1B;
				8'hD2: dout_reg <= 8'h83;
				8'hD3: dout_reg <= 8'h49;
				8'hD4: dout_reg <= 8'h4C;
				8'hD5: dout_reg <= 8'h3F;
				8'hD6: dout_reg <= 8'hF8;
				8'hD7: dout_reg <= 8'hFE;
				8'hD8: dout_reg <= 8'h8D;
				8'hD9: dout_reg <= 8'h53;
				8'hDA: dout_reg <= 8'hAA;
				8'hDB: dout_reg <= 8'h90;
				8'hDC: dout_reg <= 8'hCA;
				8'hDD: dout_reg <= 8'hD8;
				8'hDE: dout_reg <= 8'h85;
				8'hDF: dout_reg <= 8'h61;
				8'hE0: dout_reg <= 8'h20;
				8'hE1: dout_reg <= 8'h71;
				8'hE2: dout_reg <= 8'h67;
				8'hE3: dout_reg <= 8'hA4;
				8'hE4: dout_reg <= 8'h2D;
				8'hE5: dout_reg <= 8'h2B;
				8'hE6: dout_reg <= 8'h09;
				8'hE7: dout_reg <= 8'h5B;
				8'hE8: dout_reg <= 8'hCB;
				8'hE9: dout_reg <= 8'h9B;
				8'hEA: dout_reg <= 8'h25;
				8'hEB: dout_reg <= 8'hD0;
				8'hEC: dout_reg <= 8'hBE;
				8'hED: dout_reg <= 8'hE5;
				8'hEE: dout_reg <= 8'h6C;
				8'hEF: dout_reg <= 8'h52;
				8'hF0: dout_reg <= 8'h59;
				8'hF1: dout_reg <= 8'hA6;
				8'hF2: dout_reg <= 8'h74;
				8'hF3: dout_reg <= 8'hD2;
				8'hF4: dout_reg <= 8'hE6;
				8'hF5: dout_reg <= 8'hF4;
				8'hF6: dout_reg <= 8'hB4;
				8'hF7: dout_reg <= 8'hC0;
				8'hF8: dout_reg <= 8'hD1;
				8'hF9: dout_reg <= 8'h66;
				8'hFA: dout_reg <= 8'hAF;
				8'hFB: dout_reg <= 8'hC2;
				8'hFC: dout_reg <= 8'h39;
				8'hFD: dout_reg <= 8'h4B;
				8'hFE: dout_reg <= 8'h63;
				8'hFF: dout_reg <= 8'hB6;
				//
			endcase // case (din)
			//
		end // if (ena)
		//
	end // always @(posedge clk)
	
	
endmodule
