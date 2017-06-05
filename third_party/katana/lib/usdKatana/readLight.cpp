//
// Copyright 2016 Pixar
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
#include "pxr/pxr.h"
#include "usdKatana/attrMap.h"
#include "usdKatana/readLight.h"
#include "usdKatana/readPrim.h"
#include "usdKatana/readXformable.h"
#include "usdKatana/utils.h"

#include "pxr/base/tf/stringUtils.h"

#include "pxr/usd/usdLux/light.h"
#include "pxr/usd/usdLux/domeLight.h"
#include "pxr/usd/usdLux/distantLight.h"
#include "pxr/usd/usdLux/geometryLight.h"
#include "pxr/usd/usdLux/sphereLight.h"
#include "pxr/usd/usdLux/diskLight.h"
#include "pxr/usd/usdLux/rectLight.h"
#include "pxr/usd/usdLux/shapingAPI.h"
#include "pxr/usd/usdLux/shadowAPI.h"
#include "pxr/usd/usdLux/linkingAPI.h"
#include "pxr/usd/usdRi/lightAPI.h"
#include "pxr/usd/usdRi/textureAPI.h"
#include "pxr/usd/usdRi/pxrEnvDayLight.h"
#include "pxr/usd/usdRi/pxrAovLight.h"

#include <FnGeolibServices/FnAttributeFunctionUtil.h>
#include <FnLogging/FnLogging.h>
#include <pystring/pystring.h>

#include <stack>

PXR_NAMESPACE_OPEN_SCOPE

FnLogSetup("PxrUsdKatanaReadLight");

using std::string;
using std::vector;
using FnKat::GroupBuilder;


// Similar to katana's group builder, but takes in usd attributes.
struct _UsdBuilder {
    GroupBuilder &_builder;
    double _time;

    _UsdBuilder& Set(const char *kat_name, UsdAttribute attr) {
        VtValue val;
        if (attr.IsValid() && attr.HasAuthoredValueOpinion()
            && attr.Get(&val, _time)) {
            FnKat::Attribute kat_attr =
                PxrUsdKatanaUtils::ConvertVtValueToKatAttr( val,
                                        /* asShaderParam */ true,
                                        /* pathAsModel */ false,
                                        /* resolvePath */ false);
            _builder.set(kat_name, kat_attr);
        }
        return *this;
    }
};

