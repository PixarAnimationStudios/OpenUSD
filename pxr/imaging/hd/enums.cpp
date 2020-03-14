//
// Copyright 2018 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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
