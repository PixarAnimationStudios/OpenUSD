//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_PY_INTERPRETER_H
#define PXR_BASE_TF_PY_INTERPRETER_H

/// \file tf/pyInterpreter.h
/// Python runtime utilities.

#include "pxr/pxr.h"

#include "pxr/base/tf/api.h"
#include "pxr/base/tf/pySafePython.h"
#include <nanobind/nanobind.h>
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
/// Callers must hold the GIL before calling; see TfPyLock.
TF_API
extern nanobind::object
TfPyRunString(const std::string & cmd, int start,
              nanobind::object const &globals = nanobind::object(),
              nanobind::object const &locals = nanobind::object()
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
/// Callers must hold the GIL before calling; see TfPyLock.
TF_API
extern nanobind::object
TfPyRunFile(const std::string &filename, int start,
            nanobind::object const &globals = nanobind::object(),
            nanobind::object const &locals = nanobind::object()
            );

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_PY_INTERPRETER_H
