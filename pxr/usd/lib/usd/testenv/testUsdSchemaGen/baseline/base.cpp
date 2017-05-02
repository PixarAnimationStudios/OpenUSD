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
    return GetPrim().GetAttribute(UsdContrivedTokens->myVaryingToken);
}

UsdAttribute
UsdContrivedBase::CreateMyVaryingTokenAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->myVaryingToken,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetMyUniformBoolAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->myUniformBool);
}

UsdAttribute
UsdContrivedBase::CreateMyUniformBoolAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->myUniformBool,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetMyDoubleAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->myDouble);
}

UsdAttribute
UsdContrivedBase::CreateMyDoubleAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->myDouble,
                       SdfValueTypeNames->Double,
                       /* custom = */ true,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetMyFloatAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->myFloat);
}

UsdAttribute
UsdContrivedBase::CreateMyFloatAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->myFloat,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetMyColorFloatAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->myColorFloat);
}

UsdAttribute
UsdContrivedBase::CreateMyColorFloatAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->myColorFloat,
                       SdfValueTypeNames->Color3f,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetMyNormalsAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->myNormals);
}

UsdAttribute
UsdContrivedBase::CreateMyNormalsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->myNormals,
                       SdfValueTypeNames->Normal3fArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetMyPointsAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->myPoints);
}

UsdAttribute
UsdContrivedBase::CreateMyPointsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->myPoints,
                       SdfValueTypeNames->Point3fArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetMyVelocitiesAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->myVelocities);
}

UsdAttribute
UsdContrivedBase::CreateMyVelocitiesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->myVelocities,
                       SdfValueTypeNames->Vector3fArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetUnsignedIntAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->unsignedInt);
}

UsdAttribute
UsdContrivedBase::CreateUnsignedIntAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->unsignedInt,
                       SdfValueTypeNames->UInt,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetUnsignedCharAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->unsignedChar);
}

UsdAttribute
UsdContrivedBase::CreateUnsignedCharAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->unsignedChar,
                       SdfValueTypeNames->UChar,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetUnsignedInt64ArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->unsignedInt64Array);
}

UsdAttribute
UsdContrivedBase::CreateUnsignedInt64ArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->unsignedInt64Array,
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
        UsdContrivedTokens->myVaryingToken,
        UsdContrivedTokens->myUniformBool,
        UsdContrivedTokens->myDouble,
        UsdContrivedTokens->myFloat,
        UsdContrivedTokens->myColorFloat,
        UsdContrivedTokens->myNormals,
        UsdContrivedTokens->myPoints,
        UsdContrivedTokens->myVelocities,
        UsdContrivedTokens->unsignedInt,
        UsdContrivedTokens->unsignedChar,
        UsdContrivedTokens->unsignedInt64Array,
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
