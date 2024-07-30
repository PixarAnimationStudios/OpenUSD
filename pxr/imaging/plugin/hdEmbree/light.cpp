//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/plugin/hdEmbree/light.h"

#include "light.h"
#include "pxr/imaging/plugin/hdEmbree/debugCodes.h"
#include "pxr/imaging/plugin/hdEmbree/renderParam.h"
#include "pxr/imaging/plugin/hdEmbree/renderer.h"

#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hio/image.h"

#include <embree3/rtcore_buffer.h>
#include <embree3/rtcore_device.h>
#include <embree3/rtcore_scene.h>

#include <fstream>
#include <sstream>
#include <vector>

namespace {

PXR_NAMESPACE_USING_DIRECTIVE

HdEmbree_LightTexture
_LoadLightTexture(std::string const& path)
{
    if (path.empty()) {
        return HdEmbree_LightTexture();
    }

    HioImageSharedPtr img = HioImage::OpenForReading(path);
    if (!img) {
        return HdEmbree_LightTexture();
    }

    int width = img->GetWidth();
    int height = img->GetHeight();

    std::vector<GfVec3f> pixels(width * height * 3.0f);

    HioImage::StorageSpec storage;
    storage.width = width;
    storage.height = height;
    storage.depth = 1;
    storage.format = HioFormatFloat32Vec3;
    storage.data = &pixels.front();

    if (img->Read(storage)) {
        return {std::move(pixels), width, height};
    }
    TF_WARN("Could not read image %s", path.c_str());
    return { std::vector<GfVec3f>(), 0, 0 };
}


void
_SyncLightTexture(const SdfPath& id, HdEmbree_LightData& light, HdSceneDelegate *sceneDelegate)
{
    std::string path;
    if (VtValue textureValue = sceneDelegate->GetLightParamValue(
            id, HdLightTokens->textureFile);
        textureValue.IsHolding<SdfAssetPath>()) {
        SdfAssetPath texturePath =
            textureValue.UncheckedGet<SdfAssetPath>();
        path = texturePath.GetResolvedPath();
        if (path.empty()) {
            path = texturePath.GetAssetPath();
        }
    }
    light.texture = _LoadLightTexture(path);
}


} // anonymous namespace
PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(_tokens,
    ((inputsVisibilityCamera, "inputs:visibility:camera"))
    ((inputsVisibilityShadow, "inputs:visibility:shadow"))
);

HdEmbree_Light::HdEmbree_Light(SdfPath const& id, TfToken const& lightType)
    : HdLight(id) {
    if (id.IsEmpty()) {
        return;
    }

    // Set the variant to the right type - Sync will fill rest of data
    if (lightType == HdSprimTypeTokens->cylinderLight) {
        _lightData.lightVariant = HdEmbree_Cylinder();
    } else if (lightType == HdSprimTypeTokens->diskLight) {
        _lightData.lightVariant = HdEmbree_Disk();
    } else if (lightType == HdSprimTypeTokens->distantLight) {
        _lightData.lightVariant = HdEmbree_Distant();
    } else if (lightType == HdSprimTypeTokens->domeLight) {
        _lightData.lightVariant = HdEmbree_Dome();
    } else if (lightType == HdSprimTypeTokens->rectLight) {
        // Get shape parameters
        _lightData.lightVariant = HdEmbree_Rect();
    } else if (lightType == HdSprimTypeTokens->sphereLight) {
        _lightData.lightVariant = HdEmbree_Sphere();
    } else {
        TF_WARN("HdEmbree - Unrecognized light type: %s", lightType.GetText());
        _lightData.lightVariant = HdEmbree_UnknownLight();
    }
}

HdEmbree_Light::~HdEmbree_Light() = default;

