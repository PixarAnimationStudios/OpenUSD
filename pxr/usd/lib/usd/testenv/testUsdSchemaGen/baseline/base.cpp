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


/* virtual */
UsdSchemaType UsdContrivedBase::_GetSchemaType() const {
    return UsdContrivedBase::schemaType;
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
UsdContrivedBase::GetBoolAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->bool);
}

UsdAttribute
UsdContrivedBase::CreateBoolAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->bool,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetUcharAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->uchar);
}

UsdAttribute
UsdContrivedBase::CreateUcharAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->uchar,
                       SdfValueTypeNames->UChar,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetIntAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->int);
}

UsdAttribute
UsdContrivedBase::CreateIntAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->int,
                       SdfValueTypeNames->Int,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetUintAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->uint);
}

UsdAttribute
UsdContrivedBase::CreateUintAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->uint,
                       SdfValueTypeNames->UInt,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetInt64Attr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->int64);
}

UsdAttribute
UsdContrivedBase::CreateInt64Attr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->int64,
                       SdfValueTypeNames->Int64,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetUint64Attr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->uint64);
}

UsdAttribute
UsdContrivedBase::CreateUint64Attr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->uint64,
                       SdfValueTypeNames->UInt64,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetHalfAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->half);
}

UsdAttribute
UsdContrivedBase::CreateHalfAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->half,
                       SdfValueTypeNames->Half,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetFloatAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->float);
}

UsdAttribute
UsdContrivedBase::CreateFloatAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->float,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetDoubleAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->double);
}

UsdAttribute
UsdContrivedBase::CreateDoubleAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->double,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetStringAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->string);
}

UsdAttribute
UsdContrivedBase::CreateStringAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->string,
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetTokenAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->token);
}

UsdAttribute
UsdContrivedBase::CreateTokenAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->token,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetAssetAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->asset);
}

UsdAttribute
UsdContrivedBase::CreateAssetAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->asset,
                       SdfValueTypeNames->Asset,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetInt2Attr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->int2);
}

UsdAttribute
UsdContrivedBase::CreateInt2Attr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->int2,
                       SdfValueTypeNames->Int2,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetInt3Attr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->int3);
}

UsdAttribute
UsdContrivedBase::CreateInt3Attr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->int3,
                       SdfValueTypeNames->Int3,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetInt4Attr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->int4);
}

UsdAttribute
UsdContrivedBase::CreateInt4Attr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->int4,
                       SdfValueTypeNames->Int4,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetHalf2Attr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->half2);
}

UsdAttribute
UsdContrivedBase::CreateHalf2Attr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->half2,
                       SdfValueTypeNames->Half2,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetHalf3Attr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->half3);
}

UsdAttribute
UsdContrivedBase::CreateHalf3Attr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->half3,
                       SdfValueTypeNames->Half3,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetHalf4Attr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->half4);
}

UsdAttribute
UsdContrivedBase::CreateHalf4Attr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->half4,
                       SdfValueTypeNames->Half4,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetFloat2Attr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->float2);
}

UsdAttribute
UsdContrivedBase::CreateFloat2Attr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->float2,
                       SdfValueTypeNames->Float2,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetFloat3Attr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->float3);
}

UsdAttribute
UsdContrivedBase::CreateFloat3Attr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->float3,
                       SdfValueTypeNames->Float3,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetFloat4Attr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->float4);
}

UsdAttribute
UsdContrivedBase::CreateFloat4Attr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->float4,
                       SdfValueTypeNames->Float4,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetDouble2Attr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->double2);
}

UsdAttribute
UsdContrivedBase::CreateDouble2Attr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->double2,
                       SdfValueTypeNames->Double2,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetDouble3Attr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->double3);
}

UsdAttribute
UsdContrivedBase::CreateDouble3Attr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->double3,
                       SdfValueTypeNames->Double3,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetDouble4Attr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->double4);
}

UsdAttribute
UsdContrivedBase::CreateDouble4Attr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->double4,
                       SdfValueTypeNames->Double4,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetPoint3hAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->point3h);
}

