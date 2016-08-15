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
#ifndef TF_PYERROR_H
#define TF_PYERROR_H

/// \file tf/error.h
/// Provide facilities for error handling in script.

#include "pxr/base/tf/errorMark.h"

#include <boost/python/default_call_policies.hpp>

/// Converts any \a TfError objects in \a m into python exceptions.  User code
/// should generally not have to call this.  User code should generally not
/// have to call this, unless it's manually bridging between C++ & Python.
bool TfPyConvertTfErrorsToPythonException(TfErrorMark const &m);

/// Convert the current python exception to \a TfError objects and post them
/// to the error system.  User code should generally not have to call this,
/// unless it's manually bridging between C++ & Python.
void TfPyConvertPythonExceptionToTfErrors();

/// \class TfPyRaiseOnError
///
/// A boost.python call policy class which, when applied to a wrapped
/// function, will create an error mark before calling the function, and check
/// that error mark after the function has completed.  If any TfErrors have
/// occured, they will be raised as python exceptions.
///
/// This facility does not need to be used by clients in general.  It is only
/// required for wrapped functions and methods that do not appear directly in an
/// extension module.  For instance, the map and sequence proxy objects use
/// this, since they are created on the fly.
template <typename Base = boost::python::default_call_policies>
struct TfPyRaiseOnError : Base
{
  public:

    // This call policy provides a customized argument_package.  We need to do
    // this to store the TfErrorMark that we use to collect TfErrors that
    // occurred during the call and convert them to a python exception at the
    // end.  It doesn't work to do this in the precall() and postcall()
    // because if the call itself throws a c++ exception, the postcall() isn't
    // executed and we can't destroy the TfErrorMark, leaving it dangling.
    // Using the argument_package solves this since it is a local variable it
    // will be destroyed whether or not the call throws.  This is not really a
    // publicly documented boost.python feature, however.  :-/
    template <class BaseArgs>
    struct ErrorMarkAndArgs {
        /* implicit */ErrorMarkAndArgs(BaseArgs base_) : base(base_) {}
        operator const BaseArgs &() const { return base; }
        operator BaseArgs &() { return base; }
        BaseArgs base;
        TfErrorMark errorMark;
    };
    typedef ErrorMarkAndArgs<typename Base::argument_package> argument_package;

    /// Default constructor.
    TfPyRaiseOnError() {}

    // Only accept our argument_package type, since we must ensure that we're
    // using it so we track a TfErrorMark.
    bool precall(argument_package const &a) { return Base::precall(a); }

    // Only accept our argument_package type, since we must ensure that we're
    // using it so we track a TfErrorMark.
    PyObject *postcall(argument_package const &a, PyObject *result) {
        result = Base::postcall(a, result);
        if (result and TfPyConvertTfErrorsToPythonException(a.errorMark)) {
            Py_DECREF(result);
            result = NULL;
        }
        return result;
    }
};

struct Tf_PyErrorClearer {
    Tf_PyErrorClearer() : clearOnDestruction(true) {}
    ~Tf_PyErrorClearer() {
        if (clearOnDestruction)
            mark.Clear();
    }
    void Dismiss() { clearOnDestruction = false; }
    TfErrorMark mark;
    bool clearOnDestruction;
};

#endif // TF_PYERROR_H
