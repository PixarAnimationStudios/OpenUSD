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
#include "pxr/base/gf/rgba.h"

#include "pxr/base/gf/math.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/rgb.h"
#include "pxr/base/tf/type.h"

#include <iostream>

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<GfRGBA>();
}

GfRGBA GfRGBA::Transform(const GfMatrix4d &m) const
{
    return GfRGBA(_rgba * m);
}

GfRGBA operator *(const GfRGBA &c, const GfMatrix4d &m)
{
    return c.Transform(m);
}

bool
GfIsClose(const GfRGBA &v1, const GfRGBA &v2, double tolerance)
{
    return GfIsClose(v1._rgba, v2._rgba, tolerance);
}

void
GfRGBA::GetHSV(float *h, float *s, float *v) const
{
    return GfRGB(GetArray()).GetHSV(h, s, v);
}

void
GfRGBA::SetHSV(float h, float s, float v)
{
    GfRGB tmp;
    tmp.SetHSV(h, s, v);
    Set(tmp[0], tmp[1], tmp[2], _rgba[3]);
}


std::ostream &
operator<<(std::ostream& out, const GfRGBA& c)
{
    return out << '(' << c[0] << ", " << c[1] << ", "
            << c[2] << ", " << c[3] << ')';
}
