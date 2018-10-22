#!/usr/bin/env python

"""
Generate core_selector.v and core_vfiles.mk for a set of cores.
"""

#=======================================================================
# Copyright (c) 2015-2017, NORDUnet A/S All rights reserved.
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
#=======================================================================

# The modexpa7 core drags in a one clock cycle delay to other cores,
# to compensate for the extra clock cycle consumed by the block
# memories used in the modexpa7 core.  We probably want a general
# solution for this, because we're going to run into this problem for
# any core that handles arguments big enough to require block memory.

# To Do:
#
# - Consider automating the one-clock-cycle delay stuff by adding
#   another boolean flag to the config file.  Default would be no
#   delay, if any included core sets the "I use block memories" flag,
#   all other cores would get the delay.  Slightly tedious but
#   something we can calculate easily enough, and probably an
#   improvement over wiring in the delay when nothing needs it.

def main():
    """
    Parse arguments and config file, generate core list, generate output.
    """

    from argparse import ArgumentParser, FileType, ArgumentDefaultsHelpFormatter
    from sys      import exit

    parser = ArgumentParser(description = __doc__, formatter_class = ArgumentDefaultsHelpFormatter)
    parser.add_argument("-d", "--debug",   help = "enable debugging",   action = "store_true")
    parser.add_argument("-c", "--config",  help = "configuration file", default = "core.cfg",       type = FileType("r"))
    parser.add_argument("-b", "--board",   help = "config file 'board' section")
    parser.add_argument("-p", "--project", help = "config file 'project' section")
    parser.add_argument("-v", "--verilog", help = "verilog output file",default = "core_selector.v",  type = FileType("w"))
    parser.add_argument("-m", "--makefile",help = "output makefile",    default = "core_vfiles.mk",   type = FileType("w"))
    parser.add_argument("core",            help = "name(s) of core(s)", nargs = "*")
    args = parser.parse_args()

    try:
        cfg = RawConfigParser()
        cfg.readfp(args.config)

        board = args.board or cfg.get("default", "board")
        board_section = "board " + board
        Core.bus_name = cfg.get(board_section, "bus name")
        Core.bus_width = int(cfg.get(board_section, "bus width"))
        Core.bus_max = Core.bus_width - 1
        Core.addr_width = Core.bus_width - 8
        Core.addr_max = Core.addr_width - 1
        Core.extra_wires = cfg.get(board_section, "extra wires")
        Core.modexp = cfg.get(board_section, "modexp")
        if Core.extra_wires:
            # restore formatting
            Core.extra_wires = Core.extra_wires.replace("\n", "\n   ") + "\n"

        if args.core:
            cores = args.core
        else:
            project = args.project or cfg.get("default", "project")
            cores = cfg.get("project " + project, "cores").split()

        for core in cfg.getvalues(board_section, "requires"):
            try:
                (c1, c2) = core.split("/")
                if c1 not in cores and c2 not in cores:
                    cores.append(c2)
            except ValueError:
                if core not in cores:
                    cores.append(core)
                

        cores.insert(0, "board_regs")
        cores.insert(1, "comm_regs")

        cores = tuple(Core(core) for core in cores)

        for core in cores:
            core.configure(cfg)

        core_number = 0
        for core in cores:
            core_number = core.assign_core_number(core_number)

        if False:

            # For some reason, attempting to optimize out the delay
            # code entirely results in a non-working bitstream.  Don't
            # know why, disabling the optimization works, so just do
            # that for now.

            Core.need_one_cycle_delay = any(core.block_memory for core in cores)

        args.verilog.write(createModule_template.format(
            core = cores[0],
            addrs = "".join(core.createAddr()     for core in cores),
            insts = "".join(core.createInstance() for core in cores),
            muxes = "".join(core.createMux()      for core in cores) ))

        args.makefile.write(listVfiles_template.format(
            vfiles = "".join(core.listVfiles()    for core in cores)))

    except Exception, e:
        if args.debug:
            raise
        exit(str(e))


