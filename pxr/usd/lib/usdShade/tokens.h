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
#ifndef USDSHADE_TOKENS_H
#define USDSHADE_TOKENS_H

/// \file usdShade/tokens.h

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// 
// This is an automatically generated file (by usdGenSchema.py).
// Do not hand-edit!
// 
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#include "pxr/pxr.h"
#include "pxr/usd/usdShade/api.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdShadeTokensType
///
/// \link UsdShadeTokens \endlink provides static, efficient
/// \link TfToken TfTokens\endlink for use in all public USD API.
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdShadeTokens also contains all of the \em allowedTokens values
/// declared for schema builtin attributes of 'token' scene description type.
/// Use UsdShadeTokens like so:
///
/// \code
///     gprim.GetMyTokenValuedAttr().Set(UsdShadeTokens->connectedSourceFor);
/// \endcode
struct UsdShadeTokensType {
    USDSHADE_API UsdShadeTokensType();
    /// \brief "connectedSourceFor:"
    /// 
    /// The prefix on UsdShadeShader relationships associated with a Parameter.  This prefixed relationship has a suffix matching the associated attribute name, and denotes a logical shader connection between UsdShadeShaders. 
    const TfToken connectedSourceFor;
    /// \brief "derivesFrom"
    /// 
    /// A legacy relationship name specifying a specializes composition on a UsdShadeMaterial. 
    const TfToken derivesFrom;
    /// \brief "displacement"
    /// 
    /// Describes the displacement relationship terminal on a UsdShadeMaterial. Used to find the terminal UsdShadeShader describing the displacement of a UsdShadeMaterial. 
    const TfToken displacement;
    /// \brief "full"
    /// 
    /// Possible value for 'connectability' metadata on  a UsdShadeInput. When connectability of an input is set to  "full", it implies that it can be connected to any input or  output. 
    const TfToken full;
    /// \brief "info:id"
    /// 
    /// UsdShadeShader
    const TfToken infoId;
    /// \brief "inputs:"
    /// 
    /// The prefix on shading attributes denoting an input. 
    const TfToken inputs;
    /// \brief "interface:"
    /// 
    /// (DEPRECATED) The prefix on UsdShadeNodeGraph  attributes denoting an interface attribute. 
    const TfToken interface_;
    /// \brief "interfaceOnly"
    /// 
    /// Possible value for 'connectability' metadata on  a UsdShadeInput. It implies that the input can only connect to  a NodeGraph Input (which represents an interface override, not  a render-time dataflow connection), or another Input whose  connectability is also 'interfaceOnly'. 
    const TfToken interfaceOnly;
    /// \brief "interfaceRecipientsOf:"
    /// 
    /// (DEPRECATED) The prefix on UsdShadeNodeGraph relationships denoting the target of an interface attribute. 
    const TfToken interfaceRecipientsOf;
    /// \brief "look:binding"
    /// 
    /// The relationship name on non shading prims to denote a binding to a UsdShadeLook. This is a deprecated relationship and is superceded by material:binding. 
    const TfToken lookBinding;
    /// \brief "materialBind"
    /// 
    /// The name of the GeomSubset family used to  identify face subsets defined for the purpose of binding  materials to facesets. 
    const TfToken materialBind;
    /// \brief "material:binding"
    /// 
    ///  The relationship name on non-shading prims to denote a binding to a UsdShadeMaterial. 
    const TfToken materialBinding;
    /// \brief "materialVariant"
    /// 
    /// The variant name of material variation described on a UsdShadeMaterial. 
    const TfToken materialVariant;
    /// \brief "outputs:"
    /// 
    /// The prefix on shading attributes denoting an output. 
    const TfToken outputs;
    /// \brief "surface"
    /// 
    /// Describes the surface relationship terminal on a UsdShadeMaterial. Used to find the terminal UsdShadeShader describing the surface of a UsdShadeMaterial. 
    const TfToken surface;
    /// A vector of all of the tokens listed above.
    const std::vector<TfToken> allTokens;
};

/// \var UsdShadeTokens
///
/// A global variable with static, efficient \link TfToken TfTokens\endlink
/// for use in all public USD API.  \sa UsdShadeTokensType
extern USDSHADE_API TfStaticData<UsdShadeTokensType> UsdShadeTokens;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
