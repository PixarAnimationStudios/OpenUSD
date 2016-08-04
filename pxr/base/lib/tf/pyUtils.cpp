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
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/error.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/pyError.h"
#include "pxr/base/tf/pyInterpreter.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/scriptModuleLoader.h"

#include "pxr/base/arch/defines.h"
#include "pxr/base/tf/registryManager.h"

#include <ciso646>
#include <vector>
#include <boost/python/object/class_detail.hpp>

using namespace boost::python;

using std::string;
using std::vector;

void
TfPyThrowIndexError(string const &msg)
{
    TfPyLock pyLock;
    PyErr_SetString(PyExc_IndexError, msg.c_str());
    boost::python::throw_error_already_set();
}

void
TfPyThrowRuntimeError(string const &msg)
{
    TfPyLock pyLock;
    PyErr_SetString(PyExc_RuntimeError, msg.c_str());
    boost::python::throw_error_already_set();
}

void
TfPyThrowStopIteration(string const &msg)
{
    TfPyLock pyLock;
    PyErr_SetString(PyExc_StopIteration, msg.c_str());
    boost::python::throw_error_already_set();
}

void
TfPyThrowKeyError(string const &msg)
{
    TfPyLock pyLock;
    PyErr_SetString(PyExc_KeyError, msg.c_str());
    boost::python::throw_error_already_set();
}

void
TfPyThrowValueError(string const &msg)
{
    TfPyLock pyLock;
    PyErr_SetString(PyExc_ValueError, msg.c_str());
    boost::python::throw_error_already_set();
}

void
TfPyThrowTypeError(string const &msg)
{
    TfPyLock pyLock;
    PyErr_SetString(PyExc_TypeError, msg.c_str());
    boost::python::throw_error_already_set();
}

bool
TfPyIsNone(boost::python::object const &obj)
{
    return obj.ptr() == Py_None;
}

bool
TfPyIsNone(boost::python::handle<> const &obj)
{
    return not obj.get() or obj.get() == Py_None;
}

void Tf_PyLoadScriptModule(std::string const &moduleName)
{
    if (TfPyIsInitialized()) {
        TfPyLock pyLock;
        string tmp(moduleName);
        PyObject *result =
            PyImport_ImportModule(const_cast<char *>(tmp.c_str()));
        if (not result) {
            // CODE_COVERAGE_OFF
            TF_WARN("Import failed for module '%s'!", moduleName.c_str());
            TfPyPrintError();
            // CODE_COVERAGE_ON
        }
    } else {
        // CODE_COVERAGE_OFF
        TF_WARN("Attempted to load module '%s' but Python is not initialized.",
                moduleName.c_str());
        // CODE_COVERAGE_ON
    }
}

bool
TfPyIsInitialized()
{
    return Py_IsInitialized();
}

string
TfPyObjectRepr(boost::python::object const &t)
{
    if (!TfPyIsInitialized()) {
        // CODE_COVERAGE_OFF
        TF_CODING_ERROR("Called TfPyRepr without python being initialized!");
        return "<error: python not initialized>";
        // CODE_COVERAGE_ON
    }
    // Take the interpreter lock as we're about to call back to Python.
    TfPyLock pyLock;

    // In case the try block throws, we'll return this string.
    string reprString("<invalid repr>");

    try {
        handle<> repr(PyObject_Repr(t.ptr()));
        reprString = extract<string>(repr.get());

        // Python's repr() for NaN and Inf are not valid python which evaluates
        // to themselves.  Special case them here to produce python which has
        // this property.  This is unpleasant since we're not producing the real
        // python repr, but we want everything coming out of here (if at all
        // possible) to have this property.
        if (reprString == "nan")
            reprString = "float('nan')";
        if (reprString == "inf")
            reprString = "float('inf')";
        if (reprString == "-inf")
            reprString = "-float('inf')";

    } catch (error_already_set const &) {
        PyErr_Clear();
    }
    return reprString;
}


boost::python::object
TfPyEvaluate(std::string const &expr, dict const& extraGlobals)
{
    TfPyLock lock;
    try {
        // Get the modules dict for the loaded script modules.
        dict modulesDict =
            TfScriptModuleLoader::GetInstance().GetModulesDict();

        // Make sure the builtins are available
        handle<> modHandle(PyImport_ImportModule("__builtin__"));
        modulesDict["__builtins__"] = object(modHandle);
        modulesDict.update(extraGlobals);

        // Eval the expression in that enviornment.
        return object(TfPyRunString(expr, Py_eval_input,
                                    modulesDict, modulesDict));
    } catch (boost::python::error_already_set const &) {
        TfPyConvertPythonExceptionToTfErrors();
        PyErr_Clear();
    }
    return boost::python::object();
}


int
TfPyNormalizeIndex(int index, unsigned int size, bool throwError)
{
    if (index < 0) 
        index += size;

    if (throwError && ( static_cast<unsigned int>(index) >= size || index < 0))
        TfPyThrowIndexError("Index out of range.");

    return index < 0 ? 0 : static_cast<unsigned int>(index) >= size ? size - 1 : index;
}


