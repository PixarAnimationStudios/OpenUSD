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
#ifndef GF_PLANE_H
#define GF_PLANE_H

/// \file gf/plane.h
/// \ingroup group_gf_BasicGeometry

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/api.h"

#include <iosfwd>

class GfRange3d;
class GfMatrix4d;

/// \class GfPlane
/// \ingroup group_gf_BasicGeometry
///
/// Basic type: 3-dimensional plane
///
/// This class represents a three-dimensional plane as a normal vector
/// and the distance of the plane from the origin, measured along the
/// normal. The plane can also be used to represent a half-space: the
/// side of the plane in the direction of the normal.
///
class GfPlane
{
  public:

    /// The default constructor leaves the plane parameters undefined.
    GfPlane() {
    }

    /// This constructor sets this to the plane perpendicular to \p normal and
    /// at \p distance units from the origin. The passed-in normal is
    /// normalized to unit length first.
    GfPlane(const GfVec3d &normal, double distanceToOrigin) {
        Set(normal, distanceToOrigin);
    }

    /// This constructor sets this to the plane perpendicular to \p normal and
    /// that passes through \p point. The passed-in normal is normalized to
    /// unit length first.
    GfPlane(const GfVec3d &normal, const GfVec3d &point) {
        Set(normal, point);
    }

    /// This constructor sets this to the plane that contains the three given
    /// points. The normal is constructed from the cross product of (\p p1 -
    /// \p p0) (\p p2 - \p p0). Results are undefined if the points are
    /// collinear.
    GfPlane(const GfVec3d &p0, const GfVec3d &p1, const GfVec3d &p2) {
        Set(p0, p1, p2);
    }

    /// Sets this to the plane perpendicular to \p normal and at \p distance
    /// units from the origin. The passed-in normal is normalized to unit
    /// length first.
    void                Set(const GfVec3d &normal, double distanceToOrigin) {
        _normal = normal.GetNormalized();
        _distance = distanceToOrigin;
    }

    /// This constructor sets this to the plane perpendicular to \p normal and
    /// that passes through \p point. The passed-in normal is normalized to
    /// unit length first.
    GF_API
    void                Set(const GfVec3d &normal, const GfVec3d &point);

    /// This constructor sets this to the plane that contains the three given
    /// points. The normal is constructed from the cross product of (\p p1 -
    /// \p p0) (\p p2 - \p p0). Results are undefined if the points are
    /// collinear.
    GF_API
    void                Set(const GfVec3d &p0,
                            const GfVec3d &p1,
                            const GfVec3d &p2);

    /// Returns the unit-length normal vector of the plane.
    const GfVec3d &     GetNormal() const {
        return _normal;
    }

    /// Returns the distance of the plane from the origin.
    double              GetDistanceFromOrigin() const {
        return _distance;
    }

    /// Component-wise equality test. The normals and distances must match
    /// exactly for planes to be considered equal.
    bool		operator ==(const GfPlane &p) const {
	return (_normal   == p._normal &&
		_distance == p._distance);
    }

    /// Component-wise inequality test. The normals and distances must match
    /// exactly for planes to be considered equal.
    bool		operator !=(const GfPlane &p) const {
	return ! (*this == p);
    }

    /// Returns the distance of point \p from the plane. This distance will be
    /// positive if the point is on the side of the plane containing the
    /// normal.
    double              GetDistance(const GfVec3d &p) const {
        return p * _normal - _distance;
    }

    /// Return the projection of \p p onto the plane.
    GfVec3d             Project(const GfVec3d& p) const {
        return p - GetDistance(p) * _normal;
    }

    /// Transforms the plane by the given matrix.
    GF_API
    GfPlane &           Transform(const GfMatrix4d &matrix);

    /// Flip the plane normal (if necessary) so that \p p is in the positive
    /// halfspace.
    void                Reorient(const GfVec3d& p) {
        if (GetDistance(p) < 0) {
            _normal = -_normal;
            _distance = -_distance;
        }
    }

    /// Returns \c true if the given aligned bounding box is at least
    /// partially on the positive side (the one the normal points into) of the
    /// plane.
    GF_API
    bool IntersectsPositiveHalfSpace(const GfRange3d &box) const;

    /// Returns true if the given point is on the plane or within its positive
    /// half space.
    bool IntersectsPositiveHalfSpace(const GfVec3d &pt) const {
        return GetDistance(pt) >= 0.0;
    }

  private:
    /// The normal to the plane. Points in direction of half-space.
    GfVec3d             _normal;

    /// Distance from the plane to the origin.
    double              _distance;
};

/// Output a GfPlane using the format [(nx ny nz) distance].
/// \ingroup group_gf_DebuggingOutput
GF_API std::ostream& operator<<(std::ostream&, const GfPlane&);

#endif // GF_PLANE_H
