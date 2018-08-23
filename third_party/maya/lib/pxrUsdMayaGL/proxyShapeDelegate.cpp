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

namespace {

/// Estimates the coefficients of the plane equation \f$ax + by + cz + d = 0\f$
/// (the formulation used by GfPlane) such that \p pointOnPlane lies on the
/// plane and the plane's orientation approximates the \p fitPoints. Returns
/// \p true if the fit was successful and sets \p *outPlane to the fit plane.
///
/// The fitting is done via linear least squares; the general technique is
/// from <http://www.ilikebigbits.com/2015_03_04_plane_from_points.html>, but
/// here is an explanation that lines up with the actual code below:
///
/// We can first simplify the problem by assuming that \p pointOnPlane is the
/// origin; we then consider all \p fitPoints relative to \p pointOnPlane.
/// (We'll later offset the plane so that it does intersect \p pointOnPlane.)
/// This gives us the simplified plane equation \f$ax + by + cz = 0\f$.
///
/// Now, a trivial solution to the equation is to let
/// \f$(a, b, c) = (0, 0, 0)\f$. But we obviously want at least one of
/// \f$a, b, c\f$ to be nonzero. So we'll split the problem into three
/// subproblems:
///     (1) let \f$a = 1\f$ and try to fit parameters \f$b, c\f$
///     (2) let \f$b = 1\f$ and try to fit parameters \f$a, c\f$
///     (3) let \f$c = 1\f$ and try to fit parameters \f$a, b\f$.
/// As long as we can fit a plane with a nonzero normal, at least one of those
/// systems should give us a proper fit.
///
/// Let's look at the case where \f$a = 1\f$. This gives us the equation
/// \f$x + by + cz = 0\f$, or \f$by + cz = -x\f$. We can put this in terms of
/// matrices, \f$ \bf{X} \bf{\beta} = \bf{y}\f$ where
/// \f[
/// \begin{array}{ccc}
///     \bf{X} = \begin{bmatrix}
///         y_0 & z_0 \\ y_1 & z_1 \\ \vdots & \vdots \\ y_n & z_n
///     \end{bmatrix}
/// &
///     \bf{\beta} = \begin{bmatrix}
///         b & c
///     \end{bmatrix}
/// &
///     \bf{y} = \begin{bmatrix}
///         -x_0 \\ -x_1 \\ \vdots \\ -x_n
///     \end{bmatrix}
/// \end{array}
/// \f]
///
/// The linear least squares solution is
/// \f$\bf{\hat\beta} = (\bf{X}^\top \bf{X})^{-1} \bf{X}^\top \bf{y}\f$.
///
/// Now \f$\bf{X}^\top \bf{X} =
/// \begin{bmatrix}
///     \sum y_n^2 & \sum y_n z_n \\ \sum y_n z_n & \sum z_n^2
/// \end{bmatrix}
/// \f$
/// and \f$\bf{X}^\top \bf{y} =
/// \begin{bmatrix}
///    \sum -x_n y_n \\ \sum -x_n z_n
/// \end{bmatrix}\f$.
///
/// So \f$\bf{\hat\beta} =
/// \frac{1}{\det{\bf{X}^\top \bf{X}}}
/// \begin{bmatrix}
///     -(\sum x_n y_n) (\sum z_n^2) + (\sum x_n z_n) (\sum y_n z_n)
///     \\ (\sum x_n y_n) (\sum y_n z_n) - (\sum x_n z_n) (\sum y_n^2)
/// \end{bmatrix}\f$
/// where \f$\det{\bf{X}^\top \bf{X}} =
/// (\sum y_n^2)(\sum z_n^2) - (\sum y_n z_n)^2\f$.
///
/// Thus our fitted coefficients are \f$a = 1\f$,
/// \f$b = \frac{-(\sum x_n y_n) (\sum z_n^2) + (\sum x_n z_n) (\sum y_n z_n)}
///     {\det{\bf{X}^\top \bf{X}}}\f$, and
/// \f$c = \frac{(\sum x_n y_n) (\sum y_n z_n) - (\sum x_n z_n) (\sum y_n^2)}
///     {\det{\bf{X}^\top \bf{X}}}\f$.
///
/// We now repeat the above computations for the cases where \f$b = 1\f$ and
/// \f$c = 1\f$. If we indeed have a nonzero normal, then
/// \f$\det{\bf{X}^\top \bf{X}}\f$ should be nonzero in at least one of the
/// three cases. We'll arbitrarily choose the case where the magnitude of the
/// determinant is the greatest.
///
/// Now that we have \f$a, b, c\f$, we can offset the plane so that it actually
/// does intersect \p pointOnPlane. We simply solve for \f$d = -ax -by -cz\f$.
static
bool
_LeastSquaresFitPlane(
    const GfVec3d& pointOnPlane,
    const std::vector<GfVec3d>& fitPoints,
    GfPlane* outPlane)
{
    // We need at least three \p fitPoints to determine the equation.
    if (fitPoints.size() < 3) {
        return false;
    }

    // These variables hold the sums $\sum x_n^2$, $\sum x_n y_n$, etc.
    double xx = 0.0;
    double xy = 0.0;
    double xz = 0.0;
    double yy = 0.0;
    double yz = 0.0;
    double zz = 0.0;

    // Loop through all the \p fitPoints, but actually consider the "offset"
    // from the \p pointOnPlane. This helps us simplify the plane equation to
    // just $ax + by + cz = 0$ for the time being.
    for (const GfVec3d& fitPoint : fitPoints) {
        const GfVec3d offset = fitPoint - pointOnPlane;
        xx += offset[0] * offset[0];
        xy += offset[0] * offset[1];
        xz += offset[0] * offset[2];
        yy += offset[1] * offset[1];
        yz += offset[1] * offset[2];
        zz += offset[2] * offset[2];
    }

    // Compute the determinant of $\bf{X}^\top \bf{X}$ for each of the three
    // cases $a = 1$, $b = 1, $c = 1$.
    const double detA = (yy * zz) - (yz * yz);
    const double detB = (xx * zz) - (xz * xz);
    const double detC = (xx * yy) - (xy * xy);

    // Since we need a nonzero determinant to compute the matrix inverse, we'll
    // just choose the case (among $a=1$, $b=1, $c=1$) that has the largest
    // absolute determinant $\det{\bf{X}^\top \bf{X}}$. (If they're all zero,
    // return false.)
    GfVec3d equation;
    if (GfAbs(detA) > 0.0 &&
            GfAbs(detA) >= GfAbs(detB) &&
            GfAbs(detA) >= GfAbs(detC)) {
        equation[0] = 1.0;
        equation[1] = ((xz * yz) - (xy * zz)) / detA;
        equation[2] = ((xy * yz) - (xz * yy)) / detA;
    }
    else if (GfAbs(detB) > 0.0 &&
            GfAbs(detB) >= GfAbs(detC)) {
        equation[0] = ((yz * xz) - (xy * zz)) / detB;
        equation[1] = 1.0;
        equation[2] = ((xy * xz) - (yz * xx)) / detB;
    }
    else if (GfAbs(detC) > 0.0)
    {
        equation[0] = ((yz * xy) - (xz * yy)) / detC;
        equation[1] = ((xz * xy) - (yz * xx)) / detC;
        equation[2] = 1.0;
    }
    else {
        // All of the determinants are zero.
        return false;
    }

    // Now move the plane to actually intersect \p pointOnPlane by solving for
    // $d$.
    const double d = -GfDot(equation, pointOnPlane);
    outPlane->Set(GfVec4d(equation[0], equation[1], equation[2], d));
    return true;
}

} // anonymous namespace

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

    // Perform a least-squares fitting over the hit "point cloud".
    const GfVec3d& worldHitPoint = hit.worldSpaceHitPoint;
    GfPlane worldPlane;
    if (!_LeastSquaresFitPlane(worldHitPoint, sameObjectHits, &worldPlane)) {
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
    const GfVec3d point = worldToLocal.Transform(worldHitPoint);
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
