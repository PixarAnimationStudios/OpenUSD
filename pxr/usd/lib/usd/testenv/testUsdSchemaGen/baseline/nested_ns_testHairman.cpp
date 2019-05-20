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
#include "pxr/usd/usdContrived/testHairman.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

namespace foo { namespace bar { namespace baz {

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdContrivedTestHairman,
        TfType::Bases< UsdTyped > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("TestPxHairman")
    // to find TfType<UsdContrivedTestHairman>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdContrivedTestHairman>("TestPxHairman");
}

/* virtual */
UsdContrivedTestHairman::~UsdContrivedTestHairman()
{
}

/* static */
UsdContrivedTestHairman
UsdContrivedTestHairman::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdContrivedTestHairman();
    }
    return UsdContrivedTestHairman(stage->GetPrimAtPath(path));
}

/* static */
UsdContrivedTestHairman
UsdContrivedTestHairman::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("TestPxHairman");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdContrivedTestHairman();
    }
    return UsdContrivedTestHairman(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaType UsdContrivedTestHairman::_GetSchemaType() const {
    return UsdContrivedTestHairman::schemaType;
}

/* static */
const TfType &
UsdContrivedTestHairman::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdContrivedTestHairman>();
    return tfType;
}

/* static */
bool 
UsdContrivedTestHairman::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdContrivedTestHairman::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdContrivedTestHairman::GetTempAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->temp);
}

UsdAttribute
UsdContrivedTestHairman::CreateTempAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->temp,
                       SdfValueTypeNames->Float,
                       /* custom = */ true,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedTestHairman::GetGofur_GeomOnHairdensityAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->riStatementsAttributesUserGofur_GeomOnHairdensity);
}

UsdAttribute
UsdContrivedTestHairman::CreateGofur_GeomOnHairdensityAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->riStatementsAttributesUserGofur_GeomOnHairdensity,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdRelationship
UsdContrivedTestHairman::GetGofur_GeomOnHairdensityRel() const
{
    return GetPrim().GetRelationship(UsdContrivedTokens->relCanShareApiNameWithAttr);
}

UsdRelationship
UsdContrivedTestHairman::CreateGofur_GeomOnHairdensityRel() const
{
    return GetPrim().CreateRelationship(UsdContrivedTokens->relCanShareApiNameWithAttr,
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
UsdContrivedTestHairman::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdContrivedTokens->temp,
        UsdContrivedTokens->riStatementsAttributesUserGofur_GeomOnHairdensity,
        UsdContrivedTokens->attrWithoutGeneratedAccessorAPI,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdTyped::GetSchemaAttributeNames(true),
            localNames);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

}}}

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
//
// Just remember to wrap code in the appropriate delimiters:
// 'namespace foo { namespace bar { namespace baz {', '}}}'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--
