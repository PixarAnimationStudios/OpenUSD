#include "light.h"
#include "ies.h"
#include "renderParam.h"
#include "renderer.h"

#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"
#include <pxr/imaging/hio/image.h>

#include <embree3/rtcore_buffer.h>
#include <embree3/rtcore_common.h>
#include <embree3/rtcore_device.h>
#include <embree3/rtcore_geometry.h>
#include <embree3/rtcore_scene.h>
#include <memory>
#include <fstream>
#include <sstream>

PXR_NAMESPACE_OPEN_SCOPE

HdEmbreeLight::HdEmbreeLight(SdfPath const &id, TfToken const &lightType)
    : HdLight(id), _lightType(lightType) {
    if (!id.IsEmpty()) {
        TF_WARN("Creating light %s: %s", id.GetText(), lightType.GetText());
    }
}

HdEmbreeLight::~HdEmbreeLight() {}

static LightTexture LoadLightTexture(std::string const& path)
{
    if (path.empty()) {
        return { nullptr, 0, 0 };
    }

    HioImageSharedPtr img = HioImage::OpenForReading(path);
    if (!img) {
        return { nullptr, 0, 0 };
    }

    int width = img->GetWidth();
    int height = img->GetHeight();

    GfVec3f* pixels = new GfVec3f[width * height * 3];

    HioImage::StorageSpec storage;
    storage.width = width;
    storage.height = height;
    storage.depth = 1;
    storage.format = HioFormatFloat32Vec3;
    storage.data = pixels;

    if (img->Read(storage)) {
        return {pixels, width, height};
    } else {
        TF_WARN("Could not read image %s", path.c_str());
        return { nullptr, 0, 0 };
    }
}

