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

#include <embree4/rtcore_common.h>
#include <embree4/rtcore_geometry.h>

#include <limits>
#include <variant>

PXR_NAMESPACE_OPEN_SCOPE

/// Reference implementation of USD Lux support, for the hdEmbree renderer.
///
/// This is a reference implementation of USD Lux support, for the hdEmbree
/// renderer. It is not intented for use in production, but instead as useful
/// reference for understanding how to implement USD Lux support for other
/// renderers.
///
/// Supported Features:
/// - LightAPI:
///   - inputs:intensity
///   - inputs:exposure
///   - inputs:diffuse
///   - inputs:normalize
///   - inputs:color
///   - inputs:enableColorTemperature
///   - inputs:colorTemperature
/// - DiskLight
///   - inputs:radius
/// - RectLight
///   - inputs:width
///   - inputs:height
/// - SphereLight
///   - inputs:radius
/// - CylinderLight
///   - inputs:radius
///   - inputs:length
/// - ShapingAPI
///   - inputs:shaping:focus
///   - inputs:shaping:focusTint
///   - inputs:shaping:cone:angle
///   - inputs:shaping:cone:softness
/// - Respects double-sidedness of meshes
///
/// Currently Unsupported Features / Limitations:
/// - Surface shaders (all surfaces are assumed to be 100% reflective diffuse
///   BRDFs)
/// - Light shaders
/// - Unsupported light types:
///   - MeshLightAPI
///   - VolumeLightAPI
///   - DistantLight
///   - GeometryLight
///   - DomeLight
///   - PortalLight
///   - PluginLight
/// - Unsupported UsdLux APIS:
///   - LightListAPI
///   - ListAPI
///   - ShadowAPI (shadows are rendered, but are unaffected by this API)
///   - LightFilter
/// - Unsupported attributes on supported light types / APIs:
///   - LightAPI:
///     - collection:lightLink:includeRoot
///     - collection:shadowLink:includeRoot
///     - light:shaderId
///     - light:materialSyncMode
///     - inputs:specular
///     - light:filters
///   - RectLight
///     - inputs:texture:file
///   - SphereLight:
///     - treatAsPoint
///   - CylinderLight:
///     - inputs:treatAsLine
/// - ShapingAPI
///   - inputs:shaping:ies:file
///   - inputs:shaping:ies:angleScale
///   - inputs:shaping:ies:normalize
/// - No support for direct-camera visibility
/// - No support for motion blur (currently, if motion blur is enabled, all
///   samples taken at the first time sample, ie, when the shutter opens).
/// - No support for instanced lights

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
    float diffuse = 1.0f;
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
