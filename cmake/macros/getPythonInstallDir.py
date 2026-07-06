#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

# Prints the platform-specific Python site-packages directory where the USD
# Python modules will be installed by default. This is a relative path,
# e.g. "lib/python3.9/site-packages", that will be anchored to the library
# install prefix.

import pathlib
import sys
import sysconfig

sitePackagePath = None

# Use the "platlib" path relative to sys.exec_prefix. These are where
# packages that are not pure Python are meant to be installed and
# should handle most cases, including installing into a virtualenv.
#
# For reference:
# https://packaging.python.org/en/latest/specifications/binary-distribution-format/#what-s-the-deal-with-purelib-vs-platlib
# https://discuss.python.org/t/understanding-site-packages-directories/12959
try:
    sitePackagePath = pathlib.PurePath(
        sysconfig.get_path("platlib")).relative_to(sys.exec_prefix)
except ValueError:
    # If the "platlib" path is not relative to sys.exec_prefix, fallback
    # to "lib/pythonX.Y/site-packages".
    #
    # One place this shows up is on MacOS when using the Python interpreter
    # installed with the Xcode Command Line Tools. Note that this Python
    # isn't intended for general development.
    major, minor = sys.version_info.major, sys.version_info.minor
    sitePackagePath = pathlib.PurePath(
        f"lib/python{major}.{minor}/site-packages")

# Ensure we have a relative path; we should never have an absolute path
# here since by default we want to install the modules somewhere within
# the USD install prefix.
if sitePackagePath.is_absolute():
    sys.exit(f"ERROR: Expected relative path, got {sitePackagePath}")

print(sitePackagePath)
