#!/usr/bin/env python
import sys
import glob
import os
import re

for ifile in ("test/Jamfile", "examples/Jamfile"):
    dir = os.path.dirname(ifile)

    cpp = set([os.path.basename(x) for x in glob.glob(dir + "/*.cpp")])
    run = set(re.findall("([a-zA-Z0-9_]+\.cpp)", open(ifile).read()))

    diff = cpp - run

    if diff:
        raise SystemExit("NOT TESTED\n" +
                         "\n".join(["%s/%s" % (dir, x) for x in diff]))
