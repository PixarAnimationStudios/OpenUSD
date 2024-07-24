#
# Copyright 2021 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Tf

def _NoArgs():
    return "_NoArgs result"

def _ReturnInt():
    return 42

def _RepeatStrings(stringList, numRepeats):
    return [s * numRepeats for s in stringList]

def _ConcatWithList(arg1, arg2, *args):
    return str.join(
        " ",
        [str(a) for a in
            [arg1, arg2]
            + list(args)])

def _ConcatWithKwArgs(arg1, arg2, arg3 = "c", arg4 = "d", **kwargs):
    return str.join(
        " ",
        [str(a) for a in
            [arg1, arg2, arg3, arg4]
            + ["%s=%s" % (k, kwargs[k]) for k in sorted(kwargs.keys())]])

def _GetTheeToANonery(arg):
    assert arg is None

_globalVar = 88

def _RaiseException():
    raise Exception("test exception")

def _RaiseTfError():
    Tf.RaiseRuntimeError("test error")
