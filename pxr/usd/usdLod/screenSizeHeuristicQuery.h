//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDLOD_SCREENSIZE_HEURISTIC_QUERY_H
#define USDLOD_SCREENSIZE_HEURISTIC_QUERY_H

/// \file usdLod/screenSizeHeuristicQuery.h

#include "pxr/pxr.h"
#include "pxr/usd/usdLod/api.h"
#include "pxr/usd/usdLod/heuristicQuery.h"
#include "pxr/usd/usdLod/tokens.h"

#include "pxr/base/gf/frustum.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/range3d.h"
#include "pxr/base/gf/vec3f.h"

#include "pxr/base/vt/array.h"

#include <iosfwd>

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdLodScreenSizeHeuristicQuery
///
/// This class implements the internals of an LOD screenSize heuristic, but
/// without any dependencies on USD scene data or any USD data types. It is
/// intended to be usable inside a renderer to make fast, run-time LOD decisions
/// without requiring access back to the original USD layers or prims.
///
/// The data types match the UsdLodScreenSizeHeuristic prim type: a center point
/// or an extent, and one or two sets of screen size thresholds. See the
/// documentation for UsdLodScreenSizeHeuristic for full details.
///
class UsdLodScreenSizeHeuristicQuery : public UsdLodHeuristicQuery
{
public:
    using Parent = UsdLodHeuristicQuery;

public:
    /// The bounding box of the geometry, stored in a GfRange3d. If the value
    /// is not empty, it will be used to compute the screenSize, otherwise the
    /// value of `center` will be used. The value is empty by default.
    GfRange3d extent;

    /// Defines how screen size is calculated.
    ///   - projectedExtent: Project the extent to the view plane
    ///   - projectedSphere: Project the extent bounding sphere to the view plane
    ///
    /// If `projectionMethod` is `projectedSphere`, a very quick approximation
    /// of the extent area is used.
    ///
    /// 1. The extent is transformed to world space.
    ///
    /// 2. A sphere's center and diameter are computed from the extent's
    ///    diagonal.
    ///
    /// 3. If the entire sphere is clipped by the frustum, the area is 0.0.
    ///
    /// 4. If the frustum uses perspective then the diameter is scaled down by
    ///    the distance from the view point to the center. The diameter is
    ///    unchanged for orthographic frustums.
    ///
    /// 5. The area of a circle with that diameter is divided by the area of the
    ///    view window to calculate the fraction of the view window that the sphere
    ///    could cover.
    ///
    /// If `projectionMethod` is `projectedExtent`, a more expensive but more
    /// accurate computation is used.
    ///
    /// 1. The extent box is transformed into the frustum's clip space.
    ///
    /// 2. If the extent is entirely clipped, the area is 0.0
    ///
    /// 3. The extent is clipped against the near clip plane, possibly
    ///    generating new vertices.
    ///
    /// 4. The convex hull of the remaining vertices is found.
    ///
    /// 5. The area of the convex hull polygon is computed and divided by the
    ///    area of the screen to calculate the fraction of the view window that
    ///    the extent could cover.
    TfToken projectionMethod = UsdLodTokens->projectedSphere;

    /// The screen size thresholds for LOD transitions in descending order as a
    /// fraction of the viewport size.
    ///
    /// For example, the values {0.25, 0.10, 0.025} would mean:
    ///   - Use LOD 0 when the bounds cover more than 25% of the screen
    ///   - Use LOD 1 when the bounds cover 25% or less but more than 10%
    ///   - Use LOD 2 when the bounds cover 10% or less but more than 2.5%
    ///   - Use LOD 3 when the bounds cover 2.5% or less of the screen
    ///
    /// \see UsdLodScreenSizeHeuristicQuery::blendThresholds
    VtFloatArray thresholds;

    /// This defines screen size thresholds for LOD transitions in descending
    /// order. The blend threshold is consulted for transitions that blend or
    /// combine multiple LOD items.
    ///
    /// For example if:
    ///   - thresholds is {0.25, 0.10, 0.025}
    ///   - blendThresholds is {0.20, 0.08, 0.020}
    ///
    /// then:
    ///   - Use LOD 0 when size > 25%  (from thresholds)
    ///   - Blend LOD 0 and LOD 1 when 25% >= size > 20% (min to max)
    ///   - Use LOD 1 when 20% >= size > 10% (max to min)
    ///   - Blend LOD 1 and LOD 2 when 10% >= size > 8%
    ///   - Use LOD 2 when 8% >= size > 2.5%
    ///   - Blend LOD 2 and LOD 3 when 2.5 >= size > 2%
    ///   - Use LOD 3 when 2% >= size
    ///
    /// The blend amount should be computed linearly between the corresponding
    /// `thresholds` and `blendThresholds` values.
    ///
    /// This field is advisory and not strongly interoperable.
    ///
    /// \see UsdLodScreenSizeHeuristicQuery::thresholds and
    /// UsdLodScreenSizeHeuristicQuery::ComputeLOD
    VtFloatArray blendThresholds;

public:
    UsdLodScreenSizeHeuristicQuery() = default;
    UsdLodScreenSizeHeuristicQuery(const UsdLodScreenSizeHeuristicQuery&) = default;
    UsdLodScreenSizeHeuristicQuery(UsdLodScreenSizeHeuristicQuery&&) = default;

