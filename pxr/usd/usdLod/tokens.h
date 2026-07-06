//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDLOD_TOKENS_H
#define USDLOD_TOKENS_H

/// \file usdLod/tokens.h

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// 
// This is an automatically generated file (by usdGenSchema.py).
// Do not hand-edit!
// 
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#include "pxr/pxr.h"
#include "pxr/usd/usdLod/api.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdLodTokensType
///
/// \link UsdLodTokens \endlink provides static, efficient
/// \link TfToken TfTokens\endlink for use in all public USD API.
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdLodTokens also contains all of the \em allowedTokens values
/// declared for schema builtin attributes of 'token' scene description type.
/// Use UsdLodTokens like so:
///
/// \code
///     gprim.GetMyTokenValuedAttr().Set(UsdLodTokens->allLOD);
/// \endcode
struct UsdLodTokensType {
    USDLOD_API UsdLodTokensType();
    /// \brief "allLOD"
    /// 
    /// Possible value for UsdLodOverrideAPI::GetLodOverrideModeAttr()
    const TfToken allLOD;
    /// \brief "audio"
    /// 
    /// Name of the LOD domain for heuristics for generic audio based renderers.
    const TfToken audio;
    /// \brief "blendThresholds"
    /// 
    /// UsdLodDistanceHeuristic, UsdLodScreenSizeHeuristic
    const TfToken blendThresholds;
    /// \brief "boundingVolume"
    /// 
    /// UsdLodDistanceHeuristic, UsdLodScreenSizeHeuristic
    const TfToken boundingVolume;
    /// \brief "center"
    /// 
    /// UsdLodDistanceHeuristic
    const TfToken center;
    /// \brief "extent"
    /// 
    /// UsdLodScreenSizeHeuristic
    const TfToken extent;
    /// \brief "imaging"
    /// 
    /// Name of the LOD domain for heuristics for generic imaging based renderers.
    const TfToken imaging;
    /// \brief "indexedLOD"
    /// 
    /// Possible value for UsdLodOverrideAPI::GetLodOverrideModeAttr()
    const TfToken indexedLOD;
    /// \brief "inherited"
    /// 
    /// Fallback value for UsdLodOverrideAPI::GetLodOverrideModeAttr()
    const TfToken inherited;
    /// \brief "lod:default:index"
    /// 
    /// UsdLodRootAPI
    const TfToken lodDefaultIndex;
    /// \brief "lod:domain"
    /// 
    /// UsdLodHeuristic
    const TfToken lodDomain;
    /// \brief "lod:heuristics"
    /// 
    /// UsdLodRootAPI
    const TfToken lodHeuristics;
    /// \brief "lod:override:index"
    /// 
    /// UsdLodOverrideAPI
    const TfToken lodOverrideIndex;
    /// \brief "lod:override:mode"
    /// 
    /// UsdLodOverrideAPI
    const TfToken lodOverrideMode;
    /// \brief "noLOD"
    /// 
    /// Possible value for UsdLodOverrideAPI::GetLodOverrideModeAttr()
    const TfToken noLOD;
    /// \brief "noOverride"
    /// 
    /// Possible value for UsdLodOverrideAPI::GetLodOverrideModeAttr()
    const TfToken noOverride;
    /// \brief "physics"
    /// 
    /// Name of the LOD domain for heuristics for generic physics based renderers.
    const TfToken physics;
    /// \brief "projectedExtent"
    /// 
    /// Possible value for UsdLodScreenSizeHeuristic::GetProjectionMethodAttr()
    const TfToken projectedExtent;
    /// \brief "projectedSphere"
    /// 
    /// Fallback value for UsdLodScreenSizeHeuristic::GetProjectionMethodAttr()
    const TfToken projectedSphere;
    /// \brief "projectionMethod"
    /// 
    /// UsdLodScreenSizeHeuristic
    const TfToken projectionMethod;
    /// \brief "thresholds"
    /// 
    /// UsdLodDistanceHeuristic, UsdLodScreenSizeHeuristic
    const TfToken thresholds;
    /// \brief "LODDistanceHeuristic"
    /// 
    /// Schema identifer and family for UsdLodDistanceHeuristic
    const TfToken LODDistanceHeuristic;
    /// \brief "LODHeuristic"
    /// 
    /// Schema identifer and family for UsdLodHeuristic
    const TfToken LODHeuristic;
    /// \brief "LODOverrideAPI"
    /// 
    /// Schema identifer and family for UsdLodOverrideAPI
    const TfToken LODOverrideAPI;
    /// \brief "LODRootAPI"
    /// 
    /// Schema identifer and family for UsdLodRootAPI
    const TfToken LODRootAPI;
    /// \brief "LODScreenSizeHeuristic"
    /// 
    /// Schema identifer and family for UsdLodScreenSizeHeuristic
    const TfToken LODScreenSizeHeuristic;
    /// A vector of all of the tokens listed above.
    const std::vector<TfToken> allTokens;
};

/// \var UsdLodTokens
///
/// A global variable with static, efficient \link TfToken TfTokens\endlink
/// for use in all public USD API.  \sa UsdLodTokensType
extern USDLOD_API TfStaticData<UsdLodTokensType> UsdLodTokens;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
