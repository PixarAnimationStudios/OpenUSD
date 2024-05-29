//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdContrived/testHairman_1.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdContrivedTestHairman_1,
        TfType::Bases< UsdTyped > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("TestPxHairman_1")
    // to find TfType<UsdContrivedTestHairman_1>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdContrivedTestHairman_1>("TestPxHairman_1");
}

/* virtual */
UsdContrivedTestHairman_1::~UsdContrivedTestHairman_1()
{
}

/* static */
UsdContrivedTestHairman_1
UsdContrivedTestHairman_1::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdContrivedTestHairman_1();
    }
    return UsdContrivedTestHairman_1(stage->GetPrimAtPath(path));
}

/* static */
UsdContrivedTestHairman_1
UsdContrivedTestHairman_1::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("TestPxHairman_1");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdContrivedTestHairman_1();
    }
    return UsdContrivedTestHairman_1(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdContrivedTestHairman_1::_GetSchemaKind() const
{
    return UsdContrivedTestHairman_1::schemaKind;
}

/* static */
const TfType &
UsdContrivedTestHairman_1::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdContrivedTestHairman_1>();
    return tfType;
}

/* static */
bool 
UsdContrivedTestHairman_1::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdContrivedTestHairman_1::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdContrivedTestHairman_1::GetTempAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->temp);
}

UsdAttribute
UsdContrivedTestHairman_1::CreateTempAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->temp,
                       SdfValueTypeNames->Double,
                       /* custom = */ true,
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
UsdContrivedTestHairman_1::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdContrivedTokens->temp,
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
