// pbrt is Copyright(c) 1998-2020 Matt Pharr, Wenzel Jakob, and Greg Humphreys.
// The pbrt source code is licensed under the Apache License, Version 2.0.
// SPDX: Apache-2.0

#ifndef PXR_IMAGING_PLUGIN_HD_EMBREE_PBRT_UTILS_H
#define PXR_IMAGING_PLUGIN_HD_EMBREE_PBRT_UTILS_H

#include "pxr/pxr.h"

#include "pxr/base/arch/math.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace pxr_pbrt {

template <typename T>
constexpr T pi = static_cast<T>(M_PI);

// Ported from PBRT
inline GfVec3f
SphericalDirection(float sinTheta, float cosTheta, float phi)
{
    return GfVec3f(GfClamp(sinTheta, -1.0f, 1.0f) * GfCos(phi),
                   GfClamp(sinTheta, -1.0f, 1.0f) * GfSin(phi),
                   GfClamp(cosTheta, -1.0f, 1.0f));
}

// Ported from PBRT
inline GfVec3f
SampleUniformCone(GfVec2f const& u, float angle)
{
    float cosAngle = GfCos(angle);
    float cosTheta = (1.0f - u[0]) + u[0] * cosAngle;
    float sinTheta = GfSqrt(GfMax(0.0f, 1.0f - cosTheta*cosTheta));
    float phi = u[1] * 2.0f * pi<float>;
    return SphericalDirection(sinTheta, cosTheta, phi);
}

// Ported from PBRT
inline float
InvUniformConePDF(float angle)
{
    return 2.0f * pi<float> * (1.0f - GfCos(angle));
}

} // namespace pxr_pbrt

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_HD_EMBREE_PBRT_UTILS_H