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
#include "pxr/usd/usdRi/pxrEnvDayLight.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdRiPxrEnvDayLight,
        TfType::Bases< UsdLuxLight > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("PxrEnvDayLight")
    // to find TfType<UsdRiPxrEnvDayLight>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdRiPxrEnvDayLight>("PxrEnvDayLight");
}

/* virtual */
UsdRiPxrEnvDayLight::~UsdRiPxrEnvDayLight()
{
}

/* static */
UsdRiPxrEnvDayLight
UsdRiPxrEnvDayLight::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRiPxrEnvDayLight();
    }
    return UsdRiPxrEnvDayLight(stage->GetPrimAtPath(path));
}

/* static */
UsdRiPxrEnvDayLight
UsdRiPxrEnvDayLight::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("PxrEnvDayLight");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRiPxrEnvDayLight();
    }
    return UsdRiPxrEnvDayLight(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* static */
const TfType &
UsdRiPxrEnvDayLight::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdRiPxrEnvDayLight>();
    return tfType;
}

/* static */
bool 
UsdRiPxrEnvDayLight::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdRiPxrEnvDayLight::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdRiPxrEnvDayLight::GetDayAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->day);
}

UsdAttribute
UsdRiPxrEnvDayLight::CreateDayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->day,
                       SdfValueTypeNames->Int,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrEnvDayLight::GetHazinessAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->haziness);
}

UsdAttribute
UsdRiPxrEnvDayLight::CreateHazinessAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->haziness,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrEnvDayLight::GetHourAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->hour);
}

UsdAttribute
UsdRiPxrEnvDayLight::CreateHourAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->hour,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrEnvDayLight::GetLatitudeAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->latitude);
}

UsdAttribute
UsdRiPxrEnvDayLight::CreateLatitudeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->latitude,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrEnvDayLight::GetLongitudeAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->longitude);
}

UsdAttribute
UsdRiPxrEnvDayLight::CreateLongitudeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->longitude,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrEnvDayLight::GetMonthAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->month);
}

UsdAttribute
UsdRiPxrEnvDayLight::CreateMonthAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->month,
                       SdfValueTypeNames->Int,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrEnvDayLight::GetSkyTintAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->skyTint);
}

UsdAttribute
UsdRiPxrEnvDayLight::CreateSkyTintAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->skyTint,
                       SdfValueTypeNames->Color3f,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrEnvDayLight::GetSunDirectionAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->sunDirection);
}

UsdAttribute
UsdRiPxrEnvDayLight::CreateSunDirectionAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->sunDirection,
                       SdfValueTypeNames->Vector3f,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrEnvDayLight::GetSunSizeAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->sunSize);
}

UsdAttribute
UsdRiPxrEnvDayLight::CreateSunSizeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->sunSize,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrEnvDayLight::GetSunTintAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->sunTint);
}

UsdAttribute
UsdRiPxrEnvDayLight::CreateSunTintAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->sunTint,
                       SdfValueTypeNames->Color3f,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrEnvDayLight::GetYearAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->year);
}

UsdAttribute
UsdRiPxrEnvDayLight::CreateYearAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->year,
                       SdfValueTypeNames->Int,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrEnvDayLight::GetZoneAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->zone);
}

UsdAttribute
UsdRiPxrEnvDayLight::CreateZoneAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->zone,
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
UsdRiPxrEnvDayLight::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdRiTokens->day,
        UsdRiTokens->haziness,
        UsdRiTokens->hour,
        UsdRiTokens->latitude,
        UsdRiTokens->longitude,
        UsdRiTokens->month,
        UsdRiTokens->skyTint,
        UsdRiTokens->sunDirection,
        UsdRiTokens->sunSize,
        UsdRiTokens->sunTint,
        UsdRiTokens->year,
        UsdRiTokens->zone,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdLuxLight::GetSchemaAttributeNames(true),
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
