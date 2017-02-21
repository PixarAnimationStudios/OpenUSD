//
// Copyright 2016 Pixar
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
#ifndef GF_LINESEG2D_H
#define GF_LINESEG2D_H

/// \file gf/lineSeg2d.h
/// \ingroup group_gf_BasicGeometry

#include "pxr/pxr.h"
#include "pxr/base/gf/line2d.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/api.h"

#include <float.h>

PXR_NAMESPACE_OPEN_SCOPE

/// \class GfLineSeg2d
/// \ingroup group_gf_BasicGeometry
///
/// Basic type: 2D line segment
///
/// This class represents a three-dimensional line segment in space.
///
class GfLineSeg2d {

  public:

    /// The default constructor leaves line parameters undefined.
    GfLineSeg2d() {
    }

    /// Construct a line segment that spans two points.
    GfLineSeg2d(const GfVec2d &p0, const GfVec2d &p1 ) {
        _length = _line.Set( p0, p1 - p0 );
    }

    /// Return the point on the segment specified by the parameter t. 
    /// p = p0 + t * (p1 - p0)
    GfVec2d GetPoint( double t ) const {return _line.GetPoint( t * _length );}

    /// Return the normalized direction of the line.
    const GfVec2d &GetDirection() const { return _line.GetDirection(); }

    /// Return the length of the line
    double GetLength() const { return _length; }

    /// Returns the point on the line that is closest to \p point. If
    /// \p t is not \c NULL, it will be set to the parametric
    /// distance along the line of the closest point.
    GF_API
    GfVec2d FindClosestPoint(const GfVec2d &point, double *t = NULL) const;

    /// Component-wise equality test. The starting points and directions,
    /// must match exactly for lines to be considered equal.
    bool        operator ==(const GfLineSeg2d &l) const {
        return (_line == l._line && _length  == l._length);
    }

    /// Component-wise inequality test. The starting points,
    /// and directions must match exactly for lines to be
    /// considered equal.
    bool        operator !=(const GfLineSeg2d &r) const {
        return ! (*this == r);
    }

  private:
    GF_API
    friend bool GfFindClosestPoints( const GfLine2d &, const GfLineSeg2d &,
                                     GfVec2d *, GfVec2d *,
                                     double *t1, double *t2 );
    GF_API
    friend bool GfFindClosestPoints( const GfLineSeg2d &, const GfLineSeg2d &,
                                     GfVec2d *, GfVec2d *,
                                     double *, double * );

    GfLine2d            _line;
    double              _length;   // distance from p0 to p1
};

/// Computes the closets points on \p line and \p seg.
///
/// The two points are returned in \p p1 and \p p2. The parametric distances
/// of \p p1 and \p p2 along the line and segment are returned in \p t1 and \p
/// t2.
///
/// This returns \c false if the lines were close enough to parallel that no
/// points could be computed; in this case, the other return values are
/// undefined.
GF_API
bool GfFindClosestPoints( const GfLine2d &line, const GfLineSeg2d &seg,
                          GfVec2d *p1 = nullptr, GfVec2d *p2 = nullptr,
                          double *t1 = nullptr, double *t2 = nullptr );

/// Computes the closets points on two line segments, \p seg1 and \p seg2.
///
/// The two points are returned in \p p1 and \p p2. The parametric distances
/// of \p p1 and \p p2 along the segments are returned in \p t1 and \p t2.
///
/// This returns \c false if the lines were close enough to parallel that no
/// points could be computed; in this case, the other return values are
/// undefined.
GF_API
bool GfFindClosestPoints( const GfLineSeg2d &seg1, const GfLineSeg2d &seg2,
                          GfVec2d *p1 = nullptr, GfVec2d *p2 = nullptr,
                          double *t1 = nullptr, double *t2 = nullptr );

PXR_NAMESPACE_CLOSE_SCOPE

#endif // GF_LINESEG2D_H
