#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
"""
Tf -- Tools Foundation
"""

# Type to help handle DLL import paths on Windows with python interpreters v3.8
# and newer. These interpreters don't search for DLLs in the path anymore, you
# have to provide a path explicitly. This re-enables path searching for USD 
# dependency libraries
import platform, sys
if sys.version_info >= (3, 8) and platform.system() == "Windows":
    import contextlib

    @contextlib.contextmanager
    def WindowsImportWrapper():
        import os
        dirs = []
        import_paths = os.getenv('PXR_USD_WINDOWS_DLL_PATH')
        if import_paths is None:
            import_paths = os.getenv('PATH', '')
        # the underlying windows API call, AddDllDirectory, states that:
        #
        # > If AddDllDirectory is used to add more than one directory to the
        # > process DLL search path, the order in which those directories are
        # > searched is unspecified.
        #
        # https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-adddlldirectory
        #
        # However, in practice, it seems that the most-recently-added ones
        # take precedence - so, reverse the order of entries in PATH to give
        # it the same precedence
        #
        # Note that we have a test (testTfPyDllLink) to alert us if this
        # undefined behavior changes.
        for path in reversed(import_paths.split(os.pathsep)):
            # Calling add_dll_directory raises an exception if paths don't
            # exist, or if you pass in dot
            if os.path.exists(path) and path != '.':
                abs_path = os.path.abspath(path)
                dirs.append(os.add_dll_directory(abs_path))
        # This block guarantees we clear the dll directories if an exception
        # is raised in the with block.
        try:
            yield
        finally:
            for dll_dir in dirs:
                dll_dir.close()
        del os
    del contextlib
else:
    class WindowsImportWrapper(object):
        def __enter__(self):
            pass
        def __exit__(self, exc_type, ex_val, exc_tb):
            pass
del platform, sys


def PreparePythonModule(moduleName=None):
    """Prepare an extension module at import time.  This will import the
    Python module associated with the caller's module (e.g. '_tf' for 'pxr.Tf')
    or the module with the specified moduleName and copy its contents into
    the caller's local namespace.

    Generally, this should only be called by the __init__.py script for a module
    upon loading a boost python module (generally '_libName.so')."""
    import importlib
    import inspect
    frame = inspect.currentframe().f_back
    try:
        f_locals = frame.f_locals

        # If an explicit moduleName is not supplied, construct it from the
        # caller's module name, like "pxr.Tf", and our naming conventions,
        # which results in "_tf".
        if moduleName is None:
            moduleName = f_locals["__name__"].split(".")[-1]
            moduleName = "_" + moduleName[0].lower() + moduleName[1:]

        with WindowsImportWrapper():
            module = importlib.import_module(
                    "." + moduleName, f_locals["__name__"])

        PrepareModule(module, f_locals)
        try:
            del f_locals[moduleName]
        except KeyError:
            pass

        try:
            module = importlib.import_module(".__DOC", f_locals["__name__"])
            module.Execute(f_locals)
            try:
                del f_locals["__DOC"]
            except KeyError:
                pass
        except Exception:
            pass

    finally:
        del frame

def PrepareModule(module, result):
    """PrepareModule(module, result) -- Prepare an extension module at import
    time.  Generally, this should only be called by the __init__.py script for a
    module upon loading a boost python module (generally '_libName.so')."""
    # inject into result.
    ignore = frozenset(['__name__', '__package__', '__builtins__',
                        '__doc__', '__file__', '__path__'])
    newModuleName = result.get('__name__')

    for key, value in module.__dict__.items():
        if not key in ignore:
            result[key] = value

            # Lie about the module from which value came.
            if newModuleName and hasattr(value, '__module__'):
                try:
                    setattr(value, '__module__', newModuleName)
                except AttributeError as e:
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
    import sys
    f_back = sys._getframe(framesUp).f_back
    return (f_back.f_globals['__name__'], f_back.f_code.co_name,
            f_back.f_code.co_filename, f_back.f_lineno)

PreparePythonModule()

# Need to provide an exception type that tf errors will show up as.
class ErrorException(RuntimeError):
    def __init__(self, *args):
        RuntimeError.__init__(self, *args)
        self.__TfException = True

    def __str__(self):
        return '\n\t' + '\n\t'.join([str(e) for e in self.args])
__SetErrorExceptionClass(ErrorException)

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
