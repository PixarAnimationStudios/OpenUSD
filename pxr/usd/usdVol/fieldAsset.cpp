//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdVol/fieldAsset.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdVolFieldAsset,
        TfType::Bases< UsdVolFieldBase > >();
    
}

/* virtual */
UsdVolFieldAsset::~UsdVolFieldAsset()
{
}

/* static */
UsdVolFieldAsset
UsdVolFieldAsset::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdVolFieldAsset();
    }
    return UsdVolFieldAsset(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdVolFieldAsset::_GetSchemaKind() const
{
    return UsdVolFieldAsset::schemaKind;
}

/* static */
const TfType &
UsdVolFieldAsset::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdVolFieldAsset>();
    return tfType;
}

/* static */
bool 
UsdVolFieldAsset::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdVolFieldAsset::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdVolFieldAsset::GetFilePathAttr() const
{
    return GetPrim().GetAttribute(UsdVolTokens->filePath);
}

UsdAttribute
UsdVolFieldAsset::CreateFilePathAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdVolTokens->filePath,
                       SdfValueTypeNames->Asset,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdVolFieldAsset::GetFieldNameAttr() const
{
    return GetPrim().GetAttribute(UsdVolTokens->fieldName);
}

UsdAttribute
UsdVolFieldAsset::CreateFieldNameAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdVolTokens->fieldName,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdVolFieldAsset::GetFieldIndexAttr() const
{
    return GetPrim().GetAttribute(UsdVolTokens->fieldIndex);
}

UsdAttribute
UsdVolFieldAsset::CreateFieldIndexAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdVolTokens->fieldIndex,
                       SdfValueTypeNames->Int,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdVolFieldAsset::GetFieldDataTypeAttr() const
{
    return GetPrim().GetAttribute(UsdVolTokens->fieldDataType);
}

UsdAttribute
UsdVolFieldAsset::CreateFieldDataTypeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdVolTokens->fieldDataType,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdVolFieldAsset::GetVectorDataRoleHintAttr() const
{
    return GetPrim().GetAttribute(UsdVolTokens->vectorDataRoleHint);
}

UsdAttribute
UsdVolFieldAsset::CreateVectorDataRoleHintAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdVolTokens->vectorDataRoleHint,
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
UsdVolFieldAsset::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdVolTokens->filePath,
        UsdVolTokens->fieldName,
        UsdVolTokens->fieldIndex,
        UsdVolTokens->fieldDataType,
        UsdVolTokens->vectorDataRoleHint,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdVolFieldBase::GetSchemaAttributeNames(true),
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
