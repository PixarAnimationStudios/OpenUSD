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

std::ostream &
operator<<(std::ostream& out, const GfPlane& p)
{
    return out
        << '[' << Gf_OstreamHelperP(p.GetNormal()) << " " 
        << Gf_OstreamHelperP(p.GetDistanceFromOrigin()) << ']';
}

PXR_NAMESPACE_CLOSE_SCOPE
