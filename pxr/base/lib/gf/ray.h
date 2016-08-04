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
#ifndef GF_RAY_H
#define GF_RAY_H

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/api.h"

#include <float.h>
#include <limits>
#include <iosfwd>

//!
// \file ray.h
// \ingroup group_gf_BasicGeometry
//


class GfLine;
class GfLineSeg;
class GfPlane;
class GfRange3d;

//!
// \class GfRay ray.h "pxr/base/gf/ray.h"
// \ingroup group_gf_BasicGeometry
// \brief Basic type: Ray used for intersection testing
//
// This class represents a three-dimensional ray in space, typically
// used for intersection testing. It consists of an origin and a
// direction.
//
// Note that by default a \c GfRay does not normalize its direction
// vector to unit length.
//
// Note for ray intersections, the start point is included in the computations, 
// i.e., a distance of zero is defined to be intersecting.
//

class GfRay {

  public:

    //!
    // The default constructor leaves the ray parameters undefined.
    GfRay() {
    }

    //!
    // This constructor takes a starting point and a direction.
    GfRay(const GfVec3d &startPoint, const GfVec3d &direction) {
        SetPointAndDirection(startPoint, direction);
    }

    //!
    // Sets the ray by specifying a starting point and a direction.
    GF_API
    void        SetPointAndDirection(const GfVec3d &startPoint,
                                     const GfVec3d &direction);

    //!
    // Sets the ray by specifying a starting point and an ending point.
    GF_API
    void        SetEnds(const GfVec3d &startPoint, const GfVec3d &endPoint);

    //!
    // Returns the starting point of the segment.
    const GfVec3d &     GetStartPoint() const {
        return _startPoint;
    }

    //!
    // Returns the direction vector of the segment. This is not
    // guaranteed to be unit length.
    const GfVec3d &     GetDirection() const {
        return _direction;
    }

    //!
    // Returns the point that is \p distance units from the starting
    // point along the direction vector, expressed in parametic
    // distance.
    GfVec3d             GetPoint(double distance) const {
        return _startPoint + distance * _direction;
    }

    //!
    // Transforms the ray by the given matrix.
    GF_API
    GfRay &     Transform(const GfMatrix4d &matrix);

    //!
    // Returns the point on the ray that is closest to \p point. If \p
    // rayDistance is not \c NULL, it will be set to the parametric
    // distance along the ray of the closest point.
    GF_API
    GfVec3d             FindClosestPoint(const GfVec3d &point,
                                         double *rayDistance = NULL) const;

    //!
    // Component-wise equality test. The starting points, directions,
    // and lengths must match exactly for rays to be considered
    // equal.
    bool		operator ==(const GfRay &r) const {
	return (_startPoint == r._startPoint &&
		_direction  == r._direction);
    }

    //!
    // Component-wise inequality test. The starting points,
    // directions, and lengths must match exactly for rays to be
    // considered equal.
    bool		operator !=(const GfRay &r) const {
	return ! (*this == r);
    }

    //!
    // \name Intersection methods.
    // The methods in this group intersect the ray with a geometric
    // entity.
    //@{

    //!
    // Intersects the ray with the triangle formed by points \p p0, \p
    // p1, and \p p2, returning \c true if it hits. If there is an
    // intersection, it also returns the parametric distance to the
    // intersection point in \p distance, the barycentric coordinates
    // of the intersection point in \p barycentricCoords and the
    // front-facing flag in \p frontFacing. The barycentric
    // coordinates are defined with respect to the three vertices
    // taken in order.  The front-facing flag is \c true if the
    // intersection hit the side of the triangle that is formed when
    // the vertices are ordered counter-clockwise (right-hand rule).
    // If any of the return pointers are \c NULL, the corresponding
    // values are not returned.
    //
    // If the distance to the intersection is greater than \p maxDist,
    // then the method will return false.
    //
    // Barycentric coordinates are defined to sum to 1 and satisfy
    // this relationsip:
    // \code
    //     intersectionPoint = (barycentricCoords[0] * p0 +
    //                          barycentricCoords[1] * p1 +
    //                          barycentricCoords[2] * p2);
    // \endcode
    GF_API
    bool    Intersect(const GfVec3d &p0,
                      const GfVec3d &p1,
                      const GfVec3d &p2,
                      double *distance = NULL,
                      GfVec3d *barycentricCoords = NULL,
                      bool *frontFacing = NULL,
                      double maxDist = std::numeric_limits<double>::infinity())
                      const;

