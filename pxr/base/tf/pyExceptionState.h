//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TF_PY_EXCEPTION_STATE_H
#define PXR_BASE_TF_PY_EXCEPTION_STATE_H

#include "pxr/pxr.h"
#include "pxr/base/tf/api.h"
#include "pxr/base/tf/pySafePython.h"
#include <nanobind/nanobind.h>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

struct TfPyExceptionState {
    TfPyExceptionState(nanobind::object const &type,
                       nanobind::object const &value,
                       nanobind::object const &trace) :
            _type(type), _value(value), _trace(trace) {}

    TF_API
    ~TfPyExceptionState();

    TF_API
    TfPyExceptionState(TfPyExceptionState const &);

    TF_API
    TfPyExceptionState &operator=(TfPyExceptionState const &);

    // Extract Python's current exception state as by PyErr_Fetch() and return
    // it in a TfPyExceptionState.  This leaves Python's current exception state
    // clear.
    TF_API
    static TfPyExceptionState Fetch();

    nanobind::object const &GetType() const { return _type; }
    nanobind::object const &GetValue() const { return _value; }
    nanobind::object const &GetTrace() const { return _trace; }

    // Move this object's exception state into Python's current exception state,
    // as by PyErr_Restore().  This leaves this object's exception state clear.
    TF_API
    void Restore();

    // Format a Python traceback for the exception state held by this object, as
    // by traceback.format_exception().
    TF_API
    std::string GetExceptionString() const;

private:
    nanobind::object _type, _value, _trace;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_PY_EXCEPTION_STATE_H
