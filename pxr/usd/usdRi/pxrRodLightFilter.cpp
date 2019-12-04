//
// Copyright 2016 Pixar
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
#include "pxr/usd/usdRi/pxrRodLightFilter.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdRiPxrRodLightFilter,
        TfType::Bases< UsdLuxLightFilter > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("PxrRodLightFilter")
    // to find TfType<UsdRiPxrRodLightFilter>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdRiPxrRodLightFilter>("PxrRodLightFilter");
}

/* virtual */
UsdRiPxrRodLightFilter::~UsdRiPxrRodLightFilter()
{
}

/* static */
UsdRiPxrRodLightFilter
UsdRiPxrRodLightFilter::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRiPxrRodLightFilter();
    }
    return UsdRiPxrRodLightFilter(stage->GetPrimAtPath(path));
}

/* static */
UsdRiPxrRodLightFilter
UsdRiPxrRodLightFilter::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("PxrRodLightFilter");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRiPxrRodLightFilter();
    }
    return UsdRiPxrRodLightFilter(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaType UsdRiPxrRodLightFilter::_GetSchemaType() const {
    return UsdRiPxrRodLightFilter::schemaType;
}

/* static */
const TfType &
UsdRiPxrRodLightFilter::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdRiPxrRodLightFilter>();
    return tfType;
}

/* static */
bool 
UsdRiPxrRodLightFilter::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdRiPxrRodLightFilter::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdRiPxrRodLightFilter::GetWidthAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->width);
}

UsdAttribute
UsdRiPxrRodLightFilter::CreateWidthAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->width,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrRodLightFilter::GetHeightAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->height);
}

UsdAttribute
UsdRiPxrRodLightFilter::CreateHeightAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->height,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrRodLightFilter::GetDepthAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->depth);
}

UsdAttribute
UsdRiPxrRodLightFilter::CreateDepthAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->depth,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrRodLightFilter::GetRadiusAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->radius);
}

UsdAttribute
UsdRiPxrRodLightFilter::CreateRadiusAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->radius,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrRodLightFilter::GetEdgeThicknessAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->edgeThickness);
}

UsdAttribute
UsdRiPxrRodLightFilter::CreateEdgeThicknessAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->edgeThickness,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrRodLightFilter::GetScaleWidthAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->scaleWidth);
}

UsdAttribute
UsdRiPxrRodLightFilter::CreateScaleWidthAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->scaleWidth,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrRodLightFilter::GetScaleHeightAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->scaleHeight);
}

UsdAttribute
UsdRiPxrRodLightFilter::CreateScaleHeightAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->scaleHeight,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrRodLightFilter::GetScaleDepthAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->scaleDepth);
}

UsdAttribute
UsdRiPxrRodLightFilter::CreateScaleDepthAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->scaleDepth,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrRodLightFilter::GetRefineTopAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->refineTop);
}

UsdAttribute
UsdRiPxrRodLightFilter::CreateRefineTopAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->refineTop,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrRodLightFilter::GetRefineBottomAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->refineBottom);
}

UsdAttribute
UsdRiPxrRodLightFilter::CreateRefineBottomAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->refineBottom,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrRodLightFilter::GetRefineLeftAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->refineLeft);
}

UsdAttribute
UsdRiPxrRodLightFilter::CreateRefineLeftAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->refineLeft,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrRodLightFilter::GetRefineRightAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->refineRight);
}

UsdAttribute
UsdRiPxrRodLightFilter::CreateRefineRightAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->refineRight,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrRodLightFilter::GetRefineFrontAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->refineFront);
}

UsdAttribute
UsdRiPxrRodLightFilter::CreateRefineFrontAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->refineFront,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrRodLightFilter::GetRefineBackAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->refineBack);
}

UsdAttribute
UsdRiPxrRodLightFilter::CreateRefineBackAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->refineBack,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrRodLightFilter::GetEdgeScaleTopAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->edgeScaleTop);
}

UsdAttribute
UsdRiPxrRodLightFilter::CreateEdgeScaleTopAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->edgeScaleTop,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrRodLightFilter::GetEdgeScaleBottomAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->edgeScaleBottom);
}

UsdAttribute
UsdRiPxrRodLightFilter::CreateEdgeScaleBottomAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->edgeScaleBottom,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrRodLightFilter::GetEdgeScaleLeftAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->edgeScaleLeft);
}

UsdAttribute
UsdRiPxrRodLightFilter::CreateEdgeScaleLeftAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->edgeScaleLeft,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrRodLightFilter::GetEdgeScaleRightAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->edgeScaleRight);
}

UsdAttribute
UsdRiPxrRodLightFilter::CreateEdgeScaleRightAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->edgeScaleRight,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrRodLightFilter::GetEdgeScaleFrontAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->edgeScaleFront);
}

UsdAttribute
UsdRiPxrRodLightFilter::CreateEdgeScaleFrontAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->edgeScaleFront,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrRodLightFilter::GetEdgeScaleBackAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->edgeScaleBack);
}

