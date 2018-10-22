#!/usr/bin/env python
#
# List branches in local repository tree.

from subprocess import check_output
from os         import walk, listdir
from sys        import argv

for root in argv[1:] or listdir("."):
    for head, dirs, files in walk(root):
        if ".git" in dirs and not head.endswith("/gitolite"):
            print head
            for line in check_output(("git", "branch", "-a"), cwd = head).splitlines():
                print " ", line
