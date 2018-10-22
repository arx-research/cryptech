#!/usr/bin/env python

# List Cryptech git repository URLs by scraping them from the
# automatically generated Trac page.  Yes, we know too much about how
# the generation script and Trac format this page, c'est la vie.

from urllib                     import urlopen
from xml.etree.ElementTree      import ElementTree

url = "https://wiki.cryptech.is/wiki/GitRepositories"

for x in ElementTree(file = urlopen(url)).iter("{http://www.w3.org/1999/xhtml}code"):
    if x.text.startswith("https://git.cryptech.is/") and x.text.endswith(".git"):
        print x.text
