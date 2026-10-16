//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_PY_ERROR_H
#define PXR_BASE_TF_PY_ERROR_H

/// \file tf/error.h
/// Provide facilities for error handling in script.

#include "pxr/pxr.h"

#include "pxr/base/tf/api.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/pySafePython.h"

#include <nanobind/nanobind.h>
#include <exception>

PXR_NAMESPACE_OPEN_SCOPE

/// Converts any \a TfError objects in \a m into python exceptions.  User code
/// should generally not have to call this.  User code should generally not
/// have to call this, unless it's manually bridging between C++ & Python.
TF_API
bool TfPyConvertTfErrorsToPythonException(TfErrorMark const &m);

/// Convert the current python exception to \a TfError objects and post them
/// to the error system.  User code should generally not have to call this,
/// unless it's manually bridging between C++ & Python.
TF_API
void TfPyConvertPythonExceptionToTfErrors();

struct Tf_PyRaiseOnErrorGuard
{
    TfErrorMark errorMark;

    ~Tf_PyRaiseOnErrorGuard() noexcept(false) {
        if (!std::uncaught_exceptions() && !errorMark.IsClean() &&
            TfPyConvertTfErrorsToPythonException(errorMark)) {
            throw nanobind::python_error();
        }
    }
};

/// \class TfPyRaiseOnError
///
/// A nanobind call_guard policy which, when applied to a wrapped
/// function, will create an error mark before calling the function, and check
/// that error mark after the function has completed.  If any TfErrors have
/// occurred, they will be raised as python exceptions.
///
/// This facility does not need to be used by clients in general.  It is only
/// required for wrapped functions and methods that do not appear directly in an
/// extension module.  For instance, the map and sequence proxy objects use
/// this, since they are created on the fly.
template <typename... Unused>
using TfPyRaiseOnError = nanobind::call_guard<Tf_PyRaiseOnErrorGuard>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_PY_ERROR_H
