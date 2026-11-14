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

// The public getters and the parser share these so they resolve the same
// bodies and local poses.

/// Return the nearest prim at or above \p prim with an enabled
/// UsdPhysicsRigidBodyAPI, or an invalid prim when there is none.
UsdPrim
UsdPhysics_FindEnclosingRigidBody(const UsdPrim& prim);

/// Return the prim targeted by a joint body relationship, or an invalid prim
/// when there is no target or it is not on the stage. \p targetPath receives
/// the target path even when the target is dangling.
UsdPrim
UsdPhysics_GetJointBodyTarget(const UsdRelationship& bodyRel,
                              SdfPath* targetPath = nullptr);

/// Return the rigid body a joint attaches to through \p target, or an invalid
/// prim for the world, and write the authored local pose expressed in that
/// frame.
UsdPrim
UsdPhysics_ResolveJointAttachment(const UsdPrim& target,
                                  const GfVec3f& authoredPosition,
                                  const GfQuatf& authoredOrientation,
                                  UsdGeomXformCache* xformCache,
                                  GfVec3f* outPosition,
                                  GfQuatf* outOrientation);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_PHYSICS_BODY_RESOLUTION_H
