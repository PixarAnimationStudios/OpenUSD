//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TS_WRAP_UTILS_H
#define PXR_BASE_TS_WRAP_UTILS_H

#include "pxr/base/tf/pyAnnotatedBoolResult.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE


// Annotated bool result for Python wrappings.
struct Ts_AnnotatedBoolResult : public TfPyAnnotatedBoolResult<std::string>
{
    Ts_AnnotatedBoolResult(bool boolVal, const std::string &reasonWhyNot)
        : TfPyAnnotatedBoolResult(boolVal, reasonWhyNot) {}
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
