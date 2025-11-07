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
    opacities("opacities", TfToken::Immortal),
    opacitiesh("opacitiesh", TfToken::Immortal),
    orientations("orientations", TfToken::Immortal),
    orientationsh("orientationsh", TfToken::Immortal),
    perspective("perspective", TfToken::Immortal),
    positions("positions", TfToken::Immortal),
    positionsh("positionsh", TfToken::Immortal),
    projectionModeHint("projectionModeHint", TfToken::Immortal),
    radianceSphericalHarmonicsCoefficients("radiance:sphericalHarmonicsCoefficients", TfToken::Immortal),
    radianceSphericalHarmonicsCoefficientsh("radiance:sphericalHarmonicsCoefficientsh", TfToken::Immortal),
    radianceSphericalHarmonicsDegree("radiance:sphericalHarmonicsDegree", TfToken::Immortal),
    scales("scales", TfToken::Immortal),
    scalesh("scalesh", TfToken::Immortal),
    sortingModeHint("sortingModeHint", TfToken::Immortal),
    sphericalBetaBeta("sphericalBeta:beta", TfToken::Immortal),
    tangential("tangential", TfToken::Immortal),
    zDepth("zDepth", TfToken::Immortal),
    LightFieldKernelBaseAPI("LightFieldKernelBaseAPI", TfToken::Immortal),
    LightFieldKernelConstantSurfletAPI("LightFieldKernelConstantSurfletAPI", TfToken::Immortal),
    LightFieldKernelGaussianEllipsoidAPI("LightFieldKernelGaussianEllipsoidAPI", TfToken::Immortal),
    LightFieldKernelGaussianSurfletAPI("LightFieldKernelGaussianSurfletAPI", TfToken::Immortal),
    LightFieldOpacityAttributeAPI("LightFieldOpacityAttributeAPI", TfToken::Immortal),
    LightFieldOrientationAttributeAPI("LightFieldOrientationAttributeAPI", TfToken::Immortal),
    LightFieldPositionAttributeAPI("LightFieldPositionAttributeAPI", TfToken::Immortal),
    LightFieldPositionBaseAPI("LightFieldPositionBaseAPI", TfToken::Immortal),
    LightFieldRadianceBaseAPI("LightFieldRadianceBaseAPI", TfToken::Immortal),
    LightFieldScaleAttributeAPI("LightFieldScaleAttributeAPI", TfToken::Immortal),
    LightFieldSphericalBetaAttributeAPI("LightFieldSphericalBetaAttributeAPI", TfToken::Immortal),
    LightFieldSphericalHarmonicsAttributeAPI("LightFieldSphericalHarmonicsAttributeAPI", TfToken::Immortal),
    ParticleField("ParticleField", TfToken::Immortal),
    ParticleField_3DGaussianSplat("ParticleField_3DGaussianSplat", TfToken::Immortal),
    allTokens({
        cameraDistance,
        opacities,
        opacitiesh,
        orientations,
        orientationsh,
        perspective,
        positions,
        positionsh,
        projectionModeHint,
        radianceSphericalHarmonicsCoefficients,
        radianceSphericalHarmonicsCoefficientsh,
        radianceSphericalHarmonicsDegree,
        scales,
        scalesh,
        sortingModeHint,
        sphericalBetaBeta,
        tangential,
        zDepth,
        LightFieldKernelBaseAPI,
        LightFieldKernelConstantSurfletAPI,
        LightFieldKernelGaussianEllipsoidAPI,
        LightFieldKernelGaussianSurfletAPI,
        LightFieldOpacityAttributeAPI,
        LightFieldOrientationAttributeAPI,
        LightFieldPositionAttributeAPI,
        LightFieldPositionBaseAPI,
        LightFieldRadianceBaseAPI,
        LightFieldScaleAttributeAPI,
        LightFieldSphericalBetaAttributeAPI,
        LightFieldSphericalHarmonicsAttributeAPI,
        ParticleField,
        ParticleField_3DGaussianSplat
    })
{
}

TfStaticData<UsdLightFieldTokensType> UsdLightFieldTokens;

PXR_NAMESPACE_CLOSE_SCOPE
