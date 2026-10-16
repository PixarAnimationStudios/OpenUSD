//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/pyErrorInternal.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyNanobindBridge.h"

using std::string;

PXR_NAMESPACE_OPEN_SCOPE

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
        nanobind::steal<nanobind::object>(excType),
        nanobind::steal<nanobind::object>(excValue),
        nanobind::steal<nanobind::object>(excTrace)
    };
}

void
TfPyExceptionState::Restore()
{
    TfPyLock lock;
    // We have to call release() here since PyErr_Restore() "steals" our
    // reference count.
    PyErr_Restore(_type.release().ptr(), _value.release().ptr(),
                  _trace.release().ptr());
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
        nanobind::object tbModule = nanobind::module_::import_("traceback");
        nanobind::object exception =
            tbModule.attr("format_exception")(
                _type.is_valid() ? _type : nanobind::none(),
                _value.is_valid() ? _value : nanobind::none(),
                _trace.is_valid() ? _trace : nanobind::none());
        Py_ssize_t size = nanobind::len(exception);
        for (Py_ssize_t i = 0; i != size; ++i) {
            s += nanobind::cast<string>(exception[i]);
        }
    } catch (nanobind::python_error const &) {
        // Just ignore the exception.
    } catch (std::exception const &) {
        PyErr_Clear();
    }
    return s;
}

PXR_NAMESPACE_CLOSE_SCOPE
