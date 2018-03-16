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

#include "pxr/pxr.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/ostreamHelpers.h"
#include "pxr/base/gf/plane.h"
#include "pxr/base/gf/range3d.h"
#include "pxr/base/gf/vec4d.h"

#include "pxr/base/tf/type.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

// CODE_COVERAGE_OFF_GCOV_BUG
TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<GfPlane>();
}
// CODE_COVERAGE_ON_GCOV_BUG

void
GfPlane::Set(const GfVec3d &normal, const GfVec3d &point)
{
    _normal = normal.GetNormalized();
    _distance = GfDot(_normal, point);
}

void
GfPlane::Set(const GfVec3d &p0, const GfVec3d &p1, const GfVec3d &p2)
{
    _normal = GfCross(p1 - p0, p2 - p0).GetNormalized();
    _distance = GfDot(_normal, p0);
}

void
GfPlane::Set(const GfVec4d &eqn)
{
    for (size_t i = 0; i < 3; i++) {
        _normal[i] = eqn[i];
    }
    _distance = -eqn[3];

    const double l = _normal.Normalize();
    if (l != 0.0) {
        _distance /= l;
    }
}

GfVec4d
GfPlane::GetEquation() const
 {
     return GfVec4d(_normal[0], _normal[1], _normal[2], -_distance);
 }

GfPlane &
GfPlane::Transform(const GfMatrix4d &matrix) 
{
    // Compute the point on the plane along the normal from the origin.
    GfVec3d pointOnPlane = _distance * _normal;

    // Transform the plane normal by the adjoint of the matrix to get
    // the new normal.  The adjoint (inverse transpose) is used to
    // multiply normals so they are not scaled incorrectly.
    GfMatrix4d adjoint = matrix.GetInverse().GetTranspose();
    _normal = adjoint.TransformDir(_normal).GetNormalized();

    // Transform the point on the plane by the matrix.
    pointOnPlane = matrix.Transform(pointOnPlane);

    // The new distance is the projected distance of the vector to the
    // transformed point onto the (unit) transformed normal. This is
    // just a dot product.
    _distance = GfDot(pointOnPlane, _normal);

    return *this;
}

bool
GfPlane::IntersectsPositiveHalfSpace(const GfRange3d &box) const
{
    if (box.IsEmpty())
	return false;
    
    // Test each vertex of the box against the positive half
    // space. Since the box is aligned with the coordinate axes, we
    // can test for a quick accept/reject at each stage.

// This macro tests one corner using the given inequality operators.
#define _GF_CORNER_TEST(X, Y, Z, XOP, YOP, ZOP)                               \
    if (X + Y + Z >= _distance)                                               \
        return true;                                                          \
    else if (_normal[0] XOP 0.0 && _normal[1] YOP 0.0 && _normal[2] ZOP 0.0)  \
        return false

    // The sum of these values is GfDot(box.GetMin(), _normal)
    double xmin = _normal[0] * box.GetMin()[0];
    double ymin = _normal[1] * box.GetMin()[1];
    double zmin = _normal[2] * box.GetMin()[2];

    // We can do the all-min corner test right now.
    _GF_CORNER_TEST(xmin, ymin, zmin, <=, <=, <=);

    // The sum of these values is GfDot(box.GetMax(), _normal)
    double xmax = _normal[0] * box.GetMax()[0];
    double ymax = _normal[1] * box.GetMax()[1];
    double zmax = _normal[2] * box.GetMax()[2];

    // Do the other 7 corner tests.
    _GF_CORNER_TEST(xmax, ymax, zmax, >=, >=, >=);
    _GF_CORNER_TEST(xmin, ymin, zmax, <=, <=, >=);
    _GF_CORNER_TEST(xmin, ymax, zmin, <=, >=, <=);
    _GF_CORNER_TEST(xmin, ymax, zmax, <=, >=, >=);
    _GF_CORNER_TEST(xmax, ymin, zmin, >=, <=, <=);
    _GF_CORNER_TEST(xmax, ymin, zmax, >=, <=, >=);
    _GF_CORNER_TEST(xmax, ymax, zmin, >=, >=, <=);

    // CODE_COVERAGE_OFF - We should never get here, but just in case...
    return false;
    // CODE_COVERAGE_ON

#undef _GF_CORNER_TEST
}

std::ostream &
operator<<(std::ostream& out, const GfPlane& p)
{
    return out
        << '[' << Gf_OstreamHelperP(p.GetNormal()) << " " 
        << Gf_OstreamHelperP(p.GetDistanceFromOrigin()) << ']';
}

PXR_NAMESPACE_CLOSE_SCOPE
