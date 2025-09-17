//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/interpolation.h"

#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(UsdInterpolationTypeHeld, "Held");
    TF_ADD_ENUM_NAME(UsdInterpolationTypeLinear, "Linear");
}

PXR_NAMESPACE_CLOSE_SCOPE

