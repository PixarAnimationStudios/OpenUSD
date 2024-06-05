#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
#
# Usage:
#   shebang shebang-str source.py dest.py
#   shebang file output.cmd
#
# The former substitutes '/pxrpythonsubst' in <source.py> with <shebang-str>
# and writes the result to <dest.py>.  The latter generates a file named
# <output.cmd> with the contents '@python "%~dp0<file>"';  this is a
# Windows command script to execute "python <file>" where <file> is in
# the same directory as the command script.

from __future__ import print_function
import sys

if len(sys.argv) < 3 or len(sys.argv) > 4:
    print("Usage: %s {shebang-str source.py dest|file output.cmd}" % sys.argv[0])
    sys.exit(1)

if len(sys.argv) == 3:
    with open(sys.argv[2], 'w') as f:
        print('@python "%%~dp0%s" %%*' % (sys.argv[1], ), file=f)

else:
    with open(sys.argv[2], 'r') as s:
        with open(sys.argv[3], 'w') as d:
            for line in s:
                d.write(line.replace('/pxrpythonsubst', sys.argv[1]))
