//
// Copyright 2018 Pixar
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

#include "pxr/imaging/glf/glew.h" // This header must absolutely come first.

#include "pxrUsdMayaGL/batchRenderer.h"

#include "usdMaya/proxyShape.h"

#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/frustum.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/range1d.h"
#include "pxr/base/gf/ray.h"
#include "pxr/base/gf/vec3d.h"

#include "pxr/base/tf/registryManager.h"

#include "pxr/imaging/glf/drawTarget.h"
#include "pxr/imaging/glf/glContext.h"

#include <maya/MFnDagNode.h>

PXR_NAMESPACE_OPEN_SCOPE

static PxrMayaHdPrimFilter _sharedPrimFilter = {
        HdRprimCollection(
                TfToken("UsdMayaGL_ClosestPointOnProxyShape"),
                HdReprSelector(HdReprTokens->refined)
        ),
        TfTokenVector()  // Render Tags
};

/// Delegate for computing a ray intersection against a UsdMayaProxyShape by
/// rendering using Hydra via the UsdMayaGLBatchRenderer.
bool
UsdMayaGL_ClosestPointOnProxyShape(
    const UsdMayaProxyShape& shape,
    const GfRay& ray,
    GfVec3d* outClosestPoint,
    GfVec3d* outClosestNormal)
{
    MStatus status;
    const MFnDagNode dagNodeFn(shape.thisMObject(), &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    MDagPath shapeDagPath;
    status = dagNodeFn.getPath(shapeDagPath);
    CHECK_MSTATUS_AND_RETURN(status, false);

    // Try to populate our shared collection with the shape. If we can't, then
    // we must bail.
    UsdMayaGLBatchRenderer& renderer = UsdMayaGLBatchRenderer::GetInstance();
    if (!renderer.PopulateCustomPrimFilter(
            shapeDagPath, _sharedPrimFilter)) {
        return false;
    }

    // Since we're just using the existing shape adapters, we'll compute
    // everything in world-space and then convert back to local space when
    // returning the hit point.
    GfMatrix4d localToWorld(shapeDagPath.inclusiveMatrix().matrix);
    GfRay worldRay(
            localToWorld.Transform(ray.GetStartPoint()),
            localToWorld.TransformDir(ray.GetDirection()).GetNormalized());

    // Create selection frustum (think very thin tube from ray origin towards
    // ray direction).
    GfRotation rotation(-GfVec3d::ZAxis(), worldRay.GetDirection());
    GfFrustum frustum(
            worldRay.GetStartPoint(),
            rotation,
            /*window*/ GfRange2d(GfVec2d(-0.1, -0.1), GfVec2d(0.1, 0.1)),
            /*nearFar*/ GfRange1d(0.1, 10000.0),
            GfFrustum::Orthographic);

    // Draw the shape into the draw target, and the intersect against the draw
    // target. Unbind after we're done.
    GfMatrix4d viewMatrix = frustum.ComputeViewMatrix();
    GfMatrix4d projectionMatrix = frustum.ComputeProjectionMatrix();

    HdxPickHitVector isectResult;
    bool didIsect = renderer.TestIntersectionCustomPrimFilter(
            _sharedPrimFilter,
            viewMatrix,
            projectionMatrix,
            &isectResult);

    if (!didIsect) {
        return false;
    }

    // Our hit point and hit normal are both in world space, so convert back
    // to local space.
    const HdxPickHit& hit = isectResult[0];
    const GfMatrix4d worldToLocal = localToWorld.GetInverse();
    const GfVec3d point = worldToLocal.Transform(hit.worldSpaceHitPoint);
    const GfVec3d normal = worldToLocal.TransformDir(hit.worldSpaceHitNormal);

    if (!std::isfinite(point.GetLengthSq()) ||
                !std::isfinite(normal.GetLengthSq())) {
        TF_CODING_ERROR(
                "point (%f, %f, %f) or normal (%f, %f, %f) is non-finite",
                point[0], point[1], point[2],
                normal[0], normal[1], normal[2]);
        return false;
    }

    *outClosestPoint = point;
    *outClosestNormal = normal;
    return true;
}

/// Delegate for returning whether object soft-select mode is currently on
/// Technically, we could make ProxyShape track this itself, but then that would
/// be making two callbacks to track the same thing... so we use BatchRenderer
/// implementation
bool
UsdMayaGL_ObjectSoftSelectEnabled()
{
    return UsdMayaGLBatchRenderer::GetInstance().GetObjectSoftSelectEnabled();
}


TF_REGISTRY_FUNCTION(UsdMayaProxyShape)
{
    UsdMayaProxyShape::SetClosestPointDelegate(
            UsdMayaGL_ClosestPointOnProxyShape);
    UsdMayaProxyShape::SetObjectSoftSelectEnabledDelegate(
            UsdMayaGL_ObjectSoftSelectEnabled);
}

PXR_NAMESPACE_CLOSE_SCOPE
