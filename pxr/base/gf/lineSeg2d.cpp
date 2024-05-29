//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/gf/lineSeg2d.h"
#include "pxr/base/gf/math.h"

#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

// CODE_COVERAGE_OFF_GCOV_BUG
TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<GfLineSeg2d>();
}
// CODE_COVERAGE_ON_GCOV_BUG

GfVec2d
GfLineSeg2d::FindClosestPoint(const GfVec2d &point, double *t) const
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
GfFindClosestPoints( const GfLine2d &line, const GfLineSeg2d &seg,
		     GfVec2d *p1, GfVec2d *p2,
		     double *t1, double *t2 )
{
    GfVec2d cp1, cp2;
    double lt1, lt2;
    if ( !GfFindClosestPoints( line, seg._line,  &cp1, &cp2, &lt1, &lt2 ) )
	return false;

    lt2 = GfClamp( lt2 / seg._length, 0, 1 );
    cp2 = seg.GetPoint( lt2 );

    // If we clamp the line segment, change the rayPoint to be 
    // the closest point on the ray to the clamped point.
    if (lt2 <= 0 || lt2 >= 1){
        cp1 = line.FindClosestPoint(cp2, &lt1);
    }

    if ( p1 )
	*p1 = cp1;

    if ( p2 )
        *p2 = cp2;

    if ( t1 )
	*t1 = lt1;

    if ( t2 )
	*t2 = lt2;

    return true;
}


bool 
GfFindClosestPoints( const GfLineSeg2d &seg1, const GfLineSeg2d &seg2,
		     GfVec2d *p1, GfVec2d *p2,
		     double *t1, double *t2 )
{
    GfVec2d cp1, cp2;
    double lt1, lt2;
    if ( !GfFindClosestPoints( seg1._line, seg2._line,  
			       &cp1, &cp2, &lt1, &lt2 ) )
	return false;

    lt1 = GfClamp( lt1 / seg1._length, 0, 1 );
    
    lt2 = GfClamp( lt2 / seg2._length, 0, 1 );

    if ( p1 )
	*p1 = seg1.GetPoint( lt1 );

    if ( p2 )
	*p2 = seg2.GetPoint( lt2 );

    if ( t1 )
	*t1 = lt1;

    if ( t2 )
	*t2 = lt2;

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
