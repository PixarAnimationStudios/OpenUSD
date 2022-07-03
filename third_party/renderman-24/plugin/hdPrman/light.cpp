//
// Copyright 2019 Pixar
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
#include "hdPrman/light.h"
#include "hdPrman/material.h"
#include "hdPrman/renderParam.h"
#include "hdPrman/debugCodes.h"
#include "hdPrman/rixStrings.h"
#include "pxr/base/arch/library.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hf/diagnostic.h"

#include "RiTypesHelper.h"
#include "RixShadingUtils.h"

#include "hdPrman/lightFilterUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // tokens for RenderMan-specific light parameters
    ((cheapCaustics,                "inputs:ri:light:cheapCaustics"))
    ((cheapCausticsExcludeGroup,    "inputs:ri:light:cheapCausticsExcludeGroup"))
    ((fixedSampleCount,             "inputs:ri:light:fixedSampleCount"))
    ((importanceMultiplier,         "inputs:ri:light:importanceMultiplier"))
    ((intensityNearDist,            "inputs:ri:light:intensityNearDist"))
    ((thinShadow,                   "inputs:ri:light:thinShadow"))
    ((traceLightPaths,              "inputs:ri:light:traceLightPaths"))
    ((visibleInRefractionPath,      "inputs:ri:light:visibleInRefractionPath"))
    ((lightGroup,                   "inputs:ri:light:lightGroup"))
    ((colorMapGamma,                "inputs:ri:light:colorMapGamma"))
    ((colorMapSaturation,           "inputs:ri:light:colorMapSaturation"))
);

TF_DEFINE_ENV_SETTING(HD_PRMAN_ENABLE_LIGHT_MATERIAL_NETWORKS, true,
                      "bool env setting to enable material networks for lights "
                      "and light filters");

HdPrmanLight::HdPrmanLight(SdfPath const& id, TfToken const& lightType)
    : HdLight(id)
    , _hdLightType(lightType)
    , _shaderId(riley::LightShaderId::InvalidId())
    , _instanceId(riley::LightInstanceId::InvalidId())
{
    /* NOTHING */
}

HdPrmanLight::~HdPrmanLight() = default;

void
HdPrmanLight::Finalize(HdRenderParam *renderParam)
{
    HdPrman_RenderParam *param =
        static_cast<HdPrman_RenderParam*>(renderParam);
    _ResetLight(param, true);
}

void
HdPrmanLight::_ResetLight(HdPrman_RenderParam *renderParam, bool clearFilterPaths)
{
    riley::Riley *riley = renderParam->AcquireRiley();

    if (!_lightLink.IsEmpty()) {
        renderParam->DecrementLightLinkCount(_lightLink);
        _lightLink = TfToken();
    }
    if (clearFilterPaths && !_lightFilterPaths.empty()) {
        _lightFilterPaths.clear();
    }
    if (!_lightFilterLinks.empty()) {
        for (const TfToken &filterLink: _lightFilterLinks)
            renderParam->DecrementLightFilterCount(filterLink);
        _lightFilterLinks.clear();
    }

    if (_instanceId != riley::LightInstanceId::InvalidId()) {
        riley->DeleteLightInstance(
            riley::GeometryPrototypeId::InvalidId(),
            _instanceId);
        _instanceId = riley::LightInstanceId::InvalidId();
    }
    if (_shaderId != riley::LightShaderId::InvalidId()) {
        riley->DeleteLightShader(_shaderId);
        _shaderId = riley::LightShaderId::InvalidId();
    }
}

static RtUString
_RtStringFromSdfAssetPath(SdfAssetPath const& ap)
{
    // Although Renderman does its own searchpath resolution,
    // scene delegates like USD may have additional path resolver
    // semantics, so try GetResolvedPath() first.
    std::string p = ap.GetResolvedPath();
    if (p.empty()) {
        p = ap.GetAssetPath();
    }
    return RtUString(p.c_str());
}

static RtUString
_RtStringFromLightColorMapAssetPath(SdfAssetPath const& ap)
{
    // Although Renderman does its own searchpath resolution,
    // scene delegates like USD may have additional path resolver
    // semantics, so try GetResolvedPath() first.
    //
    // Also, use the RtxHioImage plugin if the path is resolved and it
    // isn't a tex file.  RtxHioImage wants to flip images by default
    // but lightColorMap images should not be flipped.
    std::string p = ap.GetResolvedPath();
    if (p.empty()) {
        p = ap.GetAssetPath();
    }
    else if (ArGetResolver().GetExtension(p) != "tex") {
        p = "rtxplugin:RtxHioImage" ARCH_LIBRARY_SUFFIX
            "?filename=" + p + "&flipped=false";
    }
    TF_DEBUG(HDPRMAN_IMAGE_ASSET_RESOLVE)
        .Msg("Resolved lightColorMap asset path: %s\n", p.c_str());
    return RtUString(p.c_str());
}

