//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/plugin/hdEmbree/pxrIES/pxrIES.h"

#include "pxr/base/gf/math.h"

#include <algorithm>


#define _USE_MATH_DEFINES
#include <cmath>

#if !defined(M_PI)
#define M_PI 3.14159265358979323846
#endif

namespace {

// -------------------------------------------------------------------------
// Constants
// -------------------------------------------------------------------------

template <typename T>
constexpr T _pi = static_cast<T>(M_PI);

constexpr float _hemisphereFudgeFactor = 0.1f;

// -------------------------------------------------------------------------
// Utility functions
// -------------------------------------------------------------------------


float
_linearstep(float x, float a, float b)
{
    if (x <= a) {
        return 0.0f;
    }

    if (x >= b) {
        return 1.0f;
    }

    return (x - a) / (b - a);
}

}  // anonymous namespace


PXR_NAMESPACE_OPEN_SCOPE

bool
PxrIESFile::load(const std::string &ies)  // non-virtual "override"
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
    if ((*v_angleMax - *v_angleMin)
            > (_pi<float> / 2.0f + _hemisphereFudgeFactor)) {
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
            float i0 = (intensity[h][v] + intensity[h][v + 1]) / 2.0f;
            float i1 =
                (intensity[h + 1][v] + intensity[h + 1][v + 1]) / 2.0f;
            float center_intensity = (i0 + i1) / 2.0f;
            // solid angle of the patch
            float dS = dh * dv * sinf(v_angles[v] + dv / 2.0f);
            _power += dS * center_intensity;
        }
    }

    // ...and divide by surface area of a unit sphere (or hemisphere)
    // (this result matches Karma & RIS)
    _power /= _pi<float> * (is_sphere ? 4.0f : 2.0f);
}

float
PxrIESFile::eval(float theta, float phi, float angleScale) const
{
    int hi = -1;
    int vi = -1;
    float dh = 0.0f;
    float dv = 0.0f;

    phi = GfMod(phi, 2.0f * _pi<float>);
    for (size_t i = 0; i < h_angles.size() - 1; ++i) {
        if (phi >= h_angles[i] && phi < h_angles[i + 1]) {
            hi = i;
            dh = _linearstep(phi, h_angles[i], h_angles[i + 1]);
            break;
        }
    }

    // This formula matches Renderman's behavior

    // Scale with origin at "top" (ie, 180 degress / pi), by a factor
    // of 1 / (1 + angleScale), offset so that angleScale = 0 yields the
    // identity function.
    const float profileScale = 1.0f + angleScale;
    theta = (theta - _pi<float>) / profileScale + _pi<float>;
    theta = GfClamp(theta, 0.0f, _pi<float>);

    if (theta < 0) {
        // vi = 0;
        // dv = 0;
        return 0.0f;
    } else if (theta >= _pi<float>) {
        vi = v_angles.size() - 2;
        dv = 1;
    } else {
        for (size_t i = 0; i < v_angles.size() - 1; ++i) {
            if (theta >= v_angles[i] && theta < v_angles[i + 1]) {
                vi = i;
                dv = _linearstep(theta, v_angles[i], v_angles[i + 1]);
                break;
            }
        }
    }

    if (hi == -1 || vi == -1) {
        // XXX: need to indicate error somehow here
        return 0.0f;
    }

    // XXX: This should be a cubic interpolation
    float i0 = GfLerp(dv, intensity[hi][vi], intensity[hi][vi + 1]);
    float i1 = GfLerp(dv, intensity[hi + 1][vi], intensity[hi + 1][vi + 1]);

    return GfLerp(dh, i0, i1);
}

PXR_NAMESPACE_CLOSE_SCOPE
