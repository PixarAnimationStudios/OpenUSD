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
#ifndef GF_HOMOGENEOUS_H
#define GF_HOMOGENEOUS_H

/// \file gf/homogeneous.h
/// \ingroup group_gf_LinearAlgebra
/// Utility functions for GfVec4f and GfVec4d as homogeneous vectors

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"

/// Returns a vector which is \p v homogenized.  If the fourth element of \p v
/// is 0, it is set to 1.
/// \ingroup group_gf_LinearAlgebra
GfVec4f GfGetHomogenized(const GfVec4f &v);

/// Homogenizes \p a and \p b and then performs the cross product on the first
/// three elements of each.  Returns the cross product as a homogenized
/// vector.
/// \ingroup group_gf_LinearAlgebra
GfVec4f GfHomogeneousCross(const GfVec4f &a, const GfVec4f &b);

GfVec4d GfGetHomogenized(const GfVec4d &v);

/// Homogenizes \p a and \p b and then performs the cross product on the first
/// three elements of each.  Returns the cross product as a homogenized
/// vector.
/// \ingroup group_gf_LinearAlgebra
GfVec4d GfHomogeneousCross(const GfVec4d &a, const GfVec4d &b);

/// Projects homogeneous \p v into Euclidean space and returns the result as a
/// Vec3f.
inline GfVec3f GfProject(const GfVec4f &v) {
    float inv = (v[3] != 0.0f) ? 1.0f/v[3] : 1.0f;
    return GfVec3f(inv * v[0], inv * v[1], inv * v[2]);
}

/// Projects homogeneous \p v into Euclidean space and returns the result as a
/// Vec3d.
inline GfVec3d GfProject(const GfVec4d &v) {
    double inv = (v[3] != 0.0) ? 1.0/v[3] : 1.0;
    return GfVec3d(inv * v[0], inv * v[1], inv * v[2]);
}

#endif /* GF_HOMOGENEOUS_H */
