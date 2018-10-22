#===================================================================
#
# Makefile
# --------
# Makefile for building and simulating coretest.
#
#
# Author: Joachim Strombergson
# Copyright (c) 2014 NORDUnet A/S
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

CORETEST_SRC=../src/rtl/coretest.v
CORETEST_TB_SRC=../src/tb/tb_coretest.v
CORETEST_TARGET = coretest.sim

CC=iverilog


all: coretest


coretest: $(CORETEST_TB_SRC) $(CORETEST_SRC)
	$(CC) -o $(CORETEST_TARGET) $(CORETEST_TB_SRC) $(CORETEST_SRC)


sim-coretest: $(CORETEST_TARGET)
	./$(CORETEST_TARGET)


debug:
	@echo "No debug available."


clean:
	rm -f $(CORETEST_TARGET)


help:
	@echo "Supported targets:"
	@echo "------------------"
	@echo "coretest:      Build coretest simulation target."
	@echo "sim-coretest:  Run coretest simulation."
	@echo "debug:         Print the internal varibles."
	@echo "clean:         Delete all built files."

#===================================================================
# EOF Makefile
#===================================================================

