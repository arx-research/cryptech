#!/usr/bin/env python
#
# Perform some command for every repository in a tree

from subprocess import call
from os         import walk
from argparse   import ArgumentParser
from sys        import exit, stdout

parser = ArgumentParser()
parser.add_argument("-t", "--tree", default = ".", help = "repository tree to walk")
parser.add_argument("-v", "--verbose", action = "store_true", help = "whistle while you work")
parser.add_argument("command", nargs = "+", help = "command to run in each repository")
args = parser.parse_args()

def log(msg):
    if args.verbose:
        stdout.write(msg)
        stdout.write("\n")
        stdout.flush()

status = 0

for head, dirs, files in walk(args.tree):
    if ".git" in dirs and not head.endswith("/gitolite"):
        log(head)
        status |= call(args.command, cwd = head)

exit(status)
