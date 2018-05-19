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
#include "pxr/usd/usdLux/light.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLuxLight,
        TfType::Bases< UsdGeomXformable > >();
    
}

/* virtual */
UsdLuxLight::~UsdLuxLight()
{
}

/* static */
UsdLuxLight
UsdLuxLight::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxLight();
    }
    return UsdLuxLight(stage->GetPrimAtPath(path));
}


/* static */
const TfType &
UsdLuxLight::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLuxLight>();
    return tfType;
}

/* static */
bool 
UsdLuxLight::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLuxLight::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdLuxLight::GetIntensityAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->intensity);
}

UsdAttribute
UsdLuxLight::CreateIntensityAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->intensity,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxLight::GetExposureAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->exposure);
}

UsdAttribute
UsdLuxLight::CreateExposureAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->exposure,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxLight::GetDiffuseAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->diffuse);
}

UsdAttribute
UsdLuxLight::CreateDiffuseAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->diffuse,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxLight::GetSpecularAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->specular);
}

UsdAttribute
UsdLuxLight::CreateSpecularAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->specular,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxLight::GetNormalizeAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->normalize);
}

UsdAttribute
UsdLuxLight::CreateNormalizeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->normalize,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxLight::GetColorAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->color);
}

UsdAttribute
UsdLuxLight::CreateColorAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->color,
                       SdfValueTypeNames->Color3f,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxLight::GetEnableColorTemperatureAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->enableColorTemperature);
}

UsdAttribute
UsdLuxLight::CreateEnableColorTemperatureAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->enableColorTemperature,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxLight::GetColorTemperatureAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->colorTemperature);
}

UsdAttribute
UsdLuxLight::CreateColorTemperatureAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->colorTemperature,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdRelationship
UsdLuxLight::GetFiltersRel() const
{
    return GetPrim().GetRelationship(UsdLuxTokens->filters);
}

UsdRelationship
UsdLuxLight::CreateFiltersRel() const
{
    return GetPrim().CreateRelationship(UsdLuxTokens->filters,
                       /* custom = */ false);
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
UsdLuxLight::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdLuxTokens->collectionLightLinkIncludeRoot,
        UsdLuxTokens->collectionLightLinkExpansionRule,
        UsdLuxTokens->collectionShadowLinkIncludeRoot,
        UsdLuxTokens->collectionShadowLinkExpansionRule,
        UsdLuxTokens->intensity,
        UsdLuxTokens->exposure,
        UsdLuxTokens->diffuse,
        UsdLuxTokens->specular,
        UsdLuxTokens->normalize,
        UsdLuxTokens->color,
        UsdLuxTokens->enableColorTemperature,
        UsdLuxTokens->colorTemperature,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdGeomXformable::GetSchemaAttributeNames(true),
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

#include "pxr/usd/usdLux/blackbody.h"

PXR_NAMESPACE_OPEN_SCOPE

GfVec3f
UsdLuxLight::ComputeBaseEmission() const
{
    GfVec3f e(1.0);

    float intensity = 1.0;
    GetIntensityAttr().Get(&intensity);
    e *= intensity;

    float exposure = 0.0;
    GetExposureAttr().Get(&exposure);
    e *= exp2(exposure);

    GfVec3f color(1.0);
    GetColorAttr().Get(&color);
    e = GfCompMult(e, color);

    bool enableColorTemp = false;
    GetEnableColorTemperatureAttr().Get(&enableColorTemp);
    if (enableColorTemp) {
        float colorTemp = 6500;
        if (GetColorTemperatureAttr().Get(&colorTemp)) {
            e = GfCompMult(e, UsdLuxBlackbodyTemperatureAsRgb(colorTemp));
        }
    }

    return e;
}

USDLUX_API
UsdCollectionAPI
UsdLuxLight::GetLightLinkCollectionAPI() const
{
    return UsdCollectionAPI(GetPrim(), UsdLuxTokens->lightLink);
}

USDLUX_API
UsdCollectionAPI
UsdLuxLight::GetShadowLinkCollectionAPI() const
{
    return UsdCollectionAPI(GetPrim(), UsdLuxTokens->shadowLink);
}

PXR_NAMESPACE_CLOSE_SCOPE
