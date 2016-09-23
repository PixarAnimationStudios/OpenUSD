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
#ifndef TF_PYERRORINTERNAL_H
#define TF_PYERRORINTERNAL_H

#include "pxr/base/tf/pyExceptionState.h"
#include <boost/python/handle.hpp>
#include <boost/python/object_fwd.hpp>

enum Tf_PyExceptionErrorCode {
    TF_PYTHON_EXCEPTION
};

TfPyExceptionState Tf_PyFetchPythonExceptionState();
void Tf_PyRestorePythonExceptionState(TfPyExceptionState state);
boost::python::handle<> Tf_PyGetErrorExceptionClass();
void Tf_PySetErrorExceptionClass(boost::python::object const &cls);

/// RAII class to save and restore the Python exception state.  The client
/// must hold the GIL during all methods, including the c'tor and d'tor.
class TfPyExceptionStateScope {
public:
    // Save the current exception state but don't unset it.
    TfPyExceptionStateScope();
    TfPyExceptionStateScope(const TfPyExceptionStateScope&) = delete;
    TfPyExceptionStateScope& operator=(const TfPyExceptionStateScope&) = delete;

    // Restore the exception state as it was in the c'tor.
    ~TfPyExceptionStateScope();

    // Restore the exception state as it was in the c'tor.
    void Restore();

private:
    TfPyExceptionState _state;
};

#endif // TF_PYERRORINTERNAL_H
