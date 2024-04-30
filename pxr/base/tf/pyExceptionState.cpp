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

#include "pxr/pxr.h"

#include "pxr/base/tf/pyErrorInternal.h"
#include "pxr/base/tf/pyLock.h"

#include <boost/python/object.hpp>
#include <boost/python/extract.hpp>

using namespace boost::python;
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
        boost::python::ssize_t size = len(exception);
        for (boost::python::ssize_t i = 0; i != size; ++i) {
            s += extract<string>(exception[i]);
        }
    } catch (boost::python::error_already_set const &) {
        // Just ignore the exception.
    }
    return s;
}

PXR_NAMESPACE_CLOSE_SCOPE
