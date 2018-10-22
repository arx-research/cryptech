# curve25519_fpga_model

This reference model was written to help debug Verilog code. It comprises two parts: **x25519_fpga_model** and **ed25519_fpga_model**. See [1] for more information about the difference. The model mimics how an FPGA would do elliptic curve point scalar multiplication. Note, that the model may do weird (from CPU point of view, of course) things at times. Another important thing is that while FPGA modules are actually written to operate in constant-time manner, this model itself doesn't take any active measures to keep run-time constant. Do **NOT** use it in production as-is!

Elliptic curve arithmetic can be split into several "layers":
1. Low-level arithmetic
2. Multi-precision arithmetic
3. Modular arithmetic
4. Curve arithmetic

**Low-level arithmetic** comprises elementary operations that the underlying hardware can do. These are typically 16-/32-/64-bit addition/subtraction and multiplication for conventional processors. Xilinx FPGA devices have specialized DSP slices that can do up to 48-bit addition/subtraction and up to 25x18-bit multiplication (latest 7 Series family at least).

**Multi-precision arithmetic** comprises operations on large (256-bit for this model) numbers using the elementary operations from layer 1.

**Modular arithmetic** comprises operations modulo certain prime based on layer 2. For this particualar model the prime is p = 2^255 - 19.

**Curve arithmetic** comprises addition and doubling of curve points and scalar multiplication based on the double-and-add algorithm.

Levels 1-3 are the same for both X25519 and Ed25519. The trick used in layer 3 is that the model internally works modulo 2p (2^256-38), because it's computationally more efficient to not fully reduce the result until the very end of calculation. See "Special Reduction" in [2] for more information. Final reduction is done by simply adding zero modulo p.

Conversion from the coordinate system used in layer 4 to affine coordinates involves modular inversion. Layer 3 offers modular inversion based on Fermat's little theorem. The addition chain used is from [3]. Thanks for reverse engineering Bernstein's "straightforward sequence of 254 squarings and 11 multiplications" :-P

Modular inversion is offered in two variants: "abstract" (easy to debug user-friendly C code) and microcoded. The latter variant mimics how an FPGA does inversion.

Layer 4 is different for X25519 and Ed25519.

Curve arithmetic for Ed25519 uses Algorithm 4 ("Joye double-and-add") from [4] to do point multiplication. Point doubling is done according to "dbl-2008-hwcd" formulae from [5]. The only difference is that E, F, G & H have opposite sign, this is equivalent to the original algorithm, because the final result depends on E * F and G * H. Point addition is done according to "add-2008-hwcd-4" from [5]. The coordinate system is (X, Y, Z, T), where T = X * Y. Conversion to affine coordinates is: x = X * Z^-1, y = Y * Z^-1. Note that the encoding of the result is somewhat tricky, see [6]. The short story is that we don't need to store entire X coordinate, just its sign is enough to recover X from Y.

_TODO: Describe layer 4 for X25519._

_TODO: Describe how microcode works._

References:

1. [StackExchange answer explaining the practical difference between Curve25519 and Ed25519](https://crypto.stackexchange.com/questions/27866/why-curve25519-for-encryption-but-ed25519-for-signatures)
2. ["High-Performance Modular Multiplication on the Cell Processor"](http://joppebos.com/files/waifi09.pdf)
3. ["The Most Efficient Known Addition Chains for Field Element & Scalar Inversion for the Most Popular & Most Unpopular Elliptic Curves"](https://briansmith.org/ecc-inversion-addition-chains-01)
4. ["Fast and Regular Algorithms for Scalar Multiplication
over Elliptic Curves"](https://eprint.iacr.org/2011/338.pdf)
5. ["Extended coordinates with a=-1 for twisted Edwards curves"](https://hyperelliptic.org/EFD/g1p/auto-twisted-extended-1.html)
6. ["decoding a Ed25519 key per RFC8032"](https://crypto.stackexchange.com/questions/58921/decoding-a-ed25519-key-per-rfc8032)
