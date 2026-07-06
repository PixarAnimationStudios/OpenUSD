//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDLOD_DISTANCE_HEURISTIC_QUERY_H
#define USDLOD_DISTANCE_HEURISTIC_QUERY_H

/// \file usdLod/distanceHeuristicQuery.h

#include "pxr/pxr.h"
#include "pxr/usd/usdLod/api.h"
#include "pxr/usd/usdLod/heuristicQuery.h"

#include "pxr/base/gf/camera.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/range3d.h"
#include "pxr/base/gf/vec3f.h"

#include "pxr/base/vt/array.h"

#include <iosfwd>

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdLodDistanceHeuristicQuery
///
/// This class implements the internals of an LOD distance heuristic, but
/// without any dependencies on USD scene data or any USD data types. It is
/// intended to be usable inside a renderer to make fast, run-time LOD decisions
/// without requiring access back to the original USD layers or prims.
///
/// The data types match the UsdLodDistanceHeuristic prim type: a center point
/// or an extent, and one or two sets of distance thresholds. See the
/// documentation for UsdLodDistanceHeuristic for full details.
///
class UsdLodDistanceHeuristicQuery : public UsdLodHeuristicQuery
{
public:
    using Parent = UsdLodHeuristicQuery;

public:
    /// The `center` point of the LOD root in the local coordinates.  If
    /// `extent` is empty then this center point is used to compute the
    /// distance. If `extent` is not empty, `center` is ignored.
    GfVec3f center = {0.0, 0.0, 0.0};

    /// The bounding box of the geometry, stored in a GfRange3d. If the value
    /// is not empty, it will be used to compute the distance, otherwise the
    /// value of `center` will be used. The value is empty by default.
    GfRange3d extent;

    /// The distance thresholds for LOD transitions in ascending order.
    ///
    /// For example, the values {10.0, 50.0, 100.0} would mean:
    ///   - Use LOD 0 when distance < 10.0
    ///   - Use LOD 1 when 10.0 <= distance < 50.0
    ///   - Use LOD 2 when 50.0 <= distance < 100.0
    ///   - Use LOD 3 when 100.0 <= distance
    ///
    /// \see UsdLodDistanceHeuristicQuery::blendThresholds
    VtFloatArray thresholds;

    /// This defines distance thresholds for LOD transitions in ascending
    /// order. If `blendThresholds` values are defined, then the calculation
    /// can return a result between two levels of detail that can be used to
    /// blend or combine two LOD items.
    ///
    /// For example if:
    ///   - thresholds is [10.0, 50.0, 100.0]
    ///   - blendThresholds is [11.0, 55.0, 110.0]
    ///
    /// then:
    ///   - Use LOD 0 when distance < 10.0  (from thresholds)
    ///   - Blend LOD 0 and LOD 1 when 10.0 <= distance < 11.0 (min to max)
    ///   - Use LOD 1 when 11.0 <= distance < 50.0 (max to min)
    ///   - Blend LOD 1 and LOD 2 when 50 <= distance < 55.0
    ///   - Use LOD 2 when 55.0 <= distance < 100.0
    ///   - Blend LOD 2 and LOD 3 when 100 <= distance < 110.0
    ///   - Use LOD 3 when 110.0 <= distance
    ///
    /// The blend amount should be computed linearly between the corresponding
    /// `thresholds` and `blendThresholds` values.
    ///
    /// This field is advisory and not strongly interoperable.
    ///
    /// \see UsdLodDistanceHeuristicQuery::thresholds and
    /// UsdLodDistanceHeuristicQuery::ComputeLOD
    VtFloatArray blendThresholds;

public:
    UsdLodDistanceHeuristicQuery() = default;
    UsdLodDistanceHeuristicQuery(const UsdLodDistanceHeuristicQuery&) = default;
    UsdLodDistanceHeuristicQuery(UsdLodDistanceHeuristicQuery&&) = default;

    UsdLodDistanceHeuristicQuery(
        const TfToken& domainIn,
        const GfVec3f& centerIn = GfVec3f(0.0, 0.0, 0.0),
        const GfRange3d& extentIn = GfRange3d(),
        const VtFloatArray& thresholdsIn = VtFloatArray(),
        const VtFloatArray& blendThresholdsIn = VtFloatArray())
    : UsdLodHeuristicQuery(domainIn)
    , center(centerIn)
    , extent(extentIn)
    , thresholds(thresholdsIn)
    , blendThresholds(blendThresholdsIn)
    { }

