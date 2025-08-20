#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

# Usage: python normalizeDotFiles.py <inputFiles>...
#
# This script reads each input file, replaces all hex values with 'xxxxxxx',
# sorts the lines, and writes the sorted lines to an output file with the same
# name, but its extension is replaced with '.out'

import re
import sys
import os

inputFiles = sys.argv[1:]
outputFile = sys.argv[2]

pattern = re.compile(r'0x\w*')
replacement = 'xxxxxxx'

for inputFile in inputFiles:
    outputFile = os.path.splitext(inputFile)[0] + '.out'
    
    # Read the input file and replace the hex values with 'xxxxxxx'.
    lines = []
    with open(inputFile, "r") as f:
        for line in f:
            updatedLine = pattern.sub(replacement, line)
            lines.append(updatedLine)

    # Sort the lines and write them to the output file. Compare lines without
    # considering the line endings, so empty lines ("\n") get sorted first.
    lines = sorted(lines, key=lambda l: l.rstrip("\r\n"))
    with open(outputFile, "w") as f:
        for line in lines:
            f.write(line)