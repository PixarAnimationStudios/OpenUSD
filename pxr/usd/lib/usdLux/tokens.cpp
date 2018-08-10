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
    angle("angle", TfToken::Immortal),
    angular("angular", TfToken::Immortal),
    automatic("automatic", TfToken::Immortal),
    collectionFilterLinkIncludeRoot("collection:filterLink:includeRoot", TfToken::Immortal),
    collectionLightLinkIncludeRoot("collection:lightLink:includeRoot", TfToken::Immortal),
    collectionShadowLinkIncludeRoot("collection:shadowLink:includeRoot", TfToken::Immortal),
    color("color", TfToken::Immortal),
    colorTemperature("colorTemperature", TfToken::Immortal),
    consumeAndContinue("consumeAndContinue", TfToken::Immortal),
    consumeAndHalt("consumeAndHalt", TfToken::Immortal),
    cubeMapVerticalCross("cubeMapVerticalCross", TfToken::Immortal),
    diffuse("diffuse", TfToken::Immortal),
    enableColorTemperature("enableColorTemperature", TfToken::Immortal),
    exposure("exposure", TfToken::Immortal),
    filterLink("filterLink", TfToken::Immortal),
    filters("filters", TfToken::Immortal),
    geometry("geometry", TfToken::Immortal),
    height("height", TfToken::Immortal),
    ignore("ignore", TfToken::Immortal),
    intensity("intensity", TfToken::Immortal),
    latlong("latlong", TfToken::Immortal),
    length("length", TfToken::Immortal),
    lightLink("lightLink", TfToken::Immortal),
    lightList("lightList", TfToken::Immortal),
    lightListCacheBehavior("lightList:cacheBehavior", TfToken::Immortal),
    mirroredBall("mirroredBall", TfToken::Immortal),
    normalize("normalize", TfToken::Immortal),
    portals("portals", TfToken::Immortal),
    radius("radius", TfToken::Immortal),
    shadowColor("shadow:color", TfToken::Immortal),
    shadowDistance("shadow:distance", TfToken::Immortal),
    shadowEnable("shadow:enable", TfToken::Immortal),
    shadowExclude("shadow:exclude", TfToken::Immortal),
    shadowFalloff("shadow:falloff", TfToken::Immortal),
    shadowFalloffGamma("shadow:falloffGamma", TfToken::Immortal),
    shadowInclude("shadow:include", TfToken::Immortal),
    shadowLink("shadowLink", TfToken::Immortal),
    shapingConeAngle("shaping:cone:angle", TfToken::Immortal),
    shapingConeSoftness("shaping:cone:softness", TfToken::Immortal),
    shapingFocus("shaping:focus", TfToken::Immortal),
    shapingFocusTint("shaping:focusTint", TfToken::Immortal),
    shapingIesAngleScale("shaping:ies:angleScale", TfToken::Immortal),
    shapingIesFile("shaping:ies:file", TfToken::Immortal),
    specular("specular", TfToken::Immortal),
    textureFile("texture:file", TfToken::Immortal),
    textureFormat("texture:format", TfToken::Immortal),
    treatAsLine("treatAsLine", TfToken::Immortal),
    treatAsPoint("treatAsPoint", TfToken::Immortal),
    width("width", TfToken::Immortal),
    allTokens({
        angle,
        angular,
        automatic,
        collectionFilterLinkIncludeRoot,
        collectionLightLinkIncludeRoot,
        collectionShadowLinkIncludeRoot,
        color,
        colorTemperature,
        consumeAndContinue,
        consumeAndHalt,
        cubeMapVerticalCross,
        diffuse,
        enableColorTemperature,
        exposure,
        filterLink,
        filters,
        geometry,
        height,
        ignore,
        intensity,
        latlong,
        length,
        lightLink,
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
        shadowLink,
        shapingConeAngle,
        shapingConeSoftness,
        shapingFocus,
        shapingFocusTint,
        shapingIesAngleScale,
        shapingIesFile,
        specular,
        textureFile,
        textureFormat,
        treatAsLine,
        treatAsPoint,
        width
    })
{
}

TfStaticData<UsdLuxTokensType> UsdLuxTokens;

PXR_NAMESPACE_CLOSE_SCOPE