void
PxrUsdKatanaReadLight(
        const UsdLuxLight& light,
        const PxrUsdKatanaUsdInPrivateData& data,
        PxrUsdKatanaAttrMap& attrs)
{
    const UsdPrim lightPrim = light.GetPrim();
    const SdfPath primPath = lightPrim.GetPath();
    const double currentTime = data.GetCurrentTime();

    GroupBuilder materialBuilder;
    GroupBuilder lightBuilder;
    _UsdBuilder usdBuilder = {lightBuilder, currentTime};

    // UsdLuxLight
    usdBuilder
        .Set("intensity", light.GetIntensityAttr())
        .Set("exposure", light.GetExposureAttr())
        .Set("diffuse", light.GetDiffuseAttr())
        .Set("specular", light.GetSpecularAttr())
        .Set("areaNormalize", light.GetNormalizeAttr())
        .Set("lightColor", light.GetColorAttr())
        .Set("enableTemperature", light.GetEnableColorTemperatureAttr())
        .Set("temperature", light.GetColorTemperatureAttr())
        ;

    if (UsdLuxShapingAPI l = UsdLuxShapingAPI(lightPrim)) {
        usdBuilder
            .Set("emissionFocus", l.GetShapingFocusAttr())
            .Set("emissionFocusTint", l.GetShapingFocusTintAttr())
            .Set("coneAngle", l.GetShapingConeAngleAttr())
            .Set("coneSoftness", l.GetShapingConeSoftnessAttr())
            .Set("iesProfile", l.GetShapingIesFileAttr())
            .Set("iesProfileScale", l.GetShapingIesAngleScaleAttr())
            ;
    }
    if (UsdLuxShadowAPI l = UsdLuxShadowAPI(lightPrim)) {
        usdBuilder
            .Set("enableShadows", l.GetShadowEnableAttr())
            .Set("shadowColor", l.GetShadowColorAttr())
            .Set("shadowDistance", l.GetShadowDistanceAttr())
            .Set("shadowFalloff", l.GetShadowFalloffAttr())
            .Set("shadowFalloffGamma", l.GetShadowFalloffGammaAttr())
            ;
    }
    if (UsdRiLightAPI l = UsdRiLightAPI(lightPrim)) {
        usdBuilder.Set("intensityNearDist", l.GetRiIntensityNearDistAttr());
        usdBuilder.Set("traceLightPaths", l.GetRiTraceLightPathsAttr());
        usdBuilder.Set("thinShadow", l.GetRiShadowThinShadowAttr());
        usdBuilder.Set("fixedSampleCount",
                       l.GetRiSamplingFixedSampleCountAttr());
        usdBuilder.Set("importanceMultiplier",
                       l.GetRiSamplingImportanceMultiplierAttr());
        usdBuilder.Set("lightGroup", l.GetRiLightGroupAttr());
    }

    if (UsdLuxSphereLight l = UsdLuxSphereLight(lightPrim)) {
        materialBuilder.set("prmanLightShader",
                            FnKat::StringAttribute("PxrSphereLight"));
        // TODO: extract scale, set as geometry.light.size
    }
    if (UsdLuxDiskLight l = UsdLuxDiskLight(lightPrim)) {
        materialBuilder.set("prmanLightShader",
                            FnKat::StringAttribute("PxrDiskLight"));
        // TODO: extract scale, set as geometry.light.size, width, height
    }
    if (UsdLuxRectLight l = UsdLuxRectLight(lightPrim)) {
        materialBuilder.set("prmanLightShader",
                            FnKat::StringAttribute("PxrRectLight"));
        usdBuilder.Set("lightColorMap", l.GetTextureFileAttr());
        if (UsdRiTextureAPI t = UsdRiTextureAPI(lightPrim)) {
            usdBuilder.Set("colorMapGamma", t.GetRiTextureGammaAttr());
            usdBuilder.Set("colorMapSaturation",
                           t.GetRiTextureSaturationAttr());
        }
        // TODO: extract scale, set as geometry.light.size, width, height
    }
    if (UsdLuxDistantLight l = UsdLuxDistantLight(lightPrim)) {
        materialBuilder.set("prmanLightShader",
                            FnKat::StringAttribute("PxrDistantLight"));
        usdBuilder.Set("angleExtent", l.GetAngleAttr());
    }
    if (UsdLuxGeometryLight l = UsdLuxGeometryLight(lightPrim)) {
        materialBuilder.set("prmanLightShader",
                            FnKat::StringAttribute("PxrMeshLight"));
        SdfPathVector geo;
        if (l.GetGeometryRel().GetForwardedTargets(&geo) && !geo.empty()) {
            if (geo.size() > 1) {
                FnLogWarn("Multiple geometry targest detected for "
                          "USD geometry light " << lightPrim.GetPath()
                          << "; using first only");
            }
            std::string kat_loc =
                PxrUsdKatanaUtils::ConvertUsdPathToKatLocation(geo[0], data);
            attrs.set("geometry.areaLightGeometrySource",
                      FnKat::StringAttribute(kat_loc));
        }
    }
    if (UsdLuxDomeLight l = UsdLuxDomeLight(lightPrim)) {
        materialBuilder.set("prmanLightShader",
                            FnKat::StringAttribute("PxrDomeLight"));
        usdBuilder.Set("lightColorMap", l.GetTextureFileAttr());
        // Note: The prman backend ignores texture:format since that is
        // specified inside the renderman texture file format.
        if (UsdRiTextureAPI t = UsdRiTextureAPI(lightPrim)) {
            usdBuilder.Set("colorMapGamma", t.GetRiTextureGammaAttr());
            usdBuilder.Set("colorMapSaturation",
                           t.GetRiTextureSaturationAttr());
        }
    }
    if (UsdRiPxrEnvDayLight l = UsdRiPxrEnvDayLight(lightPrim)) {
        materialBuilder.set("prmanLightShader",
                            FnKat::StringAttribute("PxrEnvDayLight"));
        usdBuilder.Set("day", l.GetDayAttr());
        usdBuilder.Set("haziness", l.GetHazinessAttr());
        usdBuilder.Set("hour", l.GetHourAttr());
        usdBuilder.Set("latitude", l.GetLatitudeAttr());
        usdBuilder.Set("longitude", l.GetLongitudeAttr());
        usdBuilder.Set("month", l.GetMonthAttr());
        usdBuilder.Set("skyTint", l.GetSkyTintAttr());
        usdBuilder.Set("sunDirection", l.GetSunDirectionAttr());
        usdBuilder.Set("sunSize", l.GetSunSizeAttr());
        usdBuilder.Set("sunTint", l.GetSunTintAttr());
        usdBuilder.Set("year", l.GetYearAttr());
        usdBuilder.Set("zone", l.GetZoneAttr());
    }
    if (UsdRiPxrAovLight l = UsdRiPxrAovLight(lightPrim)) {
        materialBuilder.set("prmanLightShader",
                            FnKat::StringAttribute("PxrAovLight"));
        usdBuilder.Set("aovName", l.GetAovNameAttr());
        usdBuilder.Set("inPrimaryHit", l.GetInPrimaryHitAttr());
        usdBuilder.Set("inReflection", l.GetInReflectionAttr());
        usdBuilder.Set("inRefraction", l.GetInRefractionAttr());
        usdBuilder.Set("invert", l.GetInvertAttr());
        usdBuilder.Set("onVolumeBoundaries", l.GetOnVolumeBoundariesAttr());
        usdBuilder.Set("useColor", l.GetUseColorAttr());
        usdBuilder.Set("useThroughput", l.GetUseThroughputAttr());
        // XXX aovSuffix, writeToDisk
    }

    // TODO: Pxr visibleInRefractionPath, cheapCaustics, maxDistance
    // surfSat, MsApprox

    // TODO portals
    // TODO UsdRi extensions
    
    // Gather prman statements
    FnKat::GroupBuilder prmanBuilder;
    PxrUsdKatanaReadPrimPrmanStatements(lightPrim, currentTime, prmanBuilder);
    attrs.set("prmanStatements", prmanBuilder.build());
    materialBuilder.set("prmanLightParams", lightBuilder.build());
    attrs.set("material", materialBuilder.build());
    PxrUsdKatanaReadXformable(light, data, attrs);
    attrs.set("type", FnKat::StringAttribute("light"));
}

PXR_NAMESPACE_CLOSE_SCOPE
