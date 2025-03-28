//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdVol/field3DAsset.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdVolField3DAsset,
        TfType::Bases< UsdVolFieldAsset > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("Field3DAsset")
    // to find TfType<UsdVolField3DAsset>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdVolField3DAsset>("Field3DAsset");
}

/* virtual */
UsdVolField3DAsset::~UsdVolField3DAsset()
{
}

/* static */
UsdVolField3DAsset
UsdVolField3DAsset::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdVolField3DAsset();
    }
    return UsdVolField3DAsset(stage->GetPrimAtPath(path));
}

/* static */
UsdVolField3DAsset
UsdVolField3DAsset::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("Field3DAsset");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdVolField3DAsset();
    }
    return UsdVolField3DAsset(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdVolField3DAsset::_GetSchemaKind() const
{
    return UsdVolField3DAsset::schemaKind;
}

/* static */
const TfType &
UsdVolField3DAsset::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdVolField3DAsset>();
    return tfType;
}

/* static */
bool 
UsdVolField3DAsset::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdVolField3DAsset::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdVolField3DAsset::GetFieldDataTypeAttr() const
{
    return GetPrim().GetAttribute(UsdVolTokens->fieldDataType);
}

UsdAttribute
UsdVolField3DAsset::CreateFieldDataTypeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdVolTokens->fieldDataType,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdVolField3DAsset::GetFieldPurposeAttr() const
{
    return GetPrim().GetAttribute(UsdVolTokens->fieldPurpose);
}

UsdAttribute
UsdVolField3DAsset::CreateFieldPurposeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdVolTokens->fieldPurpose,
                       SdfValueTypeNames->Token,
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
UsdVolField3DAsset::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdVolTokens->fieldDataType,
        UsdVolTokens->fieldPurpose,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdVolFieldAsset::GetSchemaAttributeNames(true),
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
