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
#include "pxr/usd/usdPhysics/prismaticJoint.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdPhysicsPrismaticJoint,
        TfType::Bases< UsdPhysicsJoint > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("PhysicsPrismaticJoint")
    // to find TfType<UsdPhysicsPrismaticJoint>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdPhysicsPrismaticJoint>("PhysicsPrismaticJoint");
}

/* virtual */
UsdPhysicsPrismaticJoint::~UsdPhysicsPrismaticJoint()
{
}

/* static */
UsdPhysicsPrismaticJoint
UsdPhysicsPrismaticJoint::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdPhysicsPrismaticJoint();
    }
    return UsdPhysicsPrismaticJoint(stage->GetPrimAtPath(path));
}

/* static */
UsdPhysicsPrismaticJoint
UsdPhysicsPrismaticJoint::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("PhysicsPrismaticJoint");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdPhysicsPrismaticJoint();
    }
    return UsdPhysicsPrismaticJoint(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdPhysicsPrismaticJoint::_GetSchemaKind() const
{
    return UsdPhysicsPrismaticJoint::schemaKind;
}

/* static */
const TfType &
UsdPhysicsPrismaticJoint::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdPhysicsPrismaticJoint>();
    return tfType;
}

/* static */
bool 
UsdPhysicsPrismaticJoint::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdPhysicsPrismaticJoint::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdPhysicsPrismaticJoint::GetAxisAttr() const
{
    return GetPrim().GetAttribute(UsdPhysicsTokens->physicsAxis);
}

UsdAttribute
UsdPhysicsPrismaticJoint::CreateAxisAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdPhysicsTokens->physicsAxis,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdPhysicsPrismaticJoint::GetLowerLimitAttr() const
{
    return GetPrim().GetAttribute(UsdPhysicsTokens->physicsLowerLimit);
}

UsdAttribute
UsdPhysicsPrismaticJoint::CreateLowerLimitAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdPhysicsTokens->physicsLowerLimit,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdPhysicsPrismaticJoint::GetUpperLimitAttr() const
{
    return GetPrim().GetAttribute(UsdPhysicsTokens->physicsUpperLimit);
}

UsdAttribute
UsdPhysicsPrismaticJoint::CreateUpperLimitAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdPhysicsTokens->physicsUpperLimit,
                       SdfValueTypeNames->Float,
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
UsdPhysicsPrismaticJoint::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdPhysicsTokens->physicsAxis,
        UsdPhysicsTokens->physicsLowerLimit,
        UsdPhysicsTokens->physicsUpperLimit,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdPhysicsJoint::GetSchemaAttributeNames(true),
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
