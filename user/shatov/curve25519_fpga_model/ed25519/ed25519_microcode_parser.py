#
# ed25519_microcode_parser.py
#
# Author: Pavel Shatov
#
# Copyright (c) 2018, NORDUnet A/S
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# * Neither the name of the NORDUnet A/S nor the
#   names of its contributors may be used to endorse or promote products
#   derived from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Imports
import re
import sys
from enum import Enum

# Source File
C_FILES = [ "ed25519_fpga_curve_microcode.cpp",
            "../curve25519/curve25519_fpga_microcode.cpp"]

class MICROCODE_PARSER:

    # enumerate microcode pieces
    class MICROCODE_PIECE_ENUM(Enum):
        NONE                = -1
        PREPARE             =  0
        BEFORE_ROUND_K0     =  1
        BEFORE_ROUND_K1     =  2
        DURING_ROUND        =  3
        AFTER_ROUND_K0      =  4
        AFTER_ROUND_K1      =  5
        BEFORE_INVERSION    =  6
        DURING_INVERSION    =  7
        AFTER_INVERSION     =  8
        FINAL_REDUCTION     =  9
        HANDLE_SIGN         = 10
        OUTPUT              = 11

    # magic pair of begin/end markers
    MARKER_BEGIN_MICROCODE  = "BEGIN_MICROCODE:"
    MARKER_END_MICROCODE    = "END_MICROCODE"

    # names of banks
    MICROCODE_C_NAME_BANK_LO    = "BANK_LO"
    MICROCODE_C_NAME_BANK_HI    = "BANK_HI"

    # micro-operation names
    MICROCODE_C_NAME_UOP_MOVE       = "uop_move"
    MICROCODE_C_NAME_UOP_CALC       = "uop_calc"
    MICROCODE_C_NAME_UOP_CYCLE      = "uop_cycle"
    MICROCODE_C_NAME_UOP_REPEAT     = "uop_repeat"
    MICROCODE_C_NAME_UOP_CALC_EVEN  = "uop_calc_if_even"
    MICROCODE_C_NAME_UOP_CALC_ODD   = "uop_calc_if_odd"

    # calculate micro-operations
    MICROCODE_C_NAME_UOP_MATH_MUL   = "MUL"
    MICROCODE_C_NAME_UOP_MATH_ADD   = "ADD"
    MICROCODE_C_NAME_UOP_MATH_SUB   = "SUB"
    
    # names of banks in C source
    MICROCODE_C_NAMES_BANKS = [MICROCODE_C_NAME_BANK_LO, MICROCODE_C_NAME_BANK_HI]

    # names of operands in C source
    MICROCODE_C_NAMES_OPERANDS = [  "CONST_ZERO",   "CONST_ONE",
                                    "INVERT_R1",    "INVERT_R2",
                                    "INVERT_T_1",   "INVERT_T_10",  "INVERT_T_1001", "INVERT_T_1011",
                                    "INVERT_T_X5",  "INVERT_T_X10", "INVERT_T_X20",  "INVERT_T_X40",
                                    "INVERT_T_X50", "INVERT_T_X100",
                                    "CONST_G_X",    "CONST_G_Y",                     "CONST_G_T",
                                    "CYCLE_R0_X",   "CYCLE_R0_Y",    "CYCLE_R0_Z",   "CYCLE_R0_T",
                                    "CYCLE_R1_X",   "CYCLE_R1_Y",    "CYCLE_R1_Z",   "CYCLE_R1_T",    
                                    "CYCLE_S_X",    "CYCLE_S_Y",     "CYCLE_S_Z",    "CYCLE_S_T",
                                    "CYCLE_T_X",    "CYCLE_T_Y",     "CYCLE_T_Z",    "CYCLE_T_T",
                                    "CYCLE_U_X",    "CYCLE_U_Y",     "CYCLE_U_Z",    "CYCLE_U_T",
                                    "CYCLE_V_X",    "CYCLE_V_Y",     "CYCLE_V_Z",    "CYCLE_V_T",
                                    "PROC_A",       "PROC_B",        "PROC_C",       "PROC_D",
                                    "PROC_E",       "PROC_F",        "PROC_G",       "PROC_H",
                                    "PROC_I",       "PROC_J"]

    # names of banks in Verilog source
    MICROCODE_V_NAME_BANKS_DUMMY = "UOP_BANKS_DUMMY"
    
    MICROCODE_V_NAMES_BANKS = ["UOP_BANKS_LO2HI", "UOP_BANKS_HI2LO"]

    # names of operands in Verilog source
    MICROCODE_V_NAMES_OPERANDS = ["UOP_OPERAND_" + op for op in MICROCODE_C_NAMES_OPERANDS]

    # names of opcodes in Verilog source
    MICROCODE_V_NAME_OPCODE_MOVE        = "UOP_OPCODE_COPY"
    MICROCODE_V_NAME_OPCODE_MUL         = "UOP_OPCODE_MUL"
    MICROCODE_V_NAME_OPCODE_ADD         = "UOP_OPCODE_ADD"
    MICROCODE_V_NAME_OPCODE_SUB         = "UOP_OPCODE_SUB"
    MICROCODE_V_NAME_OPCODE_STOP        = "UOP_OPCODE_STOP"
    
    MICROCODE_V_NAME_OPERAND_DONTCARE   = "UOP_OPERAND_DONTCARE"

    # match group to catch operand names
    MATCH_GROUP_09AZ        = "([0-9A-Z_]+)"
    
    # match group to catch bank names
    MATCH_GROUP_BANK        = "(" + MICROCODE_C_NAME_BANK_LO + "|" + MICROCODE_C_NAME_BANK_HI + ")"

    # match group to catch number of loop iterations
    MATCH_GROUP_NUMBER      = "(\d+)"
    
    # match group to catch math instruction
    MATCH_GROUP_MATH        = "(" + MICROCODE_C_NAME_UOP_MATH_MUL + "|" + MICROCODE_C_NAME_UOP_MATH_ADD + "|" + MICROCODE_C_NAME_UOP_MATH_SUB + ")"
    
    # match groups to catch dummy C pointers
    MATCH_GROUP_DUMMY_C_PTR = "BUF_LO\,\s*BUF_HI"
    MATCH_GROUP_DUMMY_C_MOD = "MOD_[1-2]P"
    
    # match group to catch calculation micro-operation
    MATCH_GROUP_CALC        = "(" + MICROCODE_C_NAME_UOP_CALC + "|" + MICROCODE_C_NAME_UOP_CALC_EVEN + "|" + MICROCODE_C_NAME_UOP_CALC_ODD + ")"
    
    # map string microcode piece names to enum values
    MICROCODE_PIECE_DICT    = {     "PREPARE":          MICROCODE_PIECE_ENUM.PREPARE,
                                    "BEFORE_ROUND_K0":  MICROCODE_PIECE_ENUM.BEFORE_ROUND_K0,
                                    "BEFORE_ROUND_K1":  MICROCODE_PIECE_ENUM.BEFORE_ROUND_K1,
                                    "DURING_ROUND":     MICROCODE_PIECE_ENUM.DURING_ROUND,
                                    "AFTER_ROUND_K0":   MICROCODE_PIECE_ENUM.AFTER_ROUND_K0,
                                    "AFTER_ROUND_K1":   MICROCODE_PIECE_ENUM.AFTER_ROUND_K1,
                                    "BEFORE_INVERSION": MICROCODE_PIECE_ENUM.BEFORE_INVERSION,
                                    "DURING_INVERSION": MICROCODE_PIECE_ENUM.DURING_INVERSION,
                                    "AFTER_INVERSION":  MICROCODE_PIECE_ENUM.AFTER_INVERSION,
                                    "FINAL_REDUCTION":  MICROCODE_PIECE_ENUM.FINAL_REDUCTION,
                                    "HANDLE_SIGN":      MICROCODE_PIECE_ENUM.HANDLE_SIGN,
                                    "OUTPUT":           MICROCODE_PIECE_ENUM.OUTPUT}
                                    
    # map C bank names to Verilog bank names
    MICROCODE_BANK_DICT     = dict(zip(MICROCODE_C_NAMES_BANKS,    MICROCODE_V_NAMES_BANKS))

    # map C operand names to Verilog operand names
    MICROCODE_OPERAND_DICT  = dict(zip(MICROCODE_C_NAMES_OPERANDS, MICROCODE_V_NAMES_OPERANDS))

    # map C calculation names to Verilog opcode names
    MICROCODE_MATH_DICT = { MICROCODE_C_NAME_UOP_MATH_MUL:  MICROCODE_V_NAME_OPCODE_MUL,
                            MICROCODE_C_NAME_UOP_MATH_ADD:  MICROCODE_V_NAME_OPCODE_ADD,
                            MICROCODE_C_NAME_UOP_MATH_SUB:  MICROCODE_V_NAME_OPCODE_SUB}

    # microcode format
    MICROCODE_FORMAT_ADDR = "9'd%03d"
    MICROCODE_FORMAT_LINE = MICROCODE_FORMAT_ADDR + ": data <= %s;"
    MICROCODE_FORMAT_OFFSET = "localparam [UOP_ADDR_WIDTH-1:0] %s = " + MICROCODE_FORMAT_ADDR + ";"
                            
    # pieces of microcode
    MICROCODE_LINES_PREPARE             = []
    MICROCODE_LINES_BEFORE_ROUND_K0     = []
    MICROCODE_LINES_BEFORE_ROUND_K1     = []
    MICROCODE_LINES_DURING_ROUND        = []
    MICROCODE_LINES_AFTER_ROUND_K0      = []
    MICROCODE_LINES_AFTER_ROUND_K1      = []
    MICROCODE_LINES_BEFORE_INVERSION    = []
    MICROCODE_LINES_DURING_INVERSION    = []
    MICROCODE_LINES_AFTER_INVERSION     = []
    MICROCODE_LINES_FINAL_REDUCTION     = []
    MICROCODE_LINES_HANDLE_SIGN         = []
    MICROCODE_LINES_OUTPUT              = []
    
    MICROCODE_LINE_STOP = "{%s, %s, %s, %s, %s}" % (    MICROCODE_V_NAME_OPCODE_STOP,
                                                        MICROCODE_V_NAME_BANKS_DUMMY,
                                                        MICROCODE_V_NAME_OPERAND_DONTCARE,
                                                        MICROCODE_V_NAME_OPERAND_DONTCARE,
                                                        MICROCODE_V_NAME_OPERAND_DONTCARE)
    
    def __init__(self, filenames):
        self.__filenames = filenames
    
    def parse(self):
        for next_filename in self.__filenames:
            print("Parsing '%s'..." % (next_filename))
            parsing_now = False
            line_num = 0
            with open(next_filename, "r") as c_file:
                c_lines = c_file.readlines()
                for next_c_line in c_lines:
                    line_num += 1
                    self.__line_num = line_num
                    self.__next_c_line = next_c_line.strip()
                    if len(self.__next_c_line) == 0: continue
                    if self.__next_c_line.startswith("//"): continue
                    if not parsing_now:
                        self.__current_piece = self.__try_start_parsing()
                        if self.__current_piece != self.MICROCODE_PIECE_ENUM.NONE:
                            print("    Found piece of microcode: %s" % (str(self.__current_piece)))
                            parsing_now = True
                    else:
                        parsing_now = self.__continue_parsing()
                    
    def format(self):

        if len(self.MICROCODE_LINES_PREPARE)            == 0: sys.exit("sys.exit(): Empty PREPARE piece!")
        if len(self.MICROCODE_LINES_BEFORE_ROUND_K0)    == 0: sys.exit("sys.exit(): Empty BEFORE_ROUND_K0 piece!")
        if len(self.MICROCODE_LINES_BEFORE_ROUND_K1)    == 0: sys.exit("sys.exit(): Empty BEFORE_ROUND_K1 piece!")
        if len(self.MICROCODE_LINES_DURING_ROUND)       == 0: sys.exit("sys.exit(): Empty DURING_ROUND piece!")
        if len(self.MICROCODE_LINES_AFTER_ROUND_K0)     == 0: sys.exit("sys.exit(): Empty AFTER_ROUND_K0 piece!")
        if len(self.MICROCODE_LINES_AFTER_ROUND_K1)     == 0: sys.exit("sys.exit(): Empty AFTER_ROUND_K1 piece!")
        if len(self.MICROCODE_LINES_BEFORE_INVERSION)   == 0: sys.exit("sys.exit(): Empty BEFORE_INVERSION piece!")
        if len(self.MICROCODE_LINES_DURING_INVERSION)   == 0: sys.exit("sys.exit(): Empty DURING_INVERSION piece!")
        if len(self.MICROCODE_LINES_AFTER_INVERSION)    == 0: sys.exit("sys.exit(): Empty AFTER_INVERSION piece!")
        if len(self.MICROCODE_LINES_FINAL_REDUCTION)    == 0: sys.exit("sys.exit(): Empty FINAL_REDUCTION piece!")
        if len(self.MICROCODE_LINES_HANDLE_SIGN)        == 0: sys.exit("sys.exit(): Empty HANDLE_SIGN piece!")
        if len(self.MICROCODE_LINES_OUTPUT)             == 0: sys.exit("sys.exit(): Empty OUTPUT piece!")
        
        length = 0
        length += len(self.MICROCODE_LINES_PREPARE)
        length += len(self.MICROCODE_LINES_BEFORE_ROUND_K0)
        length += len(self.MICROCODE_LINES_BEFORE_ROUND_K1)
        length += len(self.MICROCODE_LINES_DURING_ROUND)
        length += len(self.MICROCODE_LINES_AFTER_ROUND_K0)
        length += len(self.MICROCODE_LINES_AFTER_ROUND_K1)
        length += len(self.MICROCODE_LINES_BEFORE_INVERSION)
        length += len(self.MICROCODE_LINES_DURING_INVERSION)
        length += len(self.MICROCODE_LINES_AFTER_INVERSION)
        length += len(self.MICROCODE_LINES_FINAL_REDUCTION)
        length += len(self.MICROCODE_LINES_OUTPUT)

        print("Total number of micro-operations (w/o stops): %s" % (length))
        
        
        self.__addr = 0
        
        print("\n -=-=-=-=-=- CUT AND PASTE BELOW -=-=-=-=-=-\n")
        
        offset_prepare = self.__addr;
        print("// PREPARE");
        for line in self.MICROCODE_LINES_PREPARE:
            self.__format_line(line)
        self.__format_line(self.MICROCODE_LINE_STOP)

        offset_before_round_k0 = self.__addr;
        print("// BEFORE_ROUND_K0");
        for line in self.MICROCODE_LINES_BEFORE_ROUND_K0:
            self.__format_line(line)
        self.__format_line(self.MICROCODE_LINE_STOP)

        offset_before_round_k1 = self.__addr;
        print("// BEFORE_ROUND_K1");
        for line in self.MICROCODE_LINES_BEFORE_ROUND_K1:
            self.__format_line(line)
        self.__format_line(self.MICROCODE_LINE_STOP)

        offset_during_round = self.__addr;
        print("// DURING_ROUND");
        for line in self.MICROCODE_LINES_DURING_ROUND:
            self.__format_line(line)
        self.__format_line(self.MICROCODE_LINE_STOP)
        
        offset_after_round_k0 = self.__addr;
        print("// AFTER_ROUND_K0");
        for line in self.MICROCODE_LINES_AFTER_ROUND_K0:
            self.__format_line(line)
        self.__format_line(self.MICROCODE_LINE_STOP)
        
        offset_after_round_k1 = self.__addr;
        print("// AFTER_ROUND_K1");
        for line in self.MICROCODE_LINES_AFTER_ROUND_K1:
            self.__format_line(line)
        self.__format_line(self.MICROCODE_LINE_STOP)

        offset_before_inversion = self.__addr;
        print("// BEFORE_INVERSION");
        for line in self.MICROCODE_LINES_BEFORE_INVERSION:
            self.__format_line(line)
        self.__format_line(self.MICROCODE_LINE_STOP)

        offset_during_inversion = self.__addr;
        print("// DURING_INVERSION");
        for line in self.MICROCODE_LINES_DURING_INVERSION:
            self.__format_line(line)
        self.__format_line(self.MICROCODE_LINE_STOP)

        offset_after_inversion = self.__addr;
        print("// AFTER_INVERSION");
        for line in self.MICROCODE_LINES_AFTER_INVERSION:
            self.__format_line(line)
        self.__format_line(self.MICROCODE_LINE_STOP)

        offset_final_reduction = self.__addr;
        print("// FINAL_REDUCTION");
        for line in self.MICROCODE_LINES_FINAL_REDUCTION:
            self.__format_line(line)
        self.__format_line(self.MICROCODE_LINE_STOP)

        offset_handle_sign = self.__addr;
        print("// HANDLE_SIGN");
        for line in self.MICROCODE_LINES_HANDLE_SIGN:
            self.__format_line(line)
        self.__format_line(self.MICROCODE_LINE_STOP)

        offset_output = self.__addr;
        print("// OUTPUT");
        for line in self.MICROCODE_LINES_OUTPUT:
            self.__format_line(line)
        self.__format_line(self.MICROCODE_LINE_STOP)

        print("\n")
        self.__format_offset("UOP_OFFSET_PREPARE         ", offset_prepare)
        self.__format_offset("UOP_OFFSET_BEFORE_ROUND_K0 ", offset_before_round_k0)
        self.__format_offset("UOP_OFFSET_BEFORE_ROUND_K1 ", offset_before_round_k1)
        self.__format_offset("UOP_OFFSET_DURING_ROUND    ", offset_during_round)
        self.__format_offset("UOP_OFFSET_AFTER_ROUND_K0  ", offset_after_round_k0)
        self.__format_offset("UOP_OFFSET_AFTER_ROUND_K1  ", offset_after_round_k1)
        self.__format_offset("UOP_OFFSET_BEFORE_INVERSION", offset_before_inversion)
        self.__format_offset("UOP_OFFSET_DURING_INVERSION", offset_during_inversion)
        self.__format_offset("UOP_OFFSET_AFTER_INVERSION ", offset_after_inversion)
        self.__format_offset("UOP_OFFSET_FINAL_REDUCTION ", offset_final_reduction)
        self.__format_offset("UOP_OFFSET_HANDLE_SIGN     ", offset_handle_sign)
        self.__format_offset("UOP_OFFSET_OUTPUT          ", offset_output)
        
    def __format_line(self, line):
        print(self.MICROCODE_FORMAT_LINE % (self.__addr, line))
        self.__addr += 1
    
    def __format_offset(self, name, offset):
        print(self.MICROCODE_FORMAT_OFFSET % (name, offset))
    
    def __try_start_parsing(self):
        piece = self.MICROCODE_PIECE_ENUM.NONE
        begin_marker = re.match("^/\*\s*" + self.MARKER_BEGIN_MICROCODE + "\s*" +
                                            self.MATCH_GROUP_09AZ + "\s*\*/$",
                                            self.__next_c_line);
        if begin_marker:
            piece = self.MICROCODE_PIECE_DICT[begin_marker.group(1)]
            
        return piece
    
    def __encode_uop(self, opcode, banks, src1, src2, dst):
        return "{%s, %s, %s, %s, %s}" % (opcode, banks, src1, src2, dst)
    
    def __continue_parsing(self):
    
        end_marker = re.match("^/\*\s*" +   self.MARKER_END_MICROCODE + "\s*\*/$",
                                            self.__next_c_line);
        if end_marker: return False
        
        # move?
        uop_move = re.match("^" +   self.MICROCODE_C_NAME_UOP_MOVE + "\(" +
                                    self.MATCH_GROUP_BANK          + "\,\s*" +
                                    self.MATCH_GROUP_09AZ          + "\,\s*" +
                                    self.MATCH_GROUP_BANK          + "\,\s*" +
                                    self.MATCH_GROUP_09AZ          + "\,\s*" +
                                    self.MATCH_GROUP_DUMMY_C_PTR   + "\)\;$",
                                    self.__next_c_line, re.IGNORECASE)
        if uop_move:
            ok = self.__parse_uop_move(uop_move)
            if ok: return True
            else:
                self.__print_parse_error("parse_uop_move() failed!")
                self.__abort()
        
        # calc?
        uop_calc = re.match("^" +   self.MATCH_GROUP_CALC           + "\s*\(" +
                                    self.MATCH_GROUP_MATH           + "\,\s*" +
                                    self.MATCH_GROUP_BANK           + "\,\s*" +
                                    self.MATCH_GROUP_09AZ           + "\,\s*" +
                                    self.MATCH_GROUP_09AZ           + "\,\s*" +
                                    self.MATCH_GROUP_BANK           + "\,\s*" +
                                    self.MATCH_GROUP_09AZ           + "\,\s*" +
                                    self.MATCH_GROUP_DUMMY_C_PTR    + "\,\s*" +
                                    self.MATCH_GROUP_DUMMY_C_MOD    + "\)\;$",
                                    self.__next_c_line, re.IGNORECASE)
        if uop_calc:
            ok = self.__parse_uop_calc(uop_calc)
            if ok: return True
            else:
                self.__print_parse_error("parse_uop_calc() failed!")
                self.__abort()
        
        # cycle?
        uop_cycle = re.match("^" +  self.MICROCODE_C_NAME_UOP_CYCLE + "\(" +
                                    self.MATCH_GROUP_NUMBER         + "\)\;$",
                                    self.__next_c_line, re.IGNORECASE)
        if uop_cycle:
            ok = self.__parse_uop_cycle(uop_cycle)
            if ok: return True
            else:
                self.__print_parse_error("parse_uop_cycle() failed!")
                self.__abort()

        # repeat?
        uop_repeat = re.match("^" + self.MICROCODE_C_NAME_UOP_REPEAT + "\(" + "\)\;$",
                                    self.__next_c_line, re.IGNORECASE)
        if uop_repeat:
            ok = self.__parse_uop_repeat()
            if ok: return True
            else:
                self.__print_parse_error("__parse_uop_repeat() failed!")
                self.__abort()
                
        self.__print_parse_error("unknown micro-operation!")
        self.__abort()

    def __check_math(self, math):
        if not math in self.MICROCODE_MATH_DICT:
            print_parse_error("bad math!")
            return False
            
        return True

    def __check_banks(self, src, dst):
        if not src in self.MICROCODE_BANK_DICT:
            print_parse_error("bad src bank!")
            return False

        if not dst in self.MICROCODE_BANK_DICT:
            print_parse_error("bad dst bank!")
            return False

        if src == dst:
            print_parse_error("src bank == dst bank!")
            return False
            
        return True

    def __check_op3(self, src1, src2, dst):

        if not src1 in self.MICROCODE_OPERAND_DICT:
            self.__print_parse_error("bad src1 operand!")
            return False

        if src2 != "" and not src2 in self.MICROCODE_OPERAND_DICT:
            self.__print_parse_error("bad src2 operand!")
            return False

        if not dst in self.MICROCODE_OPERAND_DICT:
            self.__print_parse_error("bad dst operand!")
            return False
            
        return True

    def __check_op2(self, src, dst):
        return self.__check_op3(src, "", dst)
    
    def __parse_uop_move(self, params):
        bank_src = params.group(1)
        bank_dst = params.group(3)
        op_src   = params.group(2)
        op_dst   = params.group(4)
    
        if not self.__check_banks(bank_src, bank_dst):
            self.__print_parse_error("check_banks() failed!")
            return False
            
        if not self.__check_op2(op_src, op_dst):
            self.__print_parse_error("check_op2() failed!")
            return False
    
        opcode = self.MICROCODE_V_NAME_OPCODE_MOVE
        bank = self.MICROCODE_BANK_DICT[bank_src]
        src1 = self.MICROCODE_OPERAND_DICT[op_src]
        src2 = self.MICROCODE_V_NAME_OPERAND_DONTCARE
        dst = self.MICROCODE_OPERAND_DICT[op_dst]
    
        data = self.__encode_uop(opcode, bank, src1, src2, dst)
        self.__store_uop(data)
        return True

    def __parse_uop_calc(self, params):
        calc     = params.group(1)
        math     = params.group(2)
        bank_src = params.group(3)
        bank_dst = params.group(6)
        op_src1  = params.group(4)
        op_src2  = params.group(5)
        op_dst   = params.group(7)
    
        if not self.__check_math(math):
            self.__print_parse_error("check_calc() failed!")
            return False
    
        if not self.__check_banks(bank_src, bank_dst):
            self.__print_parse_error("check_banks() failed!")
            return False
            
        if not self.__check_op3(op_src1, op_src2, op_dst):
            self.__print_parse_error("check_op3() failed!")
            return False
    
        opcode = self.MICROCODE_MATH_DICT[math]
        banks = self.MICROCODE_BANK_DICT[bank_src]
        src1 = self.MICROCODE_OPERAND_DICT[op_src1]
        src2 = self.MICROCODE_OPERAND_DICT[op_src2]
        dst = self.MICROCODE_OPERAND_DICT[op_dst]

        data = self.__encode_uop(opcode, banks, src1, src2, dst)
        if   calc == self.MICROCODE_C_NAME_UOP_CALC:        self.__store_uop(data)
        elif calc == self.MICROCODE_C_NAME_UOP_CALC_EVEN:   self.__loop_even = data
        elif calc == self.MICROCODE_C_NAME_UOP_CALC_ODD:    self.__loop_odd = data
        else: return False
        
        return True

    def __parse_uop_cycle(self, params):
        self.__loop_iters = int(params.group(1))
        return True

    def __parse_uop_repeat(self):
        print("        Unrolling loop (%d iters)..." % (self.__loop_iters))
        
        for i in range(0, self.__loop_iters):
            if i % 2 == 0:  self.__store_uop(self.__loop_even)
            else:           self.__store_uop(self.__loop_odd)
        
        return True
        
    def __store_uop(self, data):
        #print("\t" + data)
        if   self.__current_piece == self.MICROCODE_PIECE_ENUM.PREPARE:             self.MICROCODE_LINES_PREPARE.append(data)
        elif self.__current_piece == self.MICROCODE_PIECE_ENUM.BEFORE_ROUND_K0:     self.MICROCODE_LINES_BEFORE_ROUND_K0.append(data)
        elif self.__current_piece == self.MICROCODE_PIECE_ENUM.BEFORE_ROUND_K1:     self.MICROCODE_LINES_BEFORE_ROUND_K1.append(data)
        elif self.__current_piece == self.MICROCODE_PIECE_ENUM.DURING_ROUND:        self.MICROCODE_LINES_DURING_ROUND.append(data)
        elif self.__current_piece == self.MICROCODE_PIECE_ENUM.AFTER_ROUND_K0:      self.MICROCODE_LINES_AFTER_ROUND_K0.append(data)
        elif self.__current_piece == self.MICROCODE_PIECE_ENUM.AFTER_ROUND_K1:      self.MICROCODE_LINES_AFTER_ROUND_K1.append(data)
        elif self.__current_piece == self.MICROCODE_PIECE_ENUM.BEFORE_INVERSION:    self.MICROCODE_LINES_BEFORE_INVERSION.append(data)
        elif self.__current_piece == self.MICROCODE_PIECE_ENUM.DURING_INVERSION:    self.MICROCODE_LINES_DURING_INVERSION.append(data)
        elif self.__current_piece == self.MICROCODE_PIECE_ENUM.AFTER_INVERSION:     self.MICROCODE_LINES_AFTER_INVERSION.append(data)
        elif self.__current_piece == self.MICROCODE_PIECE_ENUM.FINAL_REDUCTION:     self.MICROCODE_LINES_FINAL_REDUCTION.append(data)
        elif self.__current_piece == self.MICROCODE_PIECE_ENUM.HANDLE_SIGN:         self.MICROCODE_LINES_HANDLE_SIGN.append(data)
        elif self.__current_piece == self.MICROCODE_PIECE_ENUM.OUTPUT:              self.MICROCODE_LINES_OUTPUT.append(data)

    def __print_parse_error(self, msg):
        print("PARSE ERROR: %s" % (msg))
  
    def __abort(self):
        sys.exit("Stopped at line #%d:\n%s" % (self.__line_num, self.__next_c_line))


# -----------------------------------------------------------------------------
def main(filenames):
# -----------------------------------------------------------------------------
    parser = MICROCODE_PARSER(filenames)
    parser.parse()
    parser.format()
    
# -----------------------------------------------------------------------------    
if __name__ == "__main__":
# -----------------------------------------------------------------------------
    main(C_FILES)
    
#
# End-of-File
#
