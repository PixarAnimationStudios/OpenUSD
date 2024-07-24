#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
# cdUtils.py
#
# Various utility functions for the convertDoxygen utility, including
# standard error/debug reporting, command line parsing, and module
# importing.
#

import os
import sys
import inspect
import traceback

__debugMode = True

ATTR_NOT_IN_PYTHON = 'notinpython'
ATTR_STATIC_METHOD = 'staticmethod'

LABEL_STATIC = '**classmethod** '

def Error(msg):
    """Output a fatal error message and exit the program."""
    print("Error: %s" % msg, flush=True)
    if __debugMode:
        traceback.print_stack()
        sys.stderr.flush()
    sys.exit(1)

def Warn(msg):
    """Output a nonfatal error message."""
    print("Warning: %s" % msg)

def Debug(msg):
    """Output a debug message if debug mode is on."""
    if __debugMode:
        print("Debug: %s" % msg)

def SetDebugMode(debugOn):
    """Turn debug mode on or off. Default is off."""
    global __debugMode
    __debugMode = debugOn

def GetArg(optnames, default=False):
    """Return True if any of the args exist in sys.argv."""
    if not isinstance(optnames, type([])):
        optnames = [optnames]
    for arg in sys.argv:
        if arg in optnames:
            return True
    return default

def GetArgValue(optnames, default=None):
    """Return the value following the specified arg in sys.argv."""
    if not isinstance(optnames, type([])):
        optnames = [optnames]
    found = False
    for arg in sys.argv:
        if found:
            return arg
        if arg in optnames:
            found = True
    return default

def Usage():
    """Output the program usage statement and exit the program."""
    progname = os.path.basename(sys.argv[0])

    usageMsg = """
%s - translate Doxygen XML comments

This utility will convert the XML format that Doxygen creates
for C++ classes into Python docstring output format.
A simple plugin scheme lets you add new output formats.

Usage: %s [global-options] [format-options]

Global options:
  --input or -i   = the location of the input Doxygen XML file
  (or) --inputIndex = the location of the input Doxygen index.xml file
  --output or -o  = the name of the output file (or directory)
                    If multiple modules are provided via --module,
                    this is treated as the output directory for the
                    installed python libs, e.g. usd/lib/python/pxr
  --format or -f  = the output format (default=Docstring)
  --debug or -d   = turn on debugging mode
  --help or -h    = display this program usage statement
  --pythonPath    = optional path to add to python lib paths
  --dllPath       = optional ;-separated paths to add to Window's list of
                    directories you may load .dll libraries from. Ignored if not
                    on Windows.

Docstring format:
  Write Python doc strings from Doyxgen C++ comments. Writes
  output to file or directory as specified by --output option.

  --package or -p = the package name, e.g. pxr
  --module or -m  = the module name, e.g. UsdGeom, or a list of
                    comma-separated modules, e.g. Usd,UsdGeom,UsdShade
    """ % (progname, progname)
    print(usageMsg)
    sys.exit(1)
