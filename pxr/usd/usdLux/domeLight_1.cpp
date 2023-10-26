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
#include "pxr/usd/usdLux/domeLight_1.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLuxDomeLight_1,
        TfType::Bases< UsdLuxNonboundableLightBase > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("DomeLight_1")
    // to find TfType<UsdLuxDomeLight_1>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdLuxDomeLight_1>("DomeLight_1");
}

/* virtual */
UsdLuxDomeLight_1::~UsdLuxDomeLight_1()
{
}

/* static */
UsdLuxDomeLight_1
UsdLuxDomeLight_1::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxDomeLight_1();
    }
    return UsdLuxDomeLight_1(stage->GetPrimAtPath(path));
}

/* static */
UsdLuxDomeLight_1
UsdLuxDomeLight_1::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("DomeLight_1");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxDomeLight_1();
    }
    return UsdLuxDomeLight_1(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdLuxDomeLight_1::_GetSchemaKind() const
{
    return UsdLuxDomeLight_1::schemaKind;
}

/* static */
const TfType &
UsdLuxDomeLight_1::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLuxDomeLight_1>();
    return tfType;
}

/* static */
bool 
UsdLuxDomeLight_1::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLuxDomeLight_1::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdLuxDomeLight_1::GetTextureFileAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsTextureFile);
}

UsdAttribute
UsdLuxDomeLight_1::CreateTextureFileAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsTextureFile,
                       SdfValueTypeNames->Asset,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxDomeLight_1::GetTextureFormatAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsTextureFormat);
}

UsdAttribute
UsdLuxDomeLight_1::CreateTextureFormatAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsTextureFormat,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxDomeLight_1::GetGuideRadiusAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->guideRadius);
}

UsdAttribute
UsdLuxDomeLight_1::CreateGuideRadiusAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->guideRadius,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxDomeLight_1::GetPoleAxisAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->poleAxis);
}

UsdAttribute
UsdLuxDomeLight_1::CreatePoleAxisAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->poleAxis,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdRelationship
UsdLuxDomeLight_1::GetPortalsRel() const
{
    return GetPrim().GetRelationship(UsdLuxTokens->portals);
}

UsdRelationship
UsdLuxDomeLight_1::CreatePortalsRel() const
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
UsdLuxDomeLight_1::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdLuxTokens->inputsTextureFile,
        UsdLuxTokens->inputsTextureFormat,
        UsdLuxTokens->guideRadius,
        UsdLuxTokens->poleAxis,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdLuxNonboundableLightBase::GetSchemaAttributeNames(true),
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
