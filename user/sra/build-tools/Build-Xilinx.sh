#!/bin/sh -

# Wrapper script for building Xilinx images on Caerbannog (32-bit Debian Wheezy VM).

# Pick a target:

#TARGET=core/platform/novena/i2c/build
TARGET=core/platform/novena/eim/build

# Pull fresh copies of all the repositories

$HOME/http-sync-repos.py

# Move to the target's build area

cd $HOME/Cryptech/$TARGET

# Run command line version of the Xilinx build environment (thanks, Paul!)
#
# Since Paul was developing this on a 64-bit VM and I'm using a 32-bit
# VM, I need to override a few settings on the make command line.

make isedir='/opt/Xilinx/14.7/ISE_DS' xil_env='. $(isedir)/settings32.sh'

# At this point, if all went well, there should be a .bit file, which
# I can now move to my Novena board.

scp -p novena_eim.bit tym:
