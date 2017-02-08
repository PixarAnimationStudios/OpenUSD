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

#include "pxr/base/tf/registryManager.h"

#include <boost/python/handle.hpp>
#include <boost/python/object.hpp>

using namespace boost::python;

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfEnum) {
    TF_ADD_ENUM_NAME(TF_PYTHON_EXCEPTION);
}

// Should probably use a better mechanism.

static handle<> _ExceptionClass;

handle<> Tf_PyGetErrorExceptionClass()
{
    return _ExceptionClass;
}

void Tf_PySetErrorExceptionClass(object const &cls)
{
    _ExceptionClass = handle<>(borrowed(cls.ptr()));
}


TfPyExceptionState
Tf_PyFetchPythonExceptionState() {
    PyObject *excType, *excValue, *excTrace;
    PyErr_Fetch(&excType, &excValue, &excTrace);
    return TfPyExceptionState(handle<>(allow_null(excType)),
                              handle<>(allow_null(excValue)),
                              handle<>(allow_null(excTrace)));
}


void Tf_PyRestorePythonExceptionState(TfPyExceptionState state) {
    PyErr_Restore(state.GetType().get(),
                  state.GetValue().get(),
                  state.GetTrace().get());
    // PyErr_Restore decrements the reference counts of each of the items passed
    // to it, so we need to make sure that we release our handles to them
    // *without* decrementing the reference count (since it has been stolen).
    state.Release();
}

TfPyExceptionStateScope::TfPyExceptionStateScope() :
    _state(Tf_PyFetchPythonExceptionState())
{
    // Tf_PyFetchPythonExceptionState() clears the exception state
    // but we want it back.
    Restore();
}

TfPyExceptionStateScope::~TfPyExceptionStateScope()
{
    Restore();
}

void
TfPyExceptionStateScope::Restore()
{
    // Note that Tf_PyRestorePythonExceptionState() leaves _state with
    // a reference to the state, unlike PyErr_Restore, because it makes
    // a copy of _state and that bumps the reference counts.
    Tf_PyRestorePythonExceptionState(_state);
}

PXR_NAMESPACE_CLOSE_SCOPE
