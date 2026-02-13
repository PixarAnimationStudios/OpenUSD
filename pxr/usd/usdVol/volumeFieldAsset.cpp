//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdVol/volumeFieldAsset.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdVolVolumeFieldAsset,
        TfType::Bases< UsdVolFieldBase > >();
    
}

/* virtual */
UsdVolVolumeFieldAsset::~UsdVolVolumeFieldAsset()
{
}

/* static */
UsdVolVolumeFieldAsset
UsdVolVolumeFieldAsset::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdVolVolumeFieldAsset();
    }
    return UsdVolVolumeFieldAsset(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdVolVolumeFieldAsset::_GetSchemaKind() const
{
    return UsdVolVolumeFieldAsset::schemaKind;
}

/* static */
const TfType &
UsdVolVolumeFieldAsset::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdVolVolumeFieldAsset>();
    return tfType;
}

/* static */
bool 
UsdVolVolumeFieldAsset::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdVolVolumeFieldAsset::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdVolVolumeFieldAsset::GetFilePathAttr() const
{
    return GetPrim().GetAttribute(UsdVolTokens->filePath);
}

UsdAttribute
UsdVolVolumeFieldAsset::CreateFilePathAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdVolTokens->filePath,
                       SdfValueTypeNames->Asset,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdVolVolumeFieldAsset::GetFieldNameAttr() const
{
    return GetPrim().GetAttribute(UsdVolTokens->fieldName);
}

UsdAttribute
UsdVolVolumeFieldAsset::CreateFieldNameAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdVolTokens->fieldName,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdVolVolumeFieldAsset::GetFieldIndexAttr() const
{
    return GetPrim().GetAttribute(UsdVolTokens->fieldIndex);
}

UsdAttribute
UsdVolVolumeFieldAsset::CreateFieldIndexAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdVolTokens->fieldIndex,
                       SdfValueTypeNames->Int,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdVolVolumeFieldAsset::GetFieldDataTypeAttr() const
{
    return GetPrim().GetAttribute(UsdVolTokens->fieldDataType);
}

UsdAttribute
UsdVolVolumeFieldAsset::CreateFieldDataTypeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdVolTokens->fieldDataType,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdVolVolumeFieldAsset::GetVectorDataRoleHintAttr() const
{
    return GetPrim().GetAttribute(UsdVolTokens->vectorDataRoleHint);
}

UsdAttribute
UsdVolVolumeFieldAsset::CreateVectorDataRoleHintAttr(VtValue const &defaultValue, bool writeSparsely) const
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
UsdVolVolumeFieldAsset::GetSchemaAttributeNames(bool includeInherited)
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
