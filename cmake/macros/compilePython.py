#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
#
# Usage: compilePython source.py dest.pyc
#
# This program compiles python code, providing a reasonable
# gcc-esque error message if errors occur.
#
# parameters:
# src.py - the source file to report errors for
# file.py - the installed location of the file
# file.pyc - the precompiled python file

from __future__ import print_function

import sys
import py_compile

if len(sys.argv) < 4:
    print("Usage: {} src.py file.py file.pyc".format(sys.argv[0]))
    sys.exit(1)

try:
    py_compile.compile(sys.argv[2], sys.argv[3], sys.argv[1], doraise=True)
except py_compile.PyCompileError as compileError:
    exc_value = compileError.exc_value
    if compileError.exc_type_name == SyntaxError.__name__:
        # py_compile.compile stashes the type name and args of the exception
        # in the raised PyCompileError rather than the exception itself.  This
        # is especially annoying because the args member of some SyntaxError
        # instances are lacking the source information tuple, but do have a
        # usable lineno.
        error = exc_value[0]
        try:
            linenumber = exc_value[1][1]
            line = exc_value[1][3]
            print('{}:{}: {}: "{}"'.format(sys.argv[1], linenumber, error, line))
        except IndexError:
            print('{}: Syntax error: "{}"'.format(sys.argv[1], error))
    else:
        print("{}: Unhandled compile error: ({}) {}".format(
            sys.argv[1], compileError.exc_type_name, exc_value))
    sys.exit(1)
except:
    exc_type, exc_value, exc_traceback = sys.exc_info()
    print("{}: Unhandled exception: {}".format(sys.argv[1], exc_value))
    sys.exit(1)
