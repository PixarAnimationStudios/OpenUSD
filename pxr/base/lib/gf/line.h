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
#ifndef GF_LINE_H
#define GF_LINE_H

/// \file gf/line.h
/// \ingroup group_gf_BasicGeometry

#include "pxr/pxr.h"
#include "pxr/base/gf/vec3d.h"

#include <float.h>
#include <iosfwd>

PXR_NAMESPACE_OPEN_SCOPE

/// \class GfLine
/// \ingroup group_gf_BasicGeometry
///
/// Basic type: 3D line
///
/// This class represents a three-dimensional line in space.  Lines are
/// constructed from a point, \p p0, and a direction, dir.  The direction is
/// normalized in the constructor. 
///
/// The line is kept in a parametric represention, p = p0 + t * dir. 
///
class GfLine {

  public:

    /// The default constructor leaves line parameters undefined.
    GfLine() {
    }

    /// Construct a line from a point and a direction.
    GfLine(const GfVec3d &p0, const GfVec3d &dir ) {
        Set( p0, dir );
    }

    double Set(const GfVec3d &p0, const GfVec3d &dir ) {
        _p0 = p0;
        _dir = dir;
        return _dir.Normalize();
    }

    /// Return the point on the line at \p ( p0 + t * dir ).
    /// Remember dir has been normalized so t represents a unit distance.
    GfVec3d GetPoint( double t ) const { return _p0 + _dir * t; }

    /// Return the normalized direction of the line.
    const GfVec3d &GetDirection() const { return _dir; }

    /// Returns the point on the line that is closest to \p point. If \p t is
    /// not \c NULL, it will be set to the parametric distance along the line
    /// of the returned point.
    GfVec3d FindClosestPoint(const GfVec3d &point, double *t = NULL) const;

    /// Component-wise equality test. The starting points and directions,
    /// must match exactly for lines to be considered equal.
    bool		operator ==(const GfLine &l) const {
	return _p0 == l._p0 &&	_dir  == l._dir;
    }

    /// Component-wise inequality test. The starting points, and directions
    /// must match exactly for lines to be considered equal.
    bool		operator !=(const GfLine &r) const {
	return ! (*this == r);
    }

  private:
    friend bool GfFindClosestPoints( const GfLine &, const GfLine &,
                                     GfVec3d *, GfVec3d *,
                                     double *, double * );
    // Parametric description:
    //  l(t) = _p0 + t * _length * _dir;
    GfVec3d             _p0;
    GfVec3d             _dir;   
};

/// Computes the closets points between two lines.
///
/// The two points are returned in \p p1 and \p p2.  The parametric distance
/// of each point on the lines is returned in \p t1 and \p t2.
///
/// This returns \c false if the lines were close enough to parallel that no
/// points could be computed; in this case, the other return values are
/// undefined.
bool GfFindClosestPoints(const GfLine &l1, const GfLine &l2,
                         GfVec3d *p1 = nullptr, GfVec3d *p2 = nullptr,
                         double *t1 = nullptr, double *t2 = nullptr);

/// Output a GfLine.
/// \ingroup group_gf_DebuggingOutput
std::ostream &operator<<(std::ostream&, const GfLine&);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // GF_LINE_H
