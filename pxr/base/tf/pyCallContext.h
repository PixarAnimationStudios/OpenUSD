//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_PY_CALL_CONTEXT_H
#define PXR_BASE_TF_PY_CALL_CONTEXT_H

#include "pxr/base/tf/callContext.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_API TfCallContext
Tf_PythonCallContext(char const *fileName,
                     char const *moduleName,
                     char const *functionName,
                     size_t line);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_PY_CALL_CONTEXT_H
