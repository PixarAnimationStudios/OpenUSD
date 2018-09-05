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
#include "pxr/base/gf/matrix2d.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/ostreamHelpers.h"
#include "pxr/base/gf/plane.h"
#include "pxr/base/gf/range3d.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec3d.h"
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
    // Transform the coefficients of the plane equation by the adjoint
    // of the matrix to get the new normal.  The adjoint (inverse
    // transpose) is also used to multiply so they are not scaled
    // incorrectly.
    const GfMatrix4d adjoint = matrix.GetInverse().GetTranspose();
    Set(GetEquation() * adjoint);

    return *this;
}

bool
GfPlane::IntersectsPositiveHalfSpace(const GfRange3d &box) const
{
    if (box.IsEmpty())
	return false;

    // The maximum of the inner product between the normal and any point in the
    // box.
    double d = 0.0;
    for (int i = 0; i < 3; i++) {
        // Add the contributions each component makes to the inner product
        // as the maximum of 
        // _normal[i] * box.GetMin()[i] and _normal[i] * box.GetMax()[i].
        // Depending on the sign of _normal[i], this will be the first or the
        // second term.
        const double b = (_normal[i] >= 0) ? box.GetMax()[i] : box.GetMin()[i];
        d += _normal[i] * b;
    }

    // If this inner product is larger than distance, we are in the positive
    // half space.
    return d >= _distance;
}

