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

import sys

if len(sys.argv) != 4:
    print(f"Usage: {sys.argv[0]} {{shebang-str source.py dest}}",
          file=sys.stderr)
    sys.exit(1)

with open(sys.argv[2], 'r') as s:
    with open(sys.argv[3], 'w') as d:
        for line in s:
            d.write(line.replace('/pxrpythonsubst', sys.argv[1]))
