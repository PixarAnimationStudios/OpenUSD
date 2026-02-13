//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TS_SEGMENT_H
#define PXR_BASE_TS_SEGMENT_H

#include "pxr/pxr.h"

#include "pxr/base/ts/api.h"
#include "pxr/base/ts/types.h"

#include "pxr/base/gf/vec2d.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Ts_SegmentInterp declares the type of interpolation that should be used
/// between the end-points of a segment.
//
/// It is a scoped enum so the use of the Ts_SegmentInterp:: prefix is required.
enum class Ts_SegmentInterp
{
    ValueBlock,                 //< This segment explicitly has no value
    Held,                       //< The value is always the starting value
    Linear,                     //< Interpolate linearly from start to end
    Bezier,                     //< The segment uses Bezier interpolation
    Hermite,                    //< The segment uses Hermite interpolation
    PreExtrap,                  //< Linear extrapolation from -infinity to
                                //< the end value with a fixed slope
    PostExtrap                  //< Linear extrapolation to +infinity from
                                //< the start value with a fixed slope
};

/// Ts_Segment represents one section of a spline. It generally contains the
/// "post" side values of one knot (time, value, tangent, and interpolation) and
/// the "pre" side of the next knot (preValue and tangent). There are special
/// case interpolation types for pre- and post-extrapolation and the curve
/// interpolation of \c TsKnot has been split into separate Bezier and Hermite
/// values.
///
/// The data is stored as 4 GfVec2d values that represent (time, value) points
/// that are the knot points and the tangent end points. Note that the tangents
/// are stored as their end points rather than as width and slope as they are in
/// the knots. Also note that held interpolation segments still store the value
/// of the post-side knot even though that value is not used for segment
/// interpolation. This allows the knots to be reconstructed from the segment
/// data.
///
/// If the interpolation is PreExtrap or PostExtrap then the starting or ending
/// point (respectively) contains (+/- infinity, slope) rather than (time,
/// value). The straight line is extrapolated to infinity from the other end
/// point of the segment with the given slope.
struct Ts_Segment
{
    GfVec2d p0 = GfVec2d(0);
    GfVec2d t0 = GfVec2d(0);
    GfVec2d t1 = GfVec2d(0);
    GfVec2d p1 = GfVec2d(0);
    Ts_SegmentInterp interp = Ts_SegmentInterp::ValueBlock;

    /// Set interp from a TsInterpMode and TsCurveType.
    void SetInterp(TsInterpMode interpMode, TsCurveType curveType);

    /// Add the (timeDelta, valueDelta) contained in the \c GfVec2d argument to
    /// the segment's time and value.
    Ts_Segment& operator +=(const GfVec2d& delta)
    {
        p0 += delta;
        t0 += delta;
        t1 += delta;
        p1 += delta;

        return *this;
    }

    /// Add the timeDelta argument to the segment's time
    Ts_Segment& operator +=(const double timeDelta)
    {
        p0[0] += timeDelta;
        t0[0] += timeDelta;
        t1[0] += timeDelta;
        p1[0] += timeDelta;

        return *this;
    }

    /// Subtract the (timeDelta, valueDelta) contained in the \c GfVec2d
    /// argument from the segment's time and value.
    Ts_Segment& operator -=(const GfVec2d& delta)
    {
        p0 -= delta;
        t0 -= delta;
        t1 -= delta;
        p1 -= delta;

        return *this;
    }

    /// Subtract the timeDelta argument from the segment's time
    Ts_Segment& operator -=(const double timeDelta)
    {
        p0[0] -= timeDelta;
        t0[0] -= timeDelta;
        t1[0] -= timeDelta;
        p1[0] -= timeDelta;

        return *this;
    }

    /// Addition operator.
    template <typename T>
    Ts_Segment operator +(const T& delta) const
    {
        Ts_Segment result = *this;
        result += delta;
        return result;
    }

    // Subtraction operator
    template <typename T>
    Ts_Segment operator -(const T& delta) const
    {
        Ts_Segment result = *this;
        result -= delta;
        return result;
    }

    // Unary negation operator - negates the times, not the values.
    Ts_Segment operator -() const
    {
        Ts_Segment result = *this;
        result.p0[0] = -result.p0[0];
        result.t0[0] = -result.t0[0];
        result.t1[0] = -result.t1[0];
        result.p1[0] = -result.p1[0];

        // Now swap the points back into the correct order (p0 always has the
        // smallest time value).
        using std::swap;
        swap(result.p0, result.p1);
        swap(result.t0, result.t1);

        return result;
    }

    /// Compare for equivalence
    bool operator ==(const Ts_Segment& rhs) const
    {
        return (p0 == rhs.p0 &&
                t0 == rhs.t0 &&
                t1 == rhs.t1 &&
                p1 == rhs.p1 &&
                interp == rhs.interp);
    }

    /// Not equivalent
    bool operator !=(const Ts_Segment& rhs) const
    {
        return !(*this == rhs);
    }

    bool IsClose(const Ts_Segment& other, const double tolerance) const
    {
        return (interp == other.interp &&
                GfIsClose(p0, other.p0, tolerance) &&
                GfIsClose(t0, other.t0, tolerance) &&
                GfIsClose(t1, other.t1, tolerance) &&
                GfIsClose(p1, other.p1, tolerance));
    }

    /// Compute dv/dt at the value u in the interval [0..1].
    /// Note: This currently only supports u == 0.0 or u == 1.0.
    TS_API
    double _ComputeDerivative(double u) const;
};

// For testing purposes.
TS_API
std::ostream& operator <<(std::ostream&, const Ts_SegmentInterp);
TS_API
std::ostream& operator <<(std::ostream&, const Ts_Segment&);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
