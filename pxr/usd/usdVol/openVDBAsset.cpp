//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdVol/openVDBAsset.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdVolOpenVDBAsset,
        TfType::Bases< UsdVolFieldAsset > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("OpenVDBAsset")
    // to find TfType<UsdVolOpenVDBAsset>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdVolOpenVDBAsset>("OpenVDBAsset");
}

/* virtual */
UsdVolOpenVDBAsset::~UsdVolOpenVDBAsset()
{
}

/* static */
UsdVolOpenVDBAsset
UsdVolOpenVDBAsset::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdVolOpenVDBAsset();
    }
    return UsdVolOpenVDBAsset(stage->GetPrimAtPath(path));
}

/* static */
UsdVolOpenVDBAsset
UsdVolOpenVDBAsset::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("OpenVDBAsset");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdVolOpenVDBAsset();
    }
    return UsdVolOpenVDBAsset(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdVolOpenVDBAsset::_GetSchemaKind() const
{
    return UsdVolOpenVDBAsset::schemaKind;
}

/* static */
const TfType &
UsdVolOpenVDBAsset::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdVolOpenVDBAsset>();
    return tfType;
}

/* static */
bool 
UsdVolOpenVDBAsset::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdVolOpenVDBAsset::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdVolOpenVDBAsset::GetFieldDataTypeAttr() const
{
    return GetPrim().GetAttribute(UsdVolTokens->fieldDataType);
}

UsdAttribute
UsdVolOpenVDBAsset::CreateFieldDataTypeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdVolTokens->fieldDataType,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdVolOpenVDBAsset::GetFieldClassAttr() const
{
    return GetPrim().GetAttribute(UsdVolTokens->fieldClass);
}

UsdAttribute
UsdVolOpenVDBAsset::CreateFieldClassAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdVolTokens->fieldClass,
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
UsdVolOpenVDBAsset::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdVolTokens->fieldDataType,
        UsdVolTokens->fieldClass,
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
