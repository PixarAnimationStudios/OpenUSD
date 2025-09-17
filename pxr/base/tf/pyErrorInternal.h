//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_PY_ERROR_INTERNAL_H
#define PXR_BASE_TF_PY_ERROR_INTERNAL_H

#include "pxr/pxr.h"

#include "pxr/base/tf/api.h"
#include "pxr/base/tf/pyExceptionState.h"
#include "pxr/external/boost/python/handle.hpp"
#include "pxr/external/boost/python/object_fwd.hpp"

PXR_NAMESPACE_OPEN_SCOPE

enum Tf_PyExceptionErrorCode {
    TF_PYTHON_EXCEPTION
};

TF_API pxr_boost::python::handle<> Tf_PyGetErrorExceptionClass();
TF_API void Tf_PySetErrorExceptionClass(pxr_boost::python::object const &cls);

/// RAII class to save and restore the Python exception state.  The client
/// must hold the GIL during all methods, including the c'tor and d'tor.
class TfPyExceptionStateScope {
public:
    // Store Python's current exception state in this object, if any, and clear
    // Python's current exception state.
    TF_API TfPyExceptionStateScope();
    TfPyExceptionStateScope(const TfPyExceptionStateScope&) = delete;
    TfPyExceptionStateScope& operator=(const TfPyExceptionStateScope&) = delete;

    // Restore Python's exception state to the state captured upon
    // construction.
    TF_API ~TfPyExceptionStateScope();

    // Return a reference to the held TfPyExceptionState.
    TfPyExceptionState const &Get() const {
        return _state;
    }

private:
    TfPyExceptionState _state;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_PY_ERROR_INTERNAL_H
