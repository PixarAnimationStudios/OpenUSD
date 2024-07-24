//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/enum.h"

#include "pxr/usd/usd/common.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(
    USD_SHADING_MODEL, "usdRi",
    "Set to usdRi when models can interchange UsdShade prims.");

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(UsdListPositionFrontOfPrependList,
                     "The front of the prepend list.");
    TF_ADD_ENUM_NAME(UsdListPositionBackOfPrependList,
                     "The back of the prepend list.");
    TF_ADD_ENUM_NAME(UsdListPositionFrontOfAppendList,
                     "The front of the append list.");
    TF_ADD_ENUM_NAME(UsdListPositionBackOfAppendList,
                     "The back of the append list.");

    TF_ADD_ENUM_NAME(UsdLoadWithDescendants, "Load prim and all descendants");
    TF_ADD_ENUM_NAME(UsdLoadWithoutDescendants, "Load prim and no descendants");
}

PXR_NAMESPACE_CLOSE_SCOPE

