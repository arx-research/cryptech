#=======================================================================
#
# Makefile
# --------
# Makefile for building the mkmif core.
#
#
# Copyright (c) 2016, NORDUnet A/S All rights reserved.
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
#=======================================================================

SPI_SRC = ../src/rtl/mkmif_spi.v
SPI_TB = ../src/tb/tb_mkmif_spi.v

CORE_SRC = ../src/rtl/mkmif_core.v $(SPI_SRC)
CORE_TB = ../src/tb/tb_mkmif_core.v

TOP_SRC = ../src/rtl/mkmif.v $(CORE_SRC)
TOP_TB = ../src/tb/tb_mkmif.v

CC = iverilog
LINT=verilator


all:    top.sim core.sim spi.sim


spi.sim:	$(SPI_TB) $(SPI_SRC)
	iverilog -Wall -o spi.sim $(SPI_TB) $(SPI_SRC)


core.sim:	$(CORE_TB) $(CORE_SRC)
	iverilog -Wall -o core.sim $(CORE_TB) $(CORE_SRC)


top.sim:	$(TOP_TB) $(TOP_SRC)
	iverilog -Wall -o top.sim $(TOP_TB) $(TOP_SRC)


lint:
	verilator +1364-2001ext+ --lint-only -Wall $(TOP_SRC)


clean:
	rm *.sim


help:
	@echo "Supported targets:"
	@echo "------------------"
	@echo "all:      Build all simulation targets."
	@echo "core:     Build the core simulation target."
	@echo "top:      Build the top simulation target."
	@echo "lint:     Run the linter on the source"
	@echo "clean:    Delete all built files."

#=======================================================================
# EOF Makefile
#=======================================================================
