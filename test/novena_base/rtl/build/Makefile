project = novena_baseline_top
vendor = xilinx
family = spartan6
part = xc6slx45csg324-3
top_module = novena_baseline_top
isedir = /opt/Xilinx/14.7/ISE_DS
xil_env = . $(isedir)/settings64.sh
ucf = ../src/ucf/novena_baseline.ucf

vfiles = \
	../src/verilog/cdc_bus_pulse.v \
	../src/verilog/cipher_selector.v \
	../src/verilog/core_selector.v \
	../src/verilog/eim_arbiter_cdc.v \
	../src/verilog/eim_arbiter.v \
	../src/verilog/eim_da_phy.v \
	../src/verilog/eim_indicator.v \
	../src/verilog/eim_memory.v \
	../src/verilog/novena_baseline_top.v \
	../src/verilog/novena_clkmgr.v \
	../src/verilog/novena_regs.v \
	../src/verilog/rng_selector.v \
	../src/verilog/sha1.v \
	../src/verilog/sha256.v \
	../src/verilog/sha512.v \
	../src/ipcore/clkmgr_dcm.v \
	../../../../core/sha1/src/rtl/sha1_core.v \
	../../../../core/sha1/src/rtl/sha1_w_mem.v \
	../../../../core/sha256/src/rtl/sha256_core.v \
	../../../../core/sha256/src/rtl/sha256_k_constants.v \
	../../../../core/sha256/src/rtl/sha256_w_mem.v \
	../../../../core/sha512/src/rtl/sha512_core.v \
	../../../../core/sha512/src/rtl/sha512_h_constants.v \
	../../../../core/sha512/src/rtl/sha512_k_constants.v \
	../../../../core/sha512/src/rtl/sha512_w_mem.v

include xilinx.mk
