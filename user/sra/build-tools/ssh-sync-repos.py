#!/usr/bin/env python
#
# Synchronize Cryptech git repositories via SSH by asking gitolite for a JSON
# listing of repositories.  Not useful unless you have an SSH account, sorry.

from subprocess import check_call, check_output
from os.path    import isdir
from json       import loads
from sys        import exit

user = "git@git.cryptech.is"
cmd  = "ssh {} info -json -lc".format(user)
info = loads(check_output(cmd.split()))
errs = 0

for repo in sorted(info["repos"]):
    try:
        if all(c not in repo for c in "*?[]"):
            print
            print repo
            pull = isdir(repo)
            if pull:
                check_call(("git", "fetch", "--all", "--prune"), cwd = repo)
                check_call(("git", "pull"), cwd = repo)
            else:
                check_call(("git", "clone", "{}:{}.git".format(user, repo), repo))
    except:
        print "** Error", "pulling" if pull else "cloning", repo
        errs += 1

exit(errs)
