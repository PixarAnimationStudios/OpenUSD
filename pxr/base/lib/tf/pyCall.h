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
#ifndef TF_PY_CALL_H
#define TF_PY_CALL_H

/// \file tf/pyCall.h
/// Utilities for calling python callables.
/// 
/// These functions handle trapping python errors and converting them to \a
/// TfErrors.

#include "pxr/pxr.h"

#include "pxr/base/tf/pyError.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyObjWrapper.h"

#include <boost/python/call.hpp>

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfPyCall
///
/// Provide a way to call a Python callable.
/// 
/// Usage is as follows:
/// \code
///     return TfPyCall<RetType>(callable)(arg1, arg2, ... argN);
/// \endcode
/// Generally speaking, TfPyCall instances may be copied, assigned, destroyed,
/// and invoked without the client holding the GIL.  However, if the \a Return
/// template parameter is a \a boost::python::object (or a derived class, such
/// as list or tuple) then the client must hold the GIL in order to invoke the
/// call operator.
template <typename Return>
struct TfPyCall {
    /// Construct with callable \a c.  Constructing with a \c
    /// boost::python::object works, since those implicitly convert to \c
    /// TfPyObjWrapper, however in that case the GIL must be held by the caller.
    explicit TfPyCall(TfPyObjWrapper const &c) : _callable(c) {}

    template <typename... Args>
    Return operator()(Args... args);

private:
    TfPyObjWrapper _callable;
};

template <typename Return>
template <typename... Args>
inline Return
TfPyCall<Return>::operator()(Args... args)
{
    TfPyLock pyLock;
    // Do *not* call through if there's an active python exception.
    if (!PyErr_Occurred()) {
        try {
            return boost::python::call<Return>
                (_callable.ptr(), args...);
        } catch (boost::python::error_already_set const &) {
            // Convert any exception to TF_ERRORs.
            TfPyConvertPythonExceptionToTfErrors();
            PyErr_Clear();
        }
    }
    return Return();
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
