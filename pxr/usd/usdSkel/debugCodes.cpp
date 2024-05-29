//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdSkel/debugCodes.h"

#include "pxr/base/tf/registryManager.h"


PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(USDSKEL_CACHE, "UsdSkel cache population.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(USDSKEL_BAKESKINNING,
                                "UsdSkelBakeSkinningLBS() method.");
}


PXR_NAMESPACE_CLOSE_SCOPE
