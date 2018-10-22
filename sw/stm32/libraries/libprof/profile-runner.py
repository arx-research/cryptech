#!/usr/bin/env python

"""
Tool to run some test code under the profiler on the Cryptech Alpha.

This assumes that the HSM code was built with DO_PROFILING=1, and
requires an ST-LINK programmer and the Python pexpect package.
"""

import subprocess
import argparse
import pexpect
import atexit
import time
import sys
import os

parser = argparse.ArgumentParser(description = __doc__,
                                 formatter_class = argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument("--hsm-elf",
                    default = os.path.expanduser("~/git.cryptech.is/sw/stm32/projects/hsm/hsm.elf"),
                    help    = "where you keep the profiled hsm.elf binary")
parser.add_argument("--openocd-config",
                    default = "/usr/share/openocd/scripts/board/st_nucleo_f401re.cfg",
                    help    = "OpenOCD ST-LINK configuration file ")
parser.add_argument("--gmon-output",
                    default = "profile-runner.gmon",
                    help    = "where to leave raw profiler output")
parser.add_argument("--gprof-output", type = argparse.FileType("w"),
                    default = "profile-runner.gprof",
                    help    = "where to leave profiler output after processing with gprof")
parser.add_argument("--user",
                    default = "wheel",
                    help    = "user name for logging in on the HSM console")
parser.add_argument("--pin",
                    default = "fnord",
                    help    = "PIN for logging in on the HSM console")
parser.add_argument("command", nargs = 1,
                    help    = "test program to run with profiling")
parser.add_argument("arguments", nargs = argparse.REMAINDER,
                    help    = argparse.SUPPRESS)
args = parser.parse_args()

openocd = subprocess.Popen(("openocd", "-f", args.openocd_config))
atexit.register(openocd.terminate)

time.sleep(5)

telnet = pexpect.spawn("telnet localhost 4444")
telnet.expect(">")
telnet.sendline("arm semihosting enable")
telnet.expect(">")
telnet.sendline("exit")

console = pexpect.spawn("cryptech_console")
console.sendline("")
if console.expect(["cryptech>", "Username:"]):
    console.sendline(args.user)
    console.expect("Password:")
    console.sendline(args.pin)
    console.expect("cryptech>")
console.sendline("profile start")
console.expect("cryptech>")

cmd = args.command + args.arguments
sys.stderr.write("Running command: {}\n".format(" ".join(cmd)))
subprocess.check_call(cmd)

console.sendline("profile stop")
console.expect("cryptech>", timeout = 900)
os.rename("gmon.out", args.gmon_output)

subprocess.check_call(("gprof", args.hsm_elf, args.gmon_output), stdout = args.gprof_output)
