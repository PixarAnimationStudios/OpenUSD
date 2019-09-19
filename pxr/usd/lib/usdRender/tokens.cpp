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
    color3f("color3f", TfToken::Immortal),
    cropAperture("cropAperture", TfToken::Immortal),
    dataType("dataType", TfToken::Immortal),
    dataWindowNDC("dataWindowNDC", TfToken::Immortal),
    expandAperture("expandAperture", TfToken::Immortal),
    full("full", TfToken::Immortal),
    includedPurposes("includedPurposes", TfToken::Immortal),
    instantaneousShutter("instantaneousShutter", TfToken::Immortal),
    intrinsic("intrinsic", TfToken::Immortal),
    lpe("lpe", TfToken::Immortal),
    materialBindingPurposes("materialBindingPurposes", TfToken::Immortal),
    orderedVars("orderedVars", TfToken::Immortal),
    pixelAspectRatio("pixelAspectRatio", TfToken::Immortal),
    preview("preview", TfToken::Immortal),
    primvar("primvar", TfToken::Immortal),
    productName("productName", TfToken::Immortal),
    products("products", TfToken::Immortal),
    productType("productType", TfToken::Immortal),
    raster("raster", TfToken::Immortal),
    raw("raw", TfToken::Immortal),
    renderSettingsPrimPath("renderSettingsPrimPath", TfToken::Immortal),
    resolution("resolution", TfToken::Immortal),
    sourceName("sourceName", TfToken::Immortal),
    sourceType("sourceType", TfToken::Immortal),
    allTokens({
        adjustApertureHeight,
        adjustApertureWidth,
        adjustPixelAspectRatio,
        aspectRatioConformPolicy,
        camera,
        color3f,
        cropAperture,
        dataType,
        dataWindowNDC,
        expandAperture,
        full,
        includedPurposes,
        instantaneousShutter,
        intrinsic,
        lpe,
        materialBindingPurposes,
        orderedVars,
        pixelAspectRatio,
        preview,
        primvar,
        productName,
        products,
        productType,
        raster,
        raw,
        renderSettingsPrimPath,
        resolution,
        sourceName,
        sourceType
    })
{
}

TfStaticData<UsdRenderTokensType> UsdRenderTokens;

PXR_NAMESPACE_CLOSE_SCOPE
