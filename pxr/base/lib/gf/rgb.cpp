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
#include "pxr/base/gf/rgb.h"
#include "pxr/base/gf/math.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/tf/type.h"

#include <iostream>

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<GfRGB>();
}

GfRGB GfRGB::Transform(const GfMatrix4d &m) const
{
    return GfRGB(m.TransformDir(_rgb));
}

GfRGB operator *(const GfRGB &c, const GfMatrix4d &m)
{
    return c.Transform(m);
}

bool
GfIsClose(const GfRGB &v1, const GfRGB &v2, double tolerance)
{
    return GfIsClose(v1._rgb, v2._rgb, tolerance);
}

void
GfRGB::GetHSV(float *h, float *s, float *v) const
{
    const GfRGB &rgb = *this;

    float max = GfMax(rgb[0], GfMax(rgb[1], rgb[2]));
    float min = GfMin(rgb[0], GfMin(rgb[1], rgb[2]));
    float diff = max - min;

    // The value is the maximum component.
    *v = max;

    // Saturation
    *s = (max != 0.0 ? diff / max : 0.0);

    // Hue
    if (*s != 0.0) {
        if (rgb[0] == max)
            *h = (rgb[1] - rgb[2]) / diff;
        else if (rgb[1] == max)
            *h = 2.0 + (rgb[2] - rgb[0]) / diff;
        else
            *h = 4.0 + (rgb[0] - rgb[1]) / diff;
        if (*h < 0.0)
            *h += 6.0;
        *h /= 6.0;
    }
    else
        *h = 0.0;
}

void
GfRGB::SetHSV(float h, float s, float v)
{
    float hue = (h == 1.0 ? 0.0 : 6.0 * h);
    int   hueSextant = int(floor(hue));
    float hueFrac = hue - hueSextant;

    float t1 = v * (1.0 - s);
    float t2 = v * (1.0 - (s * hueFrac));
    float t3 = v * (1.0 - (s * (1.0 - hueFrac)));

    switch (hueSextant) {
      case 0:
        Set(v, t3, t1);
        break;
      case 1:
        Set(t2, v, t1);
        break;
      case 2:
        Set(t1, v, t3);
        break;
      case 3:
        Set(t1, t2, v);
        break;
      case 4:
        Set(t3, t1, v);
        break;
      case 5:
        Set(v, t1, t2);
        break;
    }
}

// Offsets are kinda funky. Essentially each component of an offset is
// a scaling term which says how far a component should be changed and in
// what direction. An offset of 0.5, for example, moves its component 50%
// of the distance between the base value and its maximum value in the
// positive direction, whereas an offset of -0.1 moves its component 10% of 
// the distance between its base value and its minimum value in the negative
// direction.
//
GfRGB GfRGB::GetColorFromOffset(const GfRGB &offsetBase, 
                                const GfRGB &offsetHSV)
{
    GfRGB offsetBaseHSV, offsetColorHSV;

    // Convert base to HSV space
    offsetBase.GetHSV(&offsetBaseHSV[0], &offsetBaseHSV[1],
                      &offsetBaseHSV[2]);

    // Offset each component of base in HSV space
    for (int c = 0; c < 3; ++c) {
        // For sanity
        float baseC = GfClamp(offsetBaseHSV[c], 0, 1);
        float off = GfClamp(offsetHSV[c], -1, 1);

        offsetColorHSV[c] = baseC + off * (off > 0 ? 1 - baseC : baseC);
    }

    // Convert offsetColorHSV to RGB space
    GfRGB offsetColor;
    offsetColor.SetHSV(offsetColorHSV[0], offsetColorHSV[1], 
                       offsetColorHSV[2]);

    return offsetColor;
}

GfRGB GfRGB::GetOffsetFromColor(const GfRGB &offsetBase,
                                const GfRGB &offsetColor)
{
    GfRGB offsetHSV;

    // Convert offsetBase and offsetColor to HSV space
    GfRGB offsetBaseHSV, offsetColorHSV;

    offsetBase.GetHSV(&offsetBaseHSV[0], &offsetBaseHSV[1],
                      &offsetBaseHSV[2]);
    offsetColor.GetHSV(&offsetColorHSV[0], &offsetColorHSV[1],
                        &offsetColorHSV[2]);

    // Determine the offset for each component in HSV space
    for (int c = 0; c < 3; ++c) {
        // For sanity
        float baseC = GfClamp(offsetBaseHSV[c], 0, 1);
        float offC = GfClamp(offsetColorHSV[c], 0, 1);

        float delta = offC - baseC;

        if (delta > 0)
            // baseC must be < 1 for delta > 0
            offsetHSV[c] = delta / (1 - baseC);

        else if (delta < 0)
            // baseC must be > 0 for delta < 0
            offsetHSV[c] = delta / baseC;

        else // delta == 0
            offsetHSV[c] = 0;
    }

    return offsetHSV;
}

std::ostream &
operator<<(std::ostream& out, const GfRGB& c)
{
    return out << '(' << c[0] << ", " << c[1] << ", " << c[2] << ')';
}
