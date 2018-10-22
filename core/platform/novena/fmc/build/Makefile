# Localize all the relative path awfulness in one variable.

CORE_TREE	:= $(abspath ../../../..)

# Figure out what the native word size is on the build host, because
# the XiLinx tools care for some reason.

WORD_SIZE	:= $(shell python -c 'from struct import pack; print len(pack("L", 0)) * 8')

# Parameters to xilinx.mk.

project		?= novena_fmc
vendor		= xilinx
family		= spartan6
part		= xc6slx45csg324-3
top_module	= novena_fmc_top
isedir		= /opt/Xilinx/14.7/ISE_DS
xil_env		= . $(isedir)/settings$(WORD_SIZE).sh
ucf		?= ../ucf/$(project).ucf

all:	$(project).bit

# Build the default core_selector if it doesn't already exist.

CONFIG          = $(CORE_TREE)/platform/common/config
core_selector.v core_vfiles.mk:
	$(CONFIG)/config.py -c $(CONFIG)/config.cfg

# Build some different configurations

bare:
	$(CONFIG)/config.py -c $(CONFIG)/config.cfg -s bare
	$(MAKE) project=$(project)_bare ucf=$(ucf)

trng:
	$(CONFIG)/config.py -c $(CONFIG)/config.cfg -s trng
	$(MAKE) project=$(project)_trng ucf=$(ucf)

hash:
	$(CONFIG)/config.py -c $(CONFIG)/config.cfg -s hash
	$(MAKE) project=$(project)_hash ucf=$(ucf)

rsa:
	$(CONFIG)/config.py -c $(CONFIG)/config.cfg -s rsa
	$(MAKE) project=$(project)_rsa ucf=$(ucf)

# Verilog files that always go with builds on this platform.

vfiles = \
	$(CORE_TREE)/platform/novena/fmc/rtl/novena_fmc_top.v \
	$(CORE_TREE)/platform/novena/common/rtl/novena_regs.v \
	$(CORE_TREE)/platform/novena/common/rtl/novena_clkmgr.v \
	$(CORE_TREE)/platform/novena/common/rtl/clkmgr_dcm.v \
	./core_selector.v \
	$(CORE_TREE)/comm/fmc/src/rtl/cdc_bus_pulse.v \
	$(CORE_TREE)/comm/fmc/src/rtl/fmc_arbiter_cdc.v \
	$(CORE_TREE)/comm/fmc/src/rtl/fmc_arbiter.v \
	$(CORE_TREE)/comm/fmc/src/rtl/fmc_d_phy.v \
	$(CORE_TREE)/comm/fmc/src/rtl/fmc_indicator.v \
	$(CORE_TREE)/comm/fmc/src/rtl/fmc_regs.v

# Verilog files selected by the core configuration script.

-include ./core_vfiles.mk

include xilinx.mk

# 'clean' target collects files by project name, and we just broke that
# by adding configurations

junk += *.bgn *.bit *.bld *.cfi *.drc *.lso *.map *.mcs *.mrp *.ncd *.ngc \
	*.ngd *.ngm *.pcf *.post_map.twr *.post_map.twx *.prj *.prm *.psr \
	*.scr *.srp *.twr *.twx *_bd.bmm *_bitgen.xwb *_bitgen.xwbt \
	*_err.twr *_err.twx *_par.grf *_par.ncd *_par.pad *_par.par \
	*_par.ptwx *_par.unroutes *_par.xpi *_par_pad.csv *_par_pad.txt \
	*_summary.xml *_usage.xml

distclean: clean
	rm core_selector.v core_vfiles.mk

# Fun extras for running verilator as a linter.

VERILATOR_FLAGS	= --lint-only --top-module $(top_module) -Wall -Wno-fatal -Wno-DECLFILENAME

lint:
	verilator ${VERILATOR_FLAGS} $(vfiles) $(CORE_TREE)/platform/novena/common/rtl/lint-dummy.v
