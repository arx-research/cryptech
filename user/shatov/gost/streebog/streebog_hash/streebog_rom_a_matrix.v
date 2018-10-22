`timescale 1ns / 1ps

module streebog_rom_a_matrix
	(
		clk,
		din, dout
	);
	
	
		//
		// Ports
		//
	input		wire				clk;
	input		wire	[ 5: 0]	din;
	output	wire	[63: 0]	dout;
	
	
		//
		// Output Register
		//
	reg	[63: 0]	dout_reg;
	assign dout = dout_reg;
	
	
		//
		// A Transformation Matrix
		//
		
		/*
		 * Original matrix from the standard was transformed to allow efficient implementation of
		 * hardware multiplication. The following matrix is effectively the transposed version
		 * of the original matrix A with reversed row order.
		 *
		 * Original 64x64 bit matrix from the standard has the following form:
		 *
		 * a[i,j] is 1-bit matrix element
		 *
		 * A_row(i) is 64-bit row of matrix
		 * A_col(j) is 64-bit column of matrix
		 *
		 *
		 *    A_col(0)  A_col(1)       A_col(62) A_col(63)
		 *       |         |              |         |
		 *       |         |              |         |
		 * +----------------------------------------------+
		 * | a[ 0,63]  a[ 0,62]  ...  a[ 0, 1]  a[ 0, 0]  | --A_row(0)
		 * | a[ 1,63]  a[ 1,62]  ...  a[ 1, 1]  a[ 1, 0]  | --A_row(1)
		 * |                      ...                     |
       * | a[62,63]  a[62,62]  ...  a[62, 1]  a[62, 0]  | --A_row(62)
		 * | a[63,63]  a[63,62]  ...  a[63, 1]  a[63, 0]  | --A_row(63)
		 * +----------------------------------------------+
		 *
		 *
		 * A_row(0)...A_row(63) are given in the original specification. Instead of row vectors we need a set of
		 * column vectors A_col(0)...A_col(63). A_col() can be obtained by transposing A_row().
		 *
		 *
		 *    A_row(0)  A_row(1)       A_row(62) A_row(63)
		 *       |         |              |         |
		 *       |         |              |         |
		 * +---------------------------------------------+
		 * | a[ 0,63]  a[ 1,63]  ...  a[62,63]  a[63,63] | --A_col(0)
		 * | a[ 0,62]  a[ 1,62]  ...  a[62,62]  a[63,62] | --A_col(1)
		 * |                     ...                     |
		 * | a[ 0, 1]  a[ 1, 1]  ...  a[62, 1]  a[63, 1] | --A_col(62)
		 * | a[ 0, 0]  a[ 1, 0]  ...  a[62, 0]  a[63, 0] | --A_col(63)
		 * +---------------------------------------------+
		 *
		 *
		 * The only problem with A_col() is that original 64-bit A_row() values in the standard are written from MSB to LSB. That implies that
		 * original matrix columns are numbered from 63 to 0, while matrix rows are numbered from 0 to 63. Because of that we need to reverse
		 * row order after transposition. Original matrix had element a[0,0] in A_row(0), but after transposition element a[0,0] turns out
		 * to be in A_col(63), not in A_col(0). Because of that addresses inside of case() below are reversed. This effectively reverses
		 * the order in which A_col() follow.
		 *
		 */
		
	always @(posedge clk) begin
		//
		case (din)
			//
			6'h3F: dout_reg <= 64'hB18285C0BA4F9506;
			6'h3E: dout_reg <= 64'h584142605DA7CA83;
			6'h3D: dout_reg <= 64'h2CA021302E53E5C1;
			6'h3C: dout_reg <= 64'h16509098172972E0;
			6'h3B: dout_reg <= 64'hBA2A4D8C315B2C76;
			6'h3A: dout_reg <= 64'hEC172386A2E2833D;
			6'h39: dout_reg <= 64'hC7091403EB3E5418;
			6'h38: dout_reg <= 64'h63040A81759F2A0C;
			6'h37: dout_reg <= 64'h025DA344601EA1B8;
			6'h36: dout_reg <= 64'h012ED1A2308FD05C;
			6'h35: dout_reg <= 64'h8017685198C7E8AE;
			6'h34: dout_reg <= 64'h408BB4284C63F457;
			6'h33: dout_reg <= 64'h2218F9D046AFDB13;
			6'h32: dout_reg <= 64'h13515FACC3C94CB1;
			6'h31: dout_reg <= 64'h0B758C12817A87E0;
			6'h30: dout_reg <= 64'h05BA4689C03D4370;
			6'h2F: dout_reg <= 64'hA1F0C986411102CC;
			6'h2E: dout_reg <= 64'hD0F864C3A0080166;
			6'h2D: dout_reg <= 64'hE87CB2E1508480B3;
			6'h2C: dout_reg <= 64'hF4BED9F0A8C24059;
			6'h2B: dout_reg <= 64'hDB2F257E95702260;
			6'h2A: dout_reg <= 64'h4C67DB398BA913FC;
			6'h29: dout_reg <= 64'h87C3241A04450B32;
			6'h28: dout_reg <= 64'h43E1920D82220599;
			6'h27: dout_reg <= 64'hE0802541868B1232;
			6'h26: dout_reg <= 64'h704012A0C3458999;
			6'h25: dout_reg <= 64'hB8208950E12244CC;
			6'h24: dout_reg <= 64'h5C1044A8F011A266;
			6'h23: dout_reg <= 64'h4E0887957E834381;
			6'h22: dout_reg <= 64'hC704668B394AB3F2;
			6'h21: dout_reg <= 64'h830296041A2E4BCB;
			6'h20: dout_reg <= 64'hC1014B820D172565;
			6'h1F: dout_reg <= 64'h7DD80C6D98218914;
			6'h1E: dout_reg <= 64'h3E6C06B64C90440A;
			6'h1D: dout_reg <= 64'h9F36835B26C8A285;
			6'h1C: dout_reg <= 64'h4F1BC1AD93E45142;
			6'h1B: dout_reg <= 64'hDA55ECBBD1D3A135;
			6'h1A: dout_reg <= 64'h10727AB0F048598E;
			6'h19: dout_reg <= 64'hF56131B560852553;
			6'h18: dout_reg <= 64'hFAB018DA30421229;
			6'h17: dout_reg <= 64'h82B12139880C7F01;
			6'h16: dout_reg <= 64'h4158909CC4063F80;
			6'h15: dout_reg <= 64'hA02CC8CEE2831F40;
			6'h14: dout_reg <= 64'h5016E46771C10F20;
			6'h13: dout_reg <= 64'h2ABAD30AB0ECF811;
			6'h12: dout_reg <= 64'h17EC48BC507A0309;
			6'h11: dout_reg <= 64'h09C785E72031FE05;
			6'h10: dout_reg <= 64'h046342731018FF02;
			6'h0F: dout_reg <= 64'h91E9E113A54E2B57;
			6'h0E: dout_reg <= 64'h4874F009522715AB;
			6'h0D: dout_reg <= 64'hA43AF804A9138A55;
			6'h0C: dout_reg <= 64'hD21D7C825409C5AA;
			6'h0B: dout_reg <= 64'h78E75F528F4A4982;
			6'h0A: dout_reg <= 64'hAD9ACEBA62EB0F16;
			6'h09: dout_reg <= 64'h47A4864E943BAC5C;
			6'h08: dout_reg <= 64'h23D2C3274A9D56AE;
			6'h07: dout_reg <= 64'h06016A5C89D498B1;
			6'h06: dout_reg <= 64'h8380B5AE446A4C58;
			6'h05: dout_reg <= 64'hC140DA57A2B5262C;
			6'h04: dout_reg <= 64'hE0206DAB51DA9316;
			6'h03: dout_reg <= 64'h7611DC09A1B9D1BA;
			6'h02: dout_reg <= 64'h3D0984585908F0EC;
			6'h01: dout_reg <= 64'h1805A870255060C7;
			6'h00: dout_reg <= 64'h0C02D4B812A83063;
			//
		endcase // case(din)
		//
	end // always @(posedge clk)
	
	
endmodule
