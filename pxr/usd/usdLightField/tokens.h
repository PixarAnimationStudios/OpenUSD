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
    /// Possible value for UsdLightFieldGaussiansAPI::GetSortingModeHintAttr()
    const TfToken cameraDistance;
    /// \brief "ellipsoid"
    /// 
    /// Fallback value for UsdLightFieldGaussiansAPI::GetGaussianShapeAttr()
    const TfToken ellipsoid;
    /// \brief "gaussianShape"
    /// 
    /// UsdLightFieldGaussiansAPI
    const TfToken gaussianShape;
    /// \brief "orientations"
    /// 
    /// UsdLightFieldGaussiansAPI
    const TfToken orientations;
    /// \brief "orientationsf"
    /// 
    /// UsdLightFieldGaussiansAPI
    const TfToken orientationsf;
    /// \brief "perspective"
    /// 
    /// Fallback value for UsdLightFieldGaussiansAPI::GetProjectionModeHintAttr()
    const TfToken perspective;
    /// \brief "plane"
    /// 
    /// Possible value for UsdLightFieldGaussiansAPI::GetGaussianShapeAttr()
    const TfToken plane;
    /// \brief "primvars:sphericalHarmonics"
    /// 
    /// UsdLightFieldSphericalHarmonicsAPI
    const TfToken primvarsSphericalHarmonics;
    /// \brief "primvars:sphericalHarmonicsf"
    /// 
    /// UsdLightFieldSphericalHarmonicsAPI
    const TfToken primvarsSphericalHarmonicsf;
    /// \brief "projectionModeHint"
    /// 
    /// UsdLightFieldGaussiansAPI
    const TfToken projectionModeHint;
    /// \brief "scales"
    /// 
    /// UsdLightFieldGaussiansAPI
    const TfToken scales;
    /// \brief "scalesf"
    /// 
    /// UsdLightFieldGaussiansAPI
    const TfToken scalesf;
    /// \brief "sortingModeHint"
    /// 
    /// UsdLightFieldGaussiansAPI
    const TfToken sortingModeHint;
    /// \brief "tangential"
    /// 
    /// Possible value for UsdLightFieldGaussiansAPI::GetProjectionModeHintAttr()
    const TfToken tangential;
    /// \brief "triangle"
    /// 
    /// Possible value for UsdLightFieldGaussiansAPI::GetGaussianShapeAttr()
    const TfToken triangle;
    /// \brief "zDepth"
    /// 
    /// Fallback value for UsdLightFieldGaussiansAPI::GetSortingModeHintAttr()
    const TfToken zDepth;
    /// \brief "GaussiansAPI"
    /// 
    /// Schema identifer and family for UsdLightFieldGaussiansAPI
    const TfToken GaussiansAPI;
    /// \brief "SphericalHarmonicsAPI"
    /// 
    /// Schema identifer and family for UsdLightFieldSphericalHarmonicsAPI
    const TfToken SphericalHarmonicsAPI;
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