void HdEmbreeLight::Sync(HdSceneDelegate *sceneDelegate,
                         HdRenderParam *renderParam, HdDirtyBits *dirtyBits) {
    HdEmbreeRenderParam *embreeRenderParam =
        static_cast<HdEmbreeRenderParam*>(renderParam);

    // grabbin this bumps the scene version and causes a re-render
    RTCScene scene = embreeRenderParam->AcquireSceneForEdit();
    RTCDevice device = embreeRenderParam->GetEmbreeDevice();

    SdfPath const &id = GetId();
    Light light;

    // Get light's transform. We'll only consider the first time sample for now
    HdTimeSampleArray<GfMatrix4d, 1> xformSamples;
    sceneDelegate->SampleTransform(id, &xformSamples);
    light.xformLightToWorld = GfMatrix4f(xformSamples.values[0]);
    light.xformWorldToLight = light.xformLightToWorld.GetInverse();

    // Store luminance parameters
    light.intensity = sceneDelegate->GetLightParamValue(id, HdLightTokens->intensity).Get<float>();
    light.exposure = sceneDelegate->GetLightParamValue(id, HdLightTokens->exposure).Get<float>();
    light.color = sceneDelegate->GetLightParamValue(id, HdLightTokens->color).Get<GfVec3f>();
    light.normalize = sceneDelegate->GetLightParamValue(id, HdLightTokens->normalize).Get<bool>();
    light.colorTemperature = sceneDelegate->GetLightParamValue(id, HdLightTokens->colorTemperature).Get<float>();
    light.enableColorTemperature = sceneDelegate->GetLightParamValue(id, HdLightTokens->enableColorTemperature).Get<bool>();

    // Get visibility
    light.visible = sceneDelegate->GetVisible(id);
    light.visible_camera = sceneDelegate->GetLightParamValue(id, TfToken("inputs:visibility:camera")).GetWithDefault<bool>(false);
    // XXX: Don't think we can get this to work in Embree unless it's built with masking
    // only solution would be to use rtcIntersect instead of rtcOccluded for shadow rays, which
    // maybe isn't the worst for a reference renderer
    light.visible_shadow = sceneDelegate->GetLightParamValue(id, TfToken("inputs:visibility:shadow")).GetWithDefault<bool>(false);

    light.rtcMeshId = RTC_INVALID_GEOMETRY_ID;

    // Switch on the light type and pull the relevant attributes from the scene delegate
    if (_lightType == HdSprimTypeTokens->cylinderLight) {
        light.kind = LightKind::Cylinder;
        light.cylinder = {
            sceneDelegate->GetLightParamValue(id, HdLightTokens->radius)
                .Get<float>(),
            sceneDelegate->GetLightParamValue(id, HdLightTokens->length)
                .Get<float>(),
        };
    } else if (_lightType == HdSprimTypeTokens->diskLight) {
        light.kind = LightKind::Disk;
        light.disk = {
            sceneDelegate->GetLightParamValue(id, HdLightTokens->radius)
                .Get<float>(),
        };
    } else if (_lightType == HdSprimTypeTokens->distantLight) {
        light.kind = LightKind::Distant;

        light.distant = {
            float(GfDegreesToRadians(sceneDelegate->GetLightParamValue(id, HdLightTokens->angle)
                .Get<float>() / 2.0f)),
        };

    } else if (_lightType == HdSprimTypeTokens->domeLight) {
        light.kind = LightKind::Dome;
        
        SdfAssetPath texturePath = sceneDelegate->GetLightParamValue(id, HdLightTokens->textureFile).Get<SdfAssetPath>();
        std::string const& resolvedPath = texturePath.GetResolvedPath();
        light.texture = LoadLightTexture(resolvedPath);
    } else if (_lightType == HdSprimTypeTokens->rectLight) {
        // Get shape parameters
        light.kind = LightKind::Rect;
        light.rect = {
            sceneDelegate->GetLightParamValue(id, HdLightTokens->width)
                .Get<float>(),
            sceneDelegate->GetLightParamValue(id, HdLightTokens->height)
                .Get<float>(),
        };
        
        // get texture
        SdfAssetPath texturePath = sceneDelegate->GetLightParamValue(id, HdLightTokens->textureFile).Get<SdfAssetPath>();
        std::string path = texturePath.GetResolvedPath();
        if (path.empty()) {
            path = texturePath.GetAssetPath();
        }
        light.texture = LoadLightTexture(path);
    } else if (_lightType == HdSprimTypeTokens->sphereLight) {
        light.kind = LightKind::Sphere;
        light.sphere = {
            sceneDelegate->GetLightParamValue(id, HdLightTokens->radius)
                .Get<float>(),
        };
    } else if (_lightType == HdSprimTypeTokens->diskLight) {
        light.kind = LightKind::Disk;
        light.disk = {
            sceneDelegate->GetLightParamValue(id, HdLightTokens->radius).Get<float>()
        };
    }

    VtValue value;
    value = sceneDelegate->GetLightParamValue(id, HdLightTokens->shapingFocus);
    if (value.IsHolding<float>()) {
        light.shaping.focus = value.UncheckedGet<float>();
    }

    value = sceneDelegate->GetLightParamValue(id, HdLightTokens->shapingFocusTint);
    if (value.IsHolding<GfVec3f>()) {
        light.shaping.focusTint = value.UncheckedGet<GfVec3f>();
    }

    value = sceneDelegate->GetLightParamValue(id, HdLightTokens->shapingConeAngle);
    if (value.IsHolding<float>()) {
        light.shaping.coneAngle = value.UncheckedGet<float>();
    }

    value = sceneDelegate->GetLightParamValue(id, HdLightTokens->shapingConeSoftness);
    if (value.IsHolding<float>()) {
        light.shaping.coneSoftness = value.UncheckedGet<float>();
    }

    value = sceneDelegate->GetLightParamValue(id, HdLightTokens->shapingIesFile);
    if (value.IsHolding<SdfAssetPath>()) {
        SdfAssetPath iesAssetPath = value.UncheckedGet<SdfAssetPath>();
        std::string iesPath = iesAssetPath.GetResolvedPath();
        if (iesPath.empty()) {
            iesPath = iesAssetPath.GetAssetPath();
        }

        if (!iesPath.empty()) {
            std::ifstream in(iesPath);
            if (!in.is_open()) {
                printf("ERROR: could not open ies file %s\n", iesPath.c_str());
            }
            std::stringstream buffer;
            buffer << in.rdbuf();

            if (!light.shaping.ies.iesFile.load(buffer.str())) {
                printf("could not load ies file\n");
            }
        }
    }

    value = sceneDelegate->GetLightParamValue(id, HdLightTokens->shapingIesNormalize);
    if (value.IsHolding<bool>()) {
        light.shaping.ies.normalize = value.UncheckedGet<bool>();
    }

    value = sceneDelegate->GetLightParamValue(id, HdLightTokens->shapingIesAngleScale);
    if (value.IsHolding<float>()) {
        light.shaping.ies.angleScale = value.UncheckedGet<float>();
    }

    HdEmbreeRenderer *renderer =
        static_cast<HdEmbreeRenderParam *>(renderParam)->GetRenderer();
    _lightId = renderer->SetLight(id, std::move(light), device);

    *dirtyBits &= ~HdLight::AllDirty;
}

HdDirtyBits HdEmbreeLight::GetInitialDirtyBitsMask() const {
    return HdLight::AllDirty;
}

void HdEmbreeLight::Finalize(HdRenderParam *renderParam) {
}

PXR_NAMESPACE_CLOSE_SCOPE
