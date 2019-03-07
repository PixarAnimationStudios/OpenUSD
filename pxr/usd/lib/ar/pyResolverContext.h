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
#ifndef AR_PY_RESOLVER_CONTEXT_H
#define AR_PY_RESOLVER_CONTEXT_H

/// \file ar/pyResolverContext.h
/// Macros for creating Python bindings for objects used with 
/// ArResolverContext.

#include <boost/python/detail/wrap_python.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/object.hpp>

#include "pxr/pxr.h"
#include "pxr/usd/ar/api.h"
#include "pxr/usd/ar/resolverContext.h"

#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyObjWrapper.h"

#include <boost/function.hpp>

PXR_NAMESPACE_OPEN_SCOPE

/// Register the specified type as a context object that may be
/// converted from a Python into a ArResolverContext object
/// in C++ and vice versa. This typically would be called in the
/// source file where the Python wrapping for the context object
/// is defined.
template <class Context>
void 
ArWrapResolverContextForPython();

#ifndef doxygen
// Private helper functions for converting ArResolverContext
// objects to and from Python.

template <class Context>
bool
Ar_ConvertResolverContextFromPython(
    PyObject* obj,
    ArResolverContext* context)
{
    boost::python::extract<const Context&> x(obj);
    if (x.check()) {
        if (context) {
            *context = ArResolverContext(x());
        }
        return true;
    }
    return false;
}

template <class Context>
bool
Ar_ConvertResolverContextToPython(
    const ArResolverContext& context,
    TfPyObjWrapper* obj)
{
    if (const Context* contextObj = context.Get<Context>()) {
        if (obj) {
            TfPyLock lock;
            *obj = boost::python::object(*contextObj);
        }
        return true;
    }
    return false;
}

typedef boost::function<bool(PyObject*, ArResolverContext*)> 
    Ar_MakeResolverContextFromPythonFn;
typedef boost::function<bool(const ArResolverContext&, TfPyObjWrapper*)>
    Ar_ResolverContextToPythonFn;

AR_API
void
Ar_RegisterResolverContextPythonConversion(
    const Ar_MakeResolverContextFromPythonFn& convertFunc,
    const Ar_ResolverContextToPythonFn& getObjectFunc);

AR_API
bool
Ar_CanConvertResolverContextFromPython(PyObject* pyObj);

AR_API
ArResolverContext
Ar_ConvertResolverContextFromPython(PyObject* pyObj);

AR_API
TfPyObjWrapper
Ar_ConvertResolverContextToPython(const ArResolverContext& context);

template <class Context>
void 
ArWrapResolverContextForPython()
{
    Ar_RegisterResolverContextPythonConversion(
        Ar_ConvertResolverContextFromPython<Context>,
        Ar_ConvertResolverContextToPython<Context>);
};

#endif //doxygen

PXR_NAMESPACE_CLOSE_SCOPE

#endif // AR_PY_RESOLVER_CONTEXT_H
