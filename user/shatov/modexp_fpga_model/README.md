# modexp_fpga_model

This reference model was written to help debug Verilog code, it mimics how an FPGA would do modular exponentiation using systolic Montgomery multiplier. Note, that the model may do weird (from CPU point of view, of course) things at times. Another important thing is that while FPGA modules are written to operate in true constant-time manner, this model itself doesn't take any active measures to keep run-time constant. Do **NOT** use it in production as-is!

The model is split into low-level primitives (32-bit adder, 32-bit subtractor, 32x32-bit multiplier with pre-adder) and higher-level arithmetic routines (multiplier and exponentiator).

This model uses tips and tricks from the following sources:
1. [High-Speed RSA Implementation](ftp://ftp.rsasecurity.com/pub/pdfs/tr201.pdf)
2. [Handbook of Applied Cryptography](http://cacr.uwaterloo.ca/hac/)
3. [Montgomery Modular Multiplication on Reconfigurable Hardware: Systolic versus Multiplexed Implementation](https://www.hindawi.com/journals/ijrc/2011/127147/)