    //!
    // Intersects the ray with a plane, returning \c true if the ray
    // is not parallel to the plane and the intersection is within the
    // ray bounds. If there is an intersection, it also returns the
    // parametric distance to the intersection point in \p distance
    // and the front-facing flag in \p frontFacing, if they are not \c
    // NULL. The front-facing flag is \c true if the intersection is
    // on the side of the plane in which its normal points.
    GF_API
    bool	Intersect(const GfPlane &plane, double *distance = NULL,
			  bool *frontFacing = NULL) const;

    //!
    // Intersects the ray with an axis-aligned box, returning \c true
    // if the ray intersects it at all within bounds. If there is an
    // intersection, this also returns the parametric distances to the
    // two intersection points in \p enterDistance and 
    // \p exitDistance.
    GF_API
    bool        Intersect(const GfRange3d &box,
                          double *enterDistance = NULL,
                          double *exitDistance = NULL) const;

    //!
    // Intersects the ray with a sphere, returning \c true if the ray 
    // intersects it at all within bounds.  If there is an intersection, returns 
    // the parametric distance to the two intersection points in 
    // \p enterDistance and \p exitDistance.
    GF_API
    bool        Intersect(const GfVec3d &center, double radius,
                          double *enterDistance = NULL,
                          double *exitDistance = NULL ) const;
    
    //!
    // Intersects the ray with an infinite cylinder, with axis \p axis,
    // centered at the \p origin, with radius \p radius.
    //
    // Returns \c true if the ray intersects it at all within bounds. If there 
    // is an intersection, returns the parametric distance to the two 
    // intersection points in \p enterDistance and \p exitDistance.
    //
    // Note this method does not validate whether the radius is valid.
    GF_API
    bool Intersect(const GfVec3d &origin,
                   const GfVec3d &axis,
                   const double  radius,
                   double        *enterDistance = NULL,
                   double        *exitDistance = NULL) const;
    
    //!
    // Intersects the ray with an infinite non-double cone, centered at \p origin, 
    // with axis \p axis, radius \p radius and apex at \p height.
    //
    // Returns \c true if the ray intersects it at all within bounds. If there 
    // is an intersection, returns the parametric distance to the two 
    // intersection points in \p enterDistance and \p exitDistance.
    //
    // Note this method does not validate whether the radius are height are 
    // valid.
    GF_API
    bool Intersect(const GfVec3d &origin,
                   const GfVec3d &axis,
                   const double  radius,
                   const double  height,
                   double        *enterDistance = NULL,
                   double        *exitDistance = NULL) const;
    //@}

  private:
    GF_API
    friend bool GfFindClosestPoints( const GfRay &, const GfLine &,
                                     GfVec3d *, GfVec3d *,
                                     double *, double * );
    GF_API
    friend bool GfFindClosestPoints( const GfRay &, const GfLineSeg &,
                                     GfVec3d *, GfVec3d *,
                                     double *, double * );
  
    //!
    // Solves the quadratic equation returning the solutions, if defined, in 
    // \p enterDistance and \p exitDistance, where \p enterDistance is less than
    // or equal to \p exitDistance.
    bool _SolveQuadratic(const double a,
                         const double b,
                         const double c,
                         double       *enterDistance = NULL,
                         double       *exitDistance = NULL) const;
  
    //! The starting point of the ray.
    GfVec3d             _startPoint;
    //! The direction vector.
    GfVec3d             _direction;
};

//!
// Computes the closest points between a ray and a line. The two points
// are returned in \p rayPoint and \p linePoint.  The parametric
// distance of each point on the lines is returned in \p rayDistance and 
// \p lineDistance.
//
// This returns \c false if the lines were close enough to
// parallel that no points could be computed; in this case, the
// other return values are undefined.
GF_API
bool GfFindClosestPoints( const GfRay &ray, const GfLine &line,
                          GfVec3d *rayPoint = nullptr,
                          GfVec3d *linePoint = nullptr,
                          double *rayDistance = nullptr,
                          double *lineDistance = nullptr );

//!
// Computes the closest points between a ray and a line segment. 
// The two points are returned in \p rayPoint and \p segPoint.  
// The parametric distance of each point is returned in \p rayDistance 
// and \p segDistance.
//
// This returns \c false if the lines were close enough to
// parallel that no points could be computed; in this case, the
// other return values are undefined.
GF_API
bool GfFindClosestPoints( const GfRay &ray, const GfLineSeg &seg,
                          GfVec3d *rayPoint = nullptr,
                          GfVec3d *segPoint = nullptr,
                          double *rayDistance = nullptr,
                          double *segDistance = nullptr );

/// Output a GfRay using the format [(x y z) >> (x y z)].
/// \ingroup group_gf_DebuggingOutput
GF_API std::ostream& operator<<(std::ostream&, const GfRay&);


#endif // GF_RAY_H
