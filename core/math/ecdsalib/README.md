# ecdsalib

## Core Description

This is a library of code common to the ecdsa256 and ecdsa384 cores.  See documentation of those cores for details.

This code was originally repelicated in both of the above cores, but the Xilinx synthesis tools got tetchy about that when trying to build a single image containing both cores.

## Vendor-specific Primitives

Cryptech Alpha platform is based on Xilinx Artix-7 200T FPGA, so this core takes advantage of Xilinx-specific DSP slices to carry out math-intensive operations. All vendor-specific math primitives are placed under /rtl/lowlevel/artix7, the core also offers generic replacements under /rtl/lowlevel/generic, they can be used for simulation with 3rd party tools, that are not aware of Xilinx-specific stuff. Selection of vendor/generic primitives is done in ecdsa_lowlevel_settings.v, when porting to other architectures, only those four low-level modules need to be ported.
