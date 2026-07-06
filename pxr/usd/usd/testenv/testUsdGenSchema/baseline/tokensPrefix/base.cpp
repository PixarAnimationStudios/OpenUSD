//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdContrived/base.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdContrivedBase,
        TfType::Bases< UsdTyped > >();
    
}

/* virtual */
UsdContrivedBase::~UsdContrivedBase()
{
}

/* static */
UsdContrivedBase
UsdContrivedBase::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdContrivedBase();
    }
    return UsdContrivedBase(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdContrivedBase::_GetSchemaKind() const
{
    return UsdContrivedBase::schemaKind;
}

/* static */
const TfType &
UsdContrivedBase::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdContrivedBase>();
    return tfType;
}

/* static */
bool 
UsdContrivedBase::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdContrivedBase::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdContrivedBase::GetMyVaryingTokenAttr() const
{
    return GetPrim().GetAttribute(ContrivedTokensPrefixTokens->myVaryingToken);
}

UsdAttribute
UsdContrivedBase::CreateMyVaryingTokenAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ContrivedTokensPrefixTokens->myVaryingToken,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetMyVaryingTokenArrayAttr() const
{
    return GetPrim().GetAttribute(ContrivedTokensPrefixTokens->myVaryingTokenArray);
}

UsdAttribute
UsdContrivedBase::CreateMyVaryingTokenArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ContrivedTokensPrefixTokens->myVaryingTokenArray,
                       SdfValueTypeNames->TokenArray,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetMyDoubleAttr() const
{
    return GetPrim().GetAttribute(ContrivedTokensPrefixTokens->myDouble);
}

UsdAttribute
UsdContrivedBase::CreateMyDoubleAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ContrivedTokensPrefixTokens->myDouble,
                       SdfValueTypeNames->Double,
                       /* custom = */ true,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetMyFloatAttr() const
{
    return GetPrim().GetAttribute(ContrivedTokensPrefixTokens->myFloat);
}

UsdAttribute
UsdContrivedBase::CreateMyFloatAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ContrivedTokensPrefixTokens->myFloat,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetMyColorFloatAttr() const
{
    return GetPrim().GetAttribute(ContrivedTokensPrefixTokens->myColorFloat);
}

UsdAttribute
UsdContrivedBase::CreateMyColorFloatAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ContrivedTokensPrefixTokens->myColorFloat,
                       SdfValueTypeNames->Color3f,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetMyNormalsAttr() const
{
    return GetPrim().GetAttribute(ContrivedTokensPrefixTokens->myNormals);
}

UsdAttribute
UsdContrivedBase::CreateMyNormalsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ContrivedTokensPrefixTokens->myNormals,
                       SdfValueTypeNames->Normal3fArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetMyPointsAttr() const
{
    return GetPrim().GetAttribute(ContrivedTokensPrefixTokens->myPoints);
}

UsdAttribute
UsdContrivedBase::CreateMyPointsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ContrivedTokensPrefixTokens->myPoints,
                       SdfValueTypeNames->Point3fArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetMyVelocitiesAttr() const
{
    return GetPrim().GetAttribute(ContrivedTokensPrefixTokens->myVelocities);
}

UsdAttribute
UsdContrivedBase::CreateMyVelocitiesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ContrivedTokensPrefixTokens->myVelocities,
                       SdfValueTypeNames->Vector3fArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetUnsignedIntAttr() const
{
    return GetPrim().GetAttribute(ContrivedTokensPrefixTokens->unsignedInt);
}

UsdAttribute
UsdContrivedBase::CreateUnsignedIntAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ContrivedTokensPrefixTokens->unsignedInt,
                       SdfValueTypeNames->UInt,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetUnsignedCharAttr() const
{
    return GetPrim().GetAttribute(ContrivedTokensPrefixTokens->unsignedChar);
}

UsdAttribute
UsdContrivedBase::CreateUnsignedCharAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ContrivedTokensPrefixTokens->unsignedChar,
                       SdfValueTypeNames->UChar,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetUnsignedInt64ArrayAttr() const
{
    return GetPrim().GetAttribute(ContrivedTokensPrefixTokens->unsignedInt64Array);
}

UsdAttribute
UsdContrivedBase::CreateUnsignedInt64ArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ContrivedTokensPrefixTokens->unsignedInt64Array,
                       SdfValueTypeNames->UInt64Array,
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
UsdContrivedBase::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        ContrivedTokensPrefixTokens->myVaryingToken,
        ContrivedTokensPrefixTokens->myVaryingTokenArray,
        ContrivedTokensPrefixTokens->myDouble,
        ContrivedTokensPrefixTokens->myFloat,
        ContrivedTokensPrefixTokens->myColorFloat,
        ContrivedTokensPrefixTokens->myNormals,
        ContrivedTokensPrefixTokens->myPoints,
        ContrivedTokensPrefixTokens->myVelocities,
        ContrivedTokensPrefixTokens->unsignedInt,
        ContrivedTokensPrefixTokens->unsignedChar,
        ContrivedTokensPrefixTokens->unsignedInt64Array,
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
