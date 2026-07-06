//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLod/screenSizeHeuristicQuery.h"
#include "pxr/usd/usdLod/tokens.h"

#include "pxr/base/arch/math.h"
#include "pxr/base/tf/diagnostic.h"

#include <algorithm>
#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE

// anonymous namespace
namespace {

// Indexes of the edges of the extent box
constexpr std::array<std::pair<int, int>, 12> boxEdges{
    {
        {0, 1}, {2, 3}, {4, 5}, {6, 7},  // parallel to x-axis
        {0, 2}, {1, 3}, {4, 6}, {5, 7},  // parallel to y-axis
        {0, 4}, {1, 5}, {2, 6}, {3, 7},  // parallel to z-axis
    }
};

inline
GfVec4d _To4d(const GfVec3d& v, double w)
{
    return GfVec4d(v[0], v[1], v[2], w);
}

double
_ComputeProjectedSphereSize(const GfFrustum& frustum,
                            const GfMatrix4d& transform,
                            const GfRange3d& extent)
{
    // Instead of dividing by 0, just return a "really big" multiple of the
    // screen size.
    constexpr double REALLY_BIG = std::numeric_limits<double>::max();
    
    // Transform the extent to world space (really, whatever space the frustum
    // is defined in).
    const GfVec3d worldMin = transform.Transform(extent.GetMin());
    const GfVec3d worldMax = transform.Transform(extent.GetMax());

    // Note that this is strictly only the correct radius and center when
    // the transform does have shear. A shearing transform could make two
    // corners other than worldMin and worldMax have the longest diagonal.
    // But it's close enough for level of detail work.
    const double worldRadius = (worldMax - worldMin).GetLength() / 2.0;
    const GfVec3d worldCenter = transform.Transform(extent.GetMidpoint());

    const GfFrustum::FrustumPlanes frustumPlanes = frustum.ComputePlanes();

    // See if the sphere is completely clipped by any of the frustum planes.
    for (const auto& plane : frustumPlanes) {
        if (plane.GetDistance(worldCenter) < -worldRadius) {
            // The entire sphere is behind the plane. Nothing is visible.
            return 0.0;
        }
    }

    // At least some part of the sphere was visible. Now approximate its
    // size in the frustum window.
    double radius;
    if (frustum.GetProjectionType() == GfFrustum::Perspective) {
        double distance = (frustum.GetPosition() - worldCenter).GetLength();
        if (distance == 0.0) {
            // The sphere is huge. Just return a large number.
            return REALLY_BIG;
        }

        radius = worldRadius * frustum.GetReferencePlaneDepth() / distance;
    } else {
        // Orthographic projection
        radius = worldRadius;
    }

    const double extentArea = M_PI * radius * radius;
    const GfVec2d windowDiagonal = frustum.GetWindow().GetSize();
    const double windowArea = windowDiagonal[0] * windowDiagonal[1];

    // This should never happen, but just to be safe.
    if (windowArea == 0.0) {
        return REALLY_BIG;
    }
    
    return extentArea / windowArea;
}

double
_ComputeProjectedExtentSize(const GfFrustum& frustum,
                            const GfMatrix4d& clipTransform,
                            const GfRange3d& extent)
{
    // Note that the caller already checked for a completely clipped extent. So
    // we know the extent intersects the frustum.

    // Get the corners of the extent, transformed into clip coordinates
    // (homogeneous NDC space without a perspective divide).
    std::array<GfVec4d, 8> extentCorners;
    for (int i = 0; i < 8; ++i) {
        extentCorners[i] = _To4d(extent.GetCorner(i), 1.0) * clipTransform;
    }

    // There are 12 edges so we need space for 24 vertices after clipping.  The
    // output points will be projected to 2d screen space in NDC coordinates
    // since that's all we need to compute the area.
    std::array<GfVec2d, 24> outputPoints;
    int outputSize = 0;

    // We don't want the screen size (for LOD purposes) of an object that is
    // sliding through the side of the frustum to change. We want it to retain
    // pretty much the same screen size metric as it moves sideways through the
    // frustum. So we're only going to clip against the near plane.
    for (const auto& edge : boxEdges) {
        const GfVec4d& p0 = extentCorners[edge.first];
        const GfVec4d& p1 = extentCorners[edge.second];

        double tMin = 0.0;
        double tMax = 1.0;

        // The equation for the near plane in clip space is:
        //
        //     GfVec4d(0, 0, 1, 1) * point = 0
        //
        // That dot product gives the signed distance from the plane to the
        // point.
        //
        //     distance = GfVec4d(0, 0, 1, 1) * point
        //
        // Which simplifies to:
        //
        //     distance = point.z + point.w
        //
        double d0 = p0[2] + p0[3];  // signed distance of p0 from nearPlane
        double d1 = p1[2] + p1[3];  // signed distance of p1 from nearPlane

        if (d0 < 0 && d1 < 0) {
            // The edge entirely clipped
            continue;
        } else if (d0 < 0 || d1 < 0) {
            // The edge is partially clipped
            double t = d0 / (d0 - d1);

            if (d0 < 0) {
                // The beginning of the edge is clipped
                tMin = std::max(tMin, t);
            } else {
                // The end of the edge is clipped
                tMax = std::min(tMax, t);
            }
            // Interpolate segment end points, project them from homogeneous
            // to NDC space, then discard the z coordinate.
            const GfVec3d ndc0 = GfProject(GfLerp(tMin, p0, p1));
            const GfVec3d ndc1 = GfProject(GfLerp(tMax, p0, p1));

            outputPoints[outputSize++] = GfVec2d(ndc0[0], ndc0[1]);
            outputPoints[outputSize++] = GfVec2d(ndc1[0], ndc1[1]);
        } else {
            // Not clipped at all, project to NDC space without interpolating
            const GfVec3d ndc0 = GfProject(p0);
            const GfVec3d ndc1 = GfProject(p1);

            outputPoints[outputSize++] = GfVec2d(ndc0[0], ndc0[1]);
            outputPoints[outputSize++] = GfVec2d(ndc1[0], ndc1[1]);
        }
    }

    // Utility lambdas:
    //
    // Comparator for points to order them by x and then y.
    auto _PointLess = [](const GfVec2d& a, const GfVec2d& b) -> bool
        {
            return ((a[0] < b[0]) ||
                    (a[0] == b[0] && a[1] < b[1]));
        };

    // Compare points for "equality". We know we're comparing clipped NDC
    // points, so all coordinates should be in [-1 .. 1] and 1e-6 is a
    // reasonable epsilon to eliminate nearly duplicate points while retaining
    // an accurate area estimate.
    auto _PointEq = [](const GfVec2d& a, const GfVec2d& b) -> bool
        {
            return (GfIsClose(a[0], b[0], 1.0e-6) &&
                    GfIsClose(a[1], b[1], 1.0e-6));
        };

    // Compute a 2D cross product of (b - a) with (c - a).
    auto _Cross2d =
        [](const GfVec2d& a, const GfVec2d& b, const GfVec2d& c) -> double
        {
            return ((b[0] - a[0]) * (c[1] - a[1]) -
                    (b[1] - a[1]) * (c[0] - a[0]));
        };

    // Order the points left to right.
    std::sort(&outputPoints[0], &outputPoints[outputSize], _PointLess);

    // Remove duplicates
    GfVec2d* endPtr = std::unique(&outputPoints[0], &outputPoints[outputSize],
                                  _PointEq);
    outputSize = std::distance(&outputPoints[0], endPtr);

    // Now construct the convex hull. Build a counter-clockwise polygon by
    // starting at the leftmost point and keeping only points that do not create
    // right turns. The first and last point in the hull are duplicates, so it
    // could be one larger than outputPoints.
    std::array<GfVec2d, outputPoints.size() + 1> hull;
    int hullSize = 0;
    
    // Build the lower hull.
    for (int i = 0; i < outputSize; ++i) {
        const GfVec2d& point = outputPoints[i];

        // If we made a right turn, eliminate the last point in the hull
        while (hullSize >= 2 &&
               _Cross2d(hull[hullSize-2], hull[hullSize-1], point) <= 0.0)
        {
            --hullSize;
        }
        
        hull[hullSize++] = point;
    }

    // Build the upper hull. We don't want to pop points off the lower hull so
    // set a lower size limit. Also, the first point of the upper hull is always
    // the last point of the lower hull and it's already in hull. So the limit
    // is the lower size plus 1.
    const int lowerLimit = hullSize + 1;

    // We want to skip the end point (outputPoints[outputSize-1]) because it was
    // already added at the end of the lower hull. So we can advance and then
    // test.
    for (int j = outputSize - 2; j >= 0; --j) {
        const GfVec2d& point = outputPoints[j];
        // If we made a right turn, eliminate the last point in the upper hull
        while (hullSize >= lowerLimit &&
               _Cross2d(hull[hullSize-2], hull[hullSize-1], point) <= 0.0)
        {
            --hullSize;
        }
        
        hull[hullSize++] = point;
    }

    // Note that the initial point in hull is conveniently duplicated as the last point.
    // This means we don't need to worry about wrapping around when using the Shoelace
    // Formula to compute the area.
    double area2 = 0;
    for (int i = 0; i < hullSize - 1; ++i) {
        area2 += hull[i][0] * hull[i+1][1] - hull[i][1] * hull[i+1][0];
    }

    // The sum is twice the area so divide by 2 to get the area and by 4 (the
    // area of the NDC view plane) to get a fraction of the view plane.
    const double screenSize = area2 / 8.0;

    return screenSize;
}

}
// end anonymous namespace


