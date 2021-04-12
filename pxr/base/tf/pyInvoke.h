//
// Copyright 2021 Pixar
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
#ifndef PXR_BASE_TF_PY_INVOKE_H
#define PXR_BASE_TF_PY_INVOKE_H

/// \file
/// Flexible, high-level interface for calling Python functions.

#include "pxr/pxr.h"
#include "pxr/base/tf/api.h"

#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/pyError.h"
#include "pxr/base/tf/pyInterpreter.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyObjWrapper.h"

#include <boost/python/dict.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/list.hpp>
#include <boost/python/object.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////
// To-Python arg conversion

#ifndef doxygen

// Convert any type to boost::python::object.
template <typename T>
boost::python::object Tf_ArgToPy(const T &value)
{
    return boost::python::object(value);
}

// Convert nullptr to None.
TF_API boost::python::object Tf_ArgToPy(const std::nullptr_t &value);

#endif // !doxygen

////////////////////////////////////////////////////////////////////////////////
// Keyword arg specification

/// Wrapper object for a keyword-argument pair in a call to TfPyInvoke*.  Any
/// value type may be provided, as long as it is convertible to Python.
/// Typically passed as an inline temporary object:
///
/// \code
/// const bool ok = TfPyInvoke(
///     "MyModule", "MyFunction",
///     arg1value, arg2value, TfPyKwArg("arg4", arg4value));
/// \endcode
///
/// \code{.py}
/// def MyFunction(arg1, arg2, arg3 = None, arg4 = None, arg5 = None):
///     # ...
/// \endcode
///
struct TfPyKwArg
{
    template <typename T>
    TfPyKwArg(const std::string &nameIn, const T &valueIn)
        : name(nameIn)
    {
        // Constructing boost::python::object requires the GIL.
        TfPyLock lock;

        // The object constructor throws if the type is not convertible.
        value = Tf_ArgToPy(valueIn);
    }

    std::string name;
    TfPyObjWrapper value;
};

////////////////////////////////////////////////////////////////////////////////
// Argument collection by variadic template functions

#ifndef doxygen

// Variadic helper: trivial base case.
TF_API void Tf_BuildPyInvokeKwArgs(
    boost::python::dict *kwArgsOut);

// Poisoned variadic template helper that provides an error message when
// non-keyword args are used after keyword args.
template <typename Arg, typename... RestArgs>
void Tf_BuildPyInvokeKwArgs(
    boost::python::dict *kwArgsOut,
    const Arg &kwArg,
    RestArgs... rest)
{
    // This assertion will always be false, since TfPyKwArg will select the
    // overload below instead.
    static_assert(
        std::is_same<Arg, TfPyKwArg>::value,
        "Non-keyword args not allowed after keyword args");
}

// Recursive variadic template helper for keyword args.
template <typename... RestArgs>
void Tf_BuildPyInvokeKwArgs(
    boost::python::dict *kwArgsOut,
    const TfPyKwArg &kwArg,
    RestArgs... rest)
{
    // Store mapping in kwargs dict.
    (*kwArgsOut)[kwArg.name] = kwArg.value.Get();

    // Recurse to handle next arg.
    Tf_BuildPyInvokeKwArgs(kwArgsOut, rest...);
}

// Variadic helper: trivial base case.
TF_API void Tf_BuildPyInvokeArgs(
    boost::python::list *posArgsOut,
    boost::python::dict *kwArgsOut);

// Recursive general-purpose variadic template helper.
template <typename Arg, typename... RestArgs>
void Tf_BuildPyInvokeArgs(
    boost::python::list *posArgsOut,
    boost::python::dict *kwArgsOut,
    const Arg &arg,
    RestArgs... rest)
{
    // Convert value to Python, and store in args list.
    // The object constructor throws if the type is not convertible.
    posArgsOut->append(Tf_ArgToPy(arg));

    // Recurse to handle next arg.
    Tf_BuildPyInvokeArgs(posArgsOut, kwArgsOut, rest...);
}

// Recursive variadic template helper for keyword args.
template <typename... RestArgs>
void Tf_BuildPyInvokeArgs(
    boost::python::list *posArgsOut,
    boost::python::dict *kwArgsOut,
    const TfPyKwArg &kwArg,
    RestArgs... rest)
{
    // Switch to kwargs-only processing, enforcing (at compile time) the Python
    // rule that there may not be non-kwargs after kwargs.  If we relaxed this
    // rule, some strange argument ordering could occur.
    Tf_BuildPyInvokeKwArgs(kwArgsOut, kwArg, rest...);
}

#endif // !doxygen

////////////////////////////////////////////////////////////////////////////////
// Declarations

#ifndef doxygen

// Helper for TfPyInvokeAndExtract.
TF_API bool Tf_PyInvokeImpl(
    const std::string &moduleName,
    const std::string &callableExpr,
    const boost::python::list &posArgs,
    const boost::python::dict &kwArgs,
    boost::python::object *resultObjOut);

