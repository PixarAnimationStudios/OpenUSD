//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(HdInterpolationConstant, "constant");
    TF_ADD_ENUM_NAME(HdInterpolationUniform, "uniform");
    TF_ADD_ENUM_NAME(HdInterpolationVarying, "varying");
    TF_ADD_ENUM_NAME(HdInterpolationVertex, "vertex");
    TF_ADD_ENUM_NAME(HdInterpolationFaceVarying, "faceVarying");
    TF_ADD_ENUM_NAME(HdInterpolationInstance, "instance");
}

HdCullStyle
HdInvertCullStyle(HdCullStyle cs)
{
    switch(cs) {
    case HdCullStyleDontCare:
        return HdCullStyleDontCare;
    case HdCullStyleNothing:
        return HdCullStyleNothing;
    case HdCullStyleBack:
        return HdCullStyleFront;
    case HdCullStyleFront:
        return HdCullStyleBack;
    case HdCullStyleBackUnlessDoubleSided:
        return HdCullStyleFrontUnlessDoubleSided;
    case HdCullStyleFrontUnlessDoubleSided:
        return HdCullStyleBackUnlessDoubleSided;
    default:
        return cs;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
