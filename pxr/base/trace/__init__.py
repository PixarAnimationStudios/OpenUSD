#
# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
"""
Trace -- Utilities for counting and recording events.
"""

from pxr import Tf
Tf.PreparePythonModule()
del Tf

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
