//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/plugin/hdEmbree/pxrIES/pxrIES.h"

#include <algorithm>


#define _USE_MATH_DEFINES
#include <cmath>

#if !defined(M_PI)
#define M_PI 3.14159265358979323846
#endif


PXR_NAMESPACE_OPEN_SCOPE

bool
PxrIESFile::load(std::string const& ies)  // non-virtual "override"
{
    clear();
    if (!Base::load(ies)) {
        return false;
    }
    pxr_extra_process();
    return true;
}

void
PxrIESFile::clear()  // non-virtual "override"
{
    Base::clear();
    _power = 0;
}

void
PxrIESFile::pxr_extra_process()
{
    // find max v_angle delta, as a way to estimate whether the distribution
    // is over a hemisphere or sphere
    const auto [v_angleMin, v_angleMax] = std::minmax_element(
        v_angles.cbegin(), v_angles.cend());

    // does the distribution cover the whole sphere?
    bool is_sphere = false;
    if ((*v_angleMax - *v_angleMin) > (M_PI / 2 + 0.1 /* fudge factor*/)) {
        is_sphere = true;
    }

    _power = 0;

    // integrate the intensity over solid angle to get power
    for (size_t h = 0; h < h_angles.size() - 1; ++h) {
        for (size_t v = 0; v < v_angles.size() - 1; ++v) {
            // approximate dimensions of the patch
            float dh = h_angles[h + 1] - h_angles[h];
            float dv = v_angles[v + 1] - v_angles[v];
            // bilinearly interpolate intensity at the patch center
            float i0 = (intensity[h][v] + intensity[h][v + 1]) / 2;
            float i1 =
                (intensity[h + 1][v] + intensity[h + 1][v + 1]) / 2;
            float center_intensity = (i0 + i1) / 2;
            // solid angle of the patch
            float dS = dh * dv * sinf(v_angles[v] + dv / 2);
            _power += dS * center_intensity;
        }
    }

    // ...and divide by surface area of a unit sphere (or hemisphere)
    // (this result matches Karma & RIS)
    _power /= M_PI * (is_sphere ? 4 : 2);
}

PXR_NAMESPACE_CLOSE_SCOPE
