//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
///     gprim.GetMyTokenValuedAttr().Set(UsdSkelTokens->bindTransforms);
/// \endcode
struct UsdSkelTokensType {
    USDSKEL_API UsdSkelTokensType();
    /// \brief "bindTransforms"
    /// 
    /// UsdSkelSkeleton
    const TfToken bindTransforms;
    /// \brief "blendShapes"
    /// 
    /// UsdSkelAnimation
    const TfToken blendShapes;
    /// \brief "blendShapeWeights"
    /// 
    /// UsdSkelAnimation
    const TfToken blendShapeWeights;
    /// \brief "classicLinear"
    /// 
    /// Fallback value for UsdSkelBindingAPI::GetSkinningMethodAttr()
    const TfToken classicLinear;
    /// \brief "dualQuaternion"
    /// 
    /// Possible value for UsdSkelBindingAPI::GetSkinningMethodAttr()
    const TfToken dualQuaternion;
    /// \brief "jointNames"
    /// 
    /// UsdSkelSkeleton
    const TfToken jointNames;
    /// \brief "joints"
    /// 
    /// UsdSkelSkeleton, UsdSkelAnimation
    const TfToken joints;
    /// \brief "normalOffsets"
    /// 
    /// UsdSkelBlendShape
    const TfToken normalOffsets;
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
    /// \brief "primvars:skel:skinningMethod"
    /// 
    /// UsdSkelBindingAPI
    const TfToken primvarsSkelSkinningMethod;
    /// \brief "restTransforms"
    /// 
    /// UsdSkelSkeleton
    const TfToken restTransforms;
    /// \brief "rotations"
    /// 
    /// UsdSkelAnimation
    const TfToken rotations;
    /// \brief "scales"
    /// 
    /// UsdSkelAnimation
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
    /// \brief "translations"
    /// 
    /// UsdSkelAnimation
    const TfToken translations;
    /// \brief "weight"
    /// 
    /// UsdSkelInbetweenShape - The weight location at which the inbetween shape applies.
    const TfToken weight;
    /// \brief "BlendShape"
    /// 
    /// Schema identifer and family for UsdSkelBlendShape
    const TfToken BlendShape;
    /// \brief "SkelAnimation"
    /// 
    /// Schema identifer and family for UsdSkelAnimation
    const TfToken SkelAnimation;
    /// \brief "SkelBindingAPI"
    /// 
    /// Schema identifer and family for UsdSkelBindingAPI
    const TfToken SkelBindingAPI;
    /// \brief "Skeleton"
    /// 
    /// Schema identifer and family for UsdSkelSkeleton
    const TfToken Skeleton;
    /// \brief "SkelRoot"
    /// 
    /// Schema identifer and family for UsdSkelRoot
    const TfToken SkelRoot;
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
