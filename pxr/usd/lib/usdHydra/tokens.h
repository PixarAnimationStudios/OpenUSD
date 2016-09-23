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

#include "pxr/base/tf/staticTokens.h"

/// \hideinitializer
#define USDHYDRA_TOKENS \
    (black) \
    (clamp) \
    ((displayLookBxdf, "displayLook:bxdf")) \
    (faceIndex) \
    (faceOffset) \
    (frame) \
    (HwPrimvar_1) \
    (HwPtexTexture_1) \
    (HwUvTexture_1) \
    ((infoFilename, "info:filename")) \
    ((infoVarname, "info:varname")) \
    (linear) \
    (linearMipmapLinear) \
    (linearMipmapNearest) \
    (magFilter) \
    (minFilter) \
    (mirror) \
    (nearest) \
    (nearestMipmapLinear) \
    (nearestMipmapNearest) \
    (repeat) \
    (textureMemory) \
    (uv) \
    (wrapS) \
    (wrapT)

/// \anchor UsdHydraTokens
///
/// <b>UsdHydraTokens</b> provides static, efficient TfToken's for
/// use in all public USD API
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdHydraTokens also contains all of the \em allowedTokens values declared
/// for schema builtin attributes of 'token' scene description type.
/// Use UsdHydraTokens like so:
///
/// \code
///     gprim.GetVisibilityAttr().Set(UsdHydraTokens->invisible);
/// \endcode
///
/// The tokens are:
/// \li <b>black</b> - Return black when sampling outside the bounds of the texture., Possible value for UsdHydraUvTexture::GetWrapTAttr(), Possible value for UsdHydraUvTexture::GetWrapSAttr()
/// \li <b>clamp</b> - The texture coordinate is clamped to [0,1]. , Possible value for UsdHydraUvTexture::GetWrapTAttr(), Possible value for UsdHydraUvTexture::GetWrapSAttr()
/// \li <b>displayLookBxdf</b> - UsdHydraLookAPI
/// \li <b>faceIndex</b> - UsdHydraPtexTexture
/// \li <b>faceOffset</b> - UsdHydraPtexTexture
/// \li <b>frame</b> - UsdHydraTexture
/// \li <b>HwPrimvar_1</b> - Special token for the usdHydra library.
/// \li <b>HwPtexTexture_1</b> - Special token for the usdHydra library.
/// \li <b>HwUvTexture_1</b> - Special token for the usdHydra library.
/// \li <b>infoFilename</b> - UsdHydraTexture, UsdHydraShader
/// \li <b>infoVarname</b> - UsdHydraPrimvar
/// \li <b>linear</b> - A weighted linear blend of nearest adjacent samples. , Possible value for UsdHydraUvTexture::GetMinFilterAttr(), Possible value for UsdHydraUvTexture::GetMagFilterAttr()
/// \li <b>linearMipmapLinear</b> - See https://www.opengl.org/wiki/Sampler_Object , Possible value for UsdHydraUvTexture::GetMinFilterAttr()
/// \li <b>linearMipmapNearest</b> - See https://www.opengl.org/wiki/Sampler_Object , Possible value for UsdHydraUvTexture::GetMinFilterAttr()
/// \li <b>magFilter</b> - UsdHydraUvTexture
/// \li <b>minFilter</b> - UsdHydraUvTexture
/// \li <b>mirror</b> - The texture coordinate wraps around like a mirror. -0.2 becomes 0.2, -1.2 becomes 0.8, etc. , Possible value for UsdHydraUvTexture::GetWrapTAttr(), Possible value for UsdHydraUvTexture::GetWrapSAttr()
/// \li <b>nearest</b> - Selects the nearest sample for the given coordinate , Possible value for UsdHydraUvTexture::GetMinFilterAttr(), Possible value for UsdHydraUvTexture::GetMagFilterAttr()
/// \li <b>nearestMipmapLinear</b> - See https://www.opengl.org/wiki/Sampler_Object , Possible value for UsdHydraUvTexture::GetMinFilterAttr()
/// \li <b>nearestMipmapNearest</b> - See https://www.opengl.org/wiki/Sampler_Object , Possible value for UsdHydraUvTexture::GetMinFilterAttr()
/// \li <b>repeat</b> - The texture coordinate wraps around the texture. So a texture coordinate of -0.2 becomes the equivalent of 0.8. , Possible value for UsdHydraUvTexture::GetWrapTAttr(), Possible value for UsdHydraUvTexture::GetWrapSAttr()
/// \li <b>textureMemory</b> - UsdHydraTexture
/// \li <b>uv</b> - UsdHydraUvTexture
/// \li <b>wrapS</b> - UsdHydraUvTexture
/// \li <b>wrapT</b> - UsdHydraUvTexture
TF_DECLARE_PUBLIC_TOKENS(UsdHydraTokens, USDHYDRA_TOKENS);

#endif
