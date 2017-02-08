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
#include "pxr/base/gf/gamma.h"

#include "pxr/base/gf/math.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"

PXR_NAMESPACE_OPEN_SCOPE

// Display colors (such as colors for UI elements) are always gamma 2.2
// and aspects of interactive rendering such as OpenGL's sRGB texture
// format assume that space as well. So, gamma 2.2 is hard coded here
// as the display gamma. In the future if those assumptions change we may
// need to move this to a higher level and get the gamma from somewhere else.
static const double _DisplayGamma = 2.2;

double GfGetDisplayGamma() {
    return _DisplayGamma;
}

GfVec3f GfApplyGamma(const GfVec3f &v, double g) {
    return GfVec3f(pow(v[0],g),pow(v[1],g),pow(v[2],g));
}

GfVec3d GfApplyGamma(const GfVec3d &v, double g) {
    return GfVec3d(pow(v[0],g),pow(v[1],g),pow(v[2],g));
}

GfVec4f GfApplyGamma(const GfVec4f &v, double g) {
    return GfVec4f(pow(v[0],g),pow(v[1],g),pow(v[2],g),v[3]);
}

GfVec4d GfApplyGamma(const GfVec4d &v, double g) {
    return GfVec4d(pow(v[0],g),pow(v[1],g),pow(v[2],g),v[3]);
}

template <class T>
T GfConvertLinearToDisplay(const T& v) {
    return GfApplyGamma(v,1.0/_DisplayGamma);
}

template <class T>
T GfConvertDisplayToLinear(const T& v) {
    return GfApplyGamma(v,_DisplayGamma);
}

template GfVec3f GfConvertLinearToDisplay<GfVec3f>(const GfVec3f&);
template GfVec3d GfConvertLinearToDisplay<GfVec3d>(const GfVec3d&);
template GfVec4f GfConvertLinearToDisplay<GfVec4f>(const GfVec4f&);
template GfVec4d GfConvertLinearToDisplay<GfVec4d>(const GfVec4d&);
template GfVec3f GfConvertDisplayToLinear<GfVec3f>(const GfVec3f&);
template GfVec3d GfConvertDisplayToLinear<GfVec3d>(const GfVec3d&);
template GfVec4f GfConvertDisplayToLinear<GfVec4f>(const GfVec4f&);
template GfVec4d GfConvertDisplayToLinear<GfVec4d>(const GfVec4d&);

PXR_NAMESPACE_CLOSE_SCOPE