try:
    import ConfigParser as configparser
except ImportError:
    import configparser   

class RawConfigParser(configparser.RawConfigParser):
    """
    RawConfigParser with a few extensions.
    """

    def getboolean(self, section, option, default = False):
        if self.has_option(section, option):
            # RawConfigParser is an old-stle class, super() doesn't work, feh.
            return configparser.RawConfigParser.getboolean(self, section, option)
        else:
            return default

    def getvalues(self, section, option):
        if self.has_option(section, option):
            for value in self.get(section, option).split():
                yield value

    def get(self, section, option, default = ""):
        try:
            return configparser.RawConfigParser.get(self, section, option)
        except configparser.NoSectionError:
            if section in ("core board_regs", "core comm_regs"):
                return default
            else:
                raise
        except configparser.NoOptionError:
            return default


class Core(object):
    """
    Data and methods for a generic core.  We can use this directly for
    most cores, a few are weird and require subclassing to override
    particular methods.
    """

    # Class variable tracking how many times a particular core has
    # been instantiated.  This controls instance numbering.

    _instance_count = {}

    # Class variable recording whether we need a one-cycle delay to
    # compensate for block memories.

    need_one_cycle_delay = True

    def __init__(self, name):
        if Core.modexp and name == "modexp":
            name = Core.modexp
        self.name = name
        self.cfg_section = "core " + name
        self.core_number = None
        self.vfiles = []
        self.error_wire = True
        self.block_memory = False
        self.instance_number = self._instance_count.get(name, 0)
        self._instance_count[name] = self.instance_number + 1
        self.subcores = []
        self.blocks = 1
        self.dummy = False
        self._parameters = dict()
        self.reset_name = "reset_n"


    def assign_core_number(self, n):
        self.core_number = n
        for i, subcore in enumerate(self.subcores):
            subcore.assign_core_number(n + i + 1)
        return n + self.blocks

    def configure(self, cfg):
        if self.instance_number == 0:
            self.vfiles.extend(cfg.getvalues(self.cfg_section, "vfiles"))
            for required in cfg.getvalues(self.cfg_section, "requires"):
                if required not in self._instance_count:
                    self.vfiles.extend(cfg.getvalues("core " + required, "vfiles"))
        self.error_wire = cfg.getboolean(self.cfg_section, "error wire", self.error_wire)
        self.block_memory = cfg.getboolean(self.cfg_section, "block memory", self.block_memory)
        self.extra_ports = cfg.get(self.cfg_section, "extra ports")
        if self.extra_ports:
            self.extra_ports = self.extra_ports.replace("\n", "\n      ") + "\n"
        self.blocks = int(cfg.get(self.cfg_section, "core blocks") or 1)
        self.block_max = self.blocks - 1
        if self.blocks > 1:
            try:
                self.block_bits = {4:2, 8:3, 16:4}[self.blocks]
            except KeyError:
                raise ValueError, "In [{}]: unexpected value \"core blocks = {}\"".format(self.cfg_section, self.blocks)
            self.block_bit_max = self.block_bits - 1
        for subcore in cfg.getvalues(self.cfg_section, "subcores"):
            self.subcores.append(SubCore(subcore, self))
        if len(self.subcores) > self.blocks - 1:
            raise ValueError, "In [{}]: number of subcores exceeds size of \"core blocks\"".format(self.cfg_section)
        self.module_name = cfg.get(self.cfg_section, "module name") or self.name
        self.dummy = cfg.get(self.cfg_section, "dummy")
        if self.dummy:
            self.dummy = self.dummy.replace("\n", "\n   ") + "\n"
        if cfg.has_section(self.cfg_section):
            for option in cfg.options(self.cfg_section):
                if option.startswith("parameter "):
                    self._parameters[option[len("parameter"):].upper().strip()] = cfg.get(self.cfg_section, option)
        self.reset_name = cfg.get(self.cfg_section, "reset name", self.reset_name)

    @property
    def instance_name(self):
        if self._instance_count[self.name] > 1:
            return "{}_{}".format(self.name, self.instance_number)
        else:
            return self.name

    @property
    def upper_instance_name(self):
        return self.instance_name.upper()

    @property
    def error_wire_decl(self):
        return "\n   wire                 error_{core.instance_name};".format(core = self) if self.error_wire else ""

    @property
    def error_port(self):
        return ",\n      .error(error_{core.instance_name})".format(core = self) if self.error_wire else ""

    @property
    def one_cycle_delay(self):
        return one_cycle_delay_template.format(core = self) if self.need_one_cycle_delay and not self.block_memory else ""

    @property
    def mux_core_addr(self):
        if self.blocks == 1 or self.subcores:
            return "CORE_ADDR_{core.upper_instance_name}".format(core=self)
        else:
            return ",\n       ".join("CORE_ADDR_{core.upper_instance_name} + {0}".format(i, core=self) for i in range(self.blocks)) 

    @property
    def mux_data_reg(self):
        return "read_data_" + self.instance_name + ("_reg" if self.need_one_cycle_delay and not self.block_memory else "")

    @property
    def mux_error_reg(self):
        return "error_" + self.instance_name if self.error_wire else "0"

    @property
    def parameters(self):
        if self._parameters:
            return "#( {} ) ".format(", ".join(".{} ({})".format(k, v) for k, v in self._parameters.iteritems()))
        else:
            return ""

    def createInstance(self):
        template = createInstance_template_dummy if self.dummy else createInstance_template_generic if self.blocks == 1 else createInstance_template_multi_block
        return template.format(core = self)

    def createAddr(self):
        if self.dummy:
            return ""
        return createAddr_template.format(core = self) + "".join(subcore.createAddr() for subcore in self.subcores)

    def createMux(self):
        if self.dummy:
            return ""
        return createMux_template.format(core = self, core0 = self) + "".join(subcore.createMux() for subcore in self.subcores)

    def listVfiles(self):
        return "".join(" \\\n\t$(CORE_TREE)/" + vfile for vfile in self.vfiles)


