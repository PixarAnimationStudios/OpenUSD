//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#ifndef PXR_IMAGING_PLUGIN_HD_EMBREE_LIGHT_SAMPLERS_H
#define PXR_IMAGING_PLUGIN_HD_EMBREE_LIGHT_SAMPLERS_H

#include "pxr/pxr.h"
#include "pxr/base/gf/vec3f.h"

#include "pxr/imaging/plugin/hdEmbree/light.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdEmbreeLightSampler
///
/// Utility class to help sample Embree lights for direct lighting.
class HdEmbreeLightSampler {
public:
    struct LightSample {
        GfVec3f Li;
        GfVec3f wI;
        float dist;
        float invPdfW;
    };

    static LightSample GetLightSample(
            HdEmbree_LightData const& lightData,
            GfVec3f const& hitPosition,
            GfVec3f const& normal,
            float u1,
            float u2);

    // callables to be used with std::visit
    LightSample operator()(HdEmbree_UnknownLight const& rect);
    LightSample operator()(HdEmbree_Rect const& rect);
    LightSample operator()(HdEmbree_Sphere const& sphere);
    LightSample operator()(HdEmbree_Disk const& disk);
    LightSample operator()(HdEmbree_Cylinder const& cylinder);

private:
    HdEmbreeLightSampler(HdEmbree_LightData const& lightData,
                         GfVec3f const& hitPosition,
                         GfVec3f const& normal,
                         float u1,
                         float u2) :
        _lightData(lightData),
        _hitPosition(hitPosition),
        _normal(normal),
        _u1(u1),
        _u2(u2)
    {}

    HdEmbree_LightData const& _lightData;
    GfVec3f const& _hitPosition;
    GfVec3f const& _normal;
    float _u1;
    float _u2;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_HD_EMBREE_LIGHT_SAMPLERS_H
