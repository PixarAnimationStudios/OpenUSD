#
# Copyright 2016 Pixar
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
Tf -- Tools Foundation
"""

def PrepareModule(module, result):
    """PrepareModule(module, result) -- Prepare an extension module at import
    time.  Generally, this should only be called by the __init__.py script for a
    module upon loading a boost python module (generally '_LibName.so')."""
    # inject into result.
    ignore = frozenset(['__name__', '__builtins__',
                        '__doc__', '__file__', '__path__'])
    newModuleName = result.get('__name__')

    for key, value in module.__dict__.items():
        if not key in ignore:
            result[key] = value

            # Lie about the module from which value came.
            if newModuleName and hasattr(value, '__module__'):
                try:
                    setattr(value, '__module__', newModuleName)
                except AttributeError, e:
                    # The __module__ attribute of Boost.Python.function
                    # objects is not writable, so we get this exception
                    # a lot.  Just ignore it.  We're really only concerned
                    # about the data objects like enum values and such.
                    #
                    pass

def GetCodeLocation(framesUp):
    """Returns a tuple (moduleName, functionName, fileName, lineNo).

    To trace the current location of python execution, use GetCodeLocation().
    By default, the information is returned at the current stack-frame; thus

        info = GetCodeLocation()

    will return information about the line that GetCodeLocation() was called 
    from. One can write:

        def genericDebugFacility():
            info = GetCodeLocation(1)
            # print out data


        def someCode():
            ...
            if bad:
                genericDebugFacility()

    and genericDebugFacility() will get information associated with its caller, 
    i.e. the function someCode()."""
    import inspect
    f_back = inspect.currentframe(framesUp).f_back
    return (f_back.f_globals['__name__'], f_back.f_code.co_name,
            f_back.f_code.co_filename, f_back.f_lineno)

# for some strange reason, this errors out when we try to reload it,
# which is odd since _tf is a DSO and can't be reloaded anyway:
import sys
if "pxr.Tf._tf" not in sys.modules:
    from . import _tf
    PrepareModule(_tf, locals())
    del _tf
del sys

# Need to provide an exception type that tf errors will show up as.
class ErrorException(RuntimeError):
    def __init__(self, *args):
        RuntimeError.__init__(self, *args)
        self.__TfException = True;
    def __str__(self):
        return '\n\t' + '\n\t'.join([str(e) for e in self])
__SetErrorExceptionClass(ErrorException)

try:
    from . import __DOC
    __DOC.Execute(locals())
    del __DOC
except Exception:
    pass

def Warn(msg, template=""):
    """Issue a warning via the TfDiagnostic system.

    At this time, template is ignored.
    """
    codeInfo = GetCodeLocation(framesUp=1)
    _Warn(msg, codeInfo[0], codeInfo[1], codeInfo[2], codeInfo[3])
    
def Status(msg, verbose=True):
    """Issues a status update to the Tf diagnostic system.

    If verbose is True (the default) then information about where in the code
    the status update was issued from is included.
    """
    if verbose:
        codeInfo = GetCodeLocation(framesUp=1)
        _Status(msg, codeInfo[0], codeInfo[1], codeInfo[2], codeInfo[3])
    else:
        _Status(msg, "", "", "", 0)

def RaiseCodingError(msg):
    """Raise a coding error to the Tf Diagnostic system."""
    codeInfo = GetCodeLocation(framesUp=1)
    _RaiseCodingError(msg, codeInfo[0], codeInfo[1], codeInfo[2], codeInfo[3])

def RaiseRuntimeError(msg):
    """Raise a runtime error to the Tf Diagnostic system."""
    codeInfo = GetCodeLocation(framesUp=1)
    _RaiseRuntimeError(msg, codeInfo[0], codeInfo[1], codeInfo[2], codeInfo[3])

def Fatal(msg):
    """Raise a fatal error to the Tf Diagnostic system."""
    codeInfo = GetCodeLocation(framesUp=1)
    _Fatal(msg, codeInfo[0], codeInfo[1], codeInfo[2], codeInfo[3])


class NamedTemporaryFile(object):
    """A named temporary file which keeps the internal file handle closed. 
       A class which constructs a temporary file(that isn't open) on __enter__,
       provides its name as an attribute, and deletes it on __exit__. 
       
       Note: The constructor args for this object match those of 
       python's tempfile.mkstemp() function, and will have the same effect on
       the underlying file created."""

    def __init__(self, suffix='', prefix='', dir=None, text=False):
        # Note that we defer creation until the enter block to 
        # prevent users from unintentionally creating a bunch of
        # temp files that don't get cleaned up.
        self._args = (suffix,  prefix, dir, text)

    def __enter__(self):
        from tempfile import mkstemp
        from os import close
        
        fd, path = mkstemp(*self._args)
        close(fd) 

        # XXX: We currently only expose the name attribute
        # more can be added based on client needs in the future.
        self._name = path

        return self

    def __exit__(self, *args):
        import os
        os.remove(self.name)

    @property
    def name(self):
        """The path for the temporary file created.""" 
        return self._name
