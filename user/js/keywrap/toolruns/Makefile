#===================================================================
#
# Makefile
# --------
# Makefile for building the keywrap module.
#
#
# Author: Joachim Strombergson
# Copyright (c) 2018, NORDUnet A/S
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
# - Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
#
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# - Neither the name of the NORDUnet nor the names of its contributors may
#   be used to endorse or promote products derived from this software
#   without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
# TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#===================================================================

AES_PATH = ../../../../core/cipher/aes_speed/src/rtl
AES_SRC = $(AES_PATH)/aes_core.v $(AES_PATH)/aes_decipher_block.v $(AES_PATH)/aes_encipher_block.v $(AES_PATH)/aes_inv_sbox.v $(AES_PATH)/aes_key_mem.v $(AES_PATH)/aes_sbox.v

MEM_SRC = ../src/rtl/keywrap_mem.v
TB_MEM_SRC = ../src/tb/tb_keywrap_mem.v

CORE_SRC = ../src/rtl/keywrap_core.v $(AES_SRC) $(MEM_SRC)
TB_CORE_SRC = ../src/tb/tb_keywrap_core.v

TOP_SRC = ../src/rtl/keywrap.v $(CORE_SRC)
TB_TOP_SRC = ../src/tb/tb_keywrap.v

CC = iverilog
CC_FLAGS = -Wall

LINT = verilator
LINT_FLAGS = +1364-2001ext+ --lint-only  -Wall -Wno-fatal -Wno-DECLFILENAME

all: top.sim core.sim mem.sim


top.sim: $(TB_TOP_SRC) $(TOP_SRC)
	$(CC) $(CC_FLAGS) -o top.sim $(TB_TOP_SRC) $(TOP_SRC)


core.sim: $(TB_CORE_SRC) $(CORE_SRC)
	$(CC) $(CC_FLAGS) -o core.sim $(TB_CORE_SRC) $(CORE_SRC)


mem.sim: $(TB_MEM_SRC) $(MEM_SRC)
	$(CC) $(CC_FLAGS) -o mem.sim $(TB_MEM_SRC) $(MEM_SRC)


sim-top: top.sim
	./top.sim


sim-core: core.sim
	./core.sim


sim-mem: mem.sim
	./mem.sim


lint:  $(TOP_SRC)
	$(LINT) $(LINT_FLAGS) $(TOP_SRC)


clean:
	rm -f top.sim
	rm -f core.sim
	rm -f mem.sim


help:
	@echo "Supported targets:"
	@echo "------------------"
	@echo "all:          Build all simulation targets."
	@echo "lint:         Lint all rtl source files."
	@echo "top.sim:      Build top simulation target."
	@echo "core.sim:     Build core simulation target."
	@echo "sim-top:      Run top simulation."
	@echo "sim-core:     Run core simulation."
	@echo "mem.sim:      Build mem simulation target."
	@echo "sim-mem:      Run mem simulation."
	@echo "clean:        Delete all built files."

#===================================================================
# EOF Makefile
#===================================================================
