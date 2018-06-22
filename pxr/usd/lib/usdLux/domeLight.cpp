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
#include "pxr/usd/usdLux/domeLight.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLuxDomeLight,
        TfType::Bases< UsdLuxLight > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("DomeLight")
    // to find TfType<UsdLuxDomeLight>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdLuxDomeLight>("DomeLight");
}

/* virtual */
UsdLuxDomeLight::~UsdLuxDomeLight()
{
}

/* static */
UsdLuxDomeLight
UsdLuxDomeLight::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxDomeLight();
    }
    return UsdLuxDomeLight(stage->GetPrimAtPath(path));
}

/* static */
UsdLuxDomeLight
UsdLuxDomeLight::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("DomeLight");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxDomeLight();
    }
    return UsdLuxDomeLight(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaType UsdLuxDomeLight::_GetSchemaType() const {
    return UsdLuxDomeLight::schemaType;
}

/* static */
const TfType &
UsdLuxDomeLight::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLuxDomeLight>();
    return tfType;
}

/* static */
bool 
UsdLuxDomeLight::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLuxDomeLight::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdLuxDomeLight::GetTextureFileAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->textureFile);
}

UsdAttribute
UsdLuxDomeLight::CreateTextureFileAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->textureFile,
                       SdfValueTypeNames->Asset,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxDomeLight::GetTextureFormatAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->textureFormat);
}

UsdAttribute
UsdLuxDomeLight::CreateTextureFormatAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->textureFormat,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdRelationship
UsdLuxDomeLight::GetPortalsRel() const
{
    return GetPrim().GetRelationship(UsdLuxTokens->portals);
}

UsdRelationship
UsdLuxDomeLight::CreatePortalsRel() const
{
    return GetPrim().CreateRelationship(UsdLuxTokens->portals,
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
UsdLuxDomeLight::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdLuxTokens->textureFile,
        UsdLuxTokens->textureFormat,
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
