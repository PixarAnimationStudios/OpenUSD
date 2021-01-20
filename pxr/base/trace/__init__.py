#
# Copyright 2018 Pixar
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
"""
Trace -- Utilities for counting and recording events.
"""

from . import _trace
from pxr import Tf
Tf.PrepareModule(_trace, locals())
del _trace, Tf

import contextlib

@contextlib.contextmanager
def TraceScope(label):
    """A context manager that calls BeginEvent on the global collector on enter 
    and EndEvent on exit."""
    try:
        collector = Collector()
        eventId = collector.BeginEvent(label)
        yield
    finally:
        collector.EndEvent(label)

del contextlib

def TraceFunction(obj):
    """A decorator that enables tracing the function that it decorates.
    If you decorate with 'TraceFunction' the function will be traced in the
    global collector."""
    
    collector = Collector()
    
    def decorate(func):
        import inspect

        if inspect.ismethod(func):
            callableTypeLabel = 'method'
            classLabel = func.__self__.__class__.__name__+'.'
        else:
            callableTypeLabel = 'func'
            classLabel = ''

        module = inspect.getmodule(func)
        if module is not None:
            moduleLabel = module.__name__+'.'
        else:
            moduleLabel = ''

        label = 'Python {0}: {1}{2}{3}'.format(
            callableTypeLabel,
            moduleLabel,
            classLabel,
            func.__name__)
        def invoke(*args, **kwargs):
            with TraceScope(label):
                return func(*args, **kwargs)

        invoke.__name__ = func.__name__
        # Make sure wrapper function gets attributes of wrapped function.
        import functools
        return functools.update_wrapper(invoke, func)

    return decorate(obj)

def TraceMethod(obj):
    """A convenience.  Same as TraceFunction but changes the recorded
    label to use the term 'method' rather than 'function'."""
    return TraceFunction(obj)


# Remove any private stuff, like test classes, if we are not being
# imported from a test.

try:
    from . import __DOC
    __DOC.Execute(locals())
    del __DOC
except Exception:
    pass
