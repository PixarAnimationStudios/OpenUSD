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
#ifndef PXR_BASE_TF_PY_INTERPRETER_H
#define PXR_BASE_TF_PY_INTERPRETER_H

/// \file tf/pyInterpreter.h
/// Python runtime utilities.

#include "pxr/pxr.h"

#include "pxr/base/tf/api.h"
#include <boost/python/handle.hpp>
#include <boost/python/object.hpp>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// Starts up the python runtime.
/// IMPORTANT: The GIL will NOT be held when this returns.
/// (If Python is already initialized, the GIL will be unchanged)
///
/// The program name and arguments are set automatically. sys.argv has no
/// arguments other than an argv[0] matching the program name.
TF_API
extern void TfPyInitialize();

/// Runs the given string using PyRun_SimpleString().
///
/// Starts the interpreter if necessary. Deals with necessary thread state
/// setup.
///
/// Callers are not required to hold the GIL, though of course they may.
///
/// Returns zero on success, nonzero on failure.
TF_API
extern int TfPyRunSimpleString(const std::string & cmd);

/// Runs the given string using PyRun_String().
///
/// \a start is Py_eval_input, Py_single_input or Py_file_input.
/// \a globals and locals can be dictionaries to use when evaluating the 
///    string in python. Defaults to reusing globals from main module. If
///    only the globals are provided, they will also be used as locals.
///
/// Starts the interpreter if necessary. Deals with necessary thread state
/// setup.
///
/// Callers must hold the GIL before calling; see TfPyLock.  This is true for
/// any boost::python call, including constructing and destroying the default
/// values of \p globals and \p locals.  Holding the GIL will also make it safe
/// to inspect the returned boost::python::handle.
TF_API
extern boost::python::handle<>
TfPyRunString(const std::string & cmd, int start,
              boost::python::object const &globals = boost::python::object(),
              boost::python::object const &locals = boost::python::object()
              );

/// Runs the given file using PyRun_File().
///
/// \a start is Py_eval_input, Py_single_input or Py_file_input.
/// \a globals and locals can be dictionaries to use when evaluating the 
///    string in python. Defaults to reusing globals from main module. If
///    only the globals are provided, they will also be used as locals.
///
/// Starts the interpreter if necessary. Deals with necessary thread state
/// setup.
///
/// Callers must hold the GIL before calling; see TfPyLock.  This is true for
/// any boost::python call, including constructing and destroying the default
/// values of \p globals and \p locals.  Holding the GIL will also make it safe
/// to inspect the returned boost::python::handle.
TF_API
extern boost::python::handle<>
TfPyRunFile(const std::string &filename, int start,
            boost::python::object const &globals = boost::python::object(),
            boost::python::object const &locals = boost::python::object()
            );

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_PY_INTERPRETER_H