UsdAttribute
UsdContrivedBase::CreatePoint3hAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->point3h,
                       SdfValueTypeNames->Point3h,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetPoint3fAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->point3f);
}

UsdAttribute
UsdContrivedBase::CreatePoint3fAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->point3f,
                       SdfValueTypeNames->Point3f,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetPoint3dAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->point3d);
}

UsdAttribute
UsdContrivedBase::CreatePoint3dAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->point3d,
                       SdfValueTypeNames->Point3d,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetVector3dAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->vector3d);
}

UsdAttribute
UsdContrivedBase::CreateVector3dAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->vector3d,
                       SdfValueTypeNames->Vector3d,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetVector3fAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->vector3f);
}

UsdAttribute
UsdContrivedBase::CreateVector3fAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->vector3f,
                       SdfValueTypeNames->Vector3f,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetVector3hAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->vector3h);
}

UsdAttribute
UsdContrivedBase::CreateVector3hAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->vector3h,
                       SdfValueTypeNames->Vector3h,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetNormal3dAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->normal3d);
}

UsdAttribute
UsdContrivedBase::CreateNormal3dAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->normal3d,
                       SdfValueTypeNames->Normal3d,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetNormal3fAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->normal3f);
}

UsdAttribute
UsdContrivedBase::CreateNormal3fAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->normal3f,
                       SdfValueTypeNames->Normal3f,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetNormal3hAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->normal3h);
}

UsdAttribute
UsdContrivedBase::CreateNormal3hAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->normal3h,
                       SdfValueTypeNames->Normal3h,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetColor3dAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->color3d);
}

UsdAttribute
UsdContrivedBase::CreateColor3dAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->color3d,
                       SdfValueTypeNames->Color3d,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetColor3fAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->color3f);
}

UsdAttribute
UsdContrivedBase::CreateColor3fAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->color3f,
                       SdfValueTypeNames->Color3f,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetColor3hAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->color3h);
}

UsdAttribute
UsdContrivedBase::CreateColor3hAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->color3h,
                       SdfValueTypeNames->Color3h,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetColor4dAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->color4d);
}

UsdAttribute
UsdContrivedBase::CreateColor4dAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->color4d,
                       SdfValueTypeNames->Color4d,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetColor4fAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->color4f);
}

UsdAttribute
UsdContrivedBase::CreateColor4fAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->color4f,
                       SdfValueTypeNames->Color4f,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetColor4hAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->color4h);
}

UsdAttribute
UsdContrivedBase::CreateColor4hAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->color4h,
                       SdfValueTypeNames->Color4h,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetQuatdAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->quatd);
}

UsdAttribute
UsdContrivedBase::CreateQuatdAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->quatd,
                       SdfValueTypeNames->Quatd,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetQuatfAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->quatf);
}

UsdAttribute
UsdContrivedBase::CreateQuatfAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->quatf,
                       SdfValueTypeNames->Quatf,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetQuathAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->quath);
}

UsdAttribute
UsdContrivedBase::CreateQuathAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->quath,
                       SdfValueTypeNames->Quath,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetMatrix2dAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->matrix2d);
}

UsdAttribute
UsdContrivedBase::CreateMatrix2dAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->matrix2d,
                       SdfValueTypeNames->Matrix2d,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetMatrix3dAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->matrix3d);
}

UsdAttribute
UsdContrivedBase::CreateMatrix3dAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->matrix3d,
                       SdfValueTypeNames->Matrix3d,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetMatrix4dAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->matrix4d);
}

UsdAttribute
UsdContrivedBase::CreateMatrix4dAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->matrix4d,
                       SdfValueTypeNames->Matrix4d,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetFrame4dAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->frame4d);
}

UsdAttribute
UsdContrivedBase::CreateFrame4dAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->frame4d,
                       SdfValueTypeNames->Frame4d,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetBoolArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->boolArray);
}

UsdAttribute
UsdContrivedBase::CreateBoolArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->boolArray,
                       SdfValueTypeNames->BoolArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetUcharArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->ucharArray);
}

UsdAttribute
UsdContrivedBase::CreateUcharArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->ucharArray,
                       SdfValueTypeNames->UCharArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetIntArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->intArray);
}

