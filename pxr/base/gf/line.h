//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_GF_LINE_H
#define PXR_BASE_GF_LINE_H

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
    GF_API
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
    GF_API
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
GF_API
bool GfFindClosestPoints(const GfLine &l1, const GfLine &l2,
                         GfVec3d *p1 = nullptr, GfVec3d *p2 = nullptr,
                         double *t1 = nullptr, double *t2 = nullptr);

/// Output a GfLine.
/// \ingroup group_gf_DebuggingOutput
GF_API std::ostream &operator<<(std::ostream&, const GfLine&);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_GF_LINE_H
