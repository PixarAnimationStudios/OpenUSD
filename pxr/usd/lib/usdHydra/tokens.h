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
#ifndef USDHYDRA_TOKENS_H
#define USDHYDRA_TOKENS_H

/// \file usdHydra/tokens.h

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// 
// This is an automatically generated file (by usdGenSchema.py).
// Do not hand-edit!
// 
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#include "pxr/pxr.h"
#include "pxr/usd/usdHydra/api.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdHydraTokensType
///
/// \link UsdHydraTokens \endlink provides static, efficient
/// \link TfToken TfTokens\endlink for use in all public USD API.
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdHydraTokens also contains all of the \em allowedTokens values
/// declared for schema builtin attributes of 'token' scene description type.
/// Use UsdHydraTokens like so:
///
/// \code
///     gprim.GetMyTokenValuedAttr().Set(UsdHydraTokens->black);
/// \endcode
struct UsdHydraTokensType {
    USDHYDRA_API UsdHydraTokensType();
    /// \brief "black"
    /// 
    /// Return black when sampling outside the bounds of the texture., Possible value for UsdHydraUvTexture::GetWrapTAttr(), Possible value for UsdHydraUvTexture::GetWrapSAttr()
    const TfToken black;
    /// \brief "clamp"
    /// 
    /// The texture coordinate is clamped to [0,1]. , Possible value for UsdHydraUvTexture::GetWrapTAttr(), Possible value for UsdHydraUvTexture::GetWrapSAttr()
    const TfToken clamp;
    /// \brief "displayLook:bxdf"
    /// 
    /// UsdHydraLookAPI
    const TfToken displayLookBxdf;
    /// \brief "faceIndex"
    /// 
    /// UsdHydraPtexTexture
    const TfToken faceIndex;
    /// \brief "faceOffset"
    /// 
    /// UsdHydraPtexTexture
    const TfToken faceOffset;
    /// \brief "frame"
    /// 
    /// UsdHydraTexture
    const TfToken frame;
    /// \brief "HwPrimvar_1"
    /// 
    /// Special token for the usdHydra library.
    const TfToken HwPrimvar_1;
    /// \brief "HwPtexTexture_1"
    /// 
    /// Special token for the usdHydra library.
    const TfToken HwPtexTexture_1;
    /// \brief "HwUvTexture_1"
    /// 
    /// Special token for the usdHydra library.
    const TfToken HwUvTexture_1;
    /// \brief "info:filename"
    /// 
    /// UsdHydraTexture, UsdHydraShader
    const TfToken infoFilename;
    /// \brief "info:varname"
    /// 
    /// UsdHydraPrimvar
    const TfToken infoVarname;
    /// \brief "linear"
    /// 
    /// A weighted linear blend of nearest adjacent samples. , Possible value for UsdHydraUvTexture::GetMinFilterAttr(), Possible value for UsdHydraUvTexture::GetMagFilterAttr()
    const TfToken linear;
    /// \brief "linearMipmapLinear"
    /// 
    /// See https://www.opengl.org/wiki/Sampler_Object , Possible value for UsdHydraUvTexture::GetMinFilterAttr()
    const TfToken linearMipmapLinear;
    /// \brief "linearMipmapNearest"
    /// 
    /// See https://www.opengl.org/wiki/Sampler_Object , Possible value for UsdHydraUvTexture::GetMinFilterAttr()
    const TfToken linearMipmapNearest;
    /// \brief "magFilter"
    /// 
    /// UsdHydraUvTexture
    const TfToken magFilter;
    /// \brief "minFilter"
    /// 
    /// UsdHydraUvTexture
    const TfToken minFilter;
    /// \brief "mirror"
    /// 
    /// The texture coordinate wraps around like a mirror. -0.2 becomes 0.2, -1.2 becomes 0.8, etc. , Possible value for UsdHydraUvTexture::GetWrapTAttr(), Possible value for UsdHydraUvTexture::GetWrapSAttr()
    const TfToken mirror;
    /// \brief "nearest"
    /// 
    /// Selects the nearest sample for the given coordinate , Possible value for UsdHydraUvTexture::GetMinFilterAttr(), Possible value for UsdHydraUvTexture::GetMagFilterAttr()
    const TfToken nearest;
    /// \brief "nearestMipmapLinear"
    /// 
    /// See https://www.opengl.org/wiki/Sampler_Object , Possible value for UsdHydraUvTexture::GetMinFilterAttr()
    const TfToken nearestMipmapLinear;
    /// \brief "nearestMipmapNearest"
    /// 
    /// See https://www.opengl.org/wiki/Sampler_Object , Possible value for UsdHydraUvTexture::GetMinFilterAttr()
    const TfToken nearestMipmapNearest;
    /// \brief "repeat"
    /// 
    /// The texture coordinate wraps around the texture. So a texture coordinate of -0.2 becomes the equivalent of 0.8. , Possible value for UsdHydraUvTexture::GetWrapTAttr(), Possible value for UsdHydraUvTexture::GetWrapSAttr()
    const TfToken repeat;
    /// \brief "textureMemory"
    /// 
    /// UsdHydraTexture
    const TfToken textureMemory;
    /// \brief "uv"
    /// 
    /// UsdHydraUvTexture
    const TfToken uv;
    /// \brief "wrapS"
    /// 
    /// UsdHydraUvTexture
    const TfToken wrapS;
    /// \brief "wrapT"
    /// 
    /// UsdHydraUvTexture
    const TfToken wrapT;
    /// A vector of all of the tokens listed above.
    const std::vector<TfToken> allTokens;
};

/// \var UsdHydraTokens
///
/// A global variable with static, efficient \link TfToken TfTokens\endlink
/// for use in all public USD API.  \sa UsdHydraTokensType
extern USDHYDRA_API TfStaticData<UsdHydraTokensType> UsdHydraTokens;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
