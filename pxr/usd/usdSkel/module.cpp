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
    TF_WRAP(UsdSkelAnimMapper);
    TF_WRAP(UsdSkelAnimQuery);
    TF_WRAP(UsdSkelBakeSkinning);
    TF_WRAP(UsdSkelBinding);
    TF_WRAP(UsdSkelBlendShapeQuery);
    TF_WRAP(UsdSkelCache);
    TF_WRAP(UsdSkelInbetweenShape);
    TF_WRAP(UsdSkelSkeletonQuery);
    TF_WRAP(UsdSkelSkinningQuery);
    TF_WRAP(UsdSkelTopology);
    TF_WRAP(UsdSkelUtils);

    // Generated Schema classes.  Do not remove or edit the following line.
    #include "generatedSchema.module.h"
}
