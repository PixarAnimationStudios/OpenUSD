#
# Copyright 2022 Pixar
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
import platform
import sys
from distutils.spawn import find_executable

def FindUsdBinary(name):
    """Returns the full path to the named executable if it can be found, or
    None if the executable cannot be located. This first searches in PATH, and
    if the executable is not found, it then searches in the parent directory
    of the current process, as identified by sys.argv[0].

    On Windows, this function searches for both name.EXE and name.CMD to
    ensure that CMD-wrapped executables are located if they exist.
    """

    # First search PATH
    binpath = find_executable(name)
    if binpath:
        return binpath

    # Then look relative to the current executable
    binpath = find_executable(name,
        path=os.path.abspath(os.path.dirname(sys.argv[0])))
    if binpath:
        return binpath

    if platform.system() == 'Windows':
        # find_executable under Windows only returns *.EXE files so we need to
        # traverse the tool path.
        path = os.environ.get('PATH', '').split(os.pathsep)
        for base in [os.path.join(p, name) for p in path]:
            # We need to test for name.cmd first because on Windows, the USD
            # executables are wrapped due to lack of UNIX style shebang support.
            for binpath in [base + ext for ext in ('.cmd', '')]:
                if os.access(binpath, os.X_OK):
                    return binpath

    return None