UsdAttribute
UsdContrivedBase::CreateIntArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->intArray,
                       SdfValueTypeNames->IntArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetUintArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->uintArray);
}

UsdAttribute
UsdContrivedBase::CreateUintArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->uintArray,
                       SdfValueTypeNames->UIntArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetInt64ArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->int64Array);
}

UsdAttribute
UsdContrivedBase::CreateInt64ArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->int64Array,
                       SdfValueTypeNames->Int64Array,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetUint64ArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->uint64Array);
}

UsdAttribute
UsdContrivedBase::CreateUint64ArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->uint64Array,
                       SdfValueTypeNames->UInt64Array,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetHalfArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->halfArray);
}

UsdAttribute
UsdContrivedBase::CreateHalfArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->halfArray,
                       SdfValueTypeNames->HalfArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetFloatArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->floatArray);
}

UsdAttribute
UsdContrivedBase::CreateFloatArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->floatArray,
                       SdfValueTypeNames->FloatArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetDoubleArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->doubleArray);
}

UsdAttribute
UsdContrivedBase::CreateDoubleArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->doubleArray,
                       SdfValueTypeNames->DoubleArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetStringArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->stringArray);
}

UsdAttribute
UsdContrivedBase::CreateStringArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->stringArray,
                       SdfValueTypeNames->StringArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetTokenArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->tokenArray);
}

UsdAttribute
UsdContrivedBase::CreateTokenArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->tokenArray,
                       SdfValueTypeNames->TokenArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetAssetArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->assetArray);
}

UsdAttribute
UsdContrivedBase::CreateAssetArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->assetArray,
                       SdfValueTypeNames->AssetArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetInt2ArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->int2Array);
}

UsdAttribute
UsdContrivedBase::CreateInt2ArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->int2Array,
                       SdfValueTypeNames->Int2Array,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetInt3ArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->int3Array);
}

UsdAttribute
UsdContrivedBase::CreateInt3ArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->int3Array,
                       SdfValueTypeNames->Int3Array,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetInt4ArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->int4Array);
}

UsdAttribute
UsdContrivedBase::CreateInt4ArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->int4Array,
                       SdfValueTypeNames->Int4Array,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetHalf2ArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->half2Array);
}

UsdAttribute
UsdContrivedBase::CreateHalf2ArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->half2Array,
                       SdfValueTypeNames->Half2Array,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetHalf3ArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->half3Array);
}

UsdAttribute
UsdContrivedBase::CreateHalf3ArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->half3Array,
                       SdfValueTypeNames->Half3Array,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetHalf4ArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->half4Array);
}

UsdAttribute
UsdContrivedBase::CreateHalf4ArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->half4Array,
                       SdfValueTypeNames->Half4Array,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetFloat2ArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->float2Array);
}

UsdAttribute
UsdContrivedBase::CreateFloat2ArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->float2Array,
                       SdfValueTypeNames->Float2Array,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetFloat3ArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->float3Array);
}

UsdAttribute
UsdContrivedBase::CreateFloat3ArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->float3Array,
                       SdfValueTypeNames->Float3Array,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetFloat4ArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->float4Array);
}

UsdAttribute
UsdContrivedBase::CreateFloat4ArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->float4Array,
                       SdfValueTypeNames->Float4Array,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetDouble2ArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->double2Array);
}

UsdAttribute
UsdContrivedBase::CreateDouble2ArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->double2Array,
                       SdfValueTypeNames->Double2Array,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetDouble3ArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->double3Array);
}

UsdAttribute
UsdContrivedBase::CreateDouble3ArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->double3Array,
                       SdfValueTypeNames->Double3Array,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetDouble4ArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->double4Array);
}

UsdAttribute
UsdContrivedBase::CreateDouble4ArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->double4Array,
                       SdfValueTypeNames->Double4Array,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetPoint3hArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->point3hArray);
}

UsdAttribute
UsdContrivedBase::CreatePoint3hArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->point3hArray,
                       SdfValueTypeNames->Point3hArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetPoint3fArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->point3fArray);
}

