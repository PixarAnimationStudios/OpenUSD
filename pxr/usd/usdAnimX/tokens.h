//
// Copyright 2020 benmalartre
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
#ifndef PXR_USD_PLUGIN_USD_ANIMX_TOKENS_H
#define PXR_USD_PLUGIN_USD_ANIMX_TOKENS_H

/// \file usdAnimX/tokens.h

#include "pxr/pxr.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"
#include "api.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdAnimXTokensType
///
/// \link UsdAnimXTokens \endlink provides static, efficient
/// \link TfToken TfTokens \endlink for use in all public USD API.
///
struct UsdAnimXTokensType {
    ANIMX_API UsdAnimXTokensType();
    /// \brief "prim"
    /// 
    /// Defines a prim
    const TfToken prim;
    /// \brief "op"
    /// 
    /// Define an op
    const TfToken op;
    /// \brief "curve"
    /// 
    /// Define a curve
    const TfToken curve;
    /// \brief "keyframes"
    /// 
    /// Define a sequence of keyframes
    const TfToken keyframes;
    /// \brief "target"
    /// 
    /// Op target 
    const TfToken target;
    /// \brief "dataType"
    /// 
    /// Op dataType
    const TfToken dataType;
    /// \brief "defaultValue"
    /// 
    /// Op defaultValue
    const TfToken defaultValue;

    /// Constant pre/post infinity type.
    const TfToken constant;
    /// \brief "curveInterpolationMethod"
    /// 
    /// Defines interpolation function within  curve segments for non-rotation curves.
    const TfToken curveInterpolationMethod;
    /// \brief "curveRotationInterpolationMethod"
    /// 
    /// Defines interpolation mode for the  rotation curves.
    const TfToken curveRotationInterpolationMethod;
    /// \brief "postInfinityType"
    /// 
    /// Defines the post-infinity type of the fcurve.
    const TfToken postInfinityType;
    /// \brief "preInfinityType"
    /// 
    /// Defines the pre-infinity type of the fcurve.
    const TfToken preInfinityType;
    /// \brief "linear"
    /// 
    /// Linear pre/post infinity type. (also tangent type)
    const TfToken linear;
    /// \brief "cycle"
    /// 
    /// Cycle pre/post infinity type.
    const TfToken cycle;
    /// \brief "cycleRelative"
    /// 
    /// Cycle Relative pre/post infinity type.
    const TfToken cycleRelative;
    /// \brief "oscillate"
    /// 
    /// Oscillate pre/post infinity type.
    const TfToken oscillate;
    /// \brief "tangentType"
    /// 
    /// Defines the type of the tangent
    const TfToken tangentType;
    /// \brief "global"
    /// 
    /// Tangent type
    const TfToken global;
    /// \brief "fixed"
    /// 
    /// Tangent type
    const TfToken fixed;
    /// \brief "flat"
    /// 
    /// Tangent type
    const TfToken flat;
    /// \brief "step"
    /// 
    /// Tangent type
    const TfToken step;
    /// \brief "slow"
    /// 
    /// Tangent type
    const TfToken slow;
    /// \brief "fast"
    /// 
    /// Tangent type
    const TfToken fast;
    /// \brief "smooth"
    /// 
    /// Tangent type
    const TfToken smooth;
    /// \brief "clamped"
    /// 
    /// Tangent type
    const TfToken clamped;
    /// \brief "auto"
    /// 
    /// Tangent type
    const TfToken automatic;
    /// \brief "sine"
    /// 
    /// Tangent type
    const TfToken sine;
    /// \brief "parabolic"
    /// 
    /// Tangent type
    const TfToken parabolic;
    /// \brief "logarithmic"
    /// 
    /// Tangent type
    const TfToken logarithmic;
    /// \brief "plateau"
    /// 
    /// Tangent type
    const TfToken plateau;
    /// \brief "stepNext"
    /// 
    /// Tangent type
    const TfToken stepNext;
    /// A vector of all of the tokens listed above.
    const std::vector<TfToken> allTokens;
};

/// \var UsdAnimXTokens
///
/// A global variable with static, efficient \link TfToken TfTokens \endlink
/// for use in all public USD API.  \sa UsdAnimXTokensType
extern ANIMX_API TfStaticData<UsdAnimXTokensType> UsdAnimXTokens;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PLUGIN_USD_ANIMX_TOKENS_H
