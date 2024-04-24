#
# Copyright 2017 Pixar
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
#
# Usage:
#   wrappy file output.cmd
#
# This generates a file named <output.cmd> with the contents
# '@python "%~dp0<file>"';  this is a Windows command script to execute
# "python <file>" where <file> is in the same directory as the command script.

import sys
import platform
import warnings

if not platform.system() == 'Windows':
    warnings.warn(f"{sys.argv[0]} should only be run on Windows")

if len(sys.argv) != 3:
    print(f"Usage: {sys.argv[0]} {{file output.cmd}}", file=sys.stderr)
    sys.exit(1)

with open(sys.argv[2], 'w') as f:
    print(f'@python "%~dp0{sys.argv[1]}" %*', file=f)
