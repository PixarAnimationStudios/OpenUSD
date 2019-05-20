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
#include "pxr/usd/usdLux/cylinderLight.h"
#include "pxr/usd/usdLux/diskLight.h"
#include "pxr/usd/usdLux/rectLight.h"
#include "pxr/usd/usdLux/shapingAPI.h"
#include "pxr/usd/usdLux/shadowAPI.h"
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

// Convert USD radius to light.size (which acts like diameter)
static void
_SetLightSizeFromRadius(PxrUsdKatanaAttrMap &geomBuilder,
                        UsdAttribute radiusAttr,
                        UsdTimeCode time)
{
    VtValue radiusVal;
    radiusAttr.Get(&radiusVal, time);
    float radius = radiusVal.Get<float>();
    geomBuilder.set("light.size", FnKat::FloatAttribute(radius*2.0));
}

void
PxrUsdKatanaReadLight(
        const UsdLuxLight& light,
        const PxrUsdKatanaUsdInPrivateData& data,
        PxrUsdKatanaAttrMap& attrs)
{
    const UsdPrim lightPrim = light.GetPrim();
    const SdfPath primPath = lightPrim.GetPath();
    const UsdTimeCode currentTimeCode = data.GetCurrentTime();

    attrs.SetUSDTimeCode(currentTimeCode);
    PxrUsdKatanaAttrMap lightBuilder;
    lightBuilder.SetUSDTimeCode(currentTimeCode);
    PxrUsdKatanaAttrMap geomBuilder;
    geomBuilder.SetUSDTimeCode(currentTimeCode);
    FnKat::GroupBuilder materialBuilder;

    // UsdLuxLight
    lightBuilder
        .Set("intensity", light.GetIntensityAttr())
        .Set("exposure", light.GetExposureAttr())
        .Set("diffuse", light.GetDiffuseAttr())
        .Set("specular", light.GetSpecularAttr())
        .Set("areaNormalize", light.GetNormalizeAttr())
        .Set("lightColor", light.GetColorAttr())
        .Set("enableTemperature", light.GetEnableColorTemperatureAttr())
        .Set("temperature", light.GetColorTemperatureAttr())
        ;

    if (lightPrim) {
        UsdLuxShapingAPI shapingAPI(lightPrim);
        lightBuilder
            .Set("emissionFocus", shapingAPI.GetShapingFocusAttr())
            .Set("emissionFocusTint", shapingAPI.GetShapingFocusTintAttr())
            .Set("coneAngle", shapingAPI.GetShapingConeAngleAttr())
            .Set("coneSoftness", shapingAPI.GetShapingConeSoftnessAttr())
            .Set("iesProfile", shapingAPI.GetShapingIesFileAttr())
            .Set("iesProfileScale", shapingAPI.GetShapingIesAngleScaleAttr())
            ;

        UsdLuxShadowAPI shadowAPI(lightPrim);
        lightBuilder
            .Set("enableShadows", shadowAPI.GetShadowEnableAttr())
            .Set("shadowColor", shadowAPI.GetShadowColorAttr())
            .Set("shadowDistance", shadowAPI.GetShadowDistanceAttr())
            .Set("shadowFalloff", shadowAPI.GetShadowFalloffAttr())
            .Set("shadowFalloffGamma", shadowAPI.GetShadowFalloffGammaAttr())
            ;

        UsdRiLightAPI riLightAPI(lightPrim);
        lightBuilder
            .Set("intensityNearDist", riLightAPI.GetRiIntensityNearDistAttr())
            .Set("traceLightPaths", riLightAPI.GetRiTraceLightPathsAttr())
            .Set("thinShadow", riLightAPI.GetRiShadowThinShadowAttr())
            .Set("fixedSampleCount",
                        riLightAPI.GetRiSamplingFixedSampleCountAttr())
            .Set("importanceMultiplier",
                        riLightAPI.GetRiSamplingImportanceMultiplierAttr())
            .Set("lightGroup", riLightAPI.GetRiLightGroupAttr())
            ;
    }

    if (UsdLuxSphereLight l = UsdLuxSphereLight(lightPrim)) {
        _SetLightSizeFromRadius(geomBuilder, l.GetRadiusAttr(),
                                currentTimeCode);
        materialBuilder.set("prmanLightShader",
                            FnKat::StringAttribute("PxrSphereLight"));
    }
    if (UsdLuxDiskLight l = UsdLuxDiskLight(lightPrim)) {
        _SetLightSizeFromRadius(geomBuilder, l.GetRadiusAttr(),
                                currentTimeCode);
        materialBuilder.set("prmanLightShader",
                            FnKat::StringAttribute("PxrDiskLight"));
    }
    if (UsdLuxCylinderLight l = UsdLuxCylinderLight(lightPrim)) {
        _SetLightSizeFromRadius(geomBuilder, l.GetRadiusAttr(),
                                currentTimeCode);
        geomBuilder.Set("light.width", l.GetLengthAttr());
        materialBuilder.set("prmanLightShader",
                            FnKat::StringAttribute("PxrCylinderLight"));
    }
    if (UsdLuxRectLight l = UsdLuxRectLight(lightPrim)) {
        geomBuilder.Set("light.width", l.GetWidthAttr());
        geomBuilder.Set("light.height", l.GetHeightAttr());
        materialBuilder.set("prmanLightShader",
                            FnKat::StringAttribute("PxrRectLight"));
        lightBuilder.Set("lightColorMap", l.GetTextureFileAttr());
        UsdRiTextureAPI textureAPI(lightPrim);
        lightBuilder
            .Set("colorMapGamma", textureAPI.GetRiTextureGammaAttr())
            .Set("colorMapSaturation", textureAPI.GetRiTextureSaturationAttr())
            ;
    }
    if (UsdLuxDistantLight l = UsdLuxDistantLight(lightPrim)) {
        materialBuilder.set("prmanLightShader",
                            FnKat::StringAttribute("PxrDistantLight"));
        lightBuilder.Set("angleExtent", l.GetAngleAttr());
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
            geomBuilder.set("areaLightGeometrySource",
                      FnKat::StringAttribute(kat_loc));
        }
    }
    if (UsdLuxDomeLight l = UsdLuxDomeLight(lightPrim)) {
        materialBuilder.set("prmanLightShader",
                            FnKat::StringAttribute("PxrDomeLight"));
        lightBuilder.Set("lightColorMap", l.GetTextureFileAttr());
        // Note: The prman backend ignores texture:format since that is
        // specified inside the renderman texture file format.
        UsdRiTextureAPI textureAPI(lightPrim);
        lightBuilder
            .Set("colorMapGamma", textureAPI.GetRiTextureGammaAttr())
            .Set("colorMapSaturation", textureAPI.GetRiTextureSaturationAttr())
            ;
    }
    if (UsdRiPxrEnvDayLight l = UsdRiPxrEnvDayLight(lightPrim)) {
        materialBuilder.set("prmanLightShader",
                            FnKat::StringAttribute("PxrEnvDayLight"));
        lightBuilder
            .Set("day", l.GetDayAttr())
            .Set("haziness", l.GetHazinessAttr())
            .Set("hour", l.GetHourAttr())
            .Set("latitude", l.GetLatitudeAttr())
            .Set("longitude", l.GetLongitudeAttr())
            .Set("month", l.GetMonthAttr())
            .Set("skyTint", l.GetSkyTintAttr())
            .Set("sunDirection", l.GetSunDirectionAttr())
            .Set("sunSize", l.GetSunSizeAttr())
            .Set("sunTint", l.GetSunTintAttr())
            .Set("year", l.GetYearAttr())
            .Set("zone", l.GetZoneAttr())
            ;
    }
    if (UsdRiPxrAovLight l = UsdRiPxrAovLight(lightPrim)) {
        materialBuilder.set("prmanLightShader",
                            FnKat::StringAttribute("PxrAovLight"));
        lightBuilder
            .Set("aovName", l.GetAovNameAttr())
            .Set("inPrimaryHit", l.GetInPrimaryHitAttr())
            .Set("inReflection", l.GetInReflectionAttr())
            .Set("inRefraction", l.GetInRefractionAttr())
            .Set("invert", l.GetInvertAttr())
            .Set("onVolumeBoundaries", l.GetOnVolumeBoundariesAttr())
            .Set("useColor", l.GetUseColorAttr())
            .Set("useThroughput", l.GetUseThroughputAttr())
            ;
        // XXX aovSuffix, writeToDisk
    }

    // TODO: Pxr visibleInRefractionPath, cheapCaustics, maxDistance
    // surfSat, MsApprox

    // TODO portals
    // TODO UsdRi extensions
    
    // Gather prman statements
    FnKat::GroupBuilder prmanBuilder;
    PxrUsdKatanaReadPrimPrmanStatements(lightPrim, currentTimeCode.GetValue(),
                                        prmanBuilder);
    attrs.set("prmanStatements", prmanBuilder.build());

    materialBuilder.set("prmanLightParams", lightBuilder.build());
    attrs.set("material", materialBuilder.build());
    attrs.set("geometry", geomBuilder.build());

    PxrUsdKatanaReadXformable(light, data, attrs);
    attrs.set("type", FnKat::StringAttribute("light"));
}

PXR_NAMESPACE_CLOSE_SCOPE
