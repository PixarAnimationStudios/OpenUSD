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

#include "pxr/imaging/hdx/intersector.h"

#include <maya/MFnDagNode.h>

PXR_NAMESPACE_OPEN_SCOPE

static constexpr size_t ISECT_RESOLUTION = 256;
static GlfDrawTargetRefPtr _sharedDrawTarget = nullptr;
static HdRprimCollection _sharedRprimCollection(
        TfToken("UsdMayaGL_ClosestPointOnProxyShape"),
        HdReprSelector(HdTokens->refined));

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
    if (!renderer.PopulateCustomCollection(
            shapeDagPath, _sharedRprimCollection)) {
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

    // Create shared draw target if it doesn't exist yet.
    // Similar to what the HdxIntersector does.
    if (!_sharedDrawTarget) {
        GlfSharedGLContextScopeHolder sharedContextHolder;
        _sharedDrawTarget = GlfDrawTarget::New(
                GfVec2i(ISECT_RESOLUTION, ISECT_RESOLUTION));
        _sharedDrawTarget->Bind();
        _sharedDrawTarget->AddAttachment("color",
                GL_RGBA, GL_FLOAT, GL_RGBA);
        _sharedDrawTarget->AddAttachment("depth",
                GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, GL_DEPTH24_STENCIL8);
        _sharedDrawTarget->Unbind();
    }

    // Use a separate drawTarget (framebuffer object) for each GL context
    // that uses this renderer, but the drawTargets share attachments/textures.
    // This ensures that things don't go haywire when changing the GL context.
    GlfDrawTargetRefPtr drawTarget = GlfDrawTarget::New(
            GfVec2i(ISECT_RESOLUTION, ISECT_RESOLUTION));
    drawTarget->Bind();
    drawTarget->CloneAttachments(_sharedDrawTarget);

    // Draw the shape into the draw target, and the intersect against the draw
    // target. Unbind after we're done.
    GfMatrix4d viewMatrix = frustum.ComputeViewMatrix();
    GfMatrix4d projectionMatrix = frustum.ComputeProjectionMatrix();
    renderer.DrawCustomCollection(
            _sharedRprimCollection,
            viewMatrix,
            projectionMatrix,
            /*viewport*/ GfVec4d(0, 0, ISECT_RESOLUTION, ISECT_RESOLUTION));

    HdxIntersector::Result isectResult;
    bool didIsect = renderer.TestIntersectionCustomCollection(
            _sharedRprimCollection,
            viewMatrix,
            projectionMatrix,
            &isectResult);
    drawTarget->Unbind();

    if (!didIsect) {
        return false;
    }

    // We use the nearest hit as our intersection point.
    HdxIntersector::Hit hit;
    if (!isectResult.ResolveNearest(&hit)) {
        return false;
    }

    // We use the set of all hit points to estimate the surface normal.
    HdxIntersector::HitVector hits;
    if (!isectResult.ResolveAll(&hits)) {
        return false;
    }

    // Cull the set of hit points to only those points on the same object as
    // the intersection point, in case the hit points span multiple objects.
    std::vector<GfVec3d> sameObjectHits;
    for (const HdxIntersector::Hit& h : hits) {
        if (h.objectId == hit.objectId &&
                h.instanceIndex == hit.instanceIndex &&
                h.elementIndex == hit.elementIndex) {
            sameObjectHits.push_back(h.worldSpaceHitPoint);
        }
    }

    // Fit a plane to the hit "point cloud" in order to find the normal.
    GfPlane worldPlane;
    if (!GfFitPlaneToPoints(sameObjectHits, &worldPlane)) {
        return false;
    }

    // Make the plane face in the opposite direction of the incoming ray.
    // Note that this isn't the same as GfPlane::Reorient().
    if (GfDot(worldRay.GetDirection(), worldPlane.GetNormal()) > 0.0) {
        worldPlane.Set(
                -worldPlane.GetNormal(),
                worldPlane.GetDistanceFromOrigin());
    }

    // Our hit point and plane normal are both in world space, so convert back
    // to local space.
    const GfMatrix4d worldToLocal = localToWorld.GetInverse();
    const GfVec3d point = worldToLocal.Transform(hit.worldSpaceHitPoint);
    const GfVec3d normal = worldPlane.Transform(worldToLocal).GetNormal();

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
