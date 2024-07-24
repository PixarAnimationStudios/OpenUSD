//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/types.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE

const double TsTraits<double>::zero = VtZero<double>();
const float TsTraits<float>::zero = VtZero<float>();
const int TsTraits<int>::zero = VtZero<int>();
const bool TsTraits<bool>::zero = VtZero<bool>();
const GfVec2d TsTraits<GfVec2d>::zero = VtZero<GfVec2d>();
const GfVec2f TsTraits<GfVec2f>::zero = VtZero<GfVec2f>();
const GfVec3d TsTraits<GfVec3d>::zero = VtZero<GfVec3d>();
const GfVec3f TsTraits<GfVec3f>::zero = VtZero<GfVec3f>();
const GfVec4d TsTraits<GfVec4d>::zero = VtZero<GfVec4d>();
const GfVec4f TsTraits<GfVec4f>::zero = VtZero<GfVec4f>();
const GfQuatf TsTraits<GfQuatf>::zero = VtZero<GfQuatf>();
const GfQuatd TsTraits<GfQuatd>::zero = VtZero<GfQuatd>();

const GfMatrix2d TsTraits<GfMatrix2d>::zero = VtZero<GfMatrix2d>();
const GfMatrix3d TsTraits<GfMatrix3d>::zero = VtZero<GfMatrix3d>();
const GfMatrix4d TsTraits<GfMatrix4d>::zero = VtZero<GfMatrix4d>();
const std::string TsTraits<std::string>::zero = VtZero<std::string>();

const VtArray<double> TsTraits< VtArray<double> >::zero;
const VtArray<float> TsTraits< VtArray<float> >::zero;

const TfToken TsTraits<TfToken>::zero;

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(TsLeft, "left");
    TF_ADD_ENUM_NAME(TsRight, "right");
}

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(TsKnotHeld, "held");
    TF_ADD_ENUM_NAME(TsKnotLinear, "linear");
    TF_ADD_ENUM_NAME(TsKnotBezier, "bezier");
}

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(TsExtrapolationHeld, "held");
    TF_ADD_ENUM_NAME(TsExtrapolationLinear, "linear");
}

PXR_NAMESPACE_CLOSE_SCOPE
