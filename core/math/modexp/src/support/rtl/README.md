This directory contains support RTL code for the modexp core. The code
here is not directly part of the core RTL.

montprod_wrapper.v
  A simple wrapper to mux together inputs and outputs from the montprod
  module. Used for test builds of versions of montprod with different
  (64, 128,2 256 bits) operand widths which means that the interface
  from the montprod can contain a huge number of bits and thus pins.


blockmem_rw32ptr_r64.v
  A synchronous block memory with two separate ports and internal
  address generator as used in the modex_core to implement the exponent,
  modulus and message memories. This version sports a 64 bit wide data
  port for core internal read access while the API facing interface uses
  32 bit wide data. When the modexp is set to use 64 bit operands, this
  module should be included into the src/rtl dir to be used in the
  modexp_core instantiation of the block memory.