static
void
_PopulateNodeFromLightParams(HdSceneDelegate *sceneDelegate,
                             const SdfPath &id,
                             const TfToken &hdLightType,
                             std::vector<riley::ShadingNode> *lightNodes)
{
    static const RtUString us_intensity("intensity");
    static const RtUString us_exposure("exposure");
    static const RtUString us_lightColor("lightColor");
    static const RtUString us_enableTemperature("enableTemperature");
    static const RtUString us_temperature("temperature");
    static const RtUString us_diffuse("diffuse");
    static const RtUString us_specular("specular");
    static const RtUString us_areaNormalize("areaNormalize");
    static const RtUString us_emissionFocus("emissionFocus");
    static const RtUString us_emissionFocusTint("emissionFocusTint");
    static const RtUString us_coneAngle("coneAngle");
    static const RtUString us_coneSoftness("coneSoftness");
    static const RtUString us_iesProfile("iesProfile");
    static const RtUString us_iesProfileScale("iesProfileScale");
    static const RtUString us_iesProfileNormalize("iesProfileNormalize");
    static const RtUString us_enableShadows("enableShadows");
    static const RtUString us_shadowColor("shadowColor");
    static const RtUString us_shadowDistance("shadowDistance");
    static const RtUString us_shadowFalloff("shadowFalloff");
    static const RtUString us_shadowFalloffGamma("shadowFalloffGamma");
    static const RtUString us_shadowSubset("shadowSubset");
    static const RtUString us_PxrDomeLight("PxrDomeLight");
    static const RtUString us_PxrRectLight("PxrRectLight");
    static const RtUString us_PxrDiskLight("PxrDiskLight");
    static const RtUString us_PxrCylinderLight("PxrCylinderLight");
    static const RtUString us_PxrSphereLight("PxrSphereLight");
    static const RtUString us_PxrDistantLight("PxrDistantLight");
    static const RtUString us_angleExtent("angleExtent");
    static const RtUString us_lightColorMap("lightColorMap");
    static const RtUString us_default("default");
    static const RtUString us_cheapCaustics("cheapCaustics");
    static const RtUString us_cheapCausticsExcludeGroup(
                                "cheapCausticsExcludeGroup");
    static const RtUString us_fixedSampleCount("fixedSampleCount");
    static const RtUString us_importanceMultiplier("importanceMultiplier");
    static const RtUString us_intensityNearDist("intensityNearDist");
    static const RtUString us_thinShadow("thinShadow");
    static const RtUString us_traceLightPaths("traceLightPaths");
    static const RtUString us_visibleInRefractionPath(
                                "visibleInRefractionPath");
    static const RtUString us_lightGroup("lightGroup");
    static const RtUString us_colorMapGamma("colorMapGamma");
    static const RtUString us_colorMapSaturation("colorMapSaturation");

    lightNodes->push_back({
        riley::ShadingNode::Type::k_Light, // type
        US_NULL, // name
        RtUString(id.GetText()), // handle
        RtParamList() // params
    });

    riley::ShadingNode &lightNode = lightNodes->back();

    // UsdLuxLight base parameters
    {
        // intensity
        VtValue intensity =
            sceneDelegate->GetLightParamValue(id, HdLightTokens->intensity);
        if (intensity.IsHolding<float>()) {
            lightNode.params.SetFloat(us_intensity, intensity.UncheckedGet<float>());
        }

        // exposure
        VtValue exposure =
            sceneDelegate->GetLightParamValue(id, HdLightTokens->exposure);
        if (exposure.IsHolding<float>()) {
            lightNode.params.SetFloat(us_exposure, exposure.UncheckedGet<float>());
        }

        // color -> lightColor
        VtValue lightColor =
            sceneDelegate->GetLightParamValue(id, HdLightTokens->color);
        if (lightColor.IsHolding<GfVec3f>()) {
            GfVec3f v = lightColor.UncheckedGet<GfVec3f>();
            lightNode.params.SetColor(us_lightColor, RtColorRGB(v[0], v[1], v[2]));
        }

        // enableColorTemperature -> enableTemperature
        VtValue enableTemperature =
            sceneDelegate->GetLightParamValue(id,
                HdLightTokens->enableColorTemperature);
        if (enableTemperature.IsHolding<bool>()) {
            int v = enableTemperature.UncheckedGet<bool>();
            lightNode.params.SetInteger(us_enableTemperature, v);
        }

        // temperature
        VtValue temperature =
            sceneDelegate->GetLightParamValue(id, 
                HdLightTokens->colorTemperature);
        if (temperature.IsHolding<float>()) {
            lightNode.params.SetFloat(us_temperature, temperature.UncheckedGet<float>());
        }

        // diffuse
        VtValue diffuse =
            sceneDelegate->GetLightParamValue(id, HdLightTokens->diffuse);
        if (diffuse.IsHolding<float>()) {
            lightNode.params.SetFloat(us_diffuse, diffuse.UncheckedGet<float>());
        }

        // specular
        VtValue specular =
            sceneDelegate->GetLightParamValue(id, HdLightTokens->specular);
        if (specular.IsHolding<float>()) {
            lightNode.params.SetFloat(us_specular, specular.UncheckedGet<float>());
        }

        // normalize -> areaNormalize
        // (Avoid unused param warnings for light types that don't have this)
        if (hdLightType != HdPrimTypeTokens->domeLight) {
            VtValue normalize =
                sceneDelegate->GetLightParamValue(id, HdLightTokens->normalize);
            if (normalize.IsHolding<bool>()) {
                int v = normalize.UncheckedGet<bool>();
                lightNode.params.SetInteger(us_areaNormalize, v);
            }
        }
    }

    // UsdLuxShapingAPI
    {
        if (hdLightType != HdPrimTypeTokens->domeLight) {
            VtValue shapingFocus =
                sceneDelegate->GetLightParamValue(id,
                                                  HdLightTokens->shapingFocus);
            if (shapingFocus.IsHolding<float>()) {
                lightNode.params.SetFloat(us_emissionFocus,
                                 shapingFocus.UncheckedGet<float>());
            }

            // XXX -- emissionFocusNormalize is missing here

            VtValue shapingFocusTint =
                sceneDelegate->GetLightParamValue(id, 
                    HdLightTokens->shapingFocusTint);
            if (shapingFocusTint.IsHolding<GfVec3f>()) {
                GfVec3f v = shapingFocusTint.UncheckedGet<GfVec3f>();
                lightNode.params.SetColor(us_emissionFocusTint,
                                 RtColorRGB(v[0], v[1], v[2]));
            }
        }

        // ies is only supported on rect, disk, cylinder and sphere light.
        // cone angle only supported on rect, disk, cylinder and sphere lights.
        // XXX -- fix for mesh/geometry light when it comes online
        if (hdLightType == HdPrimTypeTokens->rectLight ||
            hdLightType == HdPrimTypeTokens->diskLight ||
            hdLightType == HdPrimTypeTokens->cylinderLight ||
            hdLightType == HdPrimTypeTokens->sphereLight) {

            VtValue shapingConeAngle =
                sceneDelegate->GetLightParamValue(id, 
                    HdLightTokens->shapingConeAngle);
            if (shapingConeAngle.IsHolding<float>()) {
                lightNode.params.SetFloat(us_coneAngle,
                                 shapingConeAngle.UncheckedGet<float>());
            }

            VtValue shapingConeSoftness =
                sceneDelegate->GetLightParamValue(id, 
                    HdLightTokens->shapingConeSoftness);
            if (shapingConeSoftness.IsHolding<float>()) {
                lightNode.params.SetFloat(us_coneSoftness,
                                 shapingConeSoftness.UncheckedGet<float>());
            }

            VtValue shapingIesFile =
                sceneDelegate->GetLightParamValue(id, 
                    HdLightTokens->shapingIesFile);
            if (shapingIesFile.IsHolding<SdfAssetPath>()) {
                SdfAssetPath ap = shapingIesFile.UncheckedGet<SdfAssetPath>();
                lightNode.params.SetString(us_iesProfile, _RtStringFromSdfAssetPath(ap));
            }

            VtValue shapingIesAngleScale =
                sceneDelegate->GetLightParamValue(id,
                    HdLightTokens->shapingIesAngleScale);
            if (shapingIesAngleScale.IsHolding<float>()) {
                lightNode.params.SetFloat(us_iesProfileScale,
                                 shapingIesAngleScale.UncheckedGet<float>());
            }

            VtValue shapingIesNormalize =
                sceneDelegate->GetLightParamValue(id,
                    HdLightTokens->shapingIesNormalize);
            if (shapingIesNormalize.IsHolding<bool>()) {
                lightNode.params.SetInteger(us_iesProfileNormalize,
                                   shapingIesNormalize.UncheckedGet<bool>());
            }
        }
    }

    // UsdLuxShadowAPI
    {
        VtValue shadowEnable =
            sceneDelegate->GetLightParamValue(id, HdLightTokens->shadowEnable);
        if (shadowEnable.IsHolding<bool>()) {
            lightNode.params.SetInteger(us_enableShadows,
                               shadowEnable.UncheckedGet<bool>());
        }

        VtValue shadowColor =
            sceneDelegate->GetLightParamValue(id, HdLightTokens->shadowColor);
        if (shadowColor.IsHolding<GfVec3f>()) {
            GfVec3f v = shadowColor.UncheckedGet<GfVec3f>();
            lightNode.params.SetColor(us_shadowColor, RtColorRGB(v[0], v[1], v[2]));
        }

        VtValue shadowDistance =
            sceneDelegate->GetLightParamValue(id, 
                HdLightTokens->shadowDistance);
        if (shadowDistance.IsHolding<float>()) {
            lightNode.params.SetFloat(us_shadowDistance,
                             shadowDistance.UncheckedGet<float>());
        }

        VtValue shadowFalloff =
            sceneDelegate->GetLightParamValue(id, HdLightTokens->shadowFalloff);
        if (shadowFalloff.IsHolding<float>()) {
            lightNode.params.SetFloat(us_shadowFalloff,
                             shadowFalloff.UncheckedGet<float>());
        }

        VtValue shadowFalloffGamma =
            sceneDelegate->GetLightParamValue(id, 
                HdLightTokens->shadowFalloffGamma);
        if (shadowFalloffGamma.IsHolding<float>()) {
            lightNode.params.SetFloat(us_shadowFalloffGamma,
                             shadowFalloffGamma.UncheckedGet<float>());
        }
    }

    // extra RenderMan parameters - "ri:light"
    {
        VtValue cheapCaustics =
            sceneDelegate->GetLightParamValue(id, _tokens->cheapCaustics);
        if (cheapCaustics.IsHolding<int>()) {
            lightNode.params.SetInteger(us_cheapCaustics,
                               cheapCaustics.UncheckedGet<int>());
        }

        VtValue cheapCausticsExcludeGroupVal =
            sceneDelegate->GetLightParamValue(id, 
                _tokens->cheapCausticsExcludeGroup);
        if (cheapCausticsExcludeGroupVal.IsHolding<TfToken>()) {
            TfToken cheapCausticsExcludeGroup = 
                cheapCausticsExcludeGroupVal.UncheckedGet<TfToken>();
            if (!cheapCausticsExcludeGroup.IsEmpty()) {
                lightNode.params.SetString(us_cheapCausticsExcludeGroup,
                                RtUString(cheapCausticsExcludeGroup.GetText()));
            }
        }

        VtValue fixedSampleCount =
            sceneDelegate->GetLightParamValue(id, _tokens->fixedSampleCount);
        if (fixedSampleCount.IsHolding<int>()) {
            lightNode.params.SetInteger(us_fixedSampleCount,
                               fixedSampleCount.UncheckedGet<int>());
        }

        VtValue importanceMultiplier =
            sceneDelegate->GetLightParamValue(id, 
                _tokens->importanceMultiplier);
        if (importanceMultiplier.IsHolding<float>()) {
            lightNode.params.SetFloat(us_importanceMultiplier,
                               importanceMultiplier.UncheckedGet<float>());
        }

        VtValue intensityNearDist =
            sceneDelegate->GetLightParamValue(id, _tokens->intensityNearDist);
        if (intensityNearDist.IsHolding<float>()) {
            lightNode.params.SetFloat(us_intensityNearDist,
                               intensityNearDist.UncheckedGet<float>());
        }

        VtValue thinShadow =
            sceneDelegate->GetLightParamValue(id, _tokens->thinShadow);
        if (thinShadow.IsHolding<int>()) {
            lightNode.params.SetInteger(us_thinShadow, thinShadow.UncheckedGet<int>());
        }

        VtValue traceLightPaths =
            sceneDelegate->GetLightParamValue(id, _tokens->traceLightPaths);
        if (traceLightPaths.IsHolding<int>()) {
            lightNode.params.SetInteger(us_traceLightPaths,
                               traceLightPaths.UncheckedGet<int>());
        }

        VtValue visibleInRefractionPath =
            sceneDelegate->GetLightParamValue(id, 
                _tokens->visibleInRefractionPath);
        if (visibleInRefractionPath.IsHolding<int>()) {
            lightNode.params.SetInteger(us_visibleInRefractionPath,
                               visibleInRefractionPath.UncheckedGet<int>());
        }

        VtValue lightGroupVal =
            sceneDelegate->GetLightParamValue(id, _tokens->lightGroup);
        if (lightGroupVal.IsHolding<TfToken>()) {
            TfToken lightGroup = lightGroupVal.UncheckedGet<TfToken>();
            if (!lightGroup.IsEmpty()) {
                lightNode.params.SetString(us_lightGroup,
                                  RtUString(lightGroup.GetText()));
            }
        }
    }


    TF_DEBUG(HDPRMAN_LIGHT_LIST)
        .Msg("HdPrman: Light <%s> lightType \"%s\"\n",
             id.GetText(), hdLightType.GetText());

    if (hdLightType == HdPrimTypeTokens->domeLight) {
        lightNode.name = us_PxrDomeLight;
    } else if (hdLightType == HdPrimTypeTokens->rectLight) {
        lightNode.name = us_PxrRectLight;
    } else if (hdLightType == HdPrimTypeTokens->diskLight) {
        lightNode.name = us_PxrDiskLight;
    } else if (hdLightType == HdPrimTypeTokens->cylinderLight) {
        lightNode.name = us_PxrCylinderLight;
    } else if (hdLightType == HdPrimTypeTokens->sphereLight) {
        lightNode.name = us_PxrSphereLight;
    } else if (hdLightType == HdPrimTypeTokens->distantLight) {
        lightNode.name = us_PxrDistantLight;
        VtValue angle =
            sceneDelegate->GetLightParamValue(id, HdLightTokens->angle);
        if (angle.IsHolding<float>()) {
            lightNode.params.SetFloat(us_angleExtent, angle.UncheckedGet<float>());
        }
    }
    
    if (hdLightType == HdPrimTypeTokens->domeLight ||
        hdLightType == HdPrimTypeTokens->rectLight) {
        // textureFile -> lightColorMap
        VtValue textureFile = sceneDelegate->GetLightParamValue(id,
            HdLightTokens->textureFile);
        if (textureFile.IsHolding<SdfAssetPath>()) {
            SdfAssetPath ap = textureFile.UncheckedGet<SdfAssetPath>();
            lightNode.params.SetString(us_lightColorMap,
                        _RtStringFromLightColorMapAssetPath(ap));
        }

        VtValue colorMapGamma =
            sceneDelegate->GetLightParamValue(id, _tokens->colorMapGamma);
        if (colorMapGamma.IsHolding<GfVec3f>()) {
            GfVec3f v = colorMapGamma.UncheckedGet<GfVec3f>();
            lightNode.params.SetVector(us_colorMapGamma, RtVector3(v[0], v[1], v[2]));
        }

        VtValue colorMapSaturation =
            sceneDelegate->GetLightParamValue(id, _tokens->colorMapSaturation);
        if (colorMapSaturation.IsHolding<float>()) {
            lightNode.params.SetFloat(us_colorMapSaturation,
                               colorMapSaturation.UncheckedGet<float>());
        }
    }
}

