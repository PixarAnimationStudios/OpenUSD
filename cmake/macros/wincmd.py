#
# Copyright 2022 Anthony Tan
#
# Licensed under the Apache License, Version 2.0 (the "Apache License").
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
#
# Usage:
#   wincmd file output.cmd
#   wincmd python_interp file output.cmd
#
# Both generate a file named <output.cmd>, in the first form, this mimics
# the old behaviour of shebang.py and generates the file with the contents 
# '@python "%~dp0<file>"'; this is a Windows command script to execute 
# "python <file>" where <file> is in the same directory as the command script.
# The second form instead uses the name supplied as the python interpreter and 
# is intended to allow the windows command to invoke things like mayapy.exe

from __future__ import print_function
import sys

if len(sys.argv) < 3 or len(sys.argv) > 4:
    print("Usage: %s {python-interpreter file output.cmd|file output.cmd}" % sys.argv[0])
    sys.exit(1)

if len(sys.argv) == 3:
    with open(sys.argv[2], 'w') as f:
        print('@python "%%~dp0%s" %%*' % (sys.argv[1], ), file=f)

if len(sys.argv) == 4:
    with open(sys.argv[3], 'w') as f:
        print('@%s "%%~dp0%s" %%*' % (sys.argv[1], sys.argv[2], ), file=f)

