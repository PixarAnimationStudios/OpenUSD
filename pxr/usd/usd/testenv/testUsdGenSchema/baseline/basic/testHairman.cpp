//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdContrived/testHairman.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

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
UsdSchemaKind UsdContrivedTestHairman::_GetSchemaKind() const
{
    return UsdContrivedTestHairman::schemaKind;
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

PXR_NAMESPACE_CLOSE_SCOPE

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
//
// Just remember to wrap code in the appropriate delimiters:
// 'PXR_NAMESPACE_OPEN_SCOPE', 'PXR_NAMESPACE_CLOSE_SCOPE'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--
