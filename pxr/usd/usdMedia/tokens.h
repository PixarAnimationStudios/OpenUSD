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
    /// Possible value for UsdMediaSpatialAudio::GetPlaybackModeAttr(), Default value for UsdMediaSpatialAudio::GetPlaybackModeAttr()
    const TfToken onceFromStart;
    /// \brief "onceFromStartToEnd"
    /// 
    /// Possible value for UsdMediaSpatialAudio::GetPlaybackModeAttr()
    const TfToken onceFromStartToEnd;
    /// \brief "playbackMode"
    /// 
    /// UsdMediaSpatialAudio
    const TfToken playbackMode;
    /// \brief "spatial"
    /// 
    /// Possible value for UsdMediaSpatialAudio::GetAuralModeAttr(), Default value for UsdMediaSpatialAudio::GetAuralModeAttr()
    const TfToken spatial;
    /// \brief "startTime"
    /// 
    /// UsdMediaSpatialAudio
    const TfToken startTime;
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