bool
GfFitPlaneToPoints(const std::vector<GfVec3d>& points, GfPlane* fitPlane)
{
    // Less than three points doesn't define a unique plane.
    if (points.size() < 3) {
        TF_CODING_ERROR("Need three points to correctly fit a plane");
        return false;
    }

    // We'll use the centroid of the points as the origin of our fit plane.
    GfVec3d sumOfPoints(0.0);
    for (const GfVec3d& p : points) {
        sumOfPoints += p;
    }
    const GfVec3d centroid = sumOfPoints / points.size();

    // The rest of this function uses linear least squares to fit the plane to
    // the equation ax + by + cz + d = 0, i.e., that used by GetEquation().
    // But as a first simplification, we'll consider all points relative to the
    // centroid, so that the plane passes through the origin. This gives us the
    // simplified equation ax + by + cz = 0. (We'll solve for the correct value
    // of d at the end.)
    // First compute the sums \sum (x_i)^2, \sum (x_i) (y_i), etc., over all the
    // points; these are used in the definition of the matrix equations.
    double xx = 0.0;
    double xy = 0.0;
    double xz = 0.0;
    double yy = 0.0;
    double yz = 0.0;
    double zz = 0.0;
    for (const GfVec3d& p : points) {
        const GfVec3d offset = p - centroid;
        xx += offset[0] * offset[0];
        xy += offset[0] * offset[1];
        xz += offset[0] * offset[2];
        yy += offset[1] * offset[1];
        yz += offset[1] * offset[2];
        zz += offset[2] * offset[2];
    }

    // If we try to solve using linear least squares now, it will give us the
    // trivial solution a = 0, b = 0, c = 0, which we'd like to avoid.
    // To prevent this, we'll force one of the coefficients to be nonzero by
    // breaking this into three possible cases:
    //   (1) a = 1, solve for b and c,
    //   (2) b = 1, solve for a and c,
    //   (3) c = 1, solve for a and b.
    //
    // Consider the first case, where a = 1 (the other cases are analogous).
    // The plane equation becomes x + by + cz = 0 or equivalently by + cz = -x.
    // For n points, we have a system of n equations by_i + cz_i = -x_i.
    // We can express that as a matrix equation AX = B, where:
    // A = {{y_1, z_1},
    //      {y_2, z_2},
    //         ...
    //      {y_n, z_n}}
    // X = {{b}, {c}}
    // B = {{-x_1}, {-x_2}, ..., {-x_m}}
    // and X contains the coefficients to the plane equation.
    // The estimate for X via linear least squares is (A^T A)^(-1) (A^T B).
    //
    // Case a = 1:
    // A^T A = {{\sum (y_i)^2,     \sum (y_i) (z_i)},
    //          {\sum (y_i) (z_i), \sum (z_i)^2    }}
    const GfMatrix2d ata1(yy, yz, yz, zz);
    // Case b = 1:
    // A = {{x_1, z_1}, {x_2, z_2}, ..., {x_n, z_n}}
    // A^T A = {{\sum (x_i)^2,     \sum (x_i) (z_i)},
    //          {\sum (x_i) (z_i), \sum (z_i)^2    }}
    const GfMatrix2d ata2(xx, xz, xz, zz);
    // Case c = 1:
    // A = {{x_1, y_1}, {x_2, y_2}, ..., {x_n, y_n}}
    // A^T A = {{\sum (x_i)^2,     \sum (x_i) (y_i)},
    //          {\sum (x_i) (y_i), \sum (y_i)^2    }}
    const GfMatrix2d ata3(xx, xy, xy, yy);

    // Since A^T A has to be invertible to estimate using least squares,
    // we won't go through all three cases; we just need a case where A^T A has
    // a nonzero determinant. We arbitrarily choose the case where the magnitude
    // of det(A^T A) is greatest.
    const double det1 = GfAbs(ata1.GetDeterminant());
    const double det2 = GfAbs(ata2.GetDeterminant());
    const double det3 = GfAbs(ata3.GetDeterminant());
    GfVec3d equation;
    if (det1 > 0.0 && det1 > det2 && det1 > det3) {
        // A^T B = {{\sum (y_i) (-x_i)},
        //          {\sum (z_i) (-x_i)}}
        // X = {{b}, {c}}
        const GfVec2d atb1(-xy, -xz);
        const GfVec2d leastSquaresEstimate = ata1.GetInverse() * atb1;
        equation[0] = 1.0;
        equation[1] = leastSquaresEstimate[0];
        equation[2] = leastSquaresEstimate[1];
    }
    else if (det2 > 0.0 && det2 > det3) {
        // A^T B = {{\sum (x_i) (-y_i)},
        //          {\sum (z_i) (-y_i)}}
        // X = {{a}, {c}}
        const GfVec2d atb2(-xy, -yz);
        const GfVec2d leastSquaresEstimate = ata2.GetInverse() * atb2;
        equation[0] = leastSquaresEstimate[0];
        equation[1] = 1.0;
        equation[2] = leastSquaresEstimate[1];
    }
    else if (det3 > 0.0) {
        // A^T B = {{\sum (x_i) (z_i)},
        //          {\sum (y_i) (z_i)}}
        // X = {{a}, {b}}
        const GfVec2d atb3(-xz, -yz);
        const GfVec2d leastSquaresEstimate = ata3.GetInverse() * atb3;
        equation[0] = leastSquaresEstimate[0];
        equation[1] = leastSquaresEstimate[1];
        equation[2] = 1.0;
    }
    else {
        // In all cases, det(A^T A) is zero. This happens when the points are
        // collinear and a plane can't be fitted, for example.
        return false;
    }

    // Our current plane is placed at the origin, so now move it to actually
    // intersect the centroid by solving for d.
    // (ax + by + cz + d = 0) => (d = -ax -by -cz)
    //                        => (d = -{a, b, c} . {x, y, z})
    const double d = -GfDot(equation, centroid);
    fitPlane->Set(GfVec4d(equation[0], equation[1], equation[2], d));
    return true;
}

std::ostream &
operator<<(std::ostream& out, const GfPlane& p)
{
    return out
        << '[' << Gf_OstreamHelperP(p.GetNormal()) << " " 
        << Gf_OstreamHelperP(p.GetDistanceFromOrigin()) << ']';
}

PXR_NAMESPACE_CLOSE_SCOPE
