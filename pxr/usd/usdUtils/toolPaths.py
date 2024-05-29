#
# Copyright 2022 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
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
