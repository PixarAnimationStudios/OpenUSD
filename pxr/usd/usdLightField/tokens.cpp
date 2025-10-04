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
    kernelFalloffImplicitShapeFalloffThreshold("kernelFalloff:implicitShapeFalloff:threshold", TfToken::Immortal),
    kernelShapeTriangleEdgeLength("kernelShape:triangle:edgeLength", TfToken::Immortal),
    opacities("opacities", TfToken::Immortal),
    opacitiesh("opacitiesh", TfToken::Immortal),
    orientations("orientations", TfToken::Immortal),
    orientationsh("orientationsh", TfToken::Immortal),
    perspective("perspective", TfToken::Immortal),
    positions("positions", TfToken::Immortal),
    positionsh("positionsh", TfToken::Immortal),
    primvarsRadianceSphericalHarmonicsCoefficients("primvars:radiance:sphericalHarmonicsCoefficients", TfToken::Immortal),
    primvarsRadianceSphericalHarmonicsCoefficientsh("primvars:radiance:sphericalHarmonicsCoefficientsh", TfToken::Immortal),
    primvarsSphericalBetaBeta("primvars:sphericalBeta:beta", TfToken::Immortal),
    projectionModeHint("projectionModeHint", TfToken::Immortal),
    radianceSphericalHarmonicsDegree("radiance:sphericalHarmonicsDegree", TfToken::Immortal),
    scales("scales", TfToken::Immortal),
    scalesh("scalesh", TfToken::Immortal),
    sortingModeHint("sortingModeHint", TfToken::Immortal),
    tangential("tangential", TfToken::Immortal),
    zDepth("zDepth", TfToken::Immortal),
    GaussianFalloffFunctionAPI("GaussianFalloffFunctionAPI", TfToken::Immortal),
    GaussianShapeAPI("GaussianShapeAPI", TfToken::Immortal),
    ImplicitShapeFalloffThresholdAPI("ImplicitShapeFalloffThresholdAPI", TfToken::Immortal),
    OpacityAttributeAPI("OpacityAttributeAPI", TfToken::Immortal),
    OrientationAttributeAPI("OrientationAttributeAPI", TfToken::Immortal),
    ParticleField("ParticleField", TfToken::Immortal),
    ParticleField_3DGaussianSplat("ParticleField_3DGaussianSplat", TfToken::Immortal),
    PositionAttributeAPI("PositionAttributeAPI", TfToken::Immortal),
    ScaleAttributeAPI("ScaleAttributeAPI", TfToken::Immortal),
    SphericalBetaAttributeAPI("SphericalBetaAttributeAPI", TfToken::Immortal),
    SphericalHarmonicsAttributeAPI("SphericalHarmonicsAttributeAPI", TfToken::Immortal),
    TriangleShapeAPI("TriangleShapeAPI", TfToken::Immortal),
    allTokens({
        cameraDistance,
        kernelFalloffImplicitShapeFalloffThreshold,
        kernelShapeTriangleEdgeLength,
        opacities,
        opacitiesh,
        orientations,
        orientationsh,
        perspective,
        positions,
        positionsh,
        primvarsRadianceSphericalHarmonicsCoefficients,
        primvarsRadianceSphericalHarmonicsCoefficientsh,
        primvarsSphericalBetaBeta,
        projectionModeHint,
        radianceSphericalHarmonicsDegree,
        scales,
        scalesh,
        sortingModeHint,
        tangential,
        zDepth,
        GaussianFalloffFunctionAPI,
        GaussianShapeAPI,
        ImplicitShapeFalloffThresholdAPI,
        OpacityAttributeAPI,
        OrientationAttributeAPI,
        ParticleField,
        ParticleField_3DGaussianSplat,
        PositionAttributeAPI,
        ScaleAttributeAPI,
        SphericalBetaAttributeAPI,
        SphericalHarmonicsAttributeAPI,
        TriangleShapeAPI
    })
{
}

TfStaticData<UsdLightFieldTokensType> UsdLightFieldTokens;

PXR_NAMESPACE_CLOSE_SCOPE
