#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#
import os
import sys

"""
This script tests SIGSEGV and SIGFPE crash handling 
"""
if len(sys.argv) != 3:
    print "Usage: testTfCrashHandler <crash_script> <crash_signal>"
    sys.exit(1)

print "=== BEGIN EXPECTED ERROR ==="
cmd = os.path.join(os.path.dirname(os.path.abspath(sys.argv[0])), sys.argv[1])
sin, sout = os.popen4(cmd)
output = " ".join(sout.readlines())
print output
print "=== END EXPECTED ERROR ==="

assert sys.argv[2] in output, "'%s' not found in crash output" % sys.argv[2]

