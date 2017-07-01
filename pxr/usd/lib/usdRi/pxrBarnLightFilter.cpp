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
#include "pxr/usd/usdRi/pxrBarnLightFilter.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdRiPxrBarnLightFilter,
        TfType::Bases< UsdLuxLightFilter > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("PxrBarnLightFilter")
    // to find TfType<UsdRiPxrBarnLightFilter>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdRiPxrBarnLightFilter>("PxrBarnLightFilter");
}

/* virtual */
UsdRiPxrBarnLightFilter::~UsdRiPxrBarnLightFilter()
{
}

/* static */
UsdRiPxrBarnLightFilter
UsdRiPxrBarnLightFilter::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRiPxrBarnLightFilter();
    }
    return UsdRiPxrBarnLightFilter(stage->GetPrimAtPath(path));
}

/* static */
UsdRiPxrBarnLightFilter
UsdRiPxrBarnLightFilter::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("PxrBarnLightFilter");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRiPxrBarnLightFilter();
    }
    return UsdRiPxrBarnLightFilter(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* static */
const TfType &
UsdRiPxrBarnLightFilter::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdRiPxrBarnLightFilter>();
    return tfType;
}

/* static */
bool 
UsdRiPxrBarnLightFilter::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdRiPxrBarnLightFilter::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdRiPxrBarnLightFilter::GetBarnModeAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->barnMode);
}

UsdAttribute
UsdRiPxrBarnLightFilter::CreateBarnModeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->barnMode,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrBarnLightFilter::GetWidthAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->width);
}

UsdAttribute
UsdRiPxrBarnLightFilter::CreateWidthAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->width,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrBarnLightFilter::GetHeightAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->height);
}

UsdAttribute
UsdRiPxrBarnLightFilter::CreateHeightAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->height,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrBarnLightFilter::GetRadiusAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->radius);
}

UsdAttribute
UsdRiPxrBarnLightFilter::CreateRadiusAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->radius,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrBarnLightFilter::GetAnalyticDirectionalAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->analyticDirectional);
}

UsdAttribute
UsdRiPxrBarnLightFilter::CreateAnalyticDirectionalAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->analyticDirectional,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrBarnLightFilter::GetAnalyticShearXAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->analyticShearX);
}

UsdAttribute
UsdRiPxrBarnLightFilter::CreateAnalyticShearXAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->analyticShearX,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrBarnLightFilter::GetAnalyticShearYAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->analyticShearY);
}

UsdAttribute
UsdRiPxrBarnLightFilter::CreateAnalyticShearYAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->analyticShearY,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrBarnLightFilter::GetAnalyticApexAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->analyticApex);
}

UsdAttribute
UsdRiPxrBarnLightFilter::CreateAnalyticApexAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->analyticApex,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrBarnLightFilter::GetAnalyticUseLightDirectionAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->analyticUseLightDirection);
}

UsdAttribute
UsdRiPxrBarnLightFilter::CreateAnalyticUseLightDirectionAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->analyticUseLightDirection,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrBarnLightFilter::GetAnalyticDensityNearDistanceAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->analyticDensityNearDistance);
}

UsdAttribute
UsdRiPxrBarnLightFilter::CreateAnalyticDensityNearDistanceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->analyticDensityNearDistance,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrBarnLightFilter::GetAnalyticDensityFarDistanceAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->analyticDensityFarDistance);
}

UsdAttribute
UsdRiPxrBarnLightFilter::CreateAnalyticDensityFarDistanceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->analyticDensityFarDistance,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrBarnLightFilter::GetAnalyticDensityNearValueAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->analyticDensityNearValue);
}

UsdAttribute
UsdRiPxrBarnLightFilter::CreateAnalyticDensityNearValueAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->analyticDensityNearValue,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrBarnLightFilter::GetAnalyticDensityFarValueAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->analyticDensityFarValue);
}

