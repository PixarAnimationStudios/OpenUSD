//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/gf/lineSeg.h"
#include "pxr/base/gf/math.h"
#include "pxr/base/gf/ostreamHelpers.h"

#include "pxr/base/tf/type.h"

#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE

// CODE_COVERAGE_OFF_GCOV_BUG
TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<GfLineSeg>();
}
// CODE_COVERAGE_ON_GCOV_BUG

GfVec3d
GfLineSeg::FindClosestPoint(const GfVec3d &point, double *t) const
{
    // Find the parametric distance, lt, of the closest point on the line
    // and then clamp lt to be on the line segment.

    double lt;
    if ( _length == 0.0 )
    {
        lt = 0.0;
    }
    else
    {
        _line.FindClosestPoint( point, &lt );

        lt = GfClamp( lt / _length, 0, 1 );
    }

    if ( t )
	*t = lt;

    return GetPoint( lt );
}

bool
GfFindClosestPoints( const GfLine &line, const GfLineSeg &seg,
		     GfVec3d *p1, GfVec3d *p2,
		     double *t1, double *t2 )
{
    GfVec3d cp1, cp2;
    double lt1, lt2;
    if ( !GfFindClosestPoints( line, seg._line,  &cp1, &cp2, &lt1, &lt2 ) ) {
	return false;
    }

    lt2 = GfClamp( lt2 / seg._length, 0, 1 );
    cp2 = seg.GetPoint( lt2 );

    // If we clamp the line segment, change the line point to be
    // the closest point on the line to the clamped point.
    if (lt2 <= 0 || lt2 >= 1) {
        cp1 = line.FindClosestPoint(cp2, &lt1);
    }

    if ( p1 ) {
	*p1 = cp1;
    }

    if ( p2 ) {
        *p2 = cp2;
    }

    if ( t1 ) {
	*t1 = lt1;
    }

    if ( t2 ) {
	*t2 = lt2;
    }

    return true;
}


bool
GfFindClosestPoints( const GfLineSeg &seg1, const GfLineSeg &seg2,
		     GfVec3d *p1, GfVec3d *p2,
		     double *t1, double *t2 )
{
    GfVec3d cp1, cp2;
    double lt1, lt2;
    bool parallel = false;

    // If either segment is zero length, just use FindClosestPoint.
    if (seg1._length < GF_MIN_VECTOR_LENGTH) {
        lt1 = 0.0;
        cp1 = seg1.GetPoint(lt1);
        cp2 = seg2.FindClosestPoint(cp1, &lt2);

        goto return_result;
    } else if (seg2._length < GF_MIN_VECTOR_LENGTH) {
        lt2 = 0.0;
        cp2 = seg2.GetPoint(lt2);
        cp1 = seg1.FindClosestPoint(cp2, &lt1);

        goto return_result;
    }

    parallel = !GfFindClosestPoints(seg1._line, seg2._line,
                                    &cp1, &cp2, &lt1, &lt2);

    // lt1 and lt2 are uninitialized if parallel
    if (!parallel) {
        // Normalize the distance along the _lines into [0..1] along
        // the line segments.
        lt1 /= seg1._length;
        lt2 /= seg2._length;
    }

    if (parallel || (lt1 < 0 || lt1 > 1 || lt2 < 0 || lt2 > 1)) {
        // The closest points of the infinite lines are not both on the
        // segments, so the minimum lies on the boundary: at an endpoint of
        // one segment and its closest point on the other segment.
        //
        // Imagine seg1 and seg2 as follows:
        //
        // |----seg1----|               *
        // 0            1             --- 1
        //                            /
        //                           /
        //                         seg2
        //                         /
        //                        /
        //                      --- 0
        //
        // The parameter values of the intersection point (*) of the
        // two lines containing seg1 and seg2 is > 1 for both
        // segments, but simply clamping both values to 1.0 and using
        // the end points is wrong. The closest points in this example
        // are at the end of seg1 and somewhere in the interior of
        // seg2.
        //
        // Similarly, if the segments are parallel they may or may
        // not have unique closest points. For example,
        // 
        // |----seg1----|
        //                    |----seg2----|
        //
        // definitely have unique closest points but
        //
        // |----seg1----|
        //           |----seg2----|
        //
        // and
        //
        // |----seg1----|
        //   |--seg2--|
        //
        // have no unique closest points.
        //
        // Try all 4 end-points
        GfVec3d q1, q2;
        double u, distSq, bestDistSq;

        q1 = seg1.GetPoint(0.0);
        q2 = seg2.FindClosestPoint(q1, &u);
        if (parallel && u > 0.0 && u < 1.0) {
            // Segments overlap.
            return false;
        }
        bestDistSq = (q2 - q1).GetLengthSq();

        lt1 = 0.0;
        lt2 = u;

        q1 = seg1.GetPoint(1.0);
        q2 = seg2.FindClosestPoint(q1, &u);
        if (parallel && u > 0.0 && u < 1.0) {
            // Segments overlap.
            return false;
        }
        distSq = (q2 - q1).GetLengthSq();

        if (distSq < bestDistSq) {
            bestDistSq = distSq;
            lt1 = 1.0;
            lt2 = u;
        }

        q2 = seg2.GetPoint(0.0);
        q1 = seg1.FindClosestPoint(q2, &u);
        if (parallel && u > 0.0 && u < 1.0) {
            // Segments overlap.
            return false;
        }
        distSq = (q2 - q1).GetLengthSq();

        if (distSq < bestDistSq) {
            bestDistSq = distSq;
            lt2 = 0.0;
            lt1 = u;
        }

        q2 = seg2.GetPoint(1.0);
        q1 = seg1.FindClosestPoint(q2, &u);
        if (parallel && u > 0.0 && u < 1.0) {
            // Segments overlap.
            return false;
        }
        distSq = (q2 - q1).GetLengthSq();

        if (distSq < bestDistSq) {
            bestDistSq = distSq;
            lt2 = 1.0;
            lt1 = u;
        }

        if (parallel) {
            // We have parallel line segments. If we made it through all 4
            // teests above then we know that none of the segment end points is
            // closest to an interior point on the other segment. This means
            // that both lt1 and lt2 must be either exactly 0.0 or exactly 1.0.
            // But one special case remains.
            //
            // If the two segments line up exactly, then the closest point to
            // each end would be an end of the other segment. So the two
            // segments either overlap exactly or do not overlap at all. Check
            // the midpoint to verify which case we have.
            q1 = seg1.GetPoint(0.5);
            q2 = seg2.FindClosestPoint(q1, &u);
            if (u > 0.0 && u < 1.0) {
                // The segments overlap.
                return false;
            }
        }
    }

return_result: // label for use by by the zero length segment tests above.
    if ( p1 ) {
	*p1 = seg1.GetPoint( lt1 );
    }

    if ( p2 ) {
	*p2 = seg2.GetPoint( lt2 );
    }

    if ( t1 ) {
	*t1 = lt1;
    }

    if ( t2 ) {
	*t2 = lt2;
    }

    return true;
}

std::ostream &
operator<<(std::ostream &out, const GfLineSeg &seg)
{
    return out << '('
        << "point 1:" << Gf_OstreamHelperP(seg.GetPoint(0.0)) << ' '
        << "point 2:" << Gf_OstreamHelperP(seg.GetPoint(1.0)) << ')';
}

PXR_NAMESPACE_CLOSE_SCOPE
