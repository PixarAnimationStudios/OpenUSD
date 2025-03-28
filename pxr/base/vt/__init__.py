#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
"""Vt Value Types library

This package defines classes for creating objects that behave like value
types.  It contains facilities for creating simple copy-on-write
implicitly shared types.
"""

from pxr import Tf
Tf.PreparePythonModule()
del Tf

# For ease-of-use, put each XXXArrayFromBuffer method on its corresponding array
# wrapper class, also alias it as 'FromNumpy' for compatibility.
def _CopyArrayFromBufferFuncs(moduleContents):
    funcs = dict([(key, val) for (key, val) in moduleContents.items()
                  if key.endswith('FromBuffer')])
    classes = dict([(key, val) for (key, val) in moduleContents.items()
                    if key.endswith('Array') and isinstance(val, type)])

    for funcName, func in funcs.items():
        className = funcName[:-len('FromBuffer')]
        cls = classes.get(className)
        if cls:
            setattr(cls, 'FromBuffer', staticmethod(func))
            setattr(cls, 'FromNumpy', staticmethod(func))

_CopyArrayFromBufferFuncs(dict(vars()))
del _CopyArrayFromBufferFuncs
