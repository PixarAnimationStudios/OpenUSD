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
#include "pxr/usd/usdLux/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdLuxTokensType::UsdLuxTokensType() :
    angle("angle"),
    angular("angular"),
    automatic("automatic"),
    color("color"),
    colorTemperature("colorTemperature"),
    consumeAndContinue("consumeAndContinue"),
    consumeAndHalt("consumeAndHalt"),
    cubeMapVerticalCross("cubeMapVerticalCross"),
    diffuse("diffuse"),
    enableColorTemperature("enableColorTemperature"),
    exposure("exposure"),
    filters("filters"),
    geometry("geometry"),
    height("height"),
    ignore("ignore"),
    intensity("intensity"),
    latlong("latlong"),
    lightList("lightList"),
    lightListCacheBehavior("lightList:cacheBehavior"),
    mirroredBall("mirroredBall"),
    normalize("normalize"),
    portals("portals"),
    radius("radius"),
    shadowColor("shadow:color"),
    shadowDistance("shadow:distance"),
    shadowEnable("shadow:enable"),
    shadowExclude("shadow:exclude"),
    shadowFalloff("shadow:falloff"),
    shadowFalloffGamma("shadow:falloffGamma"),
    shadowInclude("shadow:include"),
    shapingConeAngle("shaping:cone:angle"),
    shapingConeSoftness("shaping:cone:softness"),
    shapingFocus("shaping:focus"),
    shapingFocusTint("shaping:focusTint"),
    shapingIesAngleScale("shaping:ies:angleScale"),
    shapingIesFile("shaping:ies:file"),
    specular("specular"),
    textureFile("texture:file"),
    textureFormat("texture:format"),
    width("width"),
    allTokens({
        angle,
        angular,
        automatic,
        color,
        colorTemperature,
        consumeAndContinue,
        consumeAndHalt,
        cubeMapVerticalCross,
        diffuse,
        enableColorTemperature,
        exposure,
        filters,
        geometry,
        height,
        ignore,
        intensity,
        latlong,
        lightList,
        lightListCacheBehavior,
        mirroredBall,
        normalize,
        portals,
        radius,
        shadowColor,
        shadowDistance,
        shadowEnable,
        shadowExclude,
        shadowFalloff,
        shadowFalloffGamma,
        shadowInclude,
        shapingConeAngle,
        shapingConeSoftness,
        shapingFocus,
        shapingFocusTint,
        shapingIesAngleScale,
        shapingIesFile,
        specular,
        textureFile,
        textureFormat,
        width
    })
{
}

TfStaticData<UsdLuxTokensType> UsdLuxTokens;

PXR_NAMESPACE_CLOSE_SCOPE
