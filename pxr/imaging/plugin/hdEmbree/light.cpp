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
    embreeRenderParam->AcquireSceneForEdit();

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

    HdEmbreeRenderer *renderer = embreeRenderParam->GetRenderer();
    renderer->AddLight(id, this);

    *dirtyBits &= ~HdLight::AllDirty;
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

    // Remove from renderer's light map
    HdEmbreeRenderer *renderer = embreeParam->GetRenderer();
    renderer->RemoveLight(GetId(), this);
}

PXR_NAMESPACE_CLOSE_SCOPE
