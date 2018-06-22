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
#include "pxr/usd/usdRi/pxrAovLight.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdRiPxrAovLight,
        TfType::Bases< UsdLuxLight > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("PxrAovLight")
    // to find TfType<UsdRiPxrAovLight>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdRiPxrAovLight>("PxrAovLight");
}

/* virtual */
UsdRiPxrAovLight::~UsdRiPxrAovLight()
{
}

/* static */
UsdRiPxrAovLight
UsdRiPxrAovLight::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRiPxrAovLight();
    }
    return UsdRiPxrAovLight(stage->GetPrimAtPath(path));
}

/* static */
UsdRiPxrAovLight
UsdRiPxrAovLight::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("PxrAovLight");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRiPxrAovLight();
    }
    return UsdRiPxrAovLight(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaType UsdRiPxrAovLight::_GetSchemaType() const {
    return UsdRiPxrAovLight::schemaType;
}

/* static */
const TfType &
UsdRiPxrAovLight::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdRiPxrAovLight>();
    return tfType;
}

/* static */
bool 
UsdRiPxrAovLight::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdRiPxrAovLight::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdRiPxrAovLight::GetAovNameAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->aovName);
}

UsdAttribute
UsdRiPxrAovLight::CreateAovNameAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->aovName,
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrAovLight::GetInPrimaryHitAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->inPrimaryHit);
}

UsdAttribute
UsdRiPxrAovLight::CreateInPrimaryHitAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->inPrimaryHit,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrAovLight::GetInReflectionAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->inReflection);
}

UsdAttribute
UsdRiPxrAovLight::CreateInReflectionAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->inReflection,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrAovLight::GetInRefractionAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->inRefraction);
}

UsdAttribute
UsdRiPxrAovLight::CreateInRefractionAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->inRefraction,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrAovLight::GetInvertAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->invert);
}

UsdAttribute
UsdRiPxrAovLight::CreateInvertAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->invert,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrAovLight::GetOnVolumeBoundariesAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->onVolumeBoundaries);
}

UsdAttribute
UsdRiPxrAovLight::CreateOnVolumeBoundariesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->onVolumeBoundaries,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrAovLight::GetUseColorAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->useColor);
}

UsdAttribute
UsdRiPxrAovLight::CreateUseColorAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->useColor,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrAovLight::GetUseThroughputAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->useThroughput);
}

UsdAttribute
UsdRiPxrAovLight::CreateUseThroughputAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->useThroughput,
                       SdfValueTypeNames->Bool,
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
UsdRiPxrAovLight::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdRiTokens->aovName,
        UsdRiTokens->inPrimaryHit,
        UsdRiTokens->inReflection,
        UsdRiTokens->inRefraction,
        UsdRiTokens->invert,
        UsdRiTokens->onVolumeBoundaries,
        UsdRiTokens->useColor,
        UsdRiTokens->useThroughput,
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
