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
#ifndef USDSKEL_TOKENS_H
#define USDSKEL_TOKENS_H

/// \file usdSkel/tokens.h

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// 
// This is an automatically generated file (by usdGenSchema.py).
// Do not hand-edit!
// 
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#include "pxr/pxr.h"
#include "pxr/usd/usdSkel/api.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdSkelTokensType
///
/// \link UsdSkelTokens \endlink provides static, efficient
/// \link TfToken TfTokens\endlink for use in all public USD API.
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdSkelTokens also contains all of the \em allowedTokens values
/// declared for schema builtin attributes of 'token' scene description type.
/// Use UsdSkelTokens like so:
///
/// \code
///     gprim.GetMyTokenValuedAttr().Set(UsdSkelTokens->blendShapes);
/// \endcode
struct UsdSkelTokensType {
    USDSKEL_API UsdSkelTokensType();
    /// \brief "blendShapes"
    /// 
    /// UsdSkelPackedJointAnimation
    const TfToken blendShapes;
    /// \brief "blendShapeWeights"
    /// 
    /// UsdSkelPackedJointAnimation
    const TfToken blendShapeWeights;
    /// \brief "joints"
    /// 
    /// UsdSkelPackedJointAnimation, UsdSkelSkeleton
    const TfToken joints;
    /// \brief "offsets"
    /// 
    /// UsdSkelBlendShape
    const TfToken offsets;
    /// \brief "pointIndices"
    /// 
    /// UsdSkelBlendShape
    const TfToken pointIndices;
    /// \brief "primvars:skel:geomBindTransform"
    /// 
    /// UsdSkelBindingAPI
    const TfToken primvarsSkelGeomBindTransform;
    /// \brief "primvars:skel:jointIndices"
    /// 
    /// UsdSkelBindingAPI
    const TfToken primvarsSkelJointIndices;
    /// \brief "primvars:skel:jointWeights"
    /// 
    /// UsdSkelBindingAPI
    const TfToken primvarsSkelJointWeights;
    /// \brief "restTransforms"
    /// 
    /// UsdSkelSkeleton
    const TfToken restTransforms;
    /// \brief "rotations"
    /// 
    /// UsdSkelPackedJointAnimation
    const TfToken rotations;
    /// \brief "scales"
    /// 
    /// UsdSkelPackedJointAnimation
    const TfToken scales;
    /// \brief "skel:animationSource"
    /// 
    /// UsdSkelBindingAPI
    const TfToken skelAnimationSource;
    /// \brief "skel:blendShapes"
    /// 
    /// UsdSkelBindingAPI
    const TfToken skelBlendShapes;
    /// \brief "skel:blendShapeTargets"
    /// 
    /// UsdSkelBindingAPI
    const TfToken skelBlendShapeTargets;
    /// \brief "skel:joints"
    /// 
    /// UsdSkelBindingAPI
    const TfToken skelJoints;
    /// \brief "skel:skeleton"
    /// 
    /// UsdSkelBindingAPI
    const TfToken skelSkeleton;
    /// \brief "skel:skeletonInstance"
    /// 
    /// UsdSkelBindingAPI
    const TfToken skelSkeletonInstance;
    /// \brief "translations"
    /// 
    /// UsdSkelPackedJointAnimation
    const TfToken translations;
    /// \brief "weight"
    /// 
    /// UsdSkelInbetweenShape - The weight location at which the inbetween shape applies.
    const TfToken weight;
    /// A vector of all of the tokens listed above.
    const std::vector<TfToken> allTokens;
};

/// \var UsdSkelTokens
///
/// A global variable with static, efficient \link TfToken TfTokens\endlink
/// for use in all public USD API.  \sa UsdSkelTokensType
extern USDSKEL_API TfStaticData<UsdSkelTokensType> UsdSkelTokens;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
