#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from __future__ import print_function

import os
import sys

from subprocess import Popen, PIPE, STDOUT

"""
This script tests SIGSEGV and SIGFPE crash handling 
"""
if len(sys.argv) != 3:
    print("Usage: testTfCrashHandler <crash_script> <crash_signal>")
    sys.exit(1)

print("=== BEGIN EXPECTED ERROR ===")
cmd = os.path.join(os.path.dirname(os.path.abspath(sys.argv[0])), sys.argv[1])
p = Popen(cmd, shell=True, stdin=PIPE, stdout=PIPE, stderr=STDOUT)
output = " ".join([line.decode() for line in p.stdout.readlines()])
print(output)
print("=== END EXPECTED ERROR ===")

assert sys.argv[2] in output, "'%s' not found in crash output" % sys.argv[2]

