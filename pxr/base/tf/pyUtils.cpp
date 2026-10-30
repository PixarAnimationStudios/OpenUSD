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
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/scriptModuleLoader.h"

#include "pxr/base/arch/defines.h"
#include "pxr/base/tf/registryManager.h"

#include <mutex>
#include <functional>
#include <vector>

using std::string;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

void
TfPyThrowIndexError(const char* msg)
{
    PyErr_SetString(PyExc_IndexError, msg);
    throw nanobind::python_error();
}

void
TfPyThrowRuntimeError(const char *msg)
{
    PyErr_SetString(PyExc_RuntimeError, msg);
    throw nanobind::python_error();
}

void
TfPyThrowStopIteration(const char *msg)
{
    PyErr_SetString(PyExc_StopIteration, msg);
    throw nanobind::python_error();
}

void
TfPyThrowKeyError(const char *msg)
{
    PyErr_SetString(PyExc_KeyError, msg);
    throw nanobind::python_error();
}

void
TfPyThrowValueError(const char *msg)
{
    PyErr_SetString(PyExc_ValueError, msg);
    throw nanobind::python_error();
}

void
TfPyThrowTypeError(const char *msg)
{
    PyErr_SetString(PyExc_TypeError, msg);
    throw nanobind::python_error();
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
        } else {
            Py_DECREF(result);
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
TfPyObjectRepr(nanobind::handle const &t)
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

    PyObject *repr = PyObject_Repr(t.ptr());
    if (repr) {
        if (const char *s = PyUnicode_AsUTF8(repr)) {
            reprString = s;
            if (reprString == "nan")
                reprString = "float('nan')";
            if (reprString == "inf")
                reprString = "float('inf')";
            if (reprString == "-inf")
                reprString = "-float('inf')";
        } else {
            PyErr_Clear();
        }
        Py_DECREF(repr);
    } else {
        PyErr_Clear();
    }
    return reprString;
}


nanobind::object
TfPyEvaluate(std::string const &expr, nanobind::dict const& extraGlobals)
{
    TfPyLock lock;
    try {
        // Get the modules dict for the loaded script modules.
        nanobind::dict modulesDict =
            TfScriptModuleLoader::GetInstance().GetModulesDict();

        // Make sure the builtins are available
        nanobind::object modHandle = nanobind::module_::import_("builtins");
        modulesDict["__builtins__"] = modHandle;
        if (extraGlobals.is_valid()) {
            PyDict_Update(modulesDict.ptr(), extraGlobals.ptr());
        }

        // Eval the expression in that environment.
        nanobind::object res =
            TfPyRunString(expr, Py_eval_input, modulesDict, modulesDict);
        return res.is_valid() ? res : nanobind::none();
    } catch (nanobind::python_error &e) {
        e.restore();
        TfPyConvertPythonExceptionToTfErrors();
        PyErr_Clear();
    } catch (std::exception const &) {
        PyErr_Clear();
    }
    return nanobind::none();
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
    std::type_info const &type,
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

    if (!NB_CALL(nb_type_lookup)(NB_CTX, &type)) {
        wrapFunc();
    }

    *isTypeWrapped = true;
}


nanobind::object
TfPyGetClassObject(std::type_info const &type) {
    TfPyLock pyLock;
    if (PyObject *nbType = NB_CALL(nb_type_lookup)(NB_CTX, &type)) {
        return nanobind::borrow<nanobind::object>(nbType);
    }
    return nanobind::none();
}

string TfPyGetClassName(nanobind::handle const &obj)
{
    // Take the interpreter lock as we're about to call back to Python.
    TfPyLock pyLock;

    if (obj.ptr()) {
        PyObject *classObject = PyObject_GetAttrString(obj.ptr(), "__class__");
        if (classObject) {
            PyObject *typeNameObject =
                PyObject_GetAttrString(classObject, "__name__");
            Py_DECREF(classObject);
            if (typeNameObject) {
                const char *s = PyUnicode_AsUTF8(typeNameObject);
                if (s) {
                    string ret(s);
                    Py_DECREF(typeNameObject);
                    return ret;
                }
                PyErr_Clear();
                Py_DECREF(typeNameObject);
            } else {
                PyErr_Clear();
            }
        } else {
            PyErr_Clear();
        }
    }

    // CODE_COVERAGE_OFF This shouldn't really happen.
    TF_WARN("Couldn't get class name for python object '%s'",
            TfPyObjectRepr(obj).c_str());
    return "<unknown>";
    // CODE_COVERAGE_ON
}

nanobind::object
TfPyCopyBufferToByteArray(const char* buffer, size_t size)
{
    TfPyLock lock;
    PyObject* buf = PyByteArray_FromStringAndSize(buffer, size);
    if (!buf) {
        TfPyConvertPythonExceptionToTfErrors();
        PyErr_Clear();
        return nanobind::object();
    }
    return nanobind::steal<nanobind::object>(buf);
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
        nanobind::object tbModule = nanobind::module_::import_("traceback");
        nanobind::object stack = tbModule.attr("format_stack")();
        size_t size = nanobind::len(stack);
        result.reserve(size);
        for (size_t i = 0; i < size; ++i) {
            string s = nanobind::cast<string>(stack[i]);
            result.push_back(s);
        }
    } catch (nanobind::python_error &e) {
        e.restore();
        TfPyConvertPythonExceptionToTfErrors();
    } catch (std::exception const &) {
        PyErr_Clear();
    }
    return result;
}

static nanobind::object
_GetOsEnviron()
{
    nanobind::object module = nanobind::module_::import_("os");
    return module.attr("environ");
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
        nanobind::object environObj = _GetOsEnviron();
        environObj[name.c_str()] = value;
        return true;
    }
    catch (nanobind::python_error &) {
        PyErr_Clear();
    }
    catch (std::exception const &) {
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
        nanobind::object environObj = _GetOsEnviron();
        if (nanobind::cast<bool>(environObj.attr("__contains__")(name))) {
            PyObject_DelItem(environObj.ptr(), nanobind::cast(name).ptr());
            if (PyErr_Occurred()) {
                PyErr_Clear();
                return false;
            }
        }
        return true;
    }
    catch (nanobind::python_error &) {
        PyErr_Clear();
    }
    catch (std::exception const &) {
        PyErr_Clear();
    }

    return false;
}

bool Tf_PyEvaluateWithErrorCheck(
    const std::string & expr, nanobind::object * obj)
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
