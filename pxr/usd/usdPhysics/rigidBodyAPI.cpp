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
#include "pxr/usd/usdPhysics/rigidBodyAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/tokens.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdPhysicsRigidBodyAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

TF_DEFINE_PRIVATE_TOKENS(
    _schemaTokens,
    (PhysicsRigidBodyAPI)
);

/* virtual */
UsdPhysicsRigidBodyAPI::~UsdPhysicsRigidBodyAPI()
{
}

/* static */
UsdPhysicsRigidBodyAPI
UsdPhysicsRigidBodyAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdPhysicsRigidBodyAPI();
    }
    return UsdPhysicsRigidBodyAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdPhysicsRigidBodyAPI::_GetSchemaKind() const
{
    return UsdPhysicsRigidBodyAPI::schemaKind;
}

/* static */
bool
UsdPhysicsRigidBodyAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdPhysicsRigidBodyAPI>(whyNot);
}

/* static */
UsdPhysicsRigidBodyAPI
UsdPhysicsRigidBodyAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdPhysicsRigidBodyAPI>()) {
        return UsdPhysicsRigidBodyAPI(prim);
    }
    return UsdPhysicsRigidBodyAPI();
}

/* static */
const TfType &
UsdPhysicsRigidBodyAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdPhysicsRigidBodyAPI>();
    return tfType;
}

/* static */
bool 
UsdPhysicsRigidBodyAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdPhysicsRigidBodyAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdPhysicsRigidBodyAPI::GetRigidBodyEnabledAttr() const
{
    return GetPrim().GetAttribute(UsdPhysicsTokens->physicsRigidBodyEnabled);
}

UsdAttribute
UsdPhysicsRigidBodyAPI::CreateRigidBodyEnabledAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdPhysicsTokens->physicsRigidBodyEnabled,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdPhysicsRigidBodyAPI::GetKinematicEnabledAttr() const
{
    return GetPrim().GetAttribute(UsdPhysicsTokens->physicsKinematicEnabled);
}

UsdAttribute
UsdPhysicsRigidBodyAPI::CreateKinematicEnabledAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdPhysicsTokens->physicsKinematicEnabled,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdPhysicsRigidBodyAPI::GetStartsAsleepAttr() const
{
    return GetPrim().GetAttribute(UsdPhysicsTokens->physicsStartsAsleep);
}

UsdAttribute
UsdPhysicsRigidBodyAPI::CreateStartsAsleepAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdPhysicsTokens->physicsStartsAsleep,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdPhysicsRigidBodyAPI::GetVelocityAttr() const
{
    return GetPrim().GetAttribute(UsdPhysicsTokens->physicsVelocity);
}

UsdAttribute
UsdPhysicsRigidBodyAPI::CreateVelocityAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdPhysicsTokens->physicsVelocity,
                       SdfValueTypeNames->Vector3f,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdPhysicsRigidBodyAPI::GetAngularVelocityAttr() const
{
    return GetPrim().GetAttribute(UsdPhysicsTokens->physicsAngularVelocity);
}

UsdAttribute
UsdPhysicsRigidBodyAPI::CreateAngularVelocityAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdPhysicsTokens->physicsAngularVelocity,
                       SdfValueTypeNames->Vector3f,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdRelationship
UsdPhysicsRigidBodyAPI::GetSimulationOwnerRel() const
{
    return GetPrim().GetRelationship(UsdPhysicsTokens->physicsSimulationOwner);
}

UsdRelationship
UsdPhysicsRigidBodyAPI::CreateSimulationOwnerRel() const
{
    return GetPrim().CreateRelationship(UsdPhysicsTokens->physicsSimulationOwner,
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
UsdPhysicsRigidBodyAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdPhysicsTokens->physicsRigidBodyEnabled,
        UsdPhysicsTokens->physicsKinematicEnabled,
        UsdPhysicsTokens->physicsStartsAsleep,
        UsdPhysicsTokens->physicsVelocity,
        UsdPhysicsTokens->physicsAngularVelocity,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdAPISchemaBase::GetSchemaAttributeNames(true),
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
