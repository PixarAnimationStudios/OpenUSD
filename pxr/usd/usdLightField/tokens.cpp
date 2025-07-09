//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLightField/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdLightFieldTokensType::UsdLightFieldTokensType() :
    cameraDistance("cameraDistance", TfToken::Immortal),
    ellipsoid("ellipsoid", TfToken::Immortal),
    gaussianShape("gaussianShape", TfToken::Immortal),
    orientations("orientations", TfToken::Immortal),
    orientationsf("orientationsf", TfToken::Immortal),
    perspective("perspective", TfToken::Immortal),
    plane("plane", TfToken::Immortal),
    primvarsSphericalHarmonics("primvars:sphericalHarmonics", TfToken::Immortal),
    primvarsSphericalHarmonicsf("primvars:sphericalHarmonicsf", TfToken::Immortal),
    projectionModeHint("projectionModeHint", TfToken::Immortal),
    scales("scales", TfToken::Immortal),
    scalesf("scalesf", TfToken::Immortal),
    sortingModeHint("sortingModeHint", TfToken::Immortal),
    tangential("tangential", TfToken::Immortal),
    triangle("triangle", TfToken::Immortal),
    zDepth("zDepth", TfToken::Immortal),
    GaussiansAPI("GaussiansAPI", TfToken::Immortal),
    SphericalHarmonicsAPI("SphericalHarmonicsAPI", TfToken::Immortal),
    allTokens({
        cameraDistance,
        ellipsoid,
        gaussianShape,
        orientations,
        orientationsf,
        perspective,
        plane,
        primvarsSphericalHarmonics,
        primvarsSphericalHarmonicsf,
        projectionModeHint,
        scales,
        scalesf,
        sortingModeHint,
        tangential,
        triangle,
        zDepth,
        GaussiansAPI,
        SphericalHarmonicsAPI
    })
{
}

TfStaticData<UsdLightFieldTokensType> UsdLightFieldTokens;

PXR_NAMESPACE_CLOSE_SCOPE
