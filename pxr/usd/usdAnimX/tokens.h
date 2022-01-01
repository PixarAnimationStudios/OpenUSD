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
#ifndef USDANIMX_TOKENS_H
#define USDANIMX_TOKENS_H

/// \file usdAnimX/tokens.h

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// 
// This is an automatically generated file (by usdGenSchema.py).
// Do not hand-edit!
// 
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#include "pxr/pxr.h"
#include "pxr/usd/usdAnimX/api.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdAnimXTokensType
///
/// \link UsdAnimXTokens \endlink provides static, efficient
/// \link TfToken TfTokens\endlink for use in all public USD API.
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdAnimXTokens also contains all of the \em allowedTokens values
/// declared for schema builtin attributes of 'token' scene description type.
/// Use UsdAnimXTokens like so:
///
/// \code
///     gprim.GetMyTokenValuedAttr().Set(UsdAnimXTokens->attributeName);
/// \endcode
struct UsdAnimXTokensType {
    USDANIMX_API UsdAnimXTokensType();
    /// \brief "attributeName"
    /// 
    /// UsdAnimXFCurveOp
    const TfToken attributeName;
    /// \brief "bool"
    /// 
    /// Possible value for UsdAnimXFCurveOp::GetDataTypeAttr()
    const TfToken bool_;
    /// \brief "constant"
    /// 
    /// Constant pre/post infinity type., Possible value for UsdAnimXFCurve::GetPostInfinityTypeAttr(), Default value for UsdAnimXFCurve::GetPostInfinityTypeAttr(), Possible value for UsdAnimXFCurve::GetPreInfinityTypeAttr(), Default value for UsdAnimXFCurve::GetPreInfinityTypeAttr()
    const TfToken constant;
    /// \brief "curveInterpolationMethod"
    /// 
    /// Defines interpolation function within  curve segments for non-rotation curves.
    const TfToken curveInterpolationMethod;
    /// \brief "curveRotationInterpolationMethod"
    /// 
    /// Defines interpolation mode for the  rotation curves.
    const TfToken curveRotationInterpolationMethod;
    /// \brief "cycle"
    /// 
    /// Cycle pre/post infinity type., Possible value for UsdAnimXFCurve::GetPostInfinityTypeAttr(), Possible value for UsdAnimXFCurve::GetPreInfinityTypeAttr()
    const TfToken cycle;
    /// \brief "cycleRelative"
    /// 
    /// Cycle Relative pre/post infinity type., Possible value for UsdAnimXFCurve::GetPostInfinityTypeAttr(), Possible value for UsdAnimXFCurve::GetPreInfinityTypeAttr()
    const TfToken cycleRelative;
    /// \brief "dataType"
    /// 
    /// UsdAnimXFCurveOp
    const TfToken dataType;
    /// \brief "double"
    /// 
    /// Possible value for UsdAnimXFCurveOp::GetDataTypeAttr()
    const TfToken double_;
    /// \brief "elementCount"
    /// 
    /// UsdAnimXFCurveOp
    const TfToken elementCount;
    /// \brief "float"
    /// 
    /// Possible value for UsdAnimXFCurveOp::GetDataTypeAttr(), Default value for UsdAnimXFCurveOp::GetDataTypeAttr()
    const TfToken float_;
    /// \brief "int"
    /// 
    /// Possible value for UsdAnimXFCurveOp::GetDataTypeAttr()
    const TfToken int_;
    /// \brief "keyframes"
    /// 
    /// UsdAnimXFCurve
    const TfToken keyframes;
    /// \brief "linear"
    /// 
    /// Linear pre/post infinity type., Possible value for UsdAnimXFCurve::GetPostInfinityTypeAttr(), Possible value for UsdAnimXFCurve::GetPreInfinityTypeAttr()
    const TfToken linear;
    /// \brief "oscillate"
    /// 
    /// Oscillate pre/post infinity type., Possible value for UsdAnimXFCurve::GetPostInfinityTypeAttr(), Possible value for UsdAnimXFCurve::GetPreInfinityTypeAttr()
    const TfToken oscillate;
    /// \brief "postInfinityType"
    /// 
    /// Defines the post-infinity type of the fcurve., UsdAnimXFCurve
    const TfToken postInfinityType;
    /// \brief "preInfinityType"
    /// 
    /// Defines the pre-infinity type of the fcurve., UsdAnimXFCurve
    const TfToken preInfinityType;
    /// \brief "tangentType"
    /// 
    /// Defines the type of the tangent
    const TfToken tangentType;
    /// A vector of all of the tokens listed above.
    const std::vector<TfToken> allTokens;
};

/// \var UsdAnimXTokens
///
/// A global variable with static, efficient \link TfToken TfTokens\endlink
/// for use in all public USD API.  \sa UsdAnimXTokensType
extern USDANIMX_API TfStaticData<UsdAnimXTokensType> UsdAnimXTokens;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