UsdAttribute
UsdRiPxrRodLightFilter::CreateEdgeScaleBackAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->edgeScaleBack,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrRodLightFilter::GetColorSaturationAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->colorSaturation);
}

UsdAttribute
UsdRiPxrRodLightFilter::CreateColorSaturationAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->colorSaturation,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrRodLightFilter::GetFalloffAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->falloff);
}

UsdAttribute
UsdRiPxrRodLightFilter::CreateFalloffAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->falloff,
                       SdfValueTypeNames->Int,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrRodLightFilter::GetFalloffKnotsAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->falloffKnots);
}

UsdAttribute
UsdRiPxrRodLightFilter::CreateFalloffKnotsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->falloffKnots,
                       SdfValueTypeNames->FloatArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrRodLightFilter::GetFalloffFloatsAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->falloffFloats);
}

UsdAttribute
UsdRiPxrRodLightFilter::CreateFalloffFloatsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->falloffFloats,
                       SdfValueTypeNames->FloatArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrRodLightFilter::GetFalloffInterpolationAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->falloffInterpolation);
}

UsdAttribute
UsdRiPxrRodLightFilter::CreateFalloffInterpolationAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->falloffInterpolation,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrRodLightFilter::GetColorRampAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->colorRamp);
}

UsdAttribute
UsdRiPxrRodLightFilter::CreateColorRampAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->colorRamp,
                       SdfValueTypeNames->Int,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrRodLightFilter::GetColorRampKnotsAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->colorRampKnots);
}

UsdAttribute
UsdRiPxrRodLightFilter::CreateColorRampKnotsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->colorRampKnots,
                       SdfValueTypeNames->FloatArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrRodLightFilter::GetColorRampColorsAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->colorRampColors);
}

UsdAttribute
UsdRiPxrRodLightFilter::CreateColorRampColorsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->colorRampColors,
                       SdfValueTypeNames->Color3fArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrRodLightFilter::GetColorRampInterpolationAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->colorRampInterpolation);
}

UsdAttribute
UsdRiPxrRodLightFilter::CreateColorRampInterpolationAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->colorRampInterpolation,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

namespace {
static inline TfTokenVector
_ConcatenateAttributeNames(const TfTokenVector& left,const TfTokenVector& right)
{
    TfTokenVector result;
    result.reserve(left.size() + right.size());
    result.insert(result.end(), left.begin(), left.end());
    result.insert(result.end(), right.begin(), right.end());
    return result;
}
}

/*static*/
const TfTokenVector&
UsdRiPxrRodLightFilter::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdRiTokens->width,
        UsdRiTokens->height,
        UsdRiTokens->depth,
        UsdRiTokens->radius,
        UsdRiTokens->edgeThickness,
        UsdRiTokens->scaleWidth,
        UsdRiTokens->scaleHeight,
        UsdRiTokens->scaleDepth,
        UsdRiTokens->refineTop,
        UsdRiTokens->refineBottom,
        UsdRiTokens->refineLeft,
        UsdRiTokens->refineRight,
        UsdRiTokens->refineFront,
        UsdRiTokens->refineBack,
        UsdRiTokens->edgeScaleTop,
        UsdRiTokens->edgeScaleBottom,
        UsdRiTokens->edgeScaleLeft,
        UsdRiTokens->edgeScaleRight,
        UsdRiTokens->edgeScaleFront,
        UsdRiTokens->edgeScaleBack,
        UsdRiTokens->colorSaturation,
        UsdRiTokens->falloff,
        UsdRiTokens->falloffKnots,
        UsdRiTokens->falloffFloats,
        UsdRiTokens->falloffInterpolation,
        UsdRiTokens->colorRamp,
        UsdRiTokens->colorRampKnots,
        UsdRiTokens->colorRampColors,
        UsdRiTokens->colorRampInterpolation,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdLuxLightFilter::GetSchemaAttributeNames(true),
            localNames);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

PXR_NAMESPACE_CLOSE_SCOPE

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
//
// Just remember to wrap code in the appropriate delimiters:
// 'PXR_NAMESPACE_OPEN_SCOPE', 'PXR_NAMESPACE_CLOSE_SCOPE'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (falloffRamp)
    (colorRamp)
    );

UsdRiSplineAPI
UsdRiPxrRodLightFilter::GetFalloffRampAPI() const
{
    return UsdRiSplineAPI(*this, _tokens->falloffRamp,
                          SdfValueTypeNames->FloatArray,
                          /* duplicate */ true);
}

UsdRiSplineAPI 
UsdRiPxrRodLightFilter::GetColorRampAPI() const
{
    return UsdRiSplineAPI(*this, _tokens->colorRamp,
                          SdfValueTypeNames->Color3fArray,
                          /* duplicate */ true);
}

PXR_NAMESPACE_CLOSE_SCOPE
