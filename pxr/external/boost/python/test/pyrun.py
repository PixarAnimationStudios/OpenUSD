#!/pythonsubst
#
# Copyright 2024 Pixar
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
import os
import platform
import sys

# On Windows and Python 3.8+ we need to manually update the DLL search
# path to include PATH manually. See WindowsImportWrapper in tf/__init__.py
# for more details.
if sys.version_info >= (3, 8) and platform.system() == "Windows":
    import_paths = os.getenv('PXR_USD_WINDOWS_DLL_PATH')
    if import_paths is None:
        import_paths = os.getenv('PATH', '')

    for path in reversed(import_paths.split(os.pathsep)):
        # Calling add_dll_directory raises an exception if paths don't
        # exist, or if you pass in dot
        if os.path.exists(path) and path != '.':
            abs_path = os.path.abspath(path)
            os.add_dll_directory(abs_path)

# Test scripts are installed next to ourself, while the corresponding
# Python modules are in the "lib" subdirectory.
testsDir = os.path.dirname(os.path.abspath(__file__))

pythonpath = os.path.join(testsDir, "lib")
scriptfile = os.path.join(testsDir, sys.argv[1])
sys.argv = sys.argv[1:]
sys.path.append(pythonpath)
exec(compile(open(scriptfile).read(), scriptfile, 'exec'))