    UsdLodScreenSizeHeuristicQuery(
        const TfToken& domainIn,
        const GfRange3d extentIn = GfRange3d(),
        const TfToken& projectionMethodIn = UsdLodTokens->projectedSphere,
        const VtFloatArray& thresholdsIn = VtFloatArray(),
        const VtFloatArray& blendThresholdsIn = VtFloatArray())
    : UsdLodHeuristicQuery(domainIn)
    , extent(extentIn)
    , projectionMethod(projectionMethodIn)
    , thresholds(thresholdsIn)
    , blendThresholds(blendThresholdsIn)
    { }

    /// Destructor. The destructor is virtual to support RTTI and allow the
    /// object to be safely held as a pointer to UsdLodHeuristicQuery.
    USDLOD_API
    virtual ~UsdLodScreenSizeHeuristicQuery();


    /// Calculate the screen size given a `frustum` and a `transform`.
    ///
    /// The matrix `transform` should convert from the root's local coordinate
    /// system to the viewpoint's coordinate system. Typically, the viewpoint
    /// will be specified in world coordinates and `transform` will convert
    /// from the root's coordinate system to world coordinates.
    ///
    /// The returned screen size is measured as the projection of the extent (or
    /// its bounding sphere) onto the viewplane and converted to a fraction of
    /// the visible window. The projected shape is not clipped by the viewing
    /// frustum as we do not want the computed size of an extent to change
    /// simply because it is moving off the side of the screen.
    ///
    /// If the extent crosses the near plane, then the calculated size will be
    /// 1.0 (i.e., 100%).  If the extent is entirely in front of the near plane
    /// or entirely behind the far plane (would be clipped by near or far plane
    /// clipping) then the calculated size will be 0.0.
    USDLOD_API
    double ComputeScreenSize(const GfFrustum& frustum,
                             const GfMatrix4d& transform) const;

    /// \overload
    /// Calculate the screen size applying hysteresis to the result.
    ///
    /// The arguments `prevSize` and `hysteresis` should contain the most
    /// recently calculated screen size and a hysteresis threshold respectively.
    /// If the computed screen size differs from `prevSize` by less than
    /// `hysteresis`, then `prevSize` is returned, otherwise the computed
    /// screen size is returned. This helps prevent flickering between LOD
    /// levels when the computed size is very close to a threshold value.
    double ComputeScreenSize(const GfFrustum& frustum,
                             const GfMatrix4d& transform,
                             double prevSize,
                             double hysteresis) const
    {
        double screenSize = ComputeScreenSize(frustum, transform);

        if (prevSize < screenSize - hysteresis) {
            // screenSize is increasing
            prevSize = screenSize - hysteresis;
        } else if (prevSize > screenSize + hysteresis) {
            // screenSize is decreasing
            prevSize = screenSize + hysteresis;
        } else {
            // prevSize is still within the hysteresis window. No change.
        }

        return prevSize;
    }

    /// Calculate an LOD index given a screen size.
    ///
    /// Compare the size against the values in `thresholds` to determine an
    /// LOD index. If `blendThresholds` has values and `size` is between a
    /// `thresholds` value and its corresponding `blendThresholds` value,
    /// then the returned value will be non-integral. If the renderer can blend
    /// between 2 LOD item children then it should use the returned index as
    /// follows:
    ///
    /// \code
    ///    float index = heuristic.ComputeLOD(size);
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
    ///
    /// If `blendThresholds` is not set then only integral valued LOD indices
    /// will ever be returned.
    USDLOD_API
    float ComputeLOD(double size) const;

    /// \overload
    /// Calculate an LOD index given a `frustum` and a `transform`.
    ///
    /// See \ref ComputeScreenSize
    float ComputeLOD(const GfFrustum& frustum,
                     const GfMatrix4d& transform) const
    {
        return ComputeLOD(ComputeScreenSize(frustum, transform));
    }

    /// \overload
    /// Calculate an LOD index given a `frustum` and a `transform` with
    /// hysteresis.
    ///
    /// The arguments `prevSize` and `hysteresis` should contain the most
    /// recently calculated screen size and a hysteresis threshold respectively.
    /// If the computed screen size differs from `prevSize` by less than
    /// `hysteresis`, then `prevSize` is used, otherwise the computed
    /// screen size is used. This helps prevent flickering between LOD
    /// levels when the computed size is very close to a threshold value.
    /// The size used (either `prevSize` or the newly computed size) will be
    /// returned in `sizeOut` so it can be used in next call to
    /// ComputeLOD.
    float ComputeLOD(const GfFrustum& frustum,
                     const GfMatrix4d& transform,
                     double prevSize,
                     double hysteresis,
                     double* sizeOut) const
    {
        return ComputeLOD(
            (*sizeOut = ComputeScreenSize(frustum, transform,
                                          prevSize, hysteresis)));
    }
};

USDLOD_API
std::ostream& operator<<(std::ostream&, const UsdLodScreenSizeHeuristicQuery&);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
