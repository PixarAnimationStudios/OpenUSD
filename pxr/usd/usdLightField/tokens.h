//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDLIGHTFIELD_TOKENS_H
#define USDLIGHTFIELD_TOKENS_H

/// \file usdLightField/tokens.h

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// 
// This is an automatically generated file (by usdGenSchema.py).
// Do not hand-edit!
// 
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#include "pxr/pxr.h"
#include "pxr/usd/usdLightField/api.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdLightFieldTokensType
///
/// \link UsdLightFieldTokens \endlink provides static, efficient
/// \link TfToken TfTokens\endlink for use in all public USD API.
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdLightFieldTokens also contains all of the \em allowedTokens values
/// declared for schema builtin attributes of 'token' scene description type.
/// Use UsdLightFieldTokens like so:
///
/// \code
///     gprim.GetMyTokenValuedAttr().Set(UsdLightFieldTokens->cameraDistance);
/// \endcode
struct UsdLightFieldTokensType {
    USDLIGHTFIELD_API UsdLightFieldTokensType();
    /// \brief "cameraDistance"
    /// 
    /// Possible value for UsdLightFieldParticleField_3DGaussianSplat::GetSortingModeHintAttr()
    const TfToken cameraDistance;
    /// \brief "kernelFalloff:implicitShapeFalloff:threshold"
    /// 
    /// UsdLightFieldImplicitShapeFalloffThresholdAPI
    const TfToken kernelFalloffImplicitShapeFalloffThreshold;
    /// \brief "kernelShape:triangle:edgeLength"
    /// 
    /// UsdLightFieldTriangleShapeAPI
    const TfToken kernelShapeTriangleEdgeLength;
    /// \brief "opacities"
    /// 
    /// UsdLightFieldOpacityAttributeAPI
    const TfToken opacities;
    /// \brief "opacitiesh"
    /// 
    /// UsdLightFieldOpacityAttributeAPI
    const TfToken opacitiesh;
    /// \brief "orientations"
    /// 
    /// UsdLightFieldOrientationAttributeAPI
    const TfToken orientations;
    /// \brief "orientationsh"
    /// 
    /// UsdLightFieldOrientationAttributeAPI
    const TfToken orientationsh;
    /// \brief "perspective"
    /// 
    /// Fallback value for UsdLightFieldParticleField_3DGaussianSplat::GetProjectionModeHintAttr()
    const TfToken perspective;
    /// \brief "positions"
    /// 
    /// UsdLightFieldPositionAttributeAPI
    const TfToken positions;
    /// \brief "positionsh"
    /// 
    /// UsdLightFieldPositionAttributeAPI
    const TfToken positionsh;
    /// \brief "primvars:radiance:sphericalHarmonicsCoefficients"
    /// 
    /// UsdLightFieldSphericalHarmonicsAttributeAPI
    const TfToken primvarsRadianceSphericalHarmonicsCoefficients;
    /// \brief "primvars:radiance:sphericalHarmonicsCoefficientsh"
    /// 
    /// UsdLightFieldSphericalHarmonicsAttributeAPI
    const TfToken primvarsRadianceSphericalHarmonicsCoefficientsh;
    /// \brief "primvars:sphericalBeta:beta"
    /// 
    /// UsdLightFieldSphericalBetaAttributeAPI
    const TfToken primvarsSphericalBetaBeta;
    /// \brief "projectionModeHint"
    /// 
    /// UsdLightFieldParticleField_3DGaussianSplat
    const TfToken projectionModeHint;
    /// \brief "radiance:sphericalHarmonicsDegree"
    /// 
    /// UsdLightFieldSphericalHarmonicsAttributeAPI
    const TfToken radianceSphericalHarmonicsDegree;
    /// \brief "scales"
    /// 
    /// UsdLightFieldScaleAttributeAPI
    const TfToken scales;
    /// \brief "scalesh"
    /// 
    /// UsdLightFieldScaleAttributeAPI
    const TfToken scalesh;
    /// \brief "sortingModeHint"
    /// 
    /// UsdLightFieldParticleField_3DGaussianSplat
    const TfToken sortingModeHint;
    /// \brief "tangential"
    /// 
    /// Possible value for UsdLightFieldParticleField_3DGaussianSplat::GetProjectionModeHintAttr()
    const TfToken tangential;
    /// \brief "zDepth"
    /// 
    /// Fallback value for UsdLightFieldParticleField_3DGaussianSplat::GetSortingModeHintAttr()
    const TfToken zDepth;
    /// \brief "GaussianFalloffFunctionAPI"
    /// 
    /// Schema identifer and family for UsdLightFieldGaussianFalloffFunctionAPI
    const TfToken GaussianFalloffFunctionAPI;
    /// \brief "GaussianShapeAPI"
    /// 
    /// Schema identifer and family for UsdLightFieldGaussianShapeAPI
    const TfToken GaussianShapeAPI;
    /// \brief "ImplicitShapeFalloffThresholdAPI"
    /// 
    /// Schema identifer and family for UsdLightFieldImplicitShapeFalloffThresholdAPI
    const TfToken ImplicitShapeFalloffThresholdAPI;
    /// \brief "OpacityAttributeAPI"
    /// 
    /// Schema identifer and family for UsdLightFieldOpacityAttributeAPI
    const TfToken OpacityAttributeAPI;
    /// \brief "OrientationAttributeAPI"
    /// 
    /// Schema identifer and family for UsdLightFieldOrientationAttributeAPI
    const TfToken OrientationAttributeAPI;
    /// \brief "ParticleField"
    /// 
    /// Schema identifer and family for UsdLightFieldParticleField
    const TfToken ParticleField;
    /// \brief "ParticleField_3DGaussianSplat"
    /// 
    /// Schema identifer and family for UsdLightFieldParticleField_3DGaussianSplat
    const TfToken ParticleField_3DGaussianSplat;
    /// \brief "PositionAttributeAPI"
    /// 
    /// Schema identifer and family for UsdLightFieldPositionAttributeAPI
    const TfToken PositionAttributeAPI;
    /// \brief "ScaleAttributeAPI"
    /// 
    /// Schema identifer and family for UsdLightFieldScaleAttributeAPI
    const TfToken ScaleAttributeAPI;
    /// \brief "SphericalBetaAttributeAPI"
    /// 
    /// Schema identifer and family for UsdLightFieldSphericalBetaAttributeAPI
    const TfToken SphericalBetaAttributeAPI;
    /// \brief "SphericalHarmonicsAttributeAPI"
    /// 
    /// Schema identifer and family for UsdLightFieldSphericalHarmonicsAttributeAPI
    const TfToken SphericalHarmonicsAttributeAPI;
    /// \brief "TriangleShapeAPI"
    /// 
    /// Schema identifer and family for UsdLightFieldTriangleShapeAPI
    const TfToken TriangleShapeAPI;
    /// A vector of all of the tokens listed above.
    const std::vector<TfToken> allTokens;
};

/// \var UsdLightFieldTokens
///
/// A global variable with static, efficient \link TfToken TfTokens\endlink
/// for use in all public USD API.  \sa UsdLightFieldTokensType
extern USDLIGHTFIELD_API TfStaticData<UsdLightFieldTokensType> UsdLightFieldTokens;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
