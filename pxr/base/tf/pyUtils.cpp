//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/error.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyError.h"
#include "pxr/base/tf/pyErrorInternal.h"
#include "pxr/base/tf/pyInterpreter.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/scriptModuleLoader.h"

#include "pxr/base/arch/defines.h"
#include "pxr/base/tf/registryManager.h"

#include <boost/python.hpp>
#include <boost/python/detail/api_placeholder.hpp>

#include <mutex>
#include <functional>
#include <vector>

using namespace boost::python;

using std::string;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

void
TfPyThrowIndexError(const char* msg)
{
    PyErr_SetString(PyExc_IndexError, msg);
    boost::python::throw_error_already_set();
}

void
TfPyThrowRuntimeError(const char *msg)
{
    PyErr_SetString(PyExc_RuntimeError, msg);
    boost::python::throw_error_already_set();
}

void
TfPyThrowStopIteration(const char *msg)
{
    PyErr_SetString(PyExc_StopIteration, msg);
    boost::python::throw_error_already_set();
}

void
TfPyThrowKeyError(const char *msg)
{
    PyErr_SetString(PyExc_KeyError, msg);
    boost::python::throw_error_already_set();
}

void
TfPyThrowValueError(const char *msg)
{
    PyErr_SetString(PyExc_ValueError, msg);
    boost::python::throw_error_already_set();
}

void
TfPyThrowTypeError(const char *msg)
{
    PyErr_SetString(PyExc_TypeError, msg);
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
    return !obj.get() || obj.get() == Py_None;
}

void Tf_PyLoadScriptModule(std::string const &moduleName)
{
    if (TfPyIsInitialized()) {
        TfPyLock pyLock;
        string tmp(moduleName);
        PyObject *result =
            PyImport_ImportModule(const_cast<char *>(tmp.c_str()));
        if (!result) {
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
        handle<> modHandle(PyImport_ImportModule("builtins"));
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


int64_t
TfPyNormalizeIndex(int64_t index, uint64_t size, bool throwError)
{
    if (index < 0) 
        index += size;

    if (throwError && (index < 0 || static_cast<uint64_t>(index) >= size)) {
        TfPyThrowIndexError("Index out of range.");
    }

    return index < 0 ? 0 :
                   static_cast<uint64_t>(index) >= size ? size - 1 : index;
}


TF_API void
Tf_PyWrapOnceImpl(
    boost::python::type_info const &type,
    std::function<void()> const &wrapFunc,
    bool * isTypeWrapped)
{
    static std::mutex pyWrapOnceMutex;

    if (!wrapFunc) {
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

    if (!pyType) {
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

boost::python::object
TfPyCopyBufferToByteArray(const char* buffer, size_t size)
{
    TfPyLock lock;
    boost::python::object result;

    try {
        // boost python doesn't include a bytearray object, but we can return
        // one through the C api directly. The c api takes an array of char
        // and a size, so uses the name FromString, but this is really just
        // a buffer.
        PyObject* buf = PyByteArray_FromStringAndSize(buffer, size);
        boost::python::handle<> hbuf(buf);
        result = boost::python::object(hbuf); 
    } catch (boost::python::error_already_set const &) {
        TfPyConvertPythonExceptionToTfErrors();
        PyErr_Clear();
    }

    return result;
}

vector<string> TfPyGetTraceback()
{
    vector<string> result;

    if (!TfPyIsInitialized())
        return result;

    TfPyLock lock;
    // Save the exception state so we can restore it -- getting a traceback
    // should not affect the exception state.
    TfPyExceptionStateScope exceptionStateScope;
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
    }
    return result;
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
    if (!TfPyIsInitialized()) {
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
    if (!TfPyIsInitialized()) {
        TF_CODING_ERROR("Python is uninitialized.");
        return false;
    }

    TfPyLock lock;

    try {
        object environObj(_GetOsEnviron());
        object has_key = environObj.attr("__contains__");
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
    if (!PyErr_ExceptionMatches(PyExc_KeyboardInterrupt)) {
        PyErr_Print();
    }
}

void
Tf_PyObjectError(bool printError)
{
    // Silently pass these exceptions through.
    if (PyErr_ExceptionMatches(PyExc_SystemExit) ||
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

PXR_NAMESPACE_CLOSE_SCOPE