void
HdEmbree_Light::Sync(HdSceneDelegate *sceneDelegate,
                         HdRenderParam *renderParam, HdDirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdEmbreeRenderParam *embreeRenderParam =
        static_cast<HdEmbreeRenderParam*>(renderParam);

    // calling this bumps the scene version and causes a re-render
    RTCScene scene = embreeRenderParam->AcquireSceneForEdit();
    RTCDevice device = embreeRenderParam->GetEmbreeDevice();

    SdfPath const& id = GetId();

    // Get _lightData's transform. We'll only consider the first time sample for now
    HdTimeSampleArray<GfMatrix4d, 1> xformSamples;
    sceneDelegate->SampleTransform(id, &xformSamples);
    _lightData.xformLightToWorld = GfMatrix4f(xformSamples.values[0]);
    _lightData.xformWorldToLight = _lightData.xformLightToWorld.GetInverse();
    _lightData.normalXformLightToWorld =
        _lightData.xformWorldToLight.ExtractRotationMatrix().GetTranspose();

    // Store luminance parameters
    _lightData.intensity = sceneDelegate->GetLightParamValue(
        id, HdLightTokens->intensity).GetWithDefault(1.0f);
    _lightData.exposure = sceneDelegate->GetLightParamValue(
        id, HdLightTokens->exposure).GetWithDefault(0.0f);
    _lightData.color = sceneDelegate->GetLightParamValue(
        id, HdLightTokens->color).GetWithDefault(GfVec3f{1.0f, 1.0f, 1.0f});
    _lightData.normalize = sceneDelegate->GetLightParamValue(
        id, HdLightTokens->normalize).GetWithDefault(false);
    _lightData.colorTemperature = sceneDelegate->GetLightParamValue(
        id, HdLightTokens->colorTemperature).GetWithDefault(6500.0f);
    _lightData.enableColorTemperature = sceneDelegate->GetLightParamValue(
        id, HdLightTokens->enableColorTemperature).GetWithDefault(false);

    // Get visibility
    _lightData.visible = sceneDelegate->GetVisible(id);
    _lightData.visible_camera = sceneDelegate->GetLightParamValue(
        id, _tokens->inputsVisibilityCamera).GetWithDefault(false);
    // XXX: Don't think we can get this to work in Embree unless it's built with
    // masking only solution would be to use rtcIntersect instead of rtcOccluded
    // for shadow rays, which maybe isn't the worst for a reference renderer
    _lightData.visible_shadow = sceneDelegate->GetLightParamValue(
        id, _tokens->inputsVisibilityShadow).GetWithDefault(false);

    // Switch on the _lightData type and pull the relevant attributes from the scene
    // delegate
    std::visit([this, &id, &sceneDelegate](auto& typedLight) {
        using T = std::decay_t<decltype(typedLight)>;
        if constexpr (std::is_same_v<T, HdEmbree_UnknownLight>) {
            // Do nothing
        } else if constexpr (std::is_same_v<T, HdEmbree_Cylinder>) {
            typedLight = HdEmbree_Cylinder{
                sceneDelegate->GetLightParamValue(id, HdLightTokens->radius)
                    .GetWithDefault(0.5f),
                sceneDelegate->GetLightParamValue(id, HdLightTokens->length)
                    .GetWithDefault(1.0f),
            };
        } else if constexpr (std::is_same_v<T, HdEmbree_Disk>) {
            typedLight = HdEmbree_Disk{
                sceneDelegate->GetLightParamValue(id, HdLightTokens->radius)
                    .GetWithDefault(0.5f),
            };
        } else if constexpr (std::is_same_v<T, HdEmbree_Distant>) {
            typedLight = HdEmbree_Distant{
                float(GfDegreesToRadians(
                    sceneDelegate->GetLightParamValue(id, HdLightTokens->angle)
                        .GetWithDefault(0.53f) / 2.0f)),
            };
        } else if constexpr (std::is_same_v<T, HdEmbree_Dome>) {
            typedLight = HdEmbree_Dome{};
            _SyncLightTexture(id, _lightData, sceneDelegate);
        } else if constexpr (std::is_same_v<T, HdEmbree_Rect>) {
            typedLight = HdEmbree_Rect{
                sceneDelegate->GetLightParamValue(id, HdLightTokens->width)
                    .Get<float>(),
                sceneDelegate->GetLightParamValue(id, HdLightTokens->height)
                    .Get<float>(),
            };
            _SyncLightTexture(id, _lightData, sceneDelegate);
        } else if constexpr (std::is_same_v<T, HdEmbree_Sphere>) {
            typedLight = HdEmbree_Sphere{
                sceneDelegate->GetLightParamValue(id, HdLightTokens->radius)
                    .GetWithDefault(0.5f),
            };
        } else {
            static_assert(false, "non-exhaustive _LightVariant visitor");
        }
    }, _lightData.lightVariant);

    if (const auto value = sceneDelegate->GetLightParamValue(
            id, HdLightTokens->shapingFocus);
        value.IsHolding<float>()) {
        _lightData.shaping.focus = value.UncheckedGet<float>();
    }

    if (const auto value = sceneDelegate->GetLightParamValue(
            id, HdLightTokens->shapingFocusTint);
        value.IsHolding<GfVec3f>()) {
        _lightData.shaping.focusTint = value.UncheckedGet<GfVec3f>();
    }

    if (const auto value = sceneDelegate->GetLightParamValue(
            id, HdLightTokens->shapingConeAngle);
        value.IsHolding<float>()) {
        _lightData.shaping.coneAngle = value.UncheckedGet<float>();
    }

    if (const auto value = sceneDelegate->GetLightParamValue(
            id, HdLightTokens->shapingConeSoftness);
        value.IsHolding<float>()) {
        _lightData.shaping.coneSoftness = value.UncheckedGet<float>();
    }

    if (const auto value = sceneDelegate->GetLightParamValue(
            id, HdLightTokens->shapingIesFile);
        value.IsHolding<SdfAssetPath>()) {
        SdfAssetPath iesAssetPath = value.UncheckedGet<SdfAssetPath>();
        std::string iesPath = iesAssetPath.GetResolvedPath();
        if (iesPath.empty()) {
            iesPath = iesAssetPath.GetAssetPath();
        }

        if (!iesPath.empty()) {
            std::ifstream in(iesPath);
            if (!in.is_open()) {
                TF_WARN("could not open ies file %s", iesPath.c_str());
            } else {
                std::stringstream buffer;
                buffer << in.rdbuf();

                if (!_lightData.shaping.ies.iesFile.load(buffer.str())) {
                    TF_WARN("could not load ies file %s", iesPath.c_str());
                }
            }
        }
    }

    if (const auto value = sceneDelegate->GetLightParamValue(
            id, HdLightTokens->shapingIesNormalize);
        value.IsHolding<bool>()) {
        _lightData.shaping.ies.normalize = value.UncheckedGet<bool>();
    }

    if (const auto value = sceneDelegate->GetLightParamValue(
            id, HdLightTokens->shapingIesAngleScale);
        value.IsHolding<float>()) {
        _lightData.shaping.ies.angleScale = value.UncheckedGet<float>();
    }

    _PopulateRtcLight(device, scene);

    HdEmbreeRenderer *renderer = embreeRenderParam->GetRenderer();
    renderer->AddLight(id, this);

    *dirtyBits &= ~HdLight::AllDirty;
}