// virtual
UsdLodScreenSizeHeuristicQuery::~UsdLodScreenSizeHeuristicQuery()
{
    // Nothing to do. This method only exists to enable RTTI and
    // holding heuristics by pointer to UsdLodHeuristicQuery.
}

double
UsdLodScreenSizeHeuristicQuery::ComputeScreenSize(
    const GfFrustum& frustum,
    const GfMatrix4d& transform) const
{
    if (extent.IsEmpty()) {
        return 0.0;
    }

    if (projectionMethod == UsdLodTokens->projectedSphere) {
        return _ComputeProjectedSphereSize(frustum, transform, extent);
    } else if (projectionMethod != UsdLodTokens->projectedExtent) {
        // Complain if the projection method is invalid but use projectedExtent
        // anyway.
        TF_CODING_ERROR("Invalid value for projectionMethod ('%s').",
                        projectionMethod.GetText());
    }

    const GfBBox3d bbox(extent, transform);
    if (!frustum.Intersects(bbox)) {
        return 0.0;
    }

    const GfMatrix4d viewTransform =
        transform * frustum.ComputeViewMatrix();
    const GfMatrix4d clipTransform =
        viewTransform * frustum.ComputeProjectionMatrix();

    return _ComputeProjectedExtentSize(frustum, clipTransform, extent);
}

