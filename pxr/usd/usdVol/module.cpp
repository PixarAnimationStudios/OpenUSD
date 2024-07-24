//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/pyModule.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_WRAP_MODULE
{
    TF_WRAP(UsdVolTokens);
    TF_WRAP(UsdVolVolume);
    TF_WRAP(UsdVolFieldBase);
    TF_WRAP(UsdVolFieldAsset);
    TF_WRAP(UsdVolField3DAsset);
    TF_WRAP(UsdVolOpenVDBAsset);
}