class SubCore(Core):
    """"
    Override mux handling for TRNG's sub-cores.
    """

    def __init__(self, name, parent):
        super(SubCore, self).__init__(name)
        self.parent = parent

    def createMux(self):
        return createMux_template.format(core = self, core0 = self.parent)


# Templates (format strings), here instead of inline in the functions
# that use them, both because some of these are shared between
# multiple functions and because it's easier to read these (and get
# the indentation right) when the're separate.

# Template used by .createAddr() methods.

createAddr_template = """\
   localparam   CORE_ADDR_{core.upper_instance_name:21s} = {core.addr_width}'h{core.core_number:02x};
"""

# Template used by Core.createInstance().

createInstance_template_generic = """\
   //----------------------------------------------------------------
   // {core.upper_instance_name}
   //----------------------------------------------------------------
   wire                 enable_{core.instance_name} = (addr_core_num == CORE_ADDR_{core.upper_instance_name});
   wire [31: 0]         read_data_{core.instance_name};{core.error_wire_decl}

   {core.module_name} {core.parameters}{core.instance_name}_inst
     (
      .clk(sys_clk),
      .{core.reset_name}(sys_rst_n),
{core.extra_ports}
      .cs(enable_{core.instance_name} & (sys_{core.bus_name}_rd | sys_{core.bus_name}_wr)),
      .we(sys_{core.bus_name}_wr),

      .address(addr_core_reg),
      .write_data(sys_write_data),
      .read_data(read_data_{core.instance_name}){core.error_port}
      );

{core.one_cycle_delay}

"""

# Template used for multi-block cores (modexp and trng).  This is different
# enough from the base template that it's easier to make this separate.

