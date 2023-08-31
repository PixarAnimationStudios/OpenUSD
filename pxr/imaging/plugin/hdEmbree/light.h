//
// Copyright 2023 Pixar
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
#ifndef PXR_IMAGING_PLUGIN_HD_EMBREE_LIGHT_H
#define PXR_IMAGING_PLUGIN_HD_EMBREE_LIGHT_H

#include "ies.h"

#include <embree3/rtcore_common.h>
#include <embree3/rtcore_geometry.h>
#include <pxr/imaging/hd/light.h>

#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/matrix4f.h>
#include <limits>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

enum class LightKind: int
{
    Cylinder,
    Disk,
    Distant,
    Dome,
    Rect,
    Sphere
};

struct Cylinder
{
    float radius;
    float length;
};

struct Disk
{
    float radius;
};

struct Distant
{
    float halfAngleRadians;
};

struct LightTexture
{
    GfVec3f* pixels = nullptr;
    int width;
    int height;
};

struct Rect 
{
    float width;
    float height;
};

struct Sphere
{
    float radius;
};

struct IES
{
    IESFile iesFile;
    bool normalize;
    float angleScale;
};

struct Shaping
{
    GfVec3f focusTint;
    float focus;
    float coneAngle;
    float coneSoftness;
    IES ies;
};

struct Light 
{
    GfMatrix4f xformLightToWorld;
    GfMatrix4f xformWorldToLight;
    GfVec3f color;
    LightTexture texture;
    float intensity;
    float exposure;
    float colorTemperature;
    bool enableColorTemperature;
    LightKind kind;
    union
    {
        Cylinder cylinder;
        Disk disk;
        Distant distant;
        Rect rect;
        Sphere sphere;
    };
    bool normalize;
    bool visible;
    bool visible_camera;
    bool visible_shadow;
    Shaping shaping;
    unsigned rtcMeshId = RTC_INVALID_GEOMETRY_ID;
    RTCGeometry rtcGeometry;
};

constexpr unsigned InvalidLightId = std::numeric_limits<unsigned>::max();

class HdEmbreeLight final : public HdLight 
{
public:
    HdEmbreeLight(SdfPath const& id, TfToken const& lightType);
    ~HdEmbreeLight();

    /// Synchronizes state from the delegate to this object.
    void Sync(HdSceneDelegate* sceneDelegate, 
              HdRenderParam* renderParam,
              HdDirtyBits* dirtyBits) override;

    /// Returns the minimal set of dirty bits to place in the
    /// change tracker for use in the first sync of this prim.
    /// Typically this would be all dirty bits.
    HdDirtyBits GetInitialDirtyBitsMask() const override;

    void Finalize(HdRenderParam *renderParam) override;

private:
    TfToken _lightType;
    unsigned _lightId = InvalidLightId;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif