//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/pyError.h"
#include "pxr/base/tf/pyInterpreter.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/scoped.h"
#include "pxr/base/tf/scriptModuleLoader.h"
#include "pxr/base/tf/stringUtils.h"

#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/arch/symbols.h"
#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/arch/threads.h"

#include "pxr/external/boost/python.hpp"
#include "pxr/external/boost/python/detail/api_placeholder.hpp"
#include <atomic>
#include <mutex>
#include <string>
#include "pxr/base/tf/pySafePython.h"
#include <signal.h>

using std::string;

PXR_NAMESPACE_OPEN_SCOPE

using namespace pxr_boost::python;

void
TfPyInitialize()
{
    static std::atomic<bool> initialized(false);
    if (initialized)
        return;

    // This mutex is, sadly, recursive since the call to the scriptModuleLoader
    // at the end of this function can end up reentering this function, while
    // importing python modules.  In this case we'll quickly return since
    // Py_IsInitialized will return true, but we need to keep other threads from
    // entering.
    static std::recursive_mutex mutex;
    std::lock_guard<std::recursive_mutex> lock(mutex);

    if (!Py_IsInitialized()) {
        // We're here when this is a C++ program initializing python (i.e. this
        // is a case of "embedding" a python interpreter, as opposed to
        // "extending" python with extension modules).
        // XXX: We may want to explore using PyConfig_InitIsolatedConfig
        // instead since that's intended for applications embedding Python.
        PyConfig config;
        PyConfig_InitPythonConfig(&config);

        TfScoped<> cleanupConfig([&config]() { PyConfig_Clear(&config); });

        // Match legacy Py_Initialize() stdio behavior.
        config.configure_c_stdio = 0;

        // Setting the program name is necessary in order for python to
        // find the correct built-in modules.
        const std::string s = ArchGetExecutablePath();
        const std::wstring programName(s.begin(), s.end());
        PyStatus status = PyConfig_SetString(&config, &config.program_name,
                                             programName.c_str());
        if (PyStatus_Exception(status)) {
            TF_FATAL_ERROR("Failed to set Python program name: %s",
                status.err_msg ? status.err_msg : "unknown error");
        }

        // Disable the interpreter's argv parsing; sys.argv defaults to [''].
        config.parse_argv = 0;

        // In this case we don't want python to change the sigint handler.  Save
        // it before calling Py_InitializeFromConfig and restore it after.
        // Another approach is setting config.install_signal_handlers = 0, but
        // that would result in all of Python's signal handlers being disabled.
        // That could lead to unexpected behavior such as other signals
        // not being handled as they were before.
         
#if !defined(ARCH_OS_WINDOWS)
        struct sigaction origSigintHandler;
        sigaction(SIGINT, NULL, &origSigintHandler);
#endif

        status = Py_InitializeFromConfig(&config);
        if (PyStatus_Exception(status)) {
            TF_FATAL_ERROR("Failed to initialize Python: %s",
                           status.err_msg ? status.err_msg : "unknown error");
        }

#if !defined(ARCH_OS_WINDOWS)
        // Restore original sigint handler.
        sigaction(SIGINT, &origSigintHandler, NULL);
#endif

        // Kick the module loading mechanism for any loaded libs that have
        // corresponding python binding modules.  We do this after we've
        // published that we're done initializing as this may reenter
        // TfPyInitialize().
        TfScriptModuleLoader::GetInstance().LoadModules();

        // Release the GIL and restore thread state.
        // When TfPyInitialize returns, we expect GIL is released 
        // and python's internal PyThreadState is NULL
        // Previously this only released the GIL without resetting the ThreadState
        // This can lead to a situation where python executes without the GIL
        // PyGILState_Ensure checks the current thread state and decides
        // to take the lock based on that; so if the GIL is released but the
        // current thread is valid it leads to cases where python executes
        // without holding the GIL and mismatched lock/release calls in TfPyLock 
        // (See the discussion in 141041)
        
        PyThreadState* currentState = PyGILState_GetThisThreadState();
        PyEval_ReleaseThread(currentState);

        // Say we're done initializing python.
        initialized = true;
    }
}

int
TfPyRunSimpleString(const std::string & cmd)
{
    TfPyInitialize();
    TfPyLock pyLock;
    return PyRun_SimpleString(cmd.c_str());
}

pxr_boost::python::handle<>
TfPyRunString(const std::string &cmd , int start,
              object const &globals, object const &locals)
{
    TfPyInitialize();
    TfPyLock pyLock;
    try {
        handle<> mainModule(borrowed(PyImport_AddModule("__main__")));
        handle<> 
            defaultGlobalsHandle(borrowed(PyModule_GetDict(mainModule.get())));

        PyObject *pyGlobals =
            TfPyIsNone(globals) ? defaultGlobalsHandle.get() : globals.ptr();
        PyObject *pyLocals =
            TfPyIsNone(locals) ? pyGlobals : locals.ptr();

        // used passed-in objects for globals and locals, or default
        // to globals from main module if no locals/globals passed in.
        return handle<>(PyRun_String(cmd.c_str(), start, pyGlobals, pyLocals));
    } catch (error_already_set const &) {
        TfPyConvertPythonExceptionToTfErrors();
        PyErr_Clear();
    }
    return handle<>();
}

pxr_boost::python::handle<>
TfPyRunFile(const std::string &filename, int start,
            object const &globals, object const &locals)
{
    FILE *f = ArchOpenFile(filename.c_str(), "r");
    if (!f) {
        TF_CODING_ERROR("Could not open file '%s'!", filename.c_str());
        return handle<>();
    }
        
    TfPyInitialize();
    TfPyLock pyLock;
    try {
        handle<> mainModule(borrowed(PyImport_AddModule("__main__")));
        handle<>
            defaultGlobalsHandle(borrowed(PyModule_GetDict(mainModule.get())));

        // used passed-in objects for globals and locals, or default
        // to globals from main module if no locals/globals passed in.
        PyObject *pyGlobals =
            TfPyIsNone(globals) ? defaultGlobalsHandle.get() : globals.ptr();
        PyObject *pyLocals =
            TfPyIsNone(locals) ? pyGlobals : locals.ptr();
        
        return handle<>(PyRun_FileEx(f, filename.c_str(), start,
                                     pyGlobals, pyLocals, 1 /* close file */));
    } catch (error_already_set const &) {
        TfPyConvertPythonExceptionToTfErrors();
        PyErr_Clear();
    }
    return handle<>();
}

PXR_NAMESPACE_CLOSE_SCOPE
