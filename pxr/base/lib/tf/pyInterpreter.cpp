//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/pyError.h"
#include "pxr/base/tf/pyInterpreter.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/scriptModuleLoader.h"
#include "pxr/base/tf/stringUtils.h"

#include "pxr/base/arch/symbols.h"
#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/arch/threads.h"

#include <boost/python.hpp>
#include <boost/python/detail/api_placeholder.hpp>
#include <atomic>
#include <mutex>
#include <string>
#include <Python.h>
#include <signal.h>

using std::string;

using namespace boost::python;

PXR_NAMESPACE_OPEN_SCOPE

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

        if (not ArchIsMainThread() and not PyEval_ThreadsInitialized()) {
            // Python claims that PyEval_InitThreads "should be called in the
            // main thread before creating a second thread or engaging in any
            // other thread operations."  So we'll issue a warning here.
            TF_WARN("Calling PyEval_InitThreads() for the first time outside "
                    "the 'main thread'.  Python doc says not to do this.");
        }

        // Initialize Python threading.  This grabs the GIL.  We'll release it
        // at the end of this function.
        PyEval_InitThreads();

        // Setting the program name is necessary in order for python to 
        // find the correct built-in modules. 
        Py_SetProgramName(const_cast<char*>(strdup(ArchGetExecutablePath().c_str())));

        // We're here when this is a C++ program initializing python (i.e. this
        // is a case of "embedding" a python interpreter, as opposed to
        // "extending" python with extension modules).
        //
        // In this case we don't want python to change the sigint handler.  Save
        // it before calling Py_Initialize and restore it after.
        struct sigaction origSigintHandler;
        sigaction(SIGINT, NULL, &origSigintHandler);
        
        Py_Initialize();

        // Restore original sigint handler.
        sigaction(SIGINT, &origSigintHandler, NULL);
        
        char emptyArg[] = {'\0'};
        char *empty[] = {emptyArg};
        PySys_SetArgv(1, empty);

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

boost::python::handle<>
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

boost::python::handle<>
TfPyRunFile(const std::string &filename, int start,
            object const &globals, object const &locals)
{
    FILE *f = fopen(filename.c_str(), "rt");
    if (not f) {
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

std::string
TfPyGetModulePath(const std::string & moduleName)
{
    TfPyInitialize();

    // Make sure imp is imported.
    static std::once_flag once;
    std::call_once(once, [](){
        TfPyRunSimpleString("import imp\n");
    });
    
    // XXX
    // If the module name is hierarchical (e.g. Animal.Primate.Chimp), then
    // we have to walk the hierarchy and import all of the containing modules
    // down the module we want to find.
    // vector< string > pkgHierarchy = TfStringTokenize(moduleName, ".");

    // Do the find_module.
    std::string cmd =
        TfStringPrintf("imp.find_module('%s')[1]\n", moduleName.c_str());
    handle<> result = TfPyRunString(cmd, Py_eval_input);
    if (not result)
        return std::string();

    // XXX close file

    // Extract result string
    extract<string> getString(result.get());
    return getString.check() ? getString() : string();
}

PXR_NAMESPACE_CLOSE_SCOPE
