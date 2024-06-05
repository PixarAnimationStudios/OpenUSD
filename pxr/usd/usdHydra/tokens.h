//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
    /// Possible value for "wrapT" and "wrapS" inputs on a "UvTexture" shader prim. Causes black to be returned when sampling outside the bounds of the texture.
    const TfToken black;
    /// \brief "clamp"
    /// 
    /// Possible value for "wrapT" and "wrapS" inputs on a "UvTexture" shader prim. Causes the the texture coordinate to be clamped to [0,1].
    const TfToken clamp;
    /// \brief "displayLook:bxdf"
    /// 
    /// deprecated This has been deprecated in favor of the glslfx:surface output.  Relationship on a material that targets the "bxdf" or the surface shader prim.
    const TfToken displayLookBxdf;
    /// \brief "faceIndex"
    /// 
    /// The "faceIndex" shader input on a hydra "PtexTexture" shader.
    const TfToken faceIndex;
    /// \brief "faceOffset"
    /// 
    /// The "faceOffset" shader input on a hydra "PtexTexture" shader.
    const TfToken faceOffset;
    /// \brief "frame"
    /// 
    /// A shader input on a "Texture" shader.
    const TfToken frame;
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
    /// \brief "hydraGenerativeProcedural"
    /// 
    /// Fallback value for UsdHydraGenerativeProceduralAPI::GetProceduralSystemAttr()
    const TfToken hydraGenerativeProcedural;
    /// \brief "inputs:file"
    /// 
    /// The special "info:filename" property of a hydra Texture shader, which points to a resolvable texture asset.
    const TfToken infoFilename;
    /// \brief "inputs:varname"
    /// 
    ///  
    const TfToken infoVarname;
    /// \brief "linear"
    /// 
    /// A weighted linear blend of nearest adjacent samples. Possible value for "minFilter" and "magFilter" inputs on a UvTextureshader.
    const TfToken linear;
    /// \brief "linearMipmapLinear"
    /// 
    /// See https://www.opengl.org/wiki/Sampler_Object , Possible value for the "minFilter" input on a UvTexture shader. 
    const TfToken linearMipmapLinear;
    /// \brief "linearMipmapNearest"
    /// 
    /// See https://www.opengl.org/wiki/Sampler_Object  Possible value for the "minFilter" input on a UvTexture shader. 
    const TfToken linearMipmapNearest;
    /// \brief "magFilter"
    /// 
    /// An input on a UvTexture shader.
    const TfToken magFilter;
    /// \brief "minFilter"
    /// 
    /// An input on a UvTexture shader.
    const TfToken minFilter;
    /// \brief "mirror"
    /// 
    /// Possible value for "wrapT" and "wrapS" inputs on a "UvTexture" shader prim. Causes the texture coordinate to wrap around like a mirror. -0.2 becomes 0.2, -1.2 becomes 0.8, etc. ,
    const TfToken mirror;
    /// \brief "nearest"
    /// 
    /// Selects the nearest sample for the given coordinate  Possible value for "minFilter" and "magFilter" inputs on a UvTexture shader.
    const TfToken nearest;
    /// \brief "nearestMipmapLinear"
    /// 
    /// See https://www.opengl.org/wiki/Sampler_Object Possible value for "minFilter" and "magFilter" inputs on a UvTexture shader.
    const TfToken nearestMipmapLinear;
    /// \brief "nearestMipmapNearest"
    /// 
    /// See https://www.opengl.org/wiki/Sampler_Object Possible value for the "minFilter" input on a UvTexture shader. 
    const TfToken nearestMipmapNearest;
    /// \brief "primvars:hdGp:proceduralType"
    /// 
    /// UsdHydraGenerativeProceduralAPI
    const TfToken primvarsHdGpProceduralType;
    /// \brief "proceduralSystem"
    /// 
    /// UsdHydraGenerativeProceduralAPI
    const TfToken proceduralSystem;
    /// \brief "repeat"
    /// 
    /// Possible value for "wrapT" and "wrapS" inputs on a "UvTexture" shader prim.  Causes the texture coordinate to wrap around the texture. So a texture coordinate of -0.2 becomes the equivalent of 0.8.
    const TfToken repeat;
    /// \brief "textureMemory"
    /// 
    /// A shader input on a hydra Texture shader.
    const TfToken textureMemory;
    /// \brief "useMetadata"
    /// 
    /// Possible value for "wrapT" and "wrapS" inputs on a "UvTexture" shader prim. Causes the wrap value to be loaded from the texture file instead of being specified in the prim.  If the texture file doesn't support metadata or the metadata doesn't contain a wrap mode,  the "black" wrap mode is used.
    const TfToken useMetadata;
    /// \brief "uv"
    /// 
    /// A shader input on a hydra UvTexture shader.
    const TfToken uv;
    /// \brief "wrapS"
    /// 
    /// A shader input on a hydra UvTexture shader which defines the behavior of texture coordinates that are outside the bounds of the texture.
    const TfToken wrapS;
    /// \brief "wrapT"
    /// 
    /// A shader input on a hydra UvTexture shader which defines the behavior of texture coordinates that are outside the bounds of the texture.
    const TfToken wrapT;
    /// \brief "HydraGenerativeProceduralAPI"
    /// 
    /// Schema identifer and family for UsdHydraGenerativeProceduralAPI
    const TfToken HydraGenerativeProceduralAPI;
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