void
HdEmbree_Light::_PopulateRtcLight(RTCDevice device, RTCScene scene)
{
    _lightData.rtcMeshId = RTC_INVALID_GEOMETRY_ID;

    // create the light geometry, if required
    if (_lightData.visible) {
        if (auto* rect = std::get_if<HdEmbree_Rect>(&_lightData.lightVariant))
        {
            // create _lightData mesh
            GfVec3f v0(-rect->width/2.0f, -rect->height/2.0f, 0.0f);
            GfVec3f v1( rect->width/2.0f, -rect->height/2.0f, 0.0f);
            GfVec3f v2( rect->width/2.0f,  rect->height/2.0f, 0.0f);
            GfVec3f v3(-rect->width/2.0f,  rect->height/2.0f, 0.0f);

            v0 = _lightData.xformLightToWorld.Transform(v0);
            v1 = _lightData.xformLightToWorld.Transform(v1);
            v2 = _lightData.xformLightToWorld.Transform(v2);
            v3 = _lightData.xformLightToWorld.Transform(v3);

            _lightData.rtcGeometry = rtcNewGeometry(device,
                                                RTC_GEOMETRY_TYPE_QUAD);
            GfVec3f* vertices = static_cast<GfVec3f*>(
                rtcSetNewGeometryBuffer(_lightData.rtcGeometry,
                                        RTC_BUFFER_TYPE_VERTEX,
                                        0,
                                        RTC_FORMAT_FLOAT3,
                                        sizeof(GfVec3f),
                                        4));
            vertices[0] = v0;
            vertices[1] = v1;
            vertices[2] = v2;
            vertices[3] = v3;

            unsigned* index = static_cast<unsigned*>(
                rtcSetNewGeometryBuffer(_lightData.rtcGeometry,
                                        RTC_BUFFER_TYPE_INDEX,
                                        0,
                                        RTC_FORMAT_UINT4,
                                        sizeof(unsigned)*4,
                                        1));
            index[0] = 0; index[1] = 1; index[2] = 2; index[3] = 3;

            auto ctx = std::make_unique<HdEmbreeInstanceContext>();
            ctx->light = this;
            rtcSetGeometryTimeStepCount(_lightData.rtcGeometry, 1);
            rtcCommitGeometry(_lightData.rtcGeometry);
            _lightData.rtcMeshId = rtcAttachGeometry(scene, _lightData.rtcGeometry);
            if (_lightData.rtcMeshId == RTC_INVALID_GEOMETRY_ID) {
                TF_WARN("could not create rect mesh for %s", GetId().GetAsString().c_str());
            } else {
                rtcSetGeometryUserData(_lightData.rtcGeometry, ctx.release());
            }
        }
    }
}

HdDirtyBits
HdEmbree_Light::GetInitialDirtyBitsMask() const
{
    return HdLight::AllDirty;
}

void
HdEmbree_Light::Finalize(HdRenderParam *renderParam)
{
    auto* embreeParam = static_cast<HdEmbreeRenderParam*>(renderParam);
    RTCScene scene = embreeParam->AcquireSceneForEdit();

    // First, remove from renderer's light map
    HdEmbreeRenderer *renderer = embreeParam->GetRenderer();
    renderer->RemoveLight(GetId(), this);

    // Then clean up the associated embree objects
    if (_lightData.rtcMeshId != RTC_INVALID_GEOMETRY_ID) {
        delete static_cast<HdEmbreeInstanceContext*>(
                    rtcGetGeometryUserData(_lightData.rtcGeometry));

        rtcDetachGeometry(scene, _lightData.rtcMeshId);
        rtcReleaseGeometry(_lightData.rtcGeometry);
        _lightData.rtcMeshId = RTC_INVALID_GEOMETRY_ID;
        _lightData.rtcGeometry = nullptr;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
