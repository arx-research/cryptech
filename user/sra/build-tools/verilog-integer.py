# Code to parse Verilog's integer constant syntax so that we can
# generate C headers from Verilog constants.
#
# Doesn't attempt to handle the analogue states ("x", "z", "?").
#
# Reference: http://verilog.renerta.com/source/vrg00020.htm

class VerilogInteger(object):

    radix = dict(b = 2, o = 8, d = 10, h = 16)

    def __init__(self, input):
        head, sep, tail = input.lower().translate(None, " \t_").partition("'")
        self.input = input
        self.code  = tail[0] if tail else None

        if not sep:
            self.width  = 32
            self.value  = int(head)

        elif self.code in self.radix:
            self.width  = int(head) if head else 32
            self.value  = int(tail[1:], self.radix[self.code])
            if self.width <= 0 or self.value < 0:
                raise ValueError

        else:
            raise ValueError

        if self.width is not None:
            mask = (1L << self.width) - 1
            if self.value > mask:
                self.value &= mask
            elif self.value < -mask:
                self.value = -( -self.value & mask)

    @property
    def C(self):
        if self.code is None:
            return str(self.value)
        elif self.code == "d":
            return "{0:d}".format(self.value)
        elif self.code == "o":
            return "0{0:0{1}o}".format(self.value, (self.width + 2) / 3)
        else:
            return "0x{0:0{1}x}".format(self.value, (self.width + 3) / 4)

    @property
    def Verilog(self):
        if self.code is None:
            return str(self.value)
        else:
            fmt = "x" if self.code == "h" else self.code
            return "{0.width}'{0.code}{0.value:{1}}".format(self, fmt)


if __name__ == "__main__":

    def show(*args):
        print "{:20} | {:20} | {:20}".format(*args)

    show("C", "Verilog", "Input")
    show("-" * 20, "-" * 20, "-" * 20)

    def test(x):
        v = VerilogInteger(x)
        show(v.C, v.Verilog, v.input)

    test("15")
    test("'h f")
    test("'o 17")
    test("'d 15")
    test("'b 1111")
    test("'b 1_1_1_1")
    test("10 'd 20")
    test("6'o 71")
    test("8'b0")
    test("8'b00000000")
    test("8'b1")
    test("8'b00000001")

    try:
        for line in open("/tmp/sample-verilog-numbers"):
            test(line.strip())
    except IOError:
        pass
