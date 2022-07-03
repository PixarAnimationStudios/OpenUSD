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
#include "pxr/usd/usdPhysics/joint.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdPhysicsJoint,
        TfType::Bases< UsdGeomImageable > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("PhysicsJoint")
    // to find TfType<UsdPhysicsJoint>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdPhysicsJoint>("PhysicsJoint");
}

/* virtual */
UsdPhysicsJoint::~UsdPhysicsJoint()
{
}

/* static */
UsdPhysicsJoint
UsdPhysicsJoint::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdPhysicsJoint();
    }
    return UsdPhysicsJoint(stage->GetPrimAtPath(path));
}

/* static */
UsdPhysicsJoint
UsdPhysicsJoint::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("PhysicsJoint");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdPhysicsJoint();
    }
    return UsdPhysicsJoint(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdPhysicsJoint::_GetSchemaKind() const
{
    return UsdPhysicsJoint::schemaKind;
}

/* static */
const TfType &
UsdPhysicsJoint::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdPhysicsJoint>();
    return tfType;
}

/* static */
bool 
UsdPhysicsJoint::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdPhysicsJoint::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdPhysicsJoint::GetLocalPos0Attr() const
{
    return GetPrim().GetAttribute(UsdPhysicsTokens->physicsLocalPos0);
}

UsdAttribute
UsdPhysicsJoint::CreateLocalPos0Attr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdPhysicsTokens->physicsLocalPos0,
                       SdfValueTypeNames->Point3f,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdPhysicsJoint::GetLocalRot0Attr() const
{
    return GetPrim().GetAttribute(UsdPhysicsTokens->physicsLocalRot0);
}

UsdAttribute
UsdPhysicsJoint::CreateLocalRot0Attr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdPhysicsTokens->physicsLocalRot0,
                       SdfValueTypeNames->Quatf,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdPhysicsJoint::GetLocalPos1Attr() const
{
    return GetPrim().GetAttribute(UsdPhysicsTokens->physicsLocalPos1);
}

UsdAttribute
UsdPhysicsJoint::CreateLocalPos1Attr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdPhysicsTokens->physicsLocalPos1,
                       SdfValueTypeNames->Point3f,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdPhysicsJoint::GetLocalRot1Attr() const
{
    return GetPrim().GetAttribute(UsdPhysicsTokens->physicsLocalRot1);
}

UsdAttribute
UsdPhysicsJoint::CreateLocalRot1Attr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdPhysicsTokens->physicsLocalRot1,
                       SdfValueTypeNames->Quatf,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdPhysicsJoint::GetJointEnabledAttr() const
{
    return GetPrim().GetAttribute(UsdPhysicsTokens->physicsJointEnabled);
}

UsdAttribute
UsdPhysicsJoint::CreateJointEnabledAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdPhysicsTokens->physicsJointEnabled,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdPhysicsJoint::GetCollisionEnabledAttr() const
{
    return GetPrim().GetAttribute(UsdPhysicsTokens->physicsCollisionEnabled);
}

UsdAttribute
UsdPhysicsJoint::CreateCollisionEnabledAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdPhysicsTokens->physicsCollisionEnabled,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdPhysicsJoint::GetExcludeFromArticulationAttr() const
{
    return GetPrim().GetAttribute(UsdPhysicsTokens->physicsExcludeFromArticulation);
}

UsdAttribute
UsdPhysicsJoint::CreateExcludeFromArticulationAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdPhysicsTokens->physicsExcludeFromArticulation,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdPhysicsJoint::GetBreakForceAttr() const
{
    return GetPrim().GetAttribute(UsdPhysicsTokens->physicsBreakForce);
}

UsdAttribute
UsdPhysicsJoint::CreateBreakForceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdPhysicsTokens->physicsBreakForce,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdPhysicsJoint::GetBreakTorqueAttr() const
{
    return GetPrim().GetAttribute(UsdPhysicsTokens->physicsBreakTorque);
}

UsdAttribute
UsdPhysicsJoint::CreateBreakTorqueAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdPhysicsTokens->physicsBreakTorque,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdRelationship
UsdPhysicsJoint::GetBody0Rel() const
{
    return GetPrim().GetRelationship(UsdPhysicsTokens->physicsBody0);
}

UsdRelationship
UsdPhysicsJoint::CreateBody0Rel() const
{
    return GetPrim().CreateRelationship(UsdPhysicsTokens->physicsBody0,
                       /* custom = */ false);
}

UsdRelationship
UsdPhysicsJoint::GetBody1Rel() const
{
    return GetPrim().GetRelationship(UsdPhysicsTokens->physicsBody1);
}

UsdRelationship
UsdPhysicsJoint::CreateBody1Rel() const
{
    return GetPrim().CreateRelationship(UsdPhysicsTokens->physicsBody1,
                       /* custom = */ false);
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
UsdPhysicsJoint::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdPhysicsTokens->physicsLocalPos0,
        UsdPhysicsTokens->physicsLocalRot0,
        UsdPhysicsTokens->physicsLocalPos1,
        UsdPhysicsTokens->physicsLocalRot1,
        UsdPhysicsTokens->physicsJointEnabled,
        UsdPhysicsTokens->physicsCollisionEnabled,
        UsdPhysicsTokens->physicsExcludeFromArticulation,
        UsdPhysicsTokens->physicsBreakForce,
        UsdPhysicsTokens->physicsBreakTorque,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdGeomImageable::GetSchemaAttributeNames(true),
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