static bool
_PopulateNodesFromMaterialResource(HdSceneDelegate *sceneDelegate,
                                   const SdfPath &id,
                                   const TfToken &terminalName,
                                   std::vector<riley::ShadingNode> *result)
{
    VtValue hdMatVal = sceneDelegate->GetMaterialResource(id);
    if (!hdMatVal.IsHolding<HdMaterialNetworkMap>()) {
        TF_WARN("Could not get HdMaterialNetworkMap for '%s'", id.GetText());
        return false;
    }

    // Convert HdMaterial to HdMaterialNetwork2 form.
    const HdMaterialNetwork2 matNetwork2 = HdConvertToHdMaterialNetwork2(
            hdMatVal.UncheckedGet<HdMaterialNetworkMap>());

    SdfPath nodePath;
    for (auto const& terminal: matNetwork2.terminals) {
        if (terminal.first == terminalName) {
            nodePath = terminal.second.upstreamNode;
            break;
        }
    }

    if (nodePath.IsEmpty()) {
        TF_WARN("Could not find terminal '%s' in HdMaterialNetworkMap for '%s'",
                terminalName.GetText(), id.GetText());
        return false;
    }

    result->reserve(matNetwork2.nodes.size());
    if (!HdPrman_ConvertHdMaterialNetwork2ToRmanNodes(
            matNetwork2, nodePath, result)) {
        TF_WARN("Failed to convert HdMaterialNetwork to Renderman shading "
                "nodes for '%s'", id.GetText());
        return false;
    }

    return true;
}

static void
_AddLightFilterCombiner(std::vector<riley::ShadingNode>* lightFilterNodes)
{
    static RtUString combineMode("combineMode");
    static RtUString mult("mult");
    static RtUString max("max");
    static RtUString min("min");
    static RtUString screen("screen");

    riley::ShadingNode combiner = riley::ShadingNode {
        riley::ShadingNode::Type::k_LightFilter,
        RtUString("PxrCombinerLightFilter"),
        RtUString("terminal.Lightfilter"),
        RtParamList()
    };

    // Build a map of light filter handles grouped by mode.
    std::unordered_map<RtUString, std::vector<RtUString>> modeMap;

    for (const auto& lightFilterNode : *lightFilterNodes) {       
        RtUString mode;
        lightFilterNode.params.GetString(combineMode, mode);
        if (mode.Empty()) {
            modeMap[mult].push_back(lightFilterNode.handle);
        } 
        else {
            modeMap[mode].push_back(lightFilterNode.handle);
        }
    }

    // Set the combiner light filter reference array for each mode.
    for (const auto& entry : modeMap) {
        combiner.params.SetLightFilterReferenceArray(
            entry.first, &entry.second[0], entry.second.size());
    }

    lightFilterNodes->push_back(combiner);
}

static void
_PopulateLightFilterNodes(
        const SdfPath &lightId,
        const SdfPathVector &lightFilterPaths,
        HdSceneDelegate *sceneDelegate,
        HdRenderParam *renderParam,
        riley::Riley *riley,
        std::vector<riley::ShadingNode> *lightFilterNodes,
        std::vector<riley::CoordinateSystemId> *coordsysIds,
        std::vector<TfToken> *lightFilterLinks)
{
    HdPrman_RenderParam * const param =
        static_cast<HdPrman_RenderParam*>(renderParam);

    if (lightFilterPaths.empty()) {
        return;
    }

    int maxFilters = lightFilterPaths.size();
    if (maxFilters > 1) {
        maxFilters += 1;  // extra for the combiner filter
    }
    lightFilterNodes->reserve(maxFilters);
    
    for (const auto& filterPath : lightFilterPaths) {
        TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
            .Msg("HdPrman: Light <%s> filter \"%s\" path \"%s\"\n",
                lightId.GetText(), filterPath.GetName().c_str(),
                filterPath.GetText());

        if (!sceneDelegate->GetVisible(filterPath)) {
            // XXX -- need to get a dependency analysis working here
            // Invis of a filter works but does not cause the light
            // to re-sync so one has to tweak the light to see the
            // effect of the invised filter
            TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                .Msg("  filter invisible\n");
            continue;
        }

        if (TfGetEnvSetting(HD_PRMAN_ENABLE_LIGHT_MATERIAL_NETWORKS)) {
            if (!_PopulateNodesFromMaterialResource(
                    sceneDelegate, filterPath,
                    HdMaterialTerminalTokens->lightFilter, 
                    lightFilterNodes)) {
                continue;
            }
        } else {
            if (!HdPrmanLightFilterPopulateNodesFromLightParams(
                lightFilterNodes, filterPath, sceneDelegate)) {
                continue;
            }
        }

        HdPrmanLightFilterGenerateCoordSysAndLinks(
            &lightFilterNodes->back(),
            filterPath,
            coordsysIds,
            lightFilterLinks,
            sceneDelegate,
            param,
            riley);
    }

    // Multiple filters requires a PxrCombinerLightFilter to combine results.
    if (lightFilterNodes->size() > 1) {
        _AddLightFilterCombiner(lightFilterNodes);
    }
}

/* virtual */
void
HdPrmanLight::Sync(HdSceneDelegate *sceneDelegate,
                   HdRenderParam   *renderParam,
                   HdDirtyBits     *dirtyBits)
{
    // Change tracking
    HdDirtyBits bits = *dirtyBits;

    static const RtUString us_PxrDomeLight("PxrDomeLight");
    static const RtUString us_PxrRectLight("PxrRectLight");
    static const RtUString us_PxrDiskLight("PxrDiskLight");
    static const RtUString us_PxrCylinderLight("PxrCylinderLight");
    static const RtUString us_PxrSphereLight("PxrSphereLight");
    static const RtUString us_PxrDistantLight("PxrDistantLight");
    static const RtUString us_shadowSubset("shadowSubset");
    static const RtUString us_default("default");

    HdPrman_RenderParam * const param =
        static_cast<HdPrman_RenderParam*>(renderParam);

    riley::Riley *const riley = param->AcquireRiley();

    SdfPath id = GetId();

    HdChangeTracker& changeTracker = 
        sceneDelegate->GetRenderIndex().GetChangeTracker();

    // Remove old dependencies before clearing the light.
    bool clearFilterPaths = false;
    if (bits & DirtyParams) {
        if (!_lightFilterPaths.empty()) {
            for (SdfPath const & filterPath : _lightFilterPaths) {
                changeTracker.RemoveSprimSprimDependency(
                    filterPath, id);
            }
        }
        clearFilterPaths = true;
    }
    
    // For simplicity just re-create the light.  In the future we may
    // want to consider adding a path to use the Modify() API in Riley.
    _ResetLight(param, clearFilterPaths);

    std::vector<riley::ShadingNode> lightNodes;

    if (TfGetEnvSetting(HD_PRMAN_ENABLE_LIGHT_MATERIAL_NETWORKS)) {
        _PopulateNodesFromMaterialResource(
            sceneDelegate, id, HdMaterialTerminalTokens->light, &lightNodes);
    } else {
        _PopulateNodeFromLightParams(
            sceneDelegate, id, _hdLightType, &lightNodes);
    }

    if (lightNodes.empty() || lightNodes.back().name.Empty()) {
        TF_WARN("Could not populate shading nodes for light '%s'. Skipping.",
                id.GetText());
        *dirtyBits = HdChangeTracker::Clean;
        return;
    }

    TF_DEBUG(HDPRMAN_LIGHT_LIST)
        .Msg("HdPrman: Light <%s> lightType \"%s\"\n",
             id.GetText(), _hdLightType.GetText());

    // The terminal light node will be updated with other parameters that 
    // aren't direct inputs of the material resource.
    riley::ShadingNode &lightNode = lightNodes.back();

    // Attributes.
    RtParamList attrs = param->ConvertAttributes(sceneDelegate, id);

    // Light linking
    {
        VtValue val =
            sceneDelegate->GetLightParamValue(id, HdTokens->lightLink);
        if (val.IsHolding<TfToken>()) {
            _lightLink = val.UncheckedGet<TfToken>();
        }
        
        if (!_lightLink.IsEmpty()) {
            param->IncrementLightLinkCount(_lightLink);
            // For lights to link geometry, the lights must
            // be assigned a grouping membership, and the
            // geometry must subscribe to that grouping.
            attrs.SetString(RixStr.k_grouping_membership,
                            RtUString(_lightLink.GetText()));
            TF_DEBUG(HDPRMAN_LIGHT_LINKING)
                .Msg("HdPrman: Light <%s> grouping membership \"%s\"\n",
                        id.GetText(), _lightLink.GetText());
        } else {
            // Default light group
            attrs.SetString(RixStr.k_grouping_membership, us_default);
            TF_DEBUG(HDPRMAN_LIGHT_LINKING)
                .Msg("HdPrman: Light <%s> grouping membership \"default\"\n",
                     id.GetText());
        }
    }

    // Shadow linking
    {
        VtValue shadowLinkVal =
            sceneDelegate->GetLightParamValue(id, HdTokens->shadowLink);
        if (shadowLinkVal.IsHolding<TfToken>()) {
            TfToken shadowLink = shadowLinkVal.UncheckedGet<TfToken>();
            if (!shadowLink.IsEmpty()) {
                lightNode.params.SetString(us_shadowSubset,
                                  RtUString(shadowLink.GetText()));
                TF_DEBUG(HDPRMAN_LIGHT_LINKING)
                    .Msg("HdPrman: Light <%s> shadowSubset \"%s\"\n",
                         id.GetText(), shadowLink.GetText());
            }
        }
    }

    // filters
    // Re-gather filter paths and add dependencies if necessary.
    if (clearFilterPaths) {
        VtValue val = sceneDelegate->GetLightParamValue(id, HdTokens->filters);
        if (val.IsHolding<SdfPathVector>()) {
            _lightFilterPaths = val.UncheckedGet<SdfPathVector>();
            for (SdfPath &filterPath: _lightFilterPaths) {
                changeTracker.AddSprimSprimDependency(filterPath, id);
            }
        }
    }

    std::vector<riley::ShadingNode> filterNodes;
    std::vector<riley::CoordinateSystemId> coordsysIds;
    _PopulateLightFilterNodes(
        id, _lightFilterPaths, sceneDelegate, renderParam, riley,
        &filterNodes, &coordsysIds, &_lightFilterLinks);

    // TODO: portals

    _shaderId = riley->CreateLightShader(
        riley::UserId(stats::AddDataLocation(id.GetText()).GetValue()),
        {static_cast<uint32_t>(lightNodes.size()), lightNodes.data()},
        {static_cast<uint32_t>(filterNodes.size()), filterNodes.data()});

    // Sample transform
    HdTimeSampleArray<GfMatrix4d, HDPRMAN_MAX_TIME_SAMPLES> xf;
    sceneDelegate->SampleTransform(id, &xf);
    
    TfSmallVector<RtMatrix4x4, HDPRMAN_MAX_TIME_SAMPLES> 
        xf_rt_values(xf.count);

    GfMatrix4d geomMat(1.0);

    // Some lights have parameters that scale the size of the light.
    GfVec3d geomScale(1.0f);

    // Type-specific parameters
    if (lightNode.name == us_PxrRectLight) {
        // width
        VtValue width = sceneDelegate->GetLightParamValue(id, 
            HdLightTokens->width);
        if (width.IsHolding<float>()) {
            geomScale[0] = width.UncheckedGet<float>();
        }
        // height
        VtValue height = sceneDelegate->GetLightParamValue(id, 
            HdLightTokens->height);
        if (height.IsHolding<float>()) {
            geomScale[1] = height.UncheckedGet<float>();
        }
    } else if (lightNode.name == us_PxrDiskLight) {
        // radius (XY only, default 0.5)
        VtValue radius = sceneDelegate->GetLightParamValue(id, 
            HdLightTokens->radius);
        if (radius.IsHolding<float>()) {
            geomScale[0] *= radius.UncheckedGet<float>() / 0.5;
            geomScale[1] *= radius.UncheckedGet<float>() / 0.5;
        }
    } else if (lightNode.name == us_PxrCylinderLight) {
        // radius (YZ only, default 0.5)
        VtValue radius = sceneDelegate->GetLightParamValue(id, 
            HdLightTokens->radius);
        if (radius.IsHolding<float>()) {
            geomScale[1] *= radius.UncheckedGet<float>() / 0.5;
            geomScale[2] *= radius.UncheckedGet<float>() / 0.5;
        }
        // length (X-axis)
        VtValue length = sceneDelegate->GetLightParamValue(id, 
            HdLightTokens->length);
        if (length.IsHolding<float>()) {
            geomScale[0] *= length.UncheckedGet<float>();
        }
    } else if (lightNode.name == us_PxrSphereLight) {
        // radius (XYZ, default 0.5)
        VtValue radius = sceneDelegate->GetLightParamValue(id, 
            HdLightTokens->radius);
        if (radius.IsHolding<float>()) {
            geomScale *= radius.UncheckedGet<float>() / 0.5;
        }
    }

    geomMat.SetScale(geomScale);

    // adjust orientation to make prman match the USD spec
    // TODO: Add another orientMat for PxrEnvDayLight when supported
    GfMatrix4d orientMat(1.0);
    if (lightNode.name == us_PxrDomeLight)
    {
        // Transform Dome to match OpenEXR spec for environment maps
        // Rotate -90 X, Rotate 90 Y
        orientMat = GfMatrix4d(0.0, 0.0, -1.0, 0.0, 
                               -1.0, 0.0, 0.0, 0.0, 
                               0.0, 1.0, 0.0, 0.0, 
                               0.0, 0.0, 0.0, 1.0);
    }
    else
    {
        // Transform lights to match correct orientation
        // Scale -1 Z, Rotate 180 Z
        orientMat = GfMatrix4d(-1.0, 0.0, 0.0, 0.0, 
                               0.0, -1.0, 0.0, 0.0, 
                               0.0, 0.0, -1.0, 0.0, 
                               0.0, 0.0, 0.0, 1.0);
    }

    geomMat = orientMat * geomMat;

    for (size_t i=0; i < xf.count; ++i) {
        xf_rt_values[i] = HdPrman_GfMatrixToRtMatrix(geomMat * xf.values[i]);
    }
    const riley::Transform xform = {
        unsigned(xf.count), xf_rt_values.data(), xf.times.data()};

    // Instance attributes.
    attrs.SetInteger(RixStr.k_lighting_mute, !sceneDelegate->GetVisible(id));

    // Light instance
    riley::CoordinateSystemList const coordsysList = {
                             unsigned(coordsysIds.size()), coordsysIds.data()};
    _instanceId = riley->CreateLightInstance(
        riley::UserId(stats::AddDataLocation(id.GetText()).GetValue()),
        riley::GeometryPrototypeId::InvalidId(), // no group
        riley::GeometryPrototypeId::InvalidId(), // no geo
        riley::MaterialId::InvalidId(), // no material
        _shaderId,
        coordsysList,
        xform,
        attrs);

    *dirtyBits = HdChangeTracker::Clean;
}

/* virtual */
HdDirtyBits
HdPrmanLight::GetInitialDirtyBitsMask() const
{
    return HdLight::AllDirty;
}

bool
HdPrmanLight::IsValid() const
{
    return _instanceId != riley::LightInstanceId::InvalidId();
}

PXR_NAMESPACE_CLOSE_SCOPE

