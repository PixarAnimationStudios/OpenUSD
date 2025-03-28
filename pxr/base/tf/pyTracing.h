//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_PY_TRACING_H
#define PXR_BASE_TF_PY_TRACING_H

#include "pxr/pxr.h"

#ifdef PXR_PYTHON_SUPPORT_ENABLED
#include "pxr/base/tf/pySafePython.h"
#endif // PXR_PYTHON_SUPPORT_ENABLED

#include "pxr/base/tf/api.h"

#include <functional>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

#ifdef PXR_PYTHON_SUPPORT_ENABLED
/// \struct TfPyTraceInfo
/// Structure passed to python trace functions.  See the Python C API
/// documentation reference for the meaning of \a what and \a arg.
struct TfPyTraceInfo {
    PyObject *arg;
    char const *funcName;
    char const *fileName;
    int funcLine;
    int what;
};

typedef std::function<void (TfPyTraceInfo const &)> TfPyTraceFn;
typedef std::shared_ptr<TfPyTraceFn> TfPyTraceFnId;

/// Register \a f as a python trace function.
/// It will be invoked for python tracing events. If python is not yet
/// initialized, the function will not be invoked until python is initialized.
TF_API TfPyTraceFnId TfPyRegisterTraceFn(TfPyTraceFn const &f);

// For internal use only.  Do not use.
void Tf_PyFabricateTraceEvent(TfPyTraceInfo const &info);

// For internal use only.  Do not use.
void Tf_PyTracingPythonInitialized();
#else
/// \struct TfPyTraceInfo
/// For storage alignment when PXR_PYTHON_SUPPORT_ENABLED is not enabled.
struct TfPyTraceInfo;
typedef std::function<void (TfPyTraceInfo const &)> TfPyTraceFn;
typedef std::shared_ptr<TfPyTraceFn> TfPyTraceFnId;
#endif // PXR_PYTHON_SUPPORT_ENABLED

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_PY_TRACING_H