// Forward declaration.
template <typename... Args>
bool TfPyInvokeAndReturn(
    const std::string &moduleName,
    const std::string &callableExpr,
    boost::python::object *resultOut,
    Args... args);

#endif // !doxygen

////////////////////////////////////////////////////////////////////////////////
// Main entry points

/// Call a Python function and obtain its return value.
///
/// Example:
/// \code
/// // Call MyModule.MyFunction(arg1, arg2), which returns a string.
/// std::string result;
/// const bool ok = TfPyInvokeAndExtract(
///     "MyModule", "MyFunction", &result, arg1Value, arg2Value);
/// \endcode
///
/// \p moduleName is the name of the module in which to find the function.  This
/// name will be directly imported in an \c import statement, so anything that
/// you know is in \c sys.path should work.  The module name will also be
/// prepended to \p callableExpr to look up the function.
///
/// \p callableExpr is a Python expression that, when appended to \p moduleName
/// (with an intervening dot), yields a callable object.  Typically this is just
/// a function name, optionally prefixed with object names (such as a class in
/// which the callable resides).
///
/// \p resultOut is a pointer that will receive the Python function's return
/// value.  A from-Python converter must be registered for the type of \c
/// *resultOut.
///
/// \p args is zero or more function arguments, of any types for which to-Python
/// conversions are registered.  Any \c nullptr arguments are converted to \c
/// None.  \p args may also contain TfPyKwArg objects to pass keyword arguments.
/// As in Python, once a keyword argument is passed, all remaining arguments
/// must also be keyword arguments.
///
/// The return value of TfPyInvokeAndExtract is true if the call completed,
/// false otherwise.  When the return value is false, at least one TfError
/// should have been raised, describing the failure.  TfPyInvokeAndExtract never
/// raises exceptions.
///
/// It should be safe to call this function without doing any other setup
/// first.  It is not necessary to call TfPyInitialize or lock the GIL; this
/// function does those things itself.
///
/// If you don't need the function's return value, call TfPyInvoke instead.
///
/// If you need the function's return value, but the return value isn't
/// guaranteed to be a consistent type that's convertible to C++, call
/// TfPyInvokeAndReturn instead.  This includes cases where the function's
/// return value may be \c None.
///
template <typename Result, typename... Args>
bool TfPyInvokeAndExtract(
    const std::string &moduleName,
    const std::string &callableExpr,
    Result *resultOut,
    Args... args)
{
    if (!resultOut) {
        TF_CODING_ERROR("Bad pointer to TfPyInvokeAndExtract");
        return false;
    }

    // Init Python and grab the GIL.
    TfPyInitialize();
    TfPyLock lock;

    boost::python::object resultObj;
    if (!TfPyInvokeAndReturn(
            moduleName, callableExpr, &resultObj, args...)) {
        return false;
    }

    // Extract return value.
    boost::python::extract<Result> extractor(resultObj);
    if (!extractor.check()) {
        TF_CODING_ERROR("Result type mismatched or not convertible");
        return false;
    }
    *resultOut = extractor();

    return true;
}

/// A version of TfPyInvokeAndExtract that provides the Python function's return
/// value as a \c boost::python::object, rather than extracting a particular C++
/// type from it.
///
template <typename... Args>
bool TfPyInvokeAndReturn(
    const std::string &moduleName,
    const std::string &callableExpr,
    boost::python::object *resultOut,
    Args... args)
{
    if (!resultOut) {
        TF_CODING_ERROR("Bad pointer to TfPyInvokeAndExtract");
        return false;
    }

    // Init Python and grab the GIL.
    TfPyInitialize();
    TfPyLock lock;

    try {
        // Convert args to Python and store in list+dict form.
        boost::python::list posArgs;
        boost::python::dict kwArgs;
        Tf_BuildPyInvokeArgs(&posArgs, &kwArgs, args...);

        // Import, find callable, and call.
        if (!Tf_PyInvokeImpl(
                moduleName, callableExpr, posArgs, kwArgs, resultOut)) {
            return false;
        }
    }
    catch (boost::python::error_already_set const &) {
        // Handle exceptions.
        TfPyConvertPythonExceptionToTfErrors();
        PyErr_Clear();
        return false;
    }

    return true;
}

/// A version of TfPyInvokeAndExtract that ignores the Python function's return
/// value.
///
template <typename... Args>
bool TfPyInvoke(
    const std::string &moduleName,
    const std::string &callableExpr,
    Args... args)
{
    // Init Python and grab the GIL.
    TfPyInitialize();
    TfPyLock lock;

    boost::python::object ignoredResult;
    return TfPyInvokeAndReturn(
        moduleName, callableExpr, &ignoredResult, args...);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_PY_INVOKE_H
