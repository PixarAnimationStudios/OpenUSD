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
#include "pxr/usd/usdRi/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdRiTokensType::UsdRiTokensType() :
    bspline("bspline", TfToken::Immortal),
    cameraVisibility("cameraVisibility", TfToken::Immortal),
    catmullRom("catmull-rom", TfToken::Immortal),
    collectionCameraVisibilityIncludeRoot("collection:cameraVisibility:includeRoot", TfToken::Immortal),
    constant("constant", TfToken::Immortal),
    interpolation("interpolation", TfToken::Immortal),
    linear("linear", TfToken::Immortal),
    matte("matte", TfToken::Immortal),
    outputsRiDisplacement("outputs:ri:displacement", TfToken::Immortal),
    outputsRiSurface("outputs:ri:surface", TfToken::Immortal),
    outputsRiVolume("outputs:ri:volume", TfToken::Immortal),
    positions("positions", TfToken::Immortal),
    renderContext("ri", TfToken::Immortal),
    spline("spline", TfToken::Immortal),
    values("values", TfToken::Immortal),
    RiMaterialAPI("RiMaterialAPI", TfToken::Immortal),
    RiRenderPassAPI("RiRenderPassAPI", TfToken::Immortal),
    RiSplineAPI("RiSplineAPI", TfToken::Immortal),
    StatementsAPI("StatementsAPI", TfToken::Immortal),
    allTokens({
        bspline,
        cameraVisibility,
        catmullRom,
        collectionCameraVisibilityIncludeRoot,
        constant,
        interpolation,
        linear,
        matte,
        outputsRiDisplacement,
        outputsRiSurface,
        outputsRiVolume,
        positions,
        renderContext,
        spline,
        values,
        RiMaterialAPI,
        RiRenderPassAPI,
        RiSplineAPI,
        StatementsAPI
    })
{
}

TfStaticData<UsdRiTokensType> UsdRiTokens;

PXR_NAMESPACE_CLOSE_SCOPE
