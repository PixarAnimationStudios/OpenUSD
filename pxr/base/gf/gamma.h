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
#ifndef PXR_BASE_GF_GAMMA_H
#define PXR_BASE_GF_GAMMA_H

#include "pxr/pxr.h"
#include "pxr/base/gf/api.h"

/// \file gf/gamma.h
/// Utilities to map colors between gamma spaces.

PXR_NAMESPACE_OPEN_SCOPE

class GfVec3f;
class GfVec3d;
class GfVec4f;
class GfVec4d;
class GfVec3h;
class GfVec4h;

/// Return a new vector with each component of \p v raised to the power \p
/// gamma
GF_API
GfVec3f GfApplyGamma(const GfVec3f &v, double gamma);

/// Return a new vector with each component of \p v raised to the power \p
/// gamma
GF_API
GfVec3d GfApplyGamma(const GfVec3d &v, double gamma);

/// \copydoc GfApplyGamma(GfVec3d,double)
GF_API
GfVec3h GfApplyGamma(const GfVec3h &v, double gamma);

/// Return a new vector with the first three components of \p v raised to the
/// power \p gamma and the fourth component unchanged.
GF_API
GfVec4f GfApplyGamma(const GfVec4f &v, double gamma);

/// Return a new vector with the first three components of \p v raised to the
/// power \p gamma and the fourth component unchanged.
GF_API
GfVec4d GfApplyGamma(const GfVec4d &v, double gamma);

/// \copydoc GfApplyGamma(GfVec4h,double)
GF_API
GfVec4h GfApplyGamma(const GfVec4h &v, double gamma);

/// Return a new float raised to the power \p gamma
GF_API
float GfApplyGamma(const float &v, double gamma);

/// Return a new char raised to the power \p gamma
GF_API
unsigned char GfApplyGamma(const unsigned char &v, double gamma);

/// Return the system display gamma
GF_API
double GfGetDisplayGamma();

/// Given a vec, \p v, representing an energy-linear RGB(A) color, return a
/// vec of the same type converted to the system's display gamma.
GF_API GfVec3f GfConvertLinearToDisplay(const GfVec3f &v);
GF_API GfVec3d GfConvertLinearToDisplay(const GfVec3d &v);
GF_API GfVec3h GfConvertLinearToDisplay(const GfVec3h &v);
GF_API GfVec4f GfConvertLinearToDisplay(const GfVec4f &v);
GF_API GfVec4d GfConvertLinearToDisplay(const GfVec4d &v);
GF_API GfVec4h GfConvertLinearToDisplay(const GfVec4h &v);
GF_API float GfConvertLinearToDisplay(const float &v);
GF_API unsigned char GfConvertLinearToDisplay(const unsigned char &v);

/// Given a vec, \p v, representing an RGB(A) color in the system's display
/// gamma space, return an energy-linear vec of the same type.
GF_API GfVec3f GfConvertDisplayToLinear(const GfVec3f &v);
GF_API GfVec3d GfConvertDisplayToLinear(const GfVec3d &v);
GF_API GfVec3h GfConvertDisplayToLinear(const GfVec3h &v);
GF_API GfVec4f GfConvertDisplayToLinear(const GfVec4f &v);
GF_API GfVec4d GfConvertDisplayToLinear(const GfVec4d &v);
GF_API GfVec4h GfConvertDisplayToLinear(const GfVec4h &v);
GF_API float GfConvertDisplayToLinear(const float &v);
GF_API unsigned char GfConvertDisplayToLinear(const unsigned char &v);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_GF_GAMMA_H 
