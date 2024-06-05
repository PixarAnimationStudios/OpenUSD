//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDMEDIA_TOKENS_H
#define USDMEDIA_TOKENS_H

/// \file usdMedia/tokens.h

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// 
// This is an automatically generated file (by usdGenSchema.py).
// Do not hand-edit!
// 
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#include "pxr/pxr.h"
#include "pxr/usd/usdMedia/api.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdMediaTokensType
///
/// \link UsdMediaTokens \endlink provides static, efficient
/// \link TfToken TfTokens\endlink for use in all public USD API.
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdMediaTokens also contains all of the \em allowedTokens values
/// declared for schema builtin attributes of 'token' scene description type.
/// Use UsdMediaTokens like so:
///
/// \code
///     gprim.GetMyTokenValuedAttr().Set(UsdMediaTokens->auralMode);
/// \endcode
struct UsdMediaTokensType {
    USDMEDIA_API UsdMediaTokensType();
    /// \brief "auralMode"
    /// 
    /// UsdMediaSpatialAudio
    const TfToken auralMode;
    /// \brief "defaultImage"
    /// 
    /// Dictionary key in a Thumbnails dictionary for the default thumbnail image. 
    const TfToken defaultImage;
    /// \brief "endTime"
    /// 
    /// UsdMediaSpatialAudio
    const TfToken endTime;
    /// \brief "filePath"
    /// 
    /// UsdMediaSpatialAudio
    const TfToken filePath;
    /// \brief "gain"
    /// 
    /// UsdMediaSpatialAudio
    const TfToken gain;
    /// \brief "loopFromStage"
    /// 
    /// Possible value for UsdMediaSpatialAudio::GetPlaybackModeAttr()
    const TfToken loopFromStage;
    /// \brief "loopFromStart"
    /// 
    /// Possible value for UsdMediaSpatialAudio::GetPlaybackModeAttr()
    const TfToken loopFromStart;
    /// \brief "loopFromStartToEnd"
    /// 
    /// Possible value for UsdMediaSpatialAudio::GetPlaybackModeAttr()
    const TfToken loopFromStartToEnd;
    /// \brief "mediaOffset"
    /// 
    /// UsdMediaSpatialAudio
    const TfToken mediaOffset;
    /// \brief "nonSpatial"
    /// 
    /// Possible value for UsdMediaSpatialAudio::GetAuralModeAttr()
    const TfToken nonSpatial;
    /// \brief "onceFromStart"
    /// 
    /// Fallback value for UsdMediaSpatialAudio::GetPlaybackModeAttr()
    const TfToken onceFromStart;
    /// \brief "onceFromStartToEnd"
    /// 
    /// Possible value for UsdMediaSpatialAudio::GetPlaybackModeAttr()
    const TfToken onceFromStartToEnd;
    /// \brief "playbackMode"
    /// 
    /// UsdMediaSpatialAudio
    const TfToken playbackMode;
    /// \brief "previews"
    /// 
    /// Dictionary key in the assetInfo dictionary for asset previews sub-dictionary. 
    const TfToken previews;
    /// \brief "previews:thumbnails"
    /// 
    /// Full key in the assetInfo dictionary for thumbnails previews dictionary. 
    const TfToken previewThumbnails;
    /// \brief "previews:thumbnails:default"
    /// 
    /// Full key in the assetInfo dictionary for the "default" thumbnails in the previews dictionary. 
    const TfToken previewThumbnailsDefault;
    /// \brief "spatial"
    /// 
    /// Fallback value for UsdMediaSpatialAudio::GetAuralModeAttr()
    const TfToken spatial;
    /// \brief "startTime"
    /// 
    /// UsdMediaSpatialAudio
    const TfToken startTime;
    /// \brief "thumbnails"
    /// 
    /// Dictionary key in the assetInfo["previews"]  dictionary for thumbnails previews sub-dictionary. 
    const TfToken thumbnails;
    /// \brief "AssetPreviewsAPI"
    /// 
    /// Schema identifer and family for UsdMediaAssetPreviewsAPI
    const TfToken AssetPreviewsAPI;
    /// \brief "SpatialAudio"
    /// 
    /// Schema identifer and family for UsdMediaSpatialAudio
    const TfToken SpatialAudio;
    /// A vector of all of the tokens listed above.
    const std::vector<TfToken> allTokens;
};

/// \var UsdMediaTokens
///
/// A global variable with static, efficient \link TfToken TfTokens\endlink
/// for use in all public USD API.  \sa UsdMediaTokensType
extern USDMEDIA_API TfStaticData<UsdMediaTokensType> UsdMediaTokens;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
