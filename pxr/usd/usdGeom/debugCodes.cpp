//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/debugCodes.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(USDGEOM_EXTENT, "Reports when Boundable "
            "extents are computed dynamically because no cached authored "
            "attribute is present in the scene.");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USDGEOM_BBOX, "UsdGeom bounding box computation");
}

PXR_NAMESPACE_CLOSE_SCOPE