UsdAttribute
UsdContrivedBase::CreatePoint3fArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->point3fArray,
                       SdfValueTypeNames->Point3fArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetPoint3dArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->point3dArray);
}

UsdAttribute
UsdContrivedBase::CreatePoint3dArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->point3dArray,
                       SdfValueTypeNames->Point3dArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetVector3hArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->vector3hArray);
}

UsdAttribute
UsdContrivedBase::CreateVector3hArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->vector3hArray,
                       SdfValueTypeNames->Vector3hArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetVector3fArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->vector3fArray);
}

UsdAttribute
UsdContrivedBase::CreateVector3fArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->vector3fArray,
                       SdfValueTypeNames->Vector3fArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetVector3dArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->vector3dArray);
}

UsdAttribute
UsdContrivedBase::CreateVector3dArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->vector3dArray,
                       SdfValueTypeNames->Vector3dArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetNormal3hArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->normal3hArray);
}

UsdAttribute
UsdContrivedBase::CreateNormal3hArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->normal3hArray,
                       SdfValueTypeNames->Normal3hArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetNormal3fArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->normal3fArray);
}

UsdAttribute
UsdContrivedBase::CreateNormal3fArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->normal3fArray,
                       SdfValueTypeNames->Normal3fArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetNormal3dArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->normal3dArray);
}

UsdAttribute
UsdContrivedBase::CreateNormal3dArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->normal3dArray,
                       SdfValueTypeNames->Normal3dArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetColor3hArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->color3hArray);
}

UsdAttribute
UsdContrivedBase::CreateColor3hArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->color3hArray,
                       SdfValueTypeNames->Color3hArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetColor3fArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->color3fArray);
}

UsdAttribute
UsdContrivedBase::CreateColor3fArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->color3fArray,
                       SdfValueTypeNames->Color3fArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetColor3dArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->color3dArray);
}

UsdAttribute
UsdContrivedBase::CreateColor3dArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->color3dArray,
                       SdfValueTypeNames->Color3dArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetColor4hArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->color4hArray);
}

UsdAttribute
UsdContrivedBase::CreateColor4hArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->color4hArray,
                       SdfValueTypeNames->Color4hArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetColor4fArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->color4fArray);
}

UsdAttribute
UsdContrivedBase::CreateColor4fArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->color4fArray,
                       SdfValueTypeNames->Color4fArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetColor4dArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->color4dArray);
}

UsdAttribute
UsdContrivedBase::CreateColor4dArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->color4dArray,
                       SdfValueTypeNames->Color4dArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetQuathArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->quathArray);
}

UsdAttribute
UsdContrivedBase::CreateQuathArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->quathArray,
                       SdfValueTypeNames->QuathArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetQuatfArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->quatfArray);
}

UsdAttribute
UsdContrivedBase::CreateQuatfArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->quatfArray,
                       SdfValueTypeNames->QuatfArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetQuatdArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->quatdArray);
}

UsdAttribute
UsdContrivedBase::CreateQuatdArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->quatdArray,
                       SdfValueTypeNames->QuatdArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetMatrix2dArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->matrix2dArray);
}

UsdAttribute
UsdContrivedBase::CreateMatrix2dArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->matrix2dArray,
                       SdfValueTypeNames->Matrix2dArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetMatrix3dArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->matrix3dArray);
}

UsdAttribute
UsdContrivedBase::CreateMatrix3dArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->matrix3dArray,
                       SdfValueTypeNames->Matrix3dArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetMatrix4dArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->matrix4dArray);
}

UsdAttribute
UsdContrivedBase::CreateMatrix4dArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->matrix4dArray,
                       SdfValueTypeNames->Matrix4dArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedBase::GetFrame4dArrayAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->frame4dArray);
}