createInstance_template_multi_block = """\
   //----------------------------------------------------------------
   // {core.upper_instance_name}
   //----------------------------------------------------------------
   wire                 enable_{core.instance_name} = (addr_core_num >= CORE_ADDR_{core.upper_instance_name}) && (addr_core_num <= CORE_ADDR_{core.upper_instance_name} + {core.addr_width}'h{core.block_max:02x});
   wire [31: 0]         read_data_{core.instance_name};{core.error_wire_decl}
   wire [{core.block_bit_max}:0]           {core.instance_name}_prefix = addr_core_num[{core.block_bit_max}:0] - CORE_ADDR_{core.upper_instance_name};

   {core.module_name} {core.parameters}{core.instance_name}_inst
     (
      .clk(sys_clk),
      .{core.reset_name}(sys_rst_n),
{core.extra_ports}
      .cs(enable_{core.instance_name} & (sys_{core.bus_name}_rd | sys_{core.bus_name}_wr)),
      .we(sys_{core.bus_name}_wr),

      .address({{{core.instance_name}_prefix, addr_core_reg}}),
      .write_data(sys_write_data),
      .read_data(read_data_{core.instance_name}){core.error_port}
      );

{core.one_cycle_delay}

"""

createInstance_template_dummy = """\
   //----------------------------------------------------------------
   // {core.upper_instance_name}
   //----------------------------------------------------------------
{core.dummy}
"""

# Template for one-cycle delay code.

one_cycle_delay_template = """\
   reg  [31: 0] read_data_{core.instance_name}_reg;
   always @(posedge sys_clk)
     read_data_{core.instance_name}_reg <= read_data_{core.instance_name};
"""

# Template for .createMux() methods.

createMux_template = """\
       {core.mux_core_addr}:
         begin
            sys_read_data_mux = {core0.mux_data_reg};
            sys_error_mux = {core0.mux_error_reg};
         end
"""

# Top-level (createModule) template.

createModule_template = """\
// NOTE: This file is generated; do not edit.

module core_selector
  (
   input wire          sys_clk,
   input wire          sys_rst_n,

   input wire [{core.bus_max}: 0]  sys_{core.bus_name}_addr,
   input wire          sys_{core.bus_name}_wr,
   input wire          sys_{core.bus_name}_rd,
   output wire [31: 0] sys_read_data,
   input wire [31: 0]  sys_write_data,
   output wire         sys_error,
{core.extra_wires}
   input wire          noise,
   output wire [7 : 0] debug
   );


   //----------------------------------------------------------------
   // Address Decoder
   //----------------------------------------------------------------
   // upper {core.addr_width} bits specify core being addressed
   wire [{core.addr_max:>2}: 0]         addr_core_num   = sys_{core.bus_name}_addr[{core.bus_max}: 8];
   // lower 8 bits specify register offset in core
   wire [ 7: 0]         addr_core_reg   = sys_{core.bus_name}_addr[ 7: 0];


   //----------------------------------------------------------------
   // Core Address Table
   //----------------------------------------------------------------
{addrs}

{insts}
   //----------------------------------------------------------------
   // Output (Read Data) Multiplexer
   //----------------------------------------------------------------
   reg [31: 0]          sys_read_data_mux;
   assign               sys_read_data = sys_read_data_mux;
   reg                  sys_error_mux;
   assign               sys_error = sys_error_mux;

   always @*

     case (addr_core_num)
{muxes}
       default:
         begin
            sys_read_data_mux = {{32{{1'b0}}}};
            sys_error_mux = 1;
         end
     endcase


endmodule


//======================================================================
// EOF core_selector.v
//======================================================================
"""

# Template for makefile snippet listing Verilog source files.

listVfiles_template = """\
# NOTE: This file is generated; do not edit by hand.

vfiles +={vfiles}
"""

# Run main program.

if __name__ == "__main__":
    main()
