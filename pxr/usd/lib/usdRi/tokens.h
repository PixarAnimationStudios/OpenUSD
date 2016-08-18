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
#ifndef USDRI_TOKENS_H
#define USDRI_TOKENS_H

/// \file usdRi/tokens.h

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// 
// This is an automatically generated file (by usdGenSchema.py).
// Do not hand-edit!
// 
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#include "pxr/usd/usdRi/api.h"
#include "pxr/base/tf/staticTokens.h"

/// \hideinitializer
#define USDRI_TOKENS \
    (argsPath) \
    (filePath) \
    ((infoArgsPath, "info:argsPath")) \
    ((infoFilePath, "info:filePath")) \
    ((infoOslPath, "info:oslPath")) \
    ((infoSloPath, "info:sloPath")) \
    ((riFocusRegion, "ri:focusRegion")) \
    ((riLookBxdf, "riLook:bxdf")) \
    ((riLookCoshaders, "riLook:coshaders")) \
    ((riLookDisplacement, "riLook:displacement")) \
    ((riLookPatterns, "riLook:patterns")) \
    ((riLookSurface, "riLook:surface")) \
    ((riLookVolume, "riLook:volume"))

/// \anchor UsdRiTokens
///
/// <b>UsdRiTokens</b> provides static, efficient TfToken's for
/// use in all public USD API
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdRiTokens also contains all of the \em allowedTokens values declared
/// for schema builtin attributes of 'token' scene description type.
/// Use UsdRiTokens like so:
///
/// \code
///     gprim.GetVisibilityAttr().Set(UsdRiTokens->invisible);
/// \endcode
///
/// The tokens are:
/// \li <b>argsPath</b> - UsdRiRisIntegrator
/// \li <b>filePath</b> - UsdRiRisIntegrator
/// \li <b>infoArgsPath</b> - UsdRiRisObject
/// \li <b>infoFilePath</b> - UsdRiRisOslPattern, UsdRiRisObject
/// \li <b>infoOslPath</b> - UsdRiRisOslPattern
/// \li <b>infoSloPath</b> - UsdRiRslShader
/// \li <b>riFocusRegion</b> - UsdRiStatements
/// \li <b>riLookBxdf</b> - UsdRiLookAPI
/// \li <b>riLookCoshaders</b> - UsdRiLookAPI
/// \li <b>riLookDisplacement</b> - UsdRiLookAPI
/// \li <b>riLookPatterns</b> - UsdRiLookAPI
/// \li <b>riLookSurface</b> - UsdRiLookAPI
/// \li <b>riLookVolume</b> - UsdRiLookAPI
TF_DECLARE_PUBLIC_TOKENS(UsdRiTokens, USDRI_API, USDRI_TOKENS);

#endif
