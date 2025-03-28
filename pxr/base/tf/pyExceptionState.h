//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/api.h"
#include "pxr/external/boost/python/handle.hpp"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

struct TfPyExceptionState {
    TfPyExceptionState(pxr_boost::python::handle<> const &type,
                       pxr_boost::python::handle<> const &value,
                       pxr_boost::python::handle<> const &trace) :
            _type(type), _value(value), _trace(trace) {}

    TF_API
    ~TfPyExceptionState();

    TF_API
    TfPyExceptionState (TfPyExceptionState const &);

    TF_API
    TfPyExceptionState &operator=(TfPyExceptionState const &);

    // Extract Python's current exception state as by PyErr_Fetch() and return
    // it in a TfPyExceptionState.  This leaves Python's current exception state
    // clear.
    TF_API
    static TfPyExceptionState Fetch();

    pxr_boost::python::handle<> const &GetType() const { return _type; }
    pxr_boost::python::handle<> const &GetValue() const { return _value; }
    pxr_boost::python::handle<> const &GetTrace() const { return _trace; }

    // Move this object's exception state into Python's current exception state,
    // as by PyErr_Restore().  This leaves this object's exception state clear.
    TF_API
    void Restore();

    // Format a Python traceback for the exception state held by this object, as
    // by traceback.format_exception().
    TF_API
    std::string GetExceptionString() const;

private:
    pxr_boost::python::handle<> _type, _value, _trace;
};

PXR_NAMESPACE_CLOSE_SCOPE
