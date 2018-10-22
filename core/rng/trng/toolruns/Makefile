#===================================================================
#
# Makefile
# --------
# Makefile for building the TRNG modules and and top simulations.
#
#
# Author: Joachim Strombergson
# Copyright (c) 2014, NORDUnet A/S All rights reserved.
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

CHACHA_SRC_DIR=../../../cipher/chacha/src/rtl/
CHACHA_SRC=$(CHACHA_SRC_DIR)chacha_core.v $(CHACHA_SRC_DIR)chacha_qr.v
CSPRNG_SRC=../src/rtl/trng_csprng.v ../src/rtl/trng_csprng_fifo.v
TB_CSPRNG_SRC=../src/tb/tb_csprng.v

CSPRNG_FIFO_SRC=../src/rtl/trng_csprng_fifo.v
TB_CSPRNG_FIFO_SRC=../src/tb/tb_csprng_fifo.v

SHA512_SRC_DIR=../../../hash/sha512/src/rtl/
SHA512_SRC=$(SHA512_SRC_DIR)sha512_core.v $(SHA512_SRC_DIR)sha512_h_constants.v $(SHA512_SRC_DIR)sha512_k_constants.v $(SHA512_SRC_DIR)sha512_w_mem.v

MIXER_SRC=../src/rtl/trng_mixer.v
TB_MIXER_SRC=../src/tb/tb_mixer.v

TRNG_SRC=../src/rtl/trng.v $(MIXER_SRC) $(SHA512_SRC) $(CSPRNG_SRC) $(CHACHA_SRC)
TB_TRNG_SRC=../src/tb/tb_trng.v
FAKE_ENTROPY_SRC=../src/tb/fake_modules/avalanche_entropy.v ../src/tb/fake_modules/pseudo_entropy.v ../src/tb/fake_modules/rosc_entropy.v

CC =iverilog
LINT = verilator -Wall --lint-only

all: trng.sim mixer.sim csprng.sim csprng_fifo.sim


csprng.sim: $(TB_CSPRNG_SRC) $(CSPRNG_SRC) $(CHACHA_SRC)
	$(CC) -o csprng.sim $(TB_CSPRNG_SRC) $(CSPRNG_SRC) $(CHACHA_SRC)


csprng_fifo.sim: $(TB_CSPRNG_FIFO_SRC) $(CSPRNG_FIFO_SRC)
	$(CC) -o csprng_fifo.sim $(TB_CSPRNG_FIFO_SRC) $(CSPRNG_FIFO_SRC)


mixer.sim: $(TB_MIXER_SRC) $(MIXER_SRC) $(SHA512_SRC)
	$(CC) -o mixer.sim $(TB_MIXER_SRC) $(MIXER_SRC) $(SHA512_SRC)


trng.sim: $(TRNG_SRC) $(FAKE_ENTROPY_SRC)
	$(CC) -o trng.sim $(TB_TRNG_SRC) $(TRNG_SRC) $(FAKE_ENTROPY_SRC)


sim-csprng_fifo: csprng_fifo.sim
	./csprng_fifo.sim


sim-csprng: csprng.sim
	./csprng.sim


sim-mixer: mixer.sim
	./mixer.sim


sim-trng: trng.sim
	./trng.sim


lint:
	@echo "NOTE: Does not lint rosc and avalance noise sources."
	@echo ""
	$(LINT) $(CSPRNG_SRC) $(CHACHA_SRC)
	$(LINT) $(MIXER_SRC) $(SHA512_SRC)


clean:
	rm -f csprng_fifo.sim
	rm -f csprng.sim
	rm -f mixer.sim
	rm -f trng.sim


help:
	@echo "Build system for simulation of TRNG core"
	@echo ""
	@echo "Supported targets:"
	@echo "------------------"
	@echo "all:         Build all simulation targets."
	@echo "csprng.sim:  Build the csprng simulation target."
	@echo "mixer.sim:   Build the mixer simulation target."
	@echo "trng.sim:    Build the trng simulation target."
	@echo "sim-csprng:  Run cprng simulation."
	@echo "sim-mixer:   Run mixer simulation."
	@echo "sim-trng:    Run trng simulation."
	@echo "clean:       Delete all built files."

#===================================================================
# EOF Makefile
#===================================================================
