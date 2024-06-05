//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