    /// Destructor. The destructor is virtual to support RTTI and allow the
    /// object to be safely held as a pointer to UsdLodHeuristicQuery.
    USDLOD_API
    virtual ~UsdLodDistanceHeuristicQuery();


    /// Compute a distance given a `viewpoint` and a `transform`.
    ///
    /// The matrix `transform` should convert from the root's local coordinate
    /// space to `viewpoint's` coordinate space.  Typically, `viewpoint`
    /// will be specified in world coordinates and `transform` will convert
    /// from the root's object coordinates to world coordinates.
    ///
    /// If `transform` is close to singular then any extent is ignored and
    /// the distance is calculated from `viewpoint` to `center`.
    ///
    /// \see UsdLodDistanceHeuristicQuery::ComputeLOD
    USDLOD_API
    double ComputeDistance(const GfVec3d& viewpoint,
                           const GfMatrix4d& transform) const;

    /// \overload
    /// Calculate the distance applying hysteresis to the result.
    ///
    /// The arguments `prevDistance` and `hysteresis` should contain the most
    /// recently calculated distance and a hysteresis threshold respectively.
    /// If the actual distance is within `hysteresis` distance of `prevDistance`
    /// then `prevDistance` is returned. If the actual distance is outside the
    /// hysteresis window then the returned value lags behind the actual value
    /// by `hysteresis`. Specifically, if the actual distance is greater than
    /// `prevDistance + hysteresis` then the calculated distance - `hysteresis`
    /// is returned and if it is less than `prevDistance - hysteresis` then the
    /// calculated distance + `hysteresis` is returned. This helps prevent
    /// flickering between LOD levels when the computed size is very close to a
    /// threshold value.
    double ComputeDistance(const GfVec3d& viewpoint,
                           const GfMatrix4d& transform,
                           double prevDistance,
                           double hysteresis) const
    {
        const double distance = ComputeDistance(viewpoint, transform);

        if (prevDistance < distance - hysteresis) {
            // distance is increasing
            prevDistance = distance - hysteresis;
        } else if (prevDistance > distance + hysteresis) {
            // distance is decreasing
            prevDistance = distance + hysteresis;
        } else {
            // prevSize is still within the hysteresis window. No change.
        }

        return prevDistance;
    }

    /// Compute an LOD index given a distance.
    ///
    /// Compare the distance against the values in `thresholds` to determine an
    /// LOD index. If `blendThresholds` has values and `distance` is between a
    /// `thresholds` value and its corresponding `blendThresholds` value, then
    /// the returned value will be non-integral. If the renderer can blend
    /// between 2 LOD item children then it should use the returned index as
    /// follows:
    ///
    /// \code
    ///    float index = heuristic.ComputeLOD(distance);
    ///    if (blending_between_LOD_levels_is_active) {
    ///        int lowIndex = int(std::floor(index));
    ///        int highIndex = int(std::ceil(index));
    ///        float alpha = index - lowIndex;
    ///
    ///        // Display a linear interpolation using:
    ///        //   (1-alpha) * item[lowIndex] + alpha * item[highIndex]
    ///    } else {
    ///        int index = int(std::round(index))
    ///
    ///        // Display item[index]
    ///    }
    /// \endcode
    USDLOD_API
    float ComputeLOD(double distance) const;

    /// \overload
    ///
    /// Compute an LOD index given a `viewpoint` and a `transform`.
    ///
    /// Compute the distance given `viewpoint` and `transform,` then compute
    /// the LOD index from that distance.
    float ComputeLOD(const GfVec3d& viewpoint,
                     const GfMatrix4d& transform) const
    {
        return ComputeLOD(ComputeDistance(viewpoint, transform));
    }

    /// \overload
    ///
    /// Compute an LOD index given a `viewpoint,` a `transform`,
    /// a `prevDistance,` and a `hysteresis` value.
    ///
    /// Compute the distance given the input arguments, then compute
    /// the LOD index from that distance. The computed distance is
    /// stored in the double pointed to by distanceOut.
    ///
    /// The computed distance will be stored in `distanceOut` so it can be
    /// used in the next call to `ComputeLOD` with hysteresis.
    ///
    /// \see ComputeDistance for details on the hysteresis calculation.
    float ComputeLOD(const GfVec3d& viewpoint,
                     const GfMatrix4d& transform,
                     double prevDistance,
                     double hysteresis,
                     double* distanceOut) const
    {
        return ComputeLOD(
            (*distanceOut = ComputeDistance(viewpoint, transform,
                                            prevDistance, hysteresis)));
    }

};

USDLOD_API
std::ostream& operator<<(std::ostream&, const UsdLodDistanceHeuristicQuery&);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