float
UsdLodScreenSizeHeuristicQuery::ComputeLOD(
    const double size) const
{
    // Assume that the data has been validated but try to not fail too badly if
    // it is invalid.
    //
    // Well behaved thresholds values means that the values are sorted in
    // decreasing order, and that each blendThresholds value is <= the
    // corresponding thresholds value and > the following thresholds value.
    //
    // If the values are not well behaved (not sorted) then upper_bound may
    // return an iterator to an unexpected threshold value and the computed
    // LOD index may be incorrect.
    //
    // If the blendThresholds values are well behaved then blendIndex will
    // always be the same as lodIndex (if there's no blending) or possibly 1
    // less (if there is blending). So we clamp to that range.
    auto lodIter = std::upper_bound(thresholds.begin(),
                                    thresholds.end(),
                                    size,
                                    std::greater<float>());
    int lodIndex = std::distance(thresholds.begin(), lodIter);

    // If there are no blendThresholds values then there's no blending
    if (blendThresholds.empty()) {
        return float(lodIndex);
    }

    // We may have a blending, find the blendIndex from blendThresholds.
    auto blendIter = std::upper_bound(blendThresholds.begin(),
                                      blendThresholds.end(),
                                      size,
                                      std::greater<float>());
    int blendIndex = std::distance(blendThresholds.begin(), blendIter);
    blendIndex = std::max(lodIndex - 1, std::min(lodIndex, blendIndex));

    // If lodIndex == blendIndex, then we found the same corresponding threshold
    // index in both vectors. This implies we're below the thresholds value and
    // are not blending. Similarly, if we're off the end of blendThresholds
    // we are not blending.
    if (blendIndex == lodIndex ||
        size_t(blendIndex) >= blendThresholds.size())
    {
        return float(lodIndex);
    }

    // Blend. Note that if lodIndex was 0 then blendIndex would also be 0 due
    // to clamping and we would have returned already.  So we know that
    // blendIndex == lodIndex - 1 and is the index into both thresholds and
    // blendThresholds of values that bracket screenSize.
    float a = thresholds[blendIndex];
    float b = blendThresholds[blendIndex];

    // Ensure that b is clamped to the next value from thresholds (just
    // in case the blendThresholds was even smaller).
    if (size_t(lodIndex) < thresholds.size()) {
        b = std::max(b, thresholds[lodIndex]);
    }

    // Validate that the data is well-behaved enough to compute the blend value.
    if (a >= size && size > b) {
        float fraction = (size - a) / (b - a);
        return blendIndex + fraction;
    }

    return float(lodIndex);
}

std::ostream& operator<<(std::ostream& out,
                         const UsdLodScreenSizeHeuristicQuery& query)
{
    out << "UsdLodScreenSizeHeuristicQuery{"
        << '"' << query.lodDomain << "\", ";

    if (query.extent.IsEmpty()) {
        out << "[--empty--], ";
    } else {
        out << query.extent << ", ";
    }

    out << "\"" << query.projectionMethod << "\", "
        << query.thresholds << ", "
        << query.blendThresholds << "}";

    return out;
}

PXR_NAMESPACE_CLOSE_SCOPE
