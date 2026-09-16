//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdPhysics/joint.h"
#include "pxr/usd/usdPhysics/rigidBodyAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usdGeom/xformable.h"
#include "pxr/usd/usdGeom/xformCache.h"
#include "pxr/base/gf/transform.h"

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

PXR_NAMESPACE_OPEN_SCOPE

// Walk from \p prim up the ancestor hierarchy to the nearest enabled
// UsdPhysicsRigidBodyAPI prim, which is the rigid body that owns \p prim. A
// body whose physics:rigidBodyEnabled resolves to false takes no part in
// simulation and is skipped. Returns an invalid prim when there is no enabled
// body in the ancestry.
static UsdPrim
_FindEnclosingBody(const UsdPrim& prim)
{
    UsdPrim cur = prim;
    while (cur && !cur.IsPseudoRoot())
    {
        const UsdPhysicsRigidBodyAPI rigidBodyAPI(cur);
        if (rigidBodyAPI)
        {
            bool rigidBodyEnabled = true;
            rigidBodyAPI.GetRigidBodyEnabledAttr().Get(&rigidBodyEnabled);
            if (rigidBodyEnabled)
            {
                return cur;
            }
        }
        cur = cur.GetParent();
    }

    return UsdPrim();
}

// Return the prim a joint body relationship targets, or an invalid prim when the
// relationship is unset or has no target.
static UsdPrim
_GetJointBodyTarget(const UsdRelationship& bodyRel)
{
    if (!bodyRel)
    {
        return UsdPrim();
    }
    SdfPathVector targets;
    bodyRel.GetTargets(&targets);
    if (targets.empty())
    {
        return UsdPrim();
    }
    return bodyRel.GetPrim().GetStage()->GetPrimAtPath(targets.front());
}

UsdPrim
UsdPhysicsJoint::GetBody0() const
{
    return _FindEnclosingBody(_GetJointBodyTarget(GetBody0Rel()));
}

UsdPrim
UsdPhysicsJoint::GetBody1() const
{
    return _FindEnclosingBody(_GetJointBodyTarget(GetBody1Rel()));
}

// Local-to-world for \p prim, using \p cache when one is supplied so repeated
// resolutions across a nested mechanism (e.g. a Franka arm) reuse ancestor
// transforms instead of walking to the root every call.
static GfMatrix4d
_LocalToWorld(const UsdPrim& prim, UsdGeomXformCache* cache)
{
    if (!prim)
    {
        return GfMatrix4d(1.0);
    }
    if (cache)
    {
        return cache->GetLocalToWorldTransform(prim);
    }
    return UsdGeomXformable(prim).ComputeLocalToWorldTransform(
        UsdTimeCode::Default());
}

// Compute the joint's local pose for one body relationship, in the frame of the
// prim the joint attaches to. Seeds from the authored localPos/localRot for that
// side, then rebases into the attachment frame when the rel target is not itself
// the attachment prim (e.g. a collider under the body, or an arbitrary anchor
// prim), and folds in that frame's scale. The out pose is always written: an
// unset or dangling relationship yields the authored local pose unchanged,
// since there is no resolved frame to rebase into or scale to bake. Returns
// false only for a dangling relationship (target does not resolve to a prim on
// the stage); an unset relationship succeeds with the authored world-anchor
// pose.
static bool
_ComputeJointLocalPose(const UsdRelationship& bodyRel,
                       const GfVec3f& authoredPosition,
                       const GfQuatf& authoredOrientation,
                       UsdGeomXformCache* cache,
                       GfVec3f* outPosition,
                       GfQuatf* outOrientation)
{
    GfVec3f position = authoredPosition;
    GfQuatf orientation = authoredOrientation;
    orientation.Normalize();

    // A relationship that is unset, or whose target does not resolve to a prim
    // on the stage, gives no frame to rebase into or scale to bake, so the
    // authored local pose is passed through unchanged in both cases. The
    // return value distinguishes them: an unset relationship is a valid world
    // anchor (true), while a dangling relationship is malformed authoring
    // (false). Either way the caller receives the pose it authored and learns
    // whether a body resolved from GetBody0 / GetBody1.
    const UsdPrim relPrim = _GetJointBodyTarget(bodyRel);
    if (!relPrim)
    {
        *outPosition = position;
        *outOrientation = orientation;

        SdfPathVector targets;
        if (bodyRel)
        {
            bodyRel.GetTargets(&targets);
        }
        return targets.empty();
    }

    // Resolve the nearest enclosing enabled body. When one exists and it is a
    // proper ancestor of the target, the authored pose is expressed against the
    // target and must be rebased into the body's frame. Otherwise (no enclosing
    // body, or the target is itself the body) the authored pose is already in
    // the right frame and only that frame's scale is baked.
    const UsdPrim body = _FindEnclosingBody(relPrim);

    GfVec3f scale;
    if (body && body != relPrim)
    {
        const GfMatrix4d worldRel = _LocalToWorld(relPrim, cache);
        const GfMatrix4d bodyMat = _LocalToWorld(body, cache);

        GfMatrix4d localAnchor;
        localAnchor.SetIdentity();
        localAnchor.SetTranslate(GfVec3d(position));
        localAnchor.SetRotateOnly(GfQuatd(orientation));

        const GfMatrix4d worldAnchor = localAnchor * worldRel;
        GfMatrix4d bodyLocalAnchor = worldAnchor * bodyMat.GetInverse();
        bodyLocalAnchor = bodyLocalAnchor.RemoveScaleShear();

        position = GfVec3f(bodyLocalAnchor.ExtractTranslation());
        orientation = GfQuatf(bodyLocalAnchor.ExtractRotationQuat());
        orientation.Normalize();

        scale = GfVec3f(GfTransform(bodyMat).GetScale());
    }
    else
    {
        scale = GfVec3f(GfTransform(_LocalToWorld(relPrim, cache)).GetScale());
    }

    // Physics simulation has no notion of scale, so the resolved frame's scale
    // is baked into the local translation here.
    for (int i = 0; i < 3; ++i)
    {
        position[i] *= scale[i];
    }

    *outPosition = position;
    *outOrientation = orientation;
    return true;
}

bool
UsdPhysicsJoint::GetLocalPose0(GfVec3f* position, GfQuatf* orientation,
                               UsdGeomXformCache* xformCache) const
{
    GfVec3f authoredPosition(0.0f);
    GfQuatf authoredOrientation(1.0f);
    GetLocalPos0Attr().Get(&authoredPosition);
    GetLocalRot0Attr().Get(&authoredOrientation);
    return _ComputeJointLocalPose(GetBody0Rel(), authoredPosition,
                                  authoredOrientation, xformCache,
                                  position, orientation);
}

bool
UsdPhysicsJoint::GetLocalPose1(GfVec3f* position, GfQuatf* orientation,
                               UsdGeomXformCache* xformCache) const
{
    GfVec3f authoredPosition(0.0f);
    GfQuatf authoredOrientation(1.0f);
    GetLocalPos1Attr().Get(&authoredPosition);
    GetLocalRot1Attr().Get(&authoredOrientation);
    return _ComputeJointLocalPose(GetBody1Rel(), authoredPosition,
                                  authoredOrientation, xformCache,
                                  position, orientation);
}

PXR_NAMESPACE_CLOSE_SCOPE
