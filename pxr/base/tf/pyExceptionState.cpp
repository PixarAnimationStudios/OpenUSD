//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/pyErrorInternal.h"
#include "pxr/base/tf/pyLock.h"

#include "pxr/external/boost/python/object.hpp"
#include "pxr/external/boost/python/extract.hpp"

using std::string;

PXR_NAMESPACE_OPEN_SCOPE

using namespace pxr_boost::python;

TfPyExceptionState::TfPyExceptionState(TfPyExceptionState const &other)
{
    TfPyLock lock;
    _type = other._type;
    _value = other._value;
    _trace = other._trace;
}

TfPyExceptionState &
TfPyExceptionState::operator=(TfPyExceptionState const &other)
{
    TfPyLock lock;
    _type = other._type;
    _value = other._value;
    _trace = other._trace;
    return *this;
}

TfPyExceptionState::~TfPyExceptionState()
{
    TfPyLock lock;
    _type.reset();
    _value.reset();
    _trace.reset();
}

TfPyExceptionState
TfPyExceptionState::Fetch() {
    TfPyLock lock;
    PyObject *excType, *excValue, *excTrace;
    PyErr_Fetch(&excType, &excValue, &excTrace);
    return TfPyExceptionState {
        handle<>(allow_null(excType)),
        handle<>(allow_null(excValue)),
        handle<>(allow_null(excTrace))
    };
}

void
TfPyExceptionState::Restore()
{
    TfPyLock lock;
    // We have to call release() here since PyErr_Restore() "steals" our
    // reference count.
    PyErr_Restore(_type.release(), _value.release(), _trace.release());
}

string 
TfPyExceptionState::GetExceptionString() const
{
    TfPyLock lock;
    string s;
    // Save the exception state so we can restore it -- getting the exception
    // string should not affect the exception state.
    TfPyExceptionStateScope exceptionStateScope;
    try {
        object tbModule(handle<>(PyImport_ImportModule("traceback")));
        object exception =
            tbModule.attr("format_exception")(_type, _value, _trace);
        pxr_boost::python::ssize_t size = len(exception);
        for (pxr_boost::python::ssize_t i = 0; i != size; ++i) {
            s += extract<string>(exception[i]);
        }
    } catch (pxr_boost::python::error_already_set const &) {
        // Just ignore the exception.
    }
    return s;
}

PXR_NAMESPACE_CLOSE_SCOPE
