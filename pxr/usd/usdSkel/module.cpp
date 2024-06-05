//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/pyModule.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_WRAP_MODULE
{
    TF_WRAP(UsdSkelAnimation);
    TF_WRAP(UsdSkelAnimMapper);
    TF_WRAP(UsdSkelAnimQuery);
    TF_WRAP(UsdSkelBakeSkinning);
    TF_WRAP(UsdSkelBinding);
    TF_WRAP(UsdSkelBindingAPI);
    TF_WRAP(UsdSkelBlendShape);
    TF_WRAP(UsdSkelBlendShapeQuery);
    TF_WRAP(UsdSkelCache);
    TF_WRAP(UsdSkelInbetweenShape);
    TF_WRAP(UsdSkelSkeleton);
    TF_WRAP(UsdSkelSkeletonQuery);
    TF_WRAP(UsdSkelSkinningQuery);
    TF_WRAP(UsdSkelRoot);
    TF_WRAP(UsdSkelTokens);
    TF_WRAP(UsdSkelTopology);
    TF_WRAP(UsdSkelUtils);
}