TF_API void
Tf_PyWrapOnceImpl(
    boost::python::type_info const &type,
    boost::function<void()> const &wrapFunc,
    bool * isTypeWrapped)
{
    static std::mutex pyWrapOnceMutex;

    if (not wrapFunc) {
        TF_CODING_ERROR("Got null wrapFunc");
        return;
    }

    // Acquire the GIL here, just so we can be sure that it is released before
    // attempting to acquire our internal mutex.
    TfPyLock pyLock;
    pyLock.BeginAllowThreads();
    std::lock_guard<std::mutex> lock(pyWrapOnceMutex);
    pyLock.EndAllowThreads();

    // XXX: Double-checked locking
    if (*isTypeWrapped) {
        return;
    }

    boost::python::type_handle pyType =
        boost::python::objects::registered_class_object(type);

    if (not pyType) {
        wrapFunc();
    }

    *isTypeWrapped = true;
}


boost::python::object
TfPyGetClassObject(std::type_info const &type) {
    TfPyLock pyLock;
    return boost::python::object
        (boost::python::objects::registered_class_object(type));
}

string TfPyGetClassName(object const &obj)
{
    // Take the interpreter lock as we're about to call back to Python.
    TfPyLock pyLock;

    object classObject(obj.attr("__class__"));
    if (classObject) {
        object typeNameObject(classObject.attr("__name__"));
        extract<string> typeName(typeNameObject);
        if (typeName.check())
            return typeName();
    }

    // CODE_COVERAGE_OFF This shouldn't really happen.
    TF_WARN("Couldn't get class name for python object '%s'",
            TfPyRepr(obj).c_str());
    return "<unknown>";
    // CODE_COVERAGE_ON
}



vector<string> TfPyGetTraceback()
{
    vector<string> result;

    if (not TfPyIsInitialized())
        return result;

    TfPyLock lock;
    try {
        object tbModule(handle<>(PyImport_ImportModule("traceback")));
        object stack = tbModule.attr("format_stack")();
        size_t size = len(stack);
        result.reserve(size);
        for (size_t i = 0; i < size; ++i) {
            string s = extract<string>(stack[i]);
            result.push_back(s);
        }
    } catch (boost::python::error_already_set const &) {
        TfPyConvertPythonExceptionToTfErrors();
        PyErr_Clear();
    }

    return result;
}

void
TfPyGetStackFrames(vector<uintptr_t> *frames)
{
    if (not TfPyIsInitialized())
        return;

    TfPyLock lock;
    try {
        object tbModule(handle<>(PyImport_ImportModule("traceback")));
        object stack = tbModule.attr("format_stack")();
        size_t size = len(stack);
        frames->reserve(size);
        // Reverse the order of stack frames so that the stack is ordered 
        // like the output of ArchGetStackFrames() (deepest function call at 
        // the top of stack).
        for (long i = static_cast<long>(size)-1; i >= 0; --i) {
            string *s = new string(extract<string>(stack[i]));
            frames->push_back((uintptr_t)s);
        }
    } catch (boost::python::error_already_set const &) {
        TfPyConvertPythonExceptionToTfErrors();
        PyErr_Clear();
    }
}

void TfPyDumpTraceback() {
    printf("Traceback (most recent call last):\n");
    vector<string> tb = TfPyGetTraceback();
    TF_FOR_ALL(i, tb)
        printf("%s", i->c_str());
}


static object
_GetOsEnviron()
{
    // In theory, we could just check that the os module has been imported,
    // rather than forcing an import ourself.  However, it's possible that
    // os.environ is actually a re-export from another module (ie. posix,
    // which is the case as of CPython 2.6) that may have been imported
    // without importing os.  Rather than check a hardcoded list of potential
    // modules, we always import os if Python is initialized.  If this turns
    // out to be problematic, we may want to consider the other approach.
    boost::python::object
        module(boost::python::handle<>(PyImport_ImportModule("os")));
    boost::python::object environObj(module.attr("environ"));
    return environObj;
}

bool
TfPySetenv(const std::string & name, const std::string & value)
{
    if (not TfPyIsInitialized()) {
        TF_CODING_ERROR("Python is uninitialized.");
        return false;
    }

    TfPyLock lock;

    try {
        object environObj(_GetOsEnviron());
		environObj[name] = value;
        return true;
    }
    catch (boost::python::error_already_set&) {
        PyErr_Clear();
    }

    return false;
}

bool
TfPyUnsetenv(const std::string & name)
{
    if (not TfPyIsInitialized()) {
        TF_CODING_ERROR("Python is uninitialized.");
        return false;
    }

    TfPyLock lock;

    try {
        object environObj(_GetOsEnviron());
        object has_key(environObj.attr("has_key"));
        if (has_key(name)) {
			environObj[name].del();
        }
        return true;
    }
    catch (boost::python::error_already_set&) {
        PyErr_Clear();
    }

    return false;
}

bool Tf_PyEvaluateWithErrorCheck(
    const std::string & expr, boost::python::object * obj)
{
    TfErrorMark m;
    *obj = TfPyEvaluate(expr);
    return m.IsClean();
}

void
TfPyPrintError()
{
    if (not PyErr_ExceptionMatches(PyExc_KeyboardInterrupt)) {
        PyErr_Print();
    }
}

void
Tf_PyObjectError(bool printError)
{
    // Silently pass these exceptions through.
    if (PyErr_ExceptionMatches(PyExc_SystemExit) or
        PyErr_ExceptionMatches(PyExc_KeyboardInterrupt)) {
        return;
    }

    // Report and clear.
    if (printError) {
        PyErr_Print();
    }
    else {
        PyErr_Clear();
    }
}