UsdAttribute
UsdRiPxrBarnLightFilter::CreateAnalyticDensityFarValueAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->analyticDensityFarValue,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrBarnLightFilter::GetAnalyticDensityExponentAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->analyticDensityExponent);
}

UsdAttribute
UsdRiPxrBarnLightFilter::CreateAnalyticDensityExponentAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->analyticDensityExponent,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrBarnLightFilter::GetEdgeThicknessAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->edgeThickness);
}

UsdAttribute
UsdRiPxrBarnLightFilter::CreateEdgeThicknessAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->edgeThickness,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrBarnLightFilter::GetPreBarnEffectAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->preBarnEffect);
}

UsdAttribute
UsdRiPxrBarnLightFilter::CreatePreBarnEffectAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->preBarnEffect,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrBarnLightFilter::GetScaleWidthAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->scaleWidth);
}

UsdAttribute
UsdRiPxrBarnLightFilter::CreateScaleWidthAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->scaleWidth,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrBarnLightFilter::GetScaleHeightAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->scaleHeight);
}

UsdAttribute
UsdRiPxrBarnLightFilter::CreateScaleHeightAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->scaleHeight,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrBarnLightFilter::GetRefineTopAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->refineTop);
}

UsdAttribute
UsdRiPxrBarnLightFilter::CreateRefineTopAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->refineTop,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrBarnLightFilter::GetRefineBottomAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->refineBottom);
}

UsdAttribute
UsdRiPxrBarnLightFilter::CreateRefineBottomAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->refineBottom,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrBarnLightFilter::GetRefineLeftAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->refineLeft);
}

UsdAttribute
UsdRiPxrBarnLightFilter::CreateRefineLeftAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->refineLeft,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrBarnLightFilter::GetRefineRightAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->refineRight);
}

UsdAttribute
UsdRiPxrBarnLightFilter::CreateRefineRightAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->refineRight,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrBarnLightFilter::GetEdgeTopAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->edgeTop);
}

UsdAttribute
UsdRiPxrBarnLightFilter::CreateEdgeTopAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->edgeTop,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrBarnLightFilter::GetEdgeBottomAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->edgeBottom);
}

UsdAttribute
UsdRiPxrBarnLightFilter::CreateEdgeBottomAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->edgeBottom,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrBarnLightFilter::GetEdgeLeftAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->edgeLeft);
}

UsdAttribute
UsdRiPxrBarnLightFilter::CreateEdgeLeftAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->edgeLeft,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrBarnLightFilter::GetEdgeRightAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->edgeRight);
}

UsdAttribute
UsdRiPxrBarnLightFilter::CreateEdgeRightAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->edgeRight,
                       SdfValueTypeNames->Float,
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
UsdRiPxrBarnLightFilter::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdRiTokens->barnMode,
        UsdRiTokens->width,
        UsdRiTokens->height,
        UsdRiTokens->radius,
        UsdRiTokens->analyticDirectional,
        UsdRiTokens->analyticShearX,
        UsdRiTokens->analyticShearY,
        UsdRiTokens->analyticApex,
        UsdRiTokens->analyticUseLightDirection,
        UsdRiTokens->analyticDensityNearDistance,
        UsdRiTokens->analyticDensityFarDistance,
        UsdRiTokens->analyticDensityNearValue,
        UsdRiTokens->analyticDensityFarValue,
        UsdRiTokens->analyticDensityExponent,
        UsdRiTokens->edgeThickness,
        UsdRiTokens->preBarnEffect,
        UsdRiTokens->scaleWidth,
        UsdRiTokens->scaleHeight,
        UsdRiTokens->refineTop,
        UsdRiTokens->refineBottom,
        UsdRiTokens->refineLeft,
        UsdRiTokens->refineRight,
        UsdRiTokens->edgeTop,
        UsdRiTokens->edgeBottom,
        UsdRiTokens->edgeLeft,
        UsdRiTokens->edgeRight,
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
