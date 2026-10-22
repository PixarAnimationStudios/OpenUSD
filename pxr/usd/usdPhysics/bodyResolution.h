//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_PHYSICS_BODY_RESOLUTION_H
#define PXR_USD_USD_PHYSICS_BODY_RESOLUTION_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/gf/quatf.h"
#include "pxr/base/gf/vec3f.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdGeomXformCache;

// UsdPhysicsJoint::GetBody0/1 and GetLocalPose0/1 and the joint parser all
// resolve a joint side through these two functions, so they report the same
// bodies and local poses. testUsdPhysicsJoint checks that they agree.

/// Return the prim the first target of a joint body relationship points at,
/// or an invalid prim when the relationship is invalid, has no targets, or its
/// target is not a prim on the stage. If \p targetPath is given, it receives
/// the first target, or an empty path when there is none; that path is set even
/// when the target is dangling and the returned prim is invalid.
UsdPrim
UsdPhysics_GetJointBodyTarget(const UsdRelationship& bodyRel,
                              SdfPath* targetPath = nullptr);

/// Resolve one side of a joint whose body relationship targets \p target.
/// Returns the nearest enclosing enabled rigid body, or an invalid prim when
/// the side is owned by the world, and writes the joint-local pose for the
/// authored pose in that body's frame. An invalid \p target (an unset or
/// dangling relationship) anchors the side to the world at its authored pose.
UsdPrim
UsdPhysics_ResolveJointSide(const UsdPrim& target,
                            const GfVec3f& authoredPosition,
                            const GfQuatf& authoredOrientation,
                            UsdGeomXformCache* xformCache,
                            GfVec3f* outPosition,
                            GfQuatf* outOrientation);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_PHYSICS_BODY_RESOLUTION_H
