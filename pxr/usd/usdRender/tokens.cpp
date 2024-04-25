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
#include "pxr/usd/usdRender/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdRenderTokensType::UsdRenderTokensType() :
    adjustApertureHeight("adjustApertureHeight", TfToken::Immortal),
    adjustApertureWidth("adjustApertureWidth", TfToken::Immortal),
    adjustPixelAspectRatio("adjustPixelAspectRatio", TfToken::Immortal),
    aspectRatioConformPolicy("aspectRatioConformPolicy", TfToken::Immortal),
    camera("camera", TfToken::Immortal),
    collectionRenderVisibilityIncludeRoot("collection:renderVisibility:includeRoot", TfToken::Immortal),
    color3f("color3f", TfToken::Immortal),
    command("command", TfToken::Immortal),
    cropAperture("cropAperture", TfToken::Immortal),
    dataType("dataType", TfToken::Immortal),
    dataWindowNDC("dataWindowNDC", TfToken::Immortal),
    deepRaster("deepRaster", TfToken::Immortal),
    denoiseEnable("denoise:enable", TfToken::Immortal),
    denoisePass("denoise:pass", TfToken::Immortal),
    disableDepthOfField("disableDepthOfField", TfToken::Immortal),
    disableMotionBlur("disableMotionBlur", TfToken::Immortal),
    expandAperture("expandAperture", TfToken::Immortal),
    fileName("fileName", TfToken::Immortal),
    full("full", TfToken::Immortal),
    includedPurposes("includedPurposes", TfToken::Immortal),
    inputPasses("inputPasses", TfToken::Immortal),
    instantaneousShutter("instantaneousShutter", TfToken::Immortal),
    intrinsic("intrinsic", TfToken::Immortal),
    lpe("lpe", TfToken::Immortal),
    materialBindingPurposes("materialBindingPurposes", TfToken::Immortal),
    orderedVars("orderedVars", TfToken::Immortal),
    passType("passType", TfToken::Immortal),
    pixelAspectRatio("pixelAspectRatio", TfToken::Immortal),
    preview("preview", TfToken::Immortal),
    primvar("primvar", TfToken::Immortal),
    productName("productName", TfToken::Immortal),
    products("products", TfToken::Immortal),
    productType("productType", TfToken::Immortal),
    raster("raster", TfToken::Immortal),
    raw("raw", TfToken::Immortal),
    renderingColorSpace("renderingColorSpace", TfToken::Immortal),
    renderSettingsPrimPath("renderSettingsPrimPath", TfToken::Immortal),
    renderSource("renderSource", TfToken::Immortal),
    renderVisibility("renderVisibility", TfToken::Immortal),
    resolution("resolution", TfToken::Immortal),
    sourceName("sourceName", TfToken::Immortal),
    sourceType("sourceType", TfToken::Immortal),
    RenderDenoisePass("RenderDenoisePass", TfToken::Immortal),
    RenderPass("RenderPass", TfToken::Immortal),
    RenderProduct("RenderProduct", TfToken::Immortal),
    RenderSettings("RenderSettings", TfToken::Immortal),
    RenderSettingsBase("RenderSettingsBase", TfToken::Immortal),
    RenderVar("RenderVar", TfToken::Immortal),
    allTokens({
        adjustApertureHeight,
        adjustApertureWidth,
        adjustPixelAspectRatio,
        aspectRatioConformPolicy,
        camera,
        collectionRenderVisibilityIncludeRoot,
        color3f,
        command,
        cropAperture,
        dataType,
        dataWindowNDC,
        deepRaster,
        denoiseEnable,
        denoisePass,
        disableDepthOfField,
        disableMotionBlur,
        expandAperture,
        fileName,
        full,
        includedPurposes,
        inputPasses,
        instantaneousShutter,
        intrinsic,
        lpe,
        materialBindingPurposes,
        orderedVars,
        passType,
        pixelAspectRatio,
        preview,
        primvar,
        productName,
        products,
        productType,
        raster,
        raw,
        renderingColorSpace,
        renderSettingsPrimPath,
        renderSource,
        renderVisibility,
        resolution,
        sourceName,
        sourceType,
        RenderDenoisePass,
        RenderPass,
        RenderProduct,
        RenderSettings,
        RenderSettingsBase,
        RenderVar
    })
{
}

TfStaticData<UsdRenderTokensType> UsdRenderTokens;

PXR_NAMESPACE_CLOSE_SCOPE
