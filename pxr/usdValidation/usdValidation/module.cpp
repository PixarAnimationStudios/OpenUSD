//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/pyModule.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_WRAP_MODULE
{
    TF_WRAP_NB(UsdValidationRegistry);
    TF_WRAP_NB(UsdValidationTimeRange);
    TF_WRAP_NB(UsdValidationValidator);
    TF_WRAP_NB(UsdValidationFixer);
    TF_WRAP_NB(UsdValidationError);
    TF_WRAP_NB(UsdValidationContext);
    TF_WRAP_NB(UsdValidationNotice);
}
