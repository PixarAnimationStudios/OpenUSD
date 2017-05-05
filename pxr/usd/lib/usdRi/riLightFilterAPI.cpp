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
#include "pxr/usd/usdRi/riLightFilterAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdRiRiLightFilterAPI,
        TfType::Bases< UsdSchemaBase > >();
    
}

/* virtual */
UsdRiRiLightFilterAPI::~UsdRiRiLightFilterAPI()
{
}

/* static */
UsdRiRiLightFilterAPI
UsdRiRiLightFilterAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRiRiLightFilterAPI();
    }
    return UsdRiRiLightFilterAPI(stage->GetPrimAtPath(path));
}


/* static */
const TfType &
UsdRiRiLightFilterAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdRiRiLightFilterAPI>();
    return tfType;
}

/* static */
bool 
UsdRiRiLightFilterAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdRiRiLightFilterAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdRiRiLightFilterAPI::GetRiCombineModeAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->riCombineMode);
}

UsdAttribute
UsdRiRiLightFilterAPI::CreateRiCombineModeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->riCombineMode,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiRiLightFilterAPI::GetRiDensityAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->riDensity);
}

UsdAttribute
UsdRiRiLightFilterAPI::CreateRiDensityAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->riDensity,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiRiLightFilterAPI::GetRiInvertAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->riInvert);
}

UsdAttribute
UsdRiRiLightFilterAPI::CreateRiInvertAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->riInvert,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiRiLightFilterAPI::GetRiIntensityAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->riIntensity);
}

UsdAttribute
UsdRiRiLightFilterAPI::CreateRiIntensityAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->riIntensity,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiRiLightFilterAPI::GetRiDiffuseAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->riDiffuse);
}

UsdAttribute
UsdRiRiLightFilterAPI::CreateRiDiffuseAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->riDiffuse,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiRiLightFilterAPI::GetRiSpecularAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->riSpecular);
}

UsdAttribute
UsdRiRiLightFilterAPI::CreateRiSpecularAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->riSpecular,
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
UsdRiRiLightFilterAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdRiTokens->riCombineMode,
        UsdRiTokens->riDensity,
        UsdRiTokens->riInvert,
        UsdRiTokens->riIntensity,
        UsdRiTokens->riDiffuse,
        UsdRiTokens->riSpecular,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdSchemaBase::GetSchemaAttributeNames(true),
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
