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
#include "pxr/usd/usdRi/pxrCookieLightFilter.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdRiPxrCookieLightFilter,
        TfType::Bases< UsdLuxLightFilter > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("PxrCookieLightFilter")
    // to find TfType<UsdRiPxrCookieLightFilter>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdRiPxrCookieLightFilter>("PxrCookieLightFilter");
}

/* virtual */
UsdRiPxrCookieLightFilter::~UsdRiPxrCookieLightFilter()
{
}

/* static */
UsdRiPxrCookieLightFilter
UsdRiPxrCookieLightFilter::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRiPxrCookieLightFilter();
    }
    return UsdRiPxrCookieLightFilter(stage->GetPrimAtPath(path));
}

/* static */
UsdRiPxrCookieLightFilter
UsdRiPxrCookieLightFilter::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("PxrCookieLightFilter");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRiPxrCookieLightFilter();
    }
    return UsdRiPxrCookieLightFilter(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* static */
const TfType &
UsdRiPxrCookieLightFilter::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdRiPxrCookieLightFilter>();
    return tfType;
}

/* static */
bool 
UsdRiPxrCookieLightFilter::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdRiPxrCookieLightFilter::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetCookieModeAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->cookieMode);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateCookieModeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->cookieMode,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetWidthAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->width);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateWidthAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->width,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetHeightAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->height);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateHeightAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->height,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetTextureMapAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->textureMap);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateTextureMapAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->textureMap,
                       SdfValueTypeNames->Asset,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetTextureWrapModeAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->textureWrapMode);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateTextureWrapModeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->textureWrapMode,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetTextureFillColorAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->textureFillColor);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateTextureFillColorAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->textureFillColor,
                       SdfValueTypeNames->Color3f,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetTextureInvertUAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->textureInvertU);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateTextureInvertUAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->textureInvertU,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetTextureInvertVAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->textureInvertV);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateTextureInvertVAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->textureInvertV,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetTextureScaleUAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->textureScaleU);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateTextureScaleUAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->textureScaleU,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetTextureScaleVAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->textureScaleV);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateTextureScaleVAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->textureScaleV,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetTextureOffsetUAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->textureOffsetU);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateTextureOffsetUAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->textureOffsetU,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetTextureOffsetVAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->textureOffsetV);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateTextureOffsetVAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->textureOffsetV,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetAnalyticDirectionalAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->analyticDirectional);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateAnalyticDirectionalAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->analyticDirectional,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetAnalyticShearXAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->analyticShearX);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateAnalyticShearXAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->analyticShearX,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetAnalyticShearYAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->analyticShearY);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateAnalyticShearYAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->analyticShearY,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetAnalyticApexAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->analyticApex);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateAnalyticApexAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->analyticApex,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetAnalyticUseLightDirectionAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->analyticUseLightDirection);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateAnalyticUseLightDirectionAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->analyticUseLightDirection,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetAnalyticBlurAmountAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->analyticBlurAmount);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateAnalyticBlurAmountAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->analyticBlurAmount,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetAnalyticBlurSMultAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->analyticBlurSMult);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateAnalyticBlurSMultAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->analyticBlurSMult,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetAnalyticBlurTMultAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->analyticBlurTMult);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateAnalyticBlurTMultAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->analyticBlurTMult,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetAnalyticBlurNearDistanceAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->analyticBlurNearDistance);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateAnalyticBlurNearDistanceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->analyticBlurNearDistance,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetAnalyticBlurMidpointAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->analyticBlurMidpoint);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateAnalyticBlurMidpointAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->analyticBlurMidpoint,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetAnalyticBlurFarDistanceAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->analyticBlurFarDistance);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateAnalyticBlurFarDistanceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->analyticBlurFarDistance,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetAnalyticBlurNearValueAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->analyticBlurNearValue);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateAnalyticBlurNearValueAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->analyticBlurNearValue,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetAnalyticBlurMidValueAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->analyticBlurMidValue);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateAnalyticBlurMidValueAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->analyticBlurMidValue,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetAnalyticBlurFarValueAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->analyticBlurFarValue);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateAnalyticBlurFarValueAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->analyticBlurFarValue,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetAnalyticBlurExponentAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->analyticBlurExponent);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateAnalyticBlurExponentAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->analyticBlurExponent,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetAnalyticDensityNearDistanceAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->analyticDensityNearDistance);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateAnalyticDensityNearDistanceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->analyticDensityNearDistance,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetAnalyticDensityMidpointAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->analyticDensityMidpoint);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateAnalyticDensityMidpointAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->analyticDensityMidpoint,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetAnalyticDensityFarDistanceAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->analyticDensityFarDistance);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateAnalyticDensityFarDistanceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->analyticDensityFarDistance,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetAnalyticDensityNearValueAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->analyticDensityNearValue);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateAnalyticDensityNearValueAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->analyticDensityNearValue,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetAnalyticDensityMidValueAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->analyticDensityMidValue);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateAnalyticDensityMidValueAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->analyticDensityMidValue,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetAnalyticDensityFarValueAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->analyticDensityFarValue);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateAnalyticDensityFarValueAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->analyticDensityFarValue,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetAnalyticDensityExponentAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->analyticDensityExponent);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateAnalyticDensityExponentAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->analyticDensityExponent,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetColorSaturationAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->colorSaturation);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateColorSaturationAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->colorSaturation,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetColorMidpointAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->colorMidpoint);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateColorMidpointAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->colorMidpoint,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetColorContrastAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->colorContrast);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateColorContrastAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->colorContrast,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetColorWhitepointAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->colorWhitepoint);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateColorWhitepointAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->colorWhitepoint,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrCookieLightFilter::GetColorTintAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->colorTint);
}

