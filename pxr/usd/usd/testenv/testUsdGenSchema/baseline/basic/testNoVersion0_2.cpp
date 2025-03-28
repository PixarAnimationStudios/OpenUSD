//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdContrived/testNoVersion0_2.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdContrivedTestNoVersion0_2,
        TfType::Bases< UsdTyped > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("TestNoVersion0_2")
    // to find TfType<UsdContrivedTestNoVersion0_2>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdContrivedTestNoVersion0_2>("TestNoVersion0_2");
}

/* virtual */
UsdContrivedTestNoVersion0_2::~UsdContrivedTestNoVersion0_2()
{
}

/* static */
UsdContrivedTestNoVersion0_2
UsdContrivedTestNoVersion0_2::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdContrivedTestNoVersion0_2();
    }
    return UsdContrivedTestNoVersion0_2(stage->GetPrimAtPath(path));
}

/* static */
UsdContrivedTestNoVersion0_2
UsdContrivedTestNoVersion0_2::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("TestNoVersion0_2");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdContrivedTestNoVersion0_2();
    }
    return UsdContrivedTestNoVersion0_2(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdContrivedTestNoVersion0_2::_GetSchemaKind() const
{
    return UsdContrivedTestNoVersion0_2::schemaKind;
}

/* static */
const TfType &
UsdContrivedTestNoVersion0_2::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdContrivedTestNoVersion0_2>();
    return tfType;
}

/* static */
bool 
UsdContrivedTestNoVersion0_2::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdContrivedTestNoVersion0_2::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdContrivedTestNoVersion0_2::GetTempAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->temp);
}

UsdAttribute
UsdContrivedTestNoVersion0_2::CreateTempAttr(VtValue const &defaultValue, bool writeSparsely) const
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
UsdContrivedTestNoVersion0_2::GetSchemaAttributeNames(bool includeInherited)
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
