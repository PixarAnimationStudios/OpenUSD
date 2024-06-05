//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdPhysics/collisionAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdPhysicsCollisionAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdPhysicsCollisionAPI::~UsdPhysicsCollisionAPI()
{
}

/* static */
UsdPhysicsCollisionAPI
UsdPhysicsCollisionAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdPhysicsCollisionAPI();
    }
    return UsdPhysicsCollisionAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdPhysicsCollisionAPI::_GetSchemaKind() const
{
    return UsdPhysicsCollisionAPI::schemaKind;
}

/* static */
bool
UsdPhysicsCollisionAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdPhysicsCollisionAPI>(whyNot);
}

/* static */
UsdPhysicsCollisionAPI
UsdPhysicsCollisionAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdPhysicsCollisionAPI>()) {
        return UsdPhysicsCollisionAPI(prim);
    }
    return UsdPhysicsCollisionAPI();
}

/* static */
const TfType &
UsdPhysicsCollisionAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdPhysicsCollisionAPI>();
    return tfType;
}

/* static */
bool 
UsdPhysicsCollisionAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdPhysicsCollisionAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdPhysicsCollisionAPI::GetCollisionEnabledAttr() const
{
    return GetPrim().GetAttribute(UsdPhysicsTokens->physicsCollisionEnabled);
}

UsdAttribute
UsdPhysicsCollisionAPI::CreateCollisionEnabledAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdPhysicsTokens->physicsCollisionEnabled,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdRelationship
UsdPhysicsCollisionAPI::GetSimulationOwnerRel() const
{
    return GetPrim().GetRelationship(UsdPhysicsTokens->physicsSimulationOwner);
}

UsdRelationship
UsdPhysicsCollisionAPI::CreateSimulationOwnerRel() const
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
UsdPhysicsCollisionAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdPhysicsTokens->physicsCollisionEnabled,
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
