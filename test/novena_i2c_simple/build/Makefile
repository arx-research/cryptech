project = coretest-novena-simple
vendor = xilinx
family = spartan6
part = xc6slx45csg324-3
top_module = novena_fpga
isedir = /opt/Xilinx/14.3/ISE_DS
xil_env = . $(isedir)/settings64.sh

vfiles = ../src/rtl/novena_fpga.v \
	../src/rtl/coretest_hashes.v \
	../src/rtl/sha1.v \
	../src/rtl/sha256.v \
	../src/rtl/sha512.v \
	../../i2c/src/rtl/i2c_core.v \
	../../sha1/src/rtl/sha1_core.v \
	../../sha1/src/rtl/sha1_w_mem.v \
	../../sha256/src/rtl/sha256_core.v \
	../../sha256/src/rtl/sha256_k_constants.v \
	../../sha256/src/rtl/sha256_w_mem.v \
	../../sha512/src/rtl/sha512_core.v \
	../../sha512/src/rtl/sha512_h_constants.v \
	../../sha512/src/rtl/sha512_k_constants.v \
	../../sha512/src/rtl/sha512_w_mem.v

include xilinx.mk
