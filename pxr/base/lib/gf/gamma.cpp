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

GfVec3h GfApplyGamma(const GfVec3h &v, double g) {
    // Explicitly cast half to float to avoid ambiguous call to pow(...)
    return GfVec3h(pow(static_cast<float>(v[0]),g),
                   pow(static_cast<float>(v[1]),g),
                   pow(static_cast<float>(v[2]),g));
}

GfVec4f GfApplyGamma(const GfVec4f &v, double g) {
    return GfVec4f(pow(v[0],g),pow(v[1],g),pow(v[2],g),v[3]);
}

GfVec4d GfApplyGamma(const GfVec4d &v, double g) {
    return GfVec4d(pow(v[0],g),pow(v[1],g),pow(v[2],g),v[3]);
}

GfVec4h GfApplyGamma(const GfVec4h &v, double g) {
    // Explicitly cast half to float to avoid ambiguous call to pow(...)
    return GfVec4h(pow(static_cast<float>(v[0]),g),
                   pow(static_cast<float>(v[1]),g),
                   pow(static_cast<float>(v[2]),g),
                   v[3]);
}

template <class T>
static T Gf_ConvertLinearToDisplay(const T& v) {
    return GfApplyGamma(v,1.0/_DisplayGamma);
}

template <class T>
static T Gf_ConvertDisplayToLinear(const T& v) {
    return GfApplyGamma(v,_DisplayGamma);
}

GfVec3f GfConvertLinearToDisplay(const GfVec3f &v) { return Gf_ConvertLinearToDisplay(v); }
GfVec3d GfConvertLinearToDisplay(const GfVec3d &v) { return Gf_ConvertLinearToDisplay(v); }
GfVec3h GfConvertLinearToDisplay(const GfVec3h &v) { return Gf_ConvertLinearToDisplay(v); }
GfVec4f GfConvertLinearToDisplay(const GfVec4f &v) { return Gf_ConvertLinearToDisplay(v); }
GfVec4d GfConvertLinearToDisplay(const GfVec4d &v) { return Gf_ConvertLinearToDisplay(v); }
GfVec4h GfConvertLinearToDisplay(const GfVec4h &v) { return Gf_ConvertLinearToDisplay(v); }
GfVec3f GfConvertDisplayToLinear(const GfVec3f &v) { return Gf_ConvertDisplayToLinear(v); }
GfVec3d GfConvertDisplayToLinear(const GfVec3d &v) { return Gf_ConvertDisplayToLinear(v); }
GfVec3h GfConvertDisplayToLinear(const GfVec3h &v) { return Gf_ConvertDisplayToLinear(v); }
GfVec4f GfConvertDisplayToLinear(const GfVec4f &v) { return Gf_ConvertDisplayToLinear(v); }
GfVec4d GfConvertDisplayToLinear(const GfVec4d &v) { return Gf_ConvertDisplayToLinear(v); }
GfVec4h GfConvertDisplayToLinear(const GfVec4h &v) { return Gf_ConvertDisplayToLinear(v); }

PXR_NAMESPACE_CLOSE_SCOPE