UsdAttribute
UsdRiPxrCookieLightFilter::CreateColorTintAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->colorTint,
                       SdfValueTypeNames->Color3f,
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
UsdRiPxrCookieLightFilter::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdRiTokens->cookieMode,
        UsdRiTokens->width,
        UsdRiTokens->height,
        UsdRiTokens->textureMap,
        UsdRiTokens->textureWrapMode,
        UsdRiTokens->textureFillColor,
        UsdRiTokens->textureInvertU,
        UsdRiTokens->textureInvertV,
        UsdRiTokens->textureScaleU,
        UsdRiTokens->textureScaleV,
        UsdRiTokens->textureOffsetU,
        UsdRiTokens->textureOffsetV,
        UsdRiTokens->analyticDirectional,
        UsdRiTokens->analyticShearX,
        UsdRiTokens->analyticShearY,
        UsdRiTokens->analyticApex,
        UsdRiTokens->analyticUseLightDirection,
        UsdRiTokens->analyticBlurAmount,
        UsdRiTokens->analyticBlurSMult,
        UsdRiTokens->analyticBlurTMult,
        UsdRiTokens->analyticBlurNearDistance,
        UsdRiTokens->analyticBlurMidpoint,
        UsdRiTokens->analyticBlurFarDistance,
        UsdRiTokens->analyticBlurNearValue,
        UsdRiTokens->analyticBlurMidValue,
        UsdRiTokens->analyticBlurFarValue,
        UsdRiTokens->analyticBlurExponent,
        UsdRiTokens->analyticDensityNearDistance,
        UsdRiTokens->analyticDensityMidpoint,
        UsdRiTokens->analyticDensityFarDistance,
        UsdRiTokens->analyticDensityNearValue,
        UsdRiTokens->analyticDensityMidValue,
        UsdRiTokens->analyticDensityFarValue,
        UsdRiTokens->analyticDensityExponent,
        UsdRiTokens->colorSaturation,
        UsdRiTokens->colorMidpoint,
        UsdRiTokens->colorContrast,
        UsdRiTokens->colorWhitepoint,
        UsdRiTokens->colorTint,
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
