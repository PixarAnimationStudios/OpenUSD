#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
#
# Usage:
#   shebang shebang-str source.py dest.py
#
# This substitutes '/pxrpythonsubst' in <source.py> with <shebang-str>
# and writes the result to <dest.py>. 

from __future__ import print_function
import sys

if len(sys.argv) != 4:
    print("Usage: %s {shebang-str source.py dest}" % sys.argv[0])
    sys.exit(1)
else:
    with open(sys.argv[2], 'r') as s:
        with open(sys.argv[3], 'w') as d:
            for line in s:
                d.write(line.replace('/pxrpythonsubst', sys.argv[1]))
