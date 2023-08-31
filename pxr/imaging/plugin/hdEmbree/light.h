//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_PLUGIN_HD_EMBREE_LIGHT_H
#define PXR_IMAGING_PLUGIN_HD_EMBREE_LIGHT_H

#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/imaging/hd/light.h"

#include <embree3/rtcore_common.h>
#include <embree3/rtcore_geometry.h>

#include <limits>
#include <variant>

PXR_NAMESPACE_OPEN_SCOPE

class HdEmbreeRenderer;

struct HdEmbree_UnknownLight
{};
struct HdEmbree_Cylinder
{
    float radius;
    float length;
};

struct HdEmbree_Disk
{
    float radius;
};

struct HdEmbree_Rect
{
    float width;
    float height;
};

struct HdEmbree_Sphere
{
    float radius;
};

using HdEmbree_LightVariant = std::variant<
    HdEmbree_UnknownLight,
    HdEmbree_Cylinder,
    HdEmbree_Disk,
    HdEmbree_Rect,
    HdEmbree_Sphere>;

struct HdEmbree_Shaping
{
    GfVec3f focusTint;
    float focus = 0.0f;
    float coneAngle = 180.0f;
    float coneSoftness = 0.0f;
};

struct HdEmbree_LightData
{
    GfMatrix4f xformLightToWorld;
    GfMatrix3f normalXformLightToWorld;
    GfMatrix4f xformWorldToLight;
    GfVec3f color;
    float intensity = 1.0f;
    float exposure = 0.0f;
    float colorTemperature = 6500.0f;
    bool enableColorTemperature = false;
    HdEmbree_LightVariant lightVariant;
    bool normalize = false;
    bool visible = true;
    HdEmbree_Shaping shaping;
};

class HdEmbree_Light final : public HdLight
{
public:
    HdEmbree_Light(SdfPath const& id, TfToken const& lightType);
    ~HdEmbree_Light();

    /// Synchronizes state from the delegate to this object.
    void Sync(HdSceneDelegate* sceneDelegate,
              HdRenderParam* renderParam,
              HdDirtyBits* dirtyBits) override;

    /// Returns the minimal set of dirty bits to place in the
    /// change tracker for use in the first sync of this prim.
    /// Typically this would be all dirty bits.
    HdDirtyBits GetInitialDirtyBitsMask() const override;

    void Finalize(HdRenderParam *renderParam) override;

    HdEmbree_LightData const& LightData() const {
        return _lightData;
    }

private:
    HdEmbree_LightData _lightData;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif