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
#include "hdPrman/context.h"
#include "hdPrman/debugCodes.h"
#include "hdPrman/renderParam.h"
#include "hdPrman/rixStrings.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hf/diagnostic.h"

#include "RixParamList.h"
#include "RixShadingUtils.h"

#include "hdPrman/lightFilterUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // tokens for RenderMan-specific light parameters
    ((cheapCaustics,                "ri:light:cheapCaustics"))
    ((cheapCausticsExcludeGroup,    "ri:light:cheapCausticsExcludeGroup"))
    ((fixedSampleCount,             "ri:light:fixedSampleCount"))
    ((importanceMultiplier,         "ri:light:importanceMultiplier"))
    ((intensityNearDist,            "ri:light:intensityNearDist"))
    ((thinShadow,                   "ri:light:thinShadow"))
    ((traceLightPaths,              "ri:light:traceLightPaths"))
    ((visibleInRefractionPath,      "ri:light:visibleInRefractionPath"))
    ((lightGroup,                   "ri:light:lightGroup"))
    ((colorMapGamma,                "ri:light:colorMapGamma"))
    ((colorMapSaturation,           "ri:light:colorMapSaturation"))
);

HdPrmanLight::HdPrmanLight(SdfPath const& id, TfToken const& lightType)
    : HdLight(id)
    , _hdLightType(lightType)
    , _shaderId(riley::LightShaderId::k_InvalidId)
    , _instanceId(riley::LightInstanceId::k_InvalidId)
{
    /* NOTHING */
}

HdPrmanLight::~HdPrmanLight()
{
}

void
HdPrmanLight::Finalize(HdRenderParam *renderParam)
{
    HdPrman_Context *context =
        static_cast<HdPrman_RenderParam*>(renderParam)->AcquireContext();
    _ResetLight(context);
}

void
HdPrmanLight::_ResetLight(HdPrman_Context *context)
{
    if (!_lightLink.IsEmpty()) {
        context->DecrementLightLinkCount(_lightLink);
        _lightLink = TfToken();
    }
    if (!_lightFilterPaths.empty()) {
        for (const SdfPath &filterPath: _lightFilterPaths)
            context->DecrementLightFilterCount(TfToken(filterPath.GetText()));
        _lightFilterPaths.clear();
    }

    riley::Riley *riley = context->riley;
    if (_instanceId != riley::LightInstanceId::k_InvalidId) {
        riley->DeleteLightInstance(
            riley::GeometryMasterId::k_InvalidId,
            _instanceId);
        _instanceId = riley::LightInstanceId::k_InvalidId;
    }
    if (_shaderId != riley::LightShaderId::k_InvalidId) {
        riley->DeleteLightShader(_shaderId);
        _shaderId = riley::LightShaderId::k_InvalidId;
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

/* virtual */
void
HdPrmanLight::Sync(HdSceneDelegate *sceneDelegate,
                   HdRenderParam   *renderParam,
                   HdDirtyBits     *dirtyBits)
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

    HdPrman_Context *context =
        static_cast<HdPrman_RenderParam*>(renderParam)->AcquireContext();

    SdfPath id = GetId();

    RixRileyManager *mgr = context->mgr;
    riley::Riley *riley = context->riley;

    // Some lights have parameters that scale the size of the light.
    GfVec3d geomScale(1.0f);

    // For simplicity just re-create the light.  In the future we may
    // want to consider adding a path to use the Modify() API in Riley.
    _ResetLight(context);

    // Attributes.
    RixParamList *attrs = context->ConvertAttributes(sceneDelegate, id);

    // Light shader.
    RixParamList *params = mgr->CreateRixParamList();
    RtUString rileyTypeName;

    // UsdLuxLight base parameters
    {
        // intensity
        VtValue intensity =
            sceneDelegate->GetLightParamValue(id, HdLightTokens->intensity);
        if (intensity.IsHolding<float>()) {
            params->SetFloat(us_intensity, intensity.UncheckedGet<float>());
        }

        // exposure
        VtValue exposure =
            sceneDelegate->GetLightParamValue(id, HdLightTokens->exposure);
        if (exposure.IsHolding<float>()) {
            params->SetFloat(us_exposure, exposure.UncheckedGet<float>());
        }

        // color -> lightColor
        VtValue lightColor =
            sceneDelegate->GetLightParamValue(id, HdLightTokens->color);
        if (lightColor.IsHolding<GfVec3f>()) {
            GfVec3f v = lightColor.UncheckedGet<GfVec3f>();
            params->SetColor(us_lightColor, RtColorRGB(v[0], v[1], v[2]));
        }

        // enableColorTemperature -> enableTemperature
        VtValue enableTemperature =
            sceneDelegate->GetLightParamValue(id,
                HdLightTokens->enableColorTemperature);
        if (enableTemperature.IsHolding<bool>()) {
            int v = enableTemperature.UncheckedGet<bool>();
            params->SetInteger(us_enableTemperature, v);
        }

        // temperature
        VtValue temperature =
            sceneDelegate->GetLightParamValue(id, 
                HdLightTokens->colorTemperature);
        if (temperature.IsHolding<float>()) {
            params->SetFloat(us_temperature, temperature.UncheckedGet<float>());
        }

        // diffuse
        VtValue diffuse =
            sceneDelegate->GetLightParamValue(id, HdLightTokens->diffuse);
        if (diffuse.IsHolding<float>()) {
            params->SetFloat(us_diffuse, diffuse.UncheckedGet<float>());
        }

        // specular
        VtValue specular =
            sceneDelegate->GetLightParamValue(id, HdLightTokens->specular);
        if (specular.IsHolding<float>()) {
            params->SetFloat(us_specular, specular.UncheckedGet<float>());
        }

        // normalize -> areaNormalize
        // (Avoid unused param warnings for light types that don't have this)
        if (_hdLightType != HdPrimTypeTokens->domeLight) {
            VtValue normalize =
                sceneDelegate->GetLightParamValue(id, HdLightTokens->normalize);
            if (normalize.IsHolding<bool>()) {
                int v = normalize.UncheckedGet<bool>();
                params->SetInteger(us_areaNormalize, v);
            }
        }
    }

    // UsdLuxShapingAPI
    {
        if (_hdLightType != HdPrimTypeTokens->domeLight) {
            VtValue shapingFocus =
                sceneDelegate->GetLightParamValue(id,
                                                  HdLightTokens->shapingFocus);
            if (shapingFocus.IsHolding<float>()) {
                params->SetFloat(us_emissionFocus,
                                 shapingFocus.UncheckedGet<float>());
            }

            // XXX -- emissionFocusNormalize is missing here

            VtValue shapingFocusTint =
                sceneDelegate->GetLightParamValue(id, 
                    HdLightTokens->shapingFocusTint);
            if (shapingFocusTint.IsHolding<GfVec3f>()) {
                GfVec3f v = shapingFocusTint.UncheckedGet<GfVec3f>();
                params->SetColor(us_emissionFocusTint,
                                 RtColorRGB(v[0], v[1], v[2]));
            }
        }

        // ies is only supported on rect, disk, cylinder and sphere light.
        // cone angle only supported on rect, disk, cylinder and sphere lights.
        // XXX -- fix for mesh/geometry light when it comes online
        if (_hdLightType == HdPrimTypeTokens->rectLight ||
            _hdLightType == HdPrimTypeTokens->diskLight ||
            _hdLightType == HdPrimTypeTokens->cylinderLight ||
            _hdLightType == HdPrimTypeTokens->sphereLight) {

            VtValue shapingConeAngle =
                sceneDelegate->GetLightParamValue(id, 
                    HdLightTokens->shapingConeAngle);
            if (shapingConeAngle.IsHolding<float>()) {
                params->SetFloat(us_coneAngle,
                                 shapingConeAngle.UncheckedGet<float>());
            }

            VtValue shapingConeSoftness =
                sceneDelegate->GetLightParamValue(id, 
                    HdLightTokens->shapingConeSoftness);
            if (shapingConeSoftness.IsHolding<float>()) {
                params->SetFloat(us_coneSoftness,
                                 shapingConeSoftness.UncheckedGet<float>());
            }

            VtValue shapingIesFile =
                sceneDelegate->GetLightParamValue(id, 
                    HdLightTokens->shapingIesFile);
            if (shapingIesFile.IsHolding<SdfAssetPath>()) {
                SdfAssetPath ap = shapingIesFile.UncheckedGet<SdfAssetPath>();
                params->SetString(us_iesProfile, _RtStringFromSdfAssetPath(ap));
            }

            VtValue shapingIesAngleScale =
                sceneDelegate->GetLightParamValue(id,
                    HdLightTokens->shapingIesAngleScale);
            if (shapingIesAngleScale.IsHolding<float>()) {
                params->SetFloat(us_iesProfileScale,
                                 shapingIesAngleScale.UncheckedGet<float>());
            }

            VtValue shapingIesNormalize =
                sceneDelegate->GetLightParamValue(id,
                    HdLightTokens->shapingIesNormalize);
            if (shapingIesNormalize.IsHolding<bool>()) {
                params->SetInteger(us_iesProfileNormalize,
                                   shapingIesNormalize.UncheckedGet<bool>());
            }
        }
    }

    // UsdLuxShadowAPI -- includes shadow linking
    {
        VtValue shadowEnable =
            sceneDelegate->GetLightParamValue(id, HdLightTokens->shadowEnable);
        if (shadowEnable.IsHolding<bool>()) {
            params->SetInteger(us_enableShadows,
                               shadowEnable.UncheckedGet<bool>());
        }

        VtValue shadowColor =
            sceneDelegate->GetLightParamValue(id, HdLightTokens->shadowColor);
        if (shadowColor.IsHolding<GfVec3f>()) {
            GfVec3f v = shadowColor.UncheckedGet<GfVec3f>();
            params->SetColor(us_shadowColor, RtColorRGB(v[0], v[1], v[2]));
        }

        VtValue shadowDistance =
            sceneDelegate->GetLightParamValue(id, 
                HdLightTokens->shadowDistance);
        if (shadowDistance.IsHolding<float>()) {
            params->SetFloat(us_shadowDistance,
                             shadowDistance.UncheckedGet<float>());
        }

        VtValue shadowFalloff =
            sceneDelegate->GetLightParamValue(id, HdLightTokens->shadowFalloff);
        if (shadowFalloff.IsHolding<float>()) {
            params->SetFloat(us_shadowFalloff,
                             shadowFalloff.UncheckedGet<float>());
        }

        VtValue shadowFalloffGamma =
            sceneDelegate->GetLightParamValue(id, 
                HdLightTokens->shadowFalloffGamma);
        if (shadowFalloffGamma.IsHolding<float>()) {
            params->SetFloat(us_shadowFalloffGamma,
                             shadowFalloffGamma.UncheckedGet<float>());
        }

        VtValue shadowLinkVal =
            sceneDelegate->GetLightParamValue(id, HdTokens->shadowLink);
        if (shadowLinkVal.IsHolding<TfToken>()) {
            TfToken shadowLink = shadowLinkVal.UncheckedGet<TfToken>();
            if (!shadowLink.IsEmpty()) {
                params->SetString(us_shadowSubset,
                                  RtUString(shadowLink.GetText()));
                TF_DEBUG(HDPRMAN_LIGHT_LINKING)
                    .Msg("HdPrman: Light <%s> shadowSubset \"%s\"\n",
                         id.GetText(), shadowLink.GetText());
            }
        }
    }

    // extra RenderMan parameters - "ri:light"
    {
        VtValue cheapCaustics =
            sceneDelegate->GetLightParamValue(id, _tokens->cheapCaustics);
        if (cheapCaustics.IsHolding<int>()) {
            params->SetInteger(us_cheapCaustics,
                               cheapCaustics.UncheckedGet<int>());
        }

        VtValue cheapCausticsExcludeGroupVal =
            sceneDelegate->GetLightParamValue(id, 
                _tokens->cheapCausticsExcludeGroup);
        if (cheapCausticsExcludeGroupVal.IsHolding<TfToken>()) {
            TfToken cheapCausticsExcludeGroup = 
                cheapCausticsExcludeGroupVal.UncheckedGet<TfToken>();
            if (!cheapCausticsExcludeGroup.IsEmpty()) {
                params->SetString(us_cheapCausticsExcludeGroup,
                                RtUString(cheapCausticsExcludeGroup.GetText()));
            }
        }

        VtValue fixedSampleCount =
            sceneDelegate->GetLightParamValue(id, _tokens->fixedSampleCount);
        if (fixedSampleCount.IsHolding<int>()) {
            params->SetInteger(us_fixedSampleCount,
                               fixedSampleCount.UncheckedGet<int>());
        }

        VtValue importanceMultiplier =
            sceneDelegate->GetLightParamValue(id, 
                _tokens->importanceMultiplier);
        if (importanceMultiplier.IsHolding<float>()) {
            params->SetFloat(us_importanceMultiplier,
                               importanceMultiplier.UncheckedGet<float>());
        }

        VtValue intensityNearDist =
            sceneDelegate->GetLightParamValue(id, _tokens->intensityNearDist);
        if (intensityNearDist.IsHolding<float>()) {
            params->SetFloat(us_intensityNearDist,
                               intensityNearDist.UncheckedGet<float>());
        }

        VtValue thinShadow =
            sceneDelegate->GetLightParamValue(id, _tokens->thinShadow);
        if (thinShadow.IsHolding<int>()) {
            params->SetInteger(us_thinShadow, thinShadow.UncheckedGet<int>());
        }

        VtValue traceLightPaths =
            sceneDelegate->GetLightParamValue(id, _tokens->traceLightPaths);
        if (traceLightPaths.IsHolding<int>()) {
            params->SetInteger(us_traceLightPaths,
                               traceLightPaths.UncheckedGet<int>());
        }

        VtValue visibleInRefractionPath =
            sceneDelegate->GetLightParamValue(id, 
                _tokens->visibleInRefractionPath);
        if (visibleInRefractionPath.IsHolding<int>()) {
            params->SetInteger(us_visibleInRefractionPath,
                               visibleInRefractionPath.UncheckedGet<int>());
        }

        VtValue lightGroupVal =
            sceneDelegate->GetLightParamValue(id, _tokens->lightGroup);
        if (lightGroupVal.IsHolding<TfToken>()) {
            TfToken lightGroup = lightGroupVal.UncheckedGet<TfToken>();
            if (!lightGroup.IsEmpty()) {
                params->SetString(us_lightGroup,
                                  RtUString(lightGroup.GetText()));
            }
        }
    }


    TF_DEBUG(HDPRMAN_LIGHT_LIST)
        .Msg("HdPrman: Light <%s> lightType \"%s\"\n",
             id.GetText(), _hdLightType.GetText());

    // Type-specific parameters
    bool supportsLightColorMap = false;
    if (_hdLightType == HdPrimTypeTokens->domeLight) {
        rileyTypeName = us_PxrDomeLight;
        supportsLightColorMap = true;
    } else if (_hdLightType == HdPrimTypeTokens->rectLight) {
        rileyTypeName = us_PxrRectLight;
        supportsLightColorMap = true;

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
    } else if (_hdLightType == HdPrimTypeTokens->diskLight) {
        rileyTypeName = us_PxrDiskLight;

        // radius (XY only, default 0.5)
        VtValue radius = sceneDelegate->GetLightParamValue(id, 
            HdLightTokens->radius);
        if (radius.IsHolding<float>()) {
            geomScale[0] *= radius.UncheckedGet<float>() / 0.5;
            geomScale[1] *= radius.UncheckedGet<float>() / 0.5;
        }
    } else if (_hdLightType == HdPrimTypeTokens->cylinderLight) {
        rileyTypeName = us_PxrCylinderLight;

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
    } else if (_hdLightType == HdPrimTypeTokens->sphereLight) {
        rileyTypeName = us_PxrSphereLight;

        // radius (XYZ, default 0.5)
        VtValue radius = sceneDelegate->GetLightParamValue(id, 
            HdLightTokens->radius);
        if (radius.IsHolding<float>()) {
            geomScale *= radius.UncheckedGet<float>() / 0.5;
        }
    } else if (_hdLightType == HdPrimTypeTokens->distantLight) {
        rileyTypeName = us_PxrDistantLight;

        VtValue angle =
            sceneDelegate->GetLightParamValue(id, HdLightTokens->angle);
        if (angle.IsHolding<float>()) {
            params->SetFloat(us_angleExtent, angle.UncheckedGet<float>());
        }
    }
    
    if (supportsLightColorMap) {
        // textureFile -> lightColorMap
        VtValue textureFile = sceneDelegate->GetLightParamValue(id,
            HdLightTokens->textureFile);
        if (textureFile.IsHolding<SdfAssetPath>()) {
            SdfAssetPath ap = textureFile.UncheckedGet<SdfAssetPath>();
            params->SetString(us_lightColorMap, _RtStringFromSdfAssetPath(ap));
        }

        VtValue colorMapGamma =
            sceneDelegate->GetLightParamValue(id, _tokens->colorMapGamma);
        if (colorMapGamma.IsHolding<GfVec3f>()) {
            GfVec3f v = colorMapGamma.UncheckedGet<GfVec3f>();
            params->SetVector(us_colorMapGamma, RtVector3(v[0], v[1], v[2]));
        }

        VtValue colorMapSaturation =
            sceneDelegate->GetLightParamValue(id, _tokens->colorMapSaturation);
        if (colorMapSaturation.IsHolding<float>()) {
            params->SetFloat(us_colorMapSaturation,
                               colorMapSaturation.UncheckedGet<float>());
        }
    }

    // Light linking
    {
        VtValue val =
            sceneDelegate->GetLightParamValue(id, HdTokens->lightLink);
        if (val.IsHolding<TfToken>()) {
            _lightLink = val.UncheckedGet<TfToken>();
        }
        
        if (!_lightLink.IsEmpty()) {
            context->IncrementLightLinkCount(_lightLink);
            // For lights to link geometry, the lights must
            // be assigned a grouping membership, and the
            // geometry must subscribe to that grouping.
            attrs->SetString(RixStr.k_grouping_membership,
                                RtUString(_lightLink.GetText()));
            TF_DEBUG(HDPRMAN_LIGHT_LINKING)
                .Msg("HdPrman: Light <%s> grouping membership \"%s\"\n",
                        id.GetText(), _lightLink.GetText());
        } else {
            // Default light group
            attrs->SetString(RixStr.k_grouping_membership, us_default);
            TF_DEBUG(HDPRMAN_LIGHT_LINKING)
                .Msg("HdPrman: Light <%s> grouping membership \"default\"\n",
                     id.GetText());
        }
    }

    // filters
    riley::ShadingNode *filterNodes = nullptr;
    int nFilterNodes = 0;
    std::vector<riley::CoordinateSystemId> coordsysIds;
    {
        VtValue val =
            sceneDelegate->GetLightParamValue(id, HdTokens->filters);
        if (val.IsHolding<SdfPathVector>()) {
            _lightFilterPaths = val.UncheckedGet<SdfPathVector>();
            if (!_lightFilterPaths.empty()) {
                int maxFilters = _lightFilterPaths.size();
                if (maxFilters > 1)
                    maxFilters += 1;  // extra for the combiner filter
                filterNodes = (riley::ShadingNode *)
                                malloc(sizeof(riley::ShadingNode) * maxFilters);
                memset(filterNodes, 0, sizeof(riley::ShadingNode) * maxFilters);

                for (SdfPath &filterPath: _lightFilterPaths) {
                    TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                        .Msg("HdPrman: Light <%s> filter \"%s\" path \"%s\"\n",
                             id.GetText(), filterPath.GetName().c_str(),
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

                    VtValue tval = sceneDelegate->GetLightParamValue(filterPath,
                                                TfToken("lightFilterType"));
                    if (!tval.IsHolding<TfToken>()) {
                        TF_DEBUG(HDPRMAN_LIGHT_FILTER_LINKING)
                            .Msg("  filter type unknown\n");
                        continue;
                    }
                    TfToken filterType = tval.UncheckedGet<TfToken>();

                    context->IncrementLightFilterCount(
                                                TfToken(filterPath.GetText()));

                    riley::ShadingNode *filter = &filterNodes[nFilterNodes];
                    filter->type = riley::ShadingNode::k_LightFilter;
                    filter->handle = RtUString(filterPath.GetName().c_str());

                    if (HdPrmanLightFilterPopulateParams(filter, filterPath,
                                    filterType, &coordsysIds,
                                    sceneDelegate, mgr, riley, rileyTypeName))
                        nFilterNodes++;
                }
                if (nFilterNodes > 1) {
                    // More than 1 light filter requires a combiner to blend
                    // their results
                    riley::ShadingNode *filter = &filterNodes[nFilterNodes];
                    filter->type = riley::ShadingNode::k_LightFilter;
                    filter->name = RtUString("PxrCombinerLightFilter");
                    filter->handle = RtUString("terminal.Lightfilter");
                    RixParamList *filter_params = mgr->CreateRixParamList();
                    std::vector<RtUString> sa;
                    for(int i = 0; i < nFilterNodes; i++) {
                        sa.push_back(filterNodes[i].handle);
                    }
                    // XXX -- assume mult for now
                    filter_params->ReferenceLightFilterArray(RtUString("mult"),
                                    (const RtUString* const&)&sa[0], sa.size());
                    filter->params = filter_params;
                    nFilterNodes++;
                }
            }
        }
    }

    // TODO: portals

    riley::ShadingNode lightNode {
        riley::ShadingNode::k_Light,
        rileyTypeName,
        RtUString(id.GetText()),
        params
    };
    _shaderId = riley->CreateLightShader(&lightNode, 1,
                                         filterNodes, nFilterNodes);
    if (filterNodes) {
        for(int i = 0; i < nFilterNodes; i++) {
            if (filterNodes[i].params)
                mgr->DestroyRixParamList((RixParamList *)filterNodes[i].params);
        }
        free(filterNodes);
    }
    mgr->DestroyRixParamList(params);

    // Sample transform
    HdTimeSampleArray<GfMatrix4d, HDPRMAN_MAX_TIME_SAMPLES> xf;
    sceneDelegate->SampleTransform(id, &xf);
    
    TfSmallVector<RtMatrix4x4, HDPRMAN_MAX_TIME_SAMPLES> 
        xf_rt_values(xf.count);

    GfMatrix4d geomMat(1.0);
    geomMat.SetScale(geomScale);

    // adjust orientation to make prman match the USD spec
    // TODO: Add another orientMat for PxrEnvDayLight when supported
    GfMatrix4d orientMat(1.0);
    if (rileyTypeName == us_PxrDomeLight)
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
    attrs->SetInteger(RixStr.k_lighting_mute, !sceneDelegate->GetVisible(id));

    // Light instance
    riley::ScopedCoordinateSystem const coordsys = {
                             unsigned(coordsysIds.size()), coordsysIds.data()};
    _instanceId = riley->CreateLightInstance(
        riley::GeometryMasterId::k_InvalidId, // no group
        riley::GeometryMasterId::k_InvalidId, // no geo
        riley::MaterialId::k_InvalidId, // no material
        _shaderId,
        coordsys,
        xform,
        *attrs);
    mgr->DestroyRixParamList(attrs);

    *dirtyBits = HdChangeTracker::Clean;
}

/* virtual */
HdDirtyBits
HdPrmanLight::GetInitialDirtyBitsMask() const
{
    return HdChangeTracker::AllDirty;
}

bool
HdPrmanLight::IsValid() const
{
    return _instanceId != riley::LightInstanceId::k_InvalidId;
}

PXR_NAMESPACE_CLOSE_SCOPE

