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
    angular("angular", TfToken::Immortal),
    automatic("automatic", TfToken::Immortal),
    collectionFilterLinkIncludeRoot("collection:filterLink:includeRoot", TfToken::Immortal),
    collectionLightLinkIncludeRoot("collection:lightLink:includeRoot", TfToken::Immortal),
    collectionShadowLinkIncludeRoot("collection:shadowLink:includeRoot", TfToken::Immortal),
    consumeAndContinue("consumeAndContinue", TfToken::Immortal),
    consumeAndHalt("consumeAndHalt", TfToken::Immortal),
    cubeMapVerticalCross("cubeMapVerticalCross", TfToken::Immortal),
    filterLink("filterLink", TfToken::Immortal),
    filters("filters", TfToken::Immortal),
    geometry("geometry", TfToken::Immortal),
    ignore("ignore", TfToken::Immortal),
    inputsAngle("inputs:angle", TfToken::Immortal),
    inputsColor("inputs:color", TfToken::Immortal),
    inputsColorTemperature("inputs:colorTemperature", TfToken::Immortal),
    inputsDiffuse("inputs:diffuse", TfToken::Immortal),
    inputsEnableColorTemperature("inputs:enableColorTemperature", TfToken::Immortal),
    inputsExposure("inputs:exposure", TfToken::Immortal),
    inputsHeight("inputs:height", TfToken::Immortal),
    inputsIntensity("inputs:intensity", TfToken::Immortal),
    inputsLength("inputs:length", TfToken::Immortal),
    inputsNormalize("inputs:normalize", TfToken::Immortal),
    inputsRadius("inputs:radius", TfToken::Immortal),
    inputsSpecular("inputs:specular", TfToken::Immortal),
    inputsTextureFile("inputs:texture:file", TfToken::Immortal),
    inputsTextureFormat("inputs:texture:format", TfToken::Immortal),
    inputsWidth("inputs:width", TfToken::Immortal),
    latlong("latlong", TfToken::Immortal),
    lightLink("lightLink", TfToken::Immortal),
    lightList("lightList", TfToken::Immortal),
    lightListCacheBehavior("lightList:cacheBehavior", TfToken::Immortal),
    mirroredBall("mirroredBall", TfToken::Immortal),
    orientToStageUpAxis("orientToStageUpAxis", TfToken::Immortal),
    portals("portals", TfToken::Immortal),
    shadowColor("shadow:color", TfToken::Immortal),
    shadowDistance("shadow:distance", TfToken::Immortal),
    shadowEnable("shadow:enable", TfToken::Immortal),
    shadowFalloff("shadow:falloff", TfToken::Immortal),
    shadowFalloffGamma("shadow:falloffGamma", TfToken::Immortal),
    shadowLink("shadowLink", TfToken::Immortal),
    shapingConeAngle("shaping:cone:angle", TfToken::Immortal),
    shapingConeSoftness("shaping:cone:softness", TfToken::Immortal),
    shapingFocus("shaping:focus", TfToken::Immortal),
    shapingFocusTint("shaping:focusTint", TfToken::Immortal),
    shapingIesAngleScale("shaping:ies:angleScale", TfToken::Immortal),
    shapingIesFile("shaping:ies:file", TfToken::Immortal),
    shapingIesNormalize("shaping:ies:normalize", TfToken::Immortal),
    treatAsLine("treatAsLine", TfToken::Immortal),
    treatAsPoint("treatAsPoint", TfToken::Immortal),
    allTokens({
        angular,
        automatic,
        collectionFilterLinkIncludeRoot,
        collectionLightLinkIncludeRoot,
        collectionShadowLinkIncludeRoot,
        consumeAndContinue,
        consumeAndHalt,
        cubeMapVerticalCross,
        filterLink,
        filters,
        geometry,
        ignore,
        inputsAngle,
        inputsColor,
        inputsColorTemperature,
        inputsDiffuse,
        inputsEnableColorTemperature,
        inputsExposure,
        inputsHeight,
        inputsIntensity,
        inputsLength,
        inputsNormalize,
        inputsRadius,
        inputsSpecular,
        inputsTextureFile,
        inputsTextureFormat,
        inputsWidth,
        latlong,
        lightLink,
        lightList,
        lightListCacheBehavior,
        mirroredBall,
        orientToStageUpAxis,
        portals,
        shadowColor,
        shadowDistance,
        shadowEnable,
        shadowFalloff,
        shadowFalloffGamma,
        shadowLink,
        shapingConeAngle,
        shapingConeSoftness,
        shapingFocus,
        shapingFocusTint,
        shapingIesAngleScale,
        shapingIesFile,
        shapingIesNormalize,
        treatAsLine,
        treatAsPoint
    })
{
}

TfStaticData<UsdLuxTokensType> UsdLuxTokens;

PXR_NAMESPACE_CLOSE_SCOPE
