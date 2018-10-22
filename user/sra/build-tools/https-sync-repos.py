#!/usr/bin/env python

# Synchronize Cryptech git repositories by scraping URLs from the
# automatically generated Trac page.  Yes, we know too much about how
# the generation script and Trac format this page, c'est la vie.

from urllib                     import urlopen
from xml.etree.ElementTree      import ElementTree
from subprocess                 import check_call
from os.path                    import isdir
from sys                        import exit

# URL for automatically generated Trac page listing repositories.

trac_page = "https://wiki.cryptech.is/wiki/GitRepositories"

head = "https://git.cryptech.is/"
tail = ".git"
errs = 0

for elt in ElementTree(file = urlopen(trac_page)).iter("{http://www.w3.org/1999/xhtml}code"):
    if elt.text.startswith(head) and elt.text.endswith(tail):
        url  = elt.text
        repo = url[len(head):-len(tail)]
        pull = isdir(repo)
        print
        print url
        try:
            if pull:
                check_call(("git", "fetch", "--all", "--prune"), cwd = repo)
                check_call(("git", "pull"), cwd = repo)
            else:
                check_call(("git", "clone", url, repo))
        except:
            print "** Error", "pulling" if pull else "cloning", repo
            errs += 1

exit(errs)
