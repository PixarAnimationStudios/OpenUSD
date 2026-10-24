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
    TF_WRAP_NB(UsdSkelAnimMapper);
    TF_WRAP_NB(UsdSkelAnimQuery);
    TF_WRAP_NB(UsdSkelBakeSkinning);
    TF_WRAP_NB(UsdSkelBinding);
    TF_WRAP_NB(UsdSkelBlendShapeQuery);
    TF_WRAP_NB(UsdSkelCache);
    TF_WRAP_NB(UsdSkelInbetweenShape);
    TF_WRAP_NB(UsdSkelSkeletonQuery);
    TF_WRAP_NB(UsdSkelSkinningQuery);
    TF_WRAP_NB(UsdSkelTopology);
    TF_WRAP_NB(UsdSkelUtils);

    // Generated Schema classes.  Do not remove or edit the following line.
    #include "generatedSchema.module.h"
}
