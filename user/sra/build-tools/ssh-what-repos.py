#!/usr/bin/env python
#
# List Cryptech git repositories via SSH by asking gitolite for a JSON
# listing.  Not useful unless you have an SSH account, sorry, and not
# really all that much more interesting than normal gitolite output.

from json       import loads
from subprocess import check_output

for repo in sorted(loads(check_output("ssh git@git.cryptech.is info -json -lc".split()))["repos"]):
    if repo != "gitolite-admin" and not any(c in repo for c in "*?[]"):
        print repo