UsdAttribute
UsdContrivedBase::CreateFrame4dArrayAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->frame4dArray,
                       SdfValueTypeNames->Frame4dArray,
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
        UsdContrivedTokens->bool,
        UsdContrivedTokens->uchar,
        UsdContrivedTokens->int,
        UsdContrivedTokens->uint,
        UsdContrivedTokens->int64,
        UsdContrivedTokens->uint64,
        UsdContrivedTokens->half,
        UsdContrivedTokens->float,
        UsdContrivedTokens->double,
        UsdContrivedTokens->string,
        UsdContrivedTokens->token,
        UsdContrivedTokens->asset,
        UsdContrivedTokens->int2,
        UsdContrivedTokens->int3,
        UsdContrivedTokens->int4,
        UsdContrivedTokens->half2,
        UsdContrivedTokens->half3,
        UsdContrivedTokens->half4,
        UsdContrivedTokens->float2,
        UsdContrivedTokens->float3,
        UsdContrivedTokens->float4,
        UsdContrivedTokens->double2,
        UsdContrivedTokens->double3,
        UsdContrivedTokens->double4,
        UsdContrivedTokens->point3h,
        UsdContrivedTokens->point3f,
        UsdContrivedTokens->point3d,
        UsdContrivedTokens->vector3d,
        UsdContrivedTokens->vector3f,
        UsdContrivedTokens->vector3h,
        UsdContrivedTokens->normal3d,
        UsdContrivedTokens->normal3f,
        UsdContrivedTokens->normal3h,
        UsdContrivedTokens->color3d,
        UsdContrivedTokens->color3f,
        UsdContrivedTokens->color3h,
        UsdContrivedTokens->color4d,
        UsdContrivedTokens->color4f,
        UsdContrivedTokens->color4h,
        UsdContrivedTokens->quatd,
        UsdContrivedTokens->quatf,
        UsdContrivedTokens->quath,
        UsdContrivedTokens->matrix2d,
        UsdContrivedTokens->matrix3d,
        UsdContrivedTokens->matrix4d,
        UsdContrivedTokens->frame4d,
        UsdContrivedTokens->boolArray,
        UsdContrivedTokens->ucharArray,
        UsdContrivedTokens->intArray,
        UsdContrivedTokens->uintArray,
        UsdContrivedTokens->int64Array,
        UsdContrivedTokens->uint64Array,
        UsdContrivedTokens->halfArray,
        UsdContrivedTokens->floatArray,
        UsdContrivedTokens->doubleArray,
        UsdContrivedTokens->stringArray,
        UsdContrivedTokens->tokenArray,
        UsdContrivedTokens->assetArray,
        UsdContrivedTokens->int2Array,
        UsdContrivedTokens->int3Array,
        UsdContrivedTokens->int4Array,
        UsdContrivedTokens->half2Array,
        UsdContrivedTokens->half3Array,
        UsdContrivedTokens->half4Array,
        UsdContrivedTokens->float2Array,
        UsdContrivedTokens->float3Array,
        UsdContrivedTokens->float4Array,
        UsdContrivedTokens->double2Array,
        UsdContrivedTokens->double3Array,
        UsdContrivedTokens->double4Array,
        UsdContrivedTokens->point3hArray,
        UsdContrivedTokens->point3fArray,
        UsdContrivedTokens->point3dArray,
        UsdContrivedTokens->vector3hArray,
        UsdContrivedTokens->vector3fArray,
        UsdContrivedTokens->vector3dArray,
        UsdContrivedTokens->normal3hArray,
        UsdContrivedTokens->normal3fArray,
        UsdContrivedTokens->normal3dArray,
        UsdContrivedTokens->color3hArray,
        UsdContrivedTokens->color3fArray,
        UsdContrivedTokens->color3dArray,
        UsdContrivedTokens->color4hArray,
        UsdContrivedTokens->color4fArray,
        UsdContrivedTokens->color4dArray,
        UsdContrivedTokens->quathArray,
        UsdContrivedTokens->quatfArray,
        UsdContrivedTokens->quatdArray,
        UsdContrivedTokens->matrix2dArray,
        UsdContrivedTokens->matrix3dArray,
        UsdContrivedTokens->matrix4dArray,
        UsdContrivedTokens->frame4dArray,
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
