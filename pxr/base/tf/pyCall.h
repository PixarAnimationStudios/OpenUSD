//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_PY_CALL_H
#define PXR_BASE_TF_PY_CALL_H

/// \file tf/pyCall.h
/// Utilities for calling python callables.
/// 
/// These functions handle trapping python errors and converting them to \a
/// TfErrors.

#include "pxr/pxr.h"

#include "pxr/base/tf/pyError.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyObjWrapper.h"

#include <nanobind/nanobind.h>
#include <type_traits>

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
/// template parameter is a \a pxr_boost::python::object (or a derived class, such
/// as list or tuple) then the client must hold the GIL in order to invoke the
/// call operator.
template <typename Return>
struct TfPyCall {
    /// Construct with callable \a c.  Constructing with a \c
    /// nanobind::object works, since those implicitly convert to \c
    /// TfPyObjWrapper, however in that case the GIL must be held by the caller.
    explicit TfPyCall(TfPyObjWrapper const &c) : _callable(c) {}
    explicit TfPyCall(nanobind::handle c) : _callable(c) {}

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
            nanobind::object res = nanobind::borrow<nanobind::object>(
                _callable.ptr())(nanobind::cast(args)...);
            if constexpr (!std::is_void_v<Return>) {
                return nanobind::cast<Return>(res);
            } else {
                return;
            }
        } catch (nanobind::python_error &e) {
            e.restore();
            TfPyConvertPythonExceptionToTfErrors();
            PyErr_Clear();
        } catch (nanobind::cast_error const &e) {
            if (!PyErr_Occurred()) {
                PyErr_SetString(PyExc_TypeError, e.what());
            }
            TfPyConvertPythonExceptionToTfErrors();
            PyErr_Clear();
        }
    }
    if constexpr (std::is_same_v<Return, nanobind::object> ||
                  std::is_same_v<Return, nanobind::handle>) {
        return nanobind::none();
    } else if constexpr (!std::is_void_v<Return>) {
        return Return();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
