#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

# Usage: python sortDotFiles.py <dotFile1> <dotFile2> ...
#
# This script reads each *.dot file passed on the command line, sorts the lines
# in each file, and writes the sorted lines to a new file. Each new file has the
# same name as its input file, with ".out" appended to the end.

import sys

dotFiles = sys.argv[1:]

for dotFile in dotFiles:
    print("Sorting", dotFile)
    with open(dotFile, "r") as f:
        lines = f.readlines()
    with open(dotFile + ".out", "w") as f:
        # We strip the line-ending before the comparison to ensure that empty
        # lines ("\n") get sorted before lines beginning with a tab
        # (e.g. "\tLineText\n")
        sortedLines = sorted(lines, key=lambda l: l.rstrip("\r\n"))
        for line in sortedLines:
            f.write(line)
