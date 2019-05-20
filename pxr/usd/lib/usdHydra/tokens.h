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
/// These tokens were auto-generated from the deprecated UsdHydra schemas. They
/// represent property names, shader input names and associated values. The 
/// schemas have been deleted, but these tokens are being kept alive to assist
/// with the gradual transition to the new style hydra shaders that are based 
/// off of the new shader registry.
/// 
struct UsdHydraTokensType {
    USDHYDRA_API UsdHydraTokensType();

    /// \brief "HwPrimvar_1"
    /// 
    /// The id value of a Primvar shader.
    const TfToken HwPrimvar_1;

    /// \brief "HwPtexTexture_1"
    /// 
    /// The id value of a PtexTexture shader.
    const TfToken HwPtexTexture_1;

    /// \brief "HwUvTexture_1"
    /// 
    /// The id value of a UvTexture shader.
    const TfToken HwUvTexture_1;

    /// \brief "displayLook:bxdf"
    /// \deprecated This has been deprecated in favor of the glslfx:surface output.
    /// 
    /// Relationship on a material that targets the "bxdf" or the surface 
    /// shader prim.
    const TfToken displayLookBxdf;

    /// \brief "info:filename"
    /// 
    /// The special "info:filename" property of a hydra Texture shader, which 
    /// points to a resolvable texture asset. 
    const TfToken infoFilename;

    /// \brief "info:varname"
    /// 
    /// 
    const TfToken infoVarname;

    /// \brief "textureMemory"
    /// 
    /// A shader input on a hydra Texture shader.
    const TfToken textureMemory;

    /// \brief "frame"
    /// 
    /// A shader input on a "Texture" shader. 
    const TfToken frame;

    /// \brief "uv"
    /// 
    /// A shader input on a hydra UvTexture shader.
    const TfToken uv;

    /// \brief "wrapS"
    /// 
    /// A shader input on a hydra UvTexture shader which defines the behavior of
    /// texture coordinates that are outside the bounds of the texture.
    const TfToken wrapS;

    /// \brief "wrapT"
    /// 
    /// A shader input on a hydra UvTexture shader which defines the behavior of
    /// texture coordinates that are outside the bounds of the texture.
    const TfToken wrapT;

    /// \brief "black"
    /// 
    /// Possible value for "wrapT" and "wrapS" inputs on a "UvTexture" 
    /// shader prim. 
    /// Causes black to be returned when sampling outside the bounds of the 
    /// texture., 
    const TfToken black;

    /// \brief "clamp"
    /// 
    /// Possible value for "wrapT" and "wrapS" inputs on a "UvTexture" 
    /// shader prim. 
    /// Causes the the texture coordinate to be clamped to [0,1].
    const TfToken clamp;

    /// \brief "mirror"
    ///
    /// Possible value for "wrapT" and "wrapS" inputs on a "UvTexture" 
    /// shader prim. 
    /// Causes the texture coordinate to wrap around like a mirror. 
    /// -0.2 becomes 0.2, -1.2 becomes 0.8, etc. ,
    const TfToken mirror;

    /// \brief "repeat"
    /// 
    /// Possible value for "wrapT" and "wrapS" inputs on a "UvTexture" 
    /// shader prim. 
    /// Causes the texture coordinate to wrap around the texture. So a texture 
    /// coordinate of -0.2 becomes the equivalent of 0.8.
    const TfToken repeat;

    /// \brief "useMetadata"
    ///
    /// Possible value for "wrapT" and "wrapS" inputs on a "UvTexture"
    /// shader prim.
    /// Causes the wrap value to be loaded from the texture file instead
    /// of being specified in the prim.  If the texture file doesn't support
    /// metadata or the metadata doesn't contain a wrap mode, the "black"
    /// wrap mode is used.
    const TfToken useMetadata;

    /// \brief "magFilter"
    /// 
    /// An input on a UvTexture shader.
    const TfToken magFilter;

    /// \brief "minFilter"
    /// 
    /// An input on a UvTexture shader.
    const TfToken minFilter;

    /// \brief "linearMipmapLinear"
    /// 
    /// See https://www.opengl.org/wiki/Sampler_Object , 
    /// Possible value for the "minFilter" input on a UvTexture shader.
    const TfToken linearMipmapLinear;

    /// \brief "linearMipmapNearest"
    /// 
    /// See https://www.opengl.org/wiki/Sampler_Object 
    /// Possible value for the "minFilter" input on a UvTexture shader.
    const TfToken linearMipmapNearest;

    /// \brief "nearestMipmapNearest"
    /// 
    /// See https://www.opengl.org/wiki/Sampler_Object 
    /// Possible value for the "minFilter" input on a UvTexture shader.
    const TfToken nearestMipmapNearest;

    /// \brief "linear"
    /// 
    /// A weighted linear blend of nearest adjacent samples. 
    /// Possible value for "minFilter" and "magFilter" inputs on a UvTexture 
    /// shader.
    const TfToken linear;

    /// 
    /// \brief "nearest"
    /// 
    /// Selects the nearest sample for the given coordinate 
    /// Possible value for "minFilter" and "magFilter" inputs on a UvTexture 
    /// shader.
    const TfToken nearest;

    /// \brief "nearestMipmapLinear"
    /// 
    /// See https://www.opengl.org/wiki/Sampler_Object 
    /// Possible value for "minFilter" and "magFilter" inputs on a UvTexture 
    /// shader.
    const TfToken nearestMipmapLinear;

    /// \brief "faceIndex"
    /// 
    /// The "faceIndex" shader input on a hydra "PtexTexture" shader.
    const TfToken faceIndex;

    /// \brief "faceOffset"
    /// 
    /// The "faceOffset" shader input on a hydra "PtexTexture" shader.
    const TfToken faceOffset;

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
