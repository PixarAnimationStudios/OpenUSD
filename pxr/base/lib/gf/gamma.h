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
#ifndef GF_GAMMA_H
#define GF_GAMMA_H

class GfVec3f;
class GfVec3d;
class GfVec4f;
class GfVec4d;

/*!
 * \file gf/gamma.h
 * \brief Utilities to map colors between gamma spaces.
 */

/// Return a new vector with each component of \p v
/// raised to the power \p gamma
GfVec3f GfApplyGamma(const GfVec3f &v, double gamma);

/// Return a new vector with each component of \p v
/// raised to the power \p gamma
GfVec3d GfApplyGamma(const GfVec3d &v, double gamma);

/// Return a new vector with the first three components of \p v
/// raised to the power \p gamma and the fourth component unchanged.
GfVec4f GfApplyGamma(const GfVec4f &v, double gamma);

/// Return a new vector with the first three components of \p v
/// raised to the power \p gamma and the fourth component unchanged.
GfVec4d GfApplyGamma(const GfVec4d &v, double gamma);

/// Return the system display gamma
double GfGetDisplayGamma();

/// Given a vec, \p v, representing an energy-linear RGB(A) color,
/// return a vec of the same type converted to the system's display gamma
/// This is instantiated for GfVec3f, GfVec4d, GfVec3d, and GfVec4d
template <class T>
T GfConvertLinearToDisplay(const T& v);

/// Given a vec, \p v, representing an RGB(A) color in the system's display
/// gamma space, return an energy-linear vec of the same type.
/// This is instantiated for GfVec3f, GfVec4d, GfVec3d, and GfVec4d
template <class T>
T GfConvertDisplayToLinear(const T& v);


#endif /* GF_GAMMA_H */
