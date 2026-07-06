//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLod/distanceHeuristicQuery.h"

#include "pxr/base/gf/bbox3d.h"
#include "pxr/base/gf/ray.h"

#include <algorithm>
#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE

// virtual
UsdLodDistanceHeuristicQuery::~UsdLodDistanceHeuristicQuery()
{
    // Nothing to do. This method only exists to enable RTTI and
    // holding heuristics by pointer to UsdLodHeuristicQuery.
}

double
UsdLodDistanceHeuristicQuery::ComputeDistance(
    const GfVec3d& viewpoint,
    const GfMatrix4d& transform) const
{
    GfVec3d target = center;

    // If we have a valid extent and transform, use them. Apologies for
    // the hard-coded epsilon.
    constexpr double epsilon = 1.0e-10;
    if (!extent.IsEmpty() &&
        std::abs(transform.GetDeterminant()) > epsilon)
    {
        const GfMatrix4d invTransform = transform.GetInverse();
        const GfVec3d invViewpoint = invTransform.Transform(viewpoint);

        if (extent.Contains(invViewpoint)) {
            // The viewpoint is inside the extent. Distance is 0.0
            return 0.0;
        }

        // set target to the closest point on the extent.
        const GfVec3d extentMin = extent.GetMin();
        const GfVec3d extentMax = extent.GetMax();

        target[0] = std::clamp(invViewpoint[0], extentMin[0], extentMax[0]);
        target[1] = std::clamp(invViewpoint[1], extentMin[1], extentMax[1]);
        target[2] = std::clamp(invViewpoint[2], extentMin[2], extentMax[2]);
    }

    target = transform.Transform(target);
    double distance = (target - viewpoint).GetLength();

    return distance;
}

float
UsdLodDistanceHeuristicQuery::ComputeLOD(double distance) const
{
    // Assume that the data has been validated but try to not fail too badly if
    // it is invalid.
    //
    // Well behaved thresholds values means that the values are sorted in
    // increasing order, and that each blendThresholds value is >= the
    // corresponding thresholds value and < the following thresholds value.
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
                                    distance);
    int lodIndex = std::distance(thresholds.begin(), lodIter);

    // If there are no blendThresholds values then there's no blending
    if (blendThresholds.empty()) {
        return float(lodIndex);
    }

    // We may have blending, find the blendIndex from blendThresholds.
    auto blendIter = std::upper_bound(blendThresholds.begin(),
                                      blendThresholds.end(),
                                      distance);
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
    // blendThresholds of values that bracket distance.
    float a = thresholds[blendIndex];
    float b = blendThresholds[blendIndex];

    // Ensure that b is clamped to the next value from thresholds (just
    // in case the blendThresholds value was huge).
    if (size_t(lodIndex) < thresholds.size()) {
        b = std::min(b, thresholds[lodIndex]);
    }

    // Validate that the data is well-behaved enough to compute the blend value.
    if (a <= distance && distance < b) {
        float fraction = (distance - a) / (b - a);
        return blendIndex + fraction;
    }

    return float(lodIndex);
}

std::ostream& operator<<(std::ostream& out,
                         const UsdLodDistanceHeuristicQuery& query)
{
    out << "UsdLodDistanceHeuristicQuery{"
        << '"' << query.lodDomain << "\", "
        << query.center << ", ";

    if (query.extent.IsEmpty()) {
        out << "[--empty--], ";
    } else {
        out << query.extent << ", ";
    }

    out << query.thresholds << ", "
        << query.blendThresholds << "}";

    return out;
}

PXR_NAMESPACE_CLOSE_SCOPE
