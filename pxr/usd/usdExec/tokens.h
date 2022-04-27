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
#ifndef USDEXEC_TOKENS_H
#define USDEXEC_TOKENS_H

/// \file usdExec/tokens.h

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// 
// This is an automatically generated file (by usdGenSchema.py).
// Do not hand-edit!
// 
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#include "pxr/pxr.h"
#include "pxr/usd/usdExec/api.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdExecTokensType
///
/// \link UsdExecTokens \endlink provides static, efficient
/// \link TfToken TfTokens\endlink for use in all public USD API.
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdExecTokens also contains all of the \em allowedTokens values
/// declared for schema builtin attributes of 'token' scene description type.
/// Use UsdExecTokens like so:
///
/// \code
///     gprim.GetMyTokenValuedAttr().Set(UsdExecTokens->execMetadata);
/// \endcode
struct UsdExecTokensType {
    USDEXEC_API UsdExecTokensType();
    /// \brief "execMetadata"
    /// 
    /// Dictionary valued metadata key authored on ExecNode prims with implementationSource value of sourceAsset or  sourceCode to pass along metadata to the node parser or  compiler. It is also used to author metadata on node  properties in a UsdExec-based node definition file. 
    const TfToken execMetadata;
    /// \brief "full"
    /// 
    /// Possible value for 'connectability' metadata on a ExecInput. When connectability of an input is set to "full", it implies that it can be connected to any input or output. 
    const TfToken full;
    /// \brief "inputs:"
    /// 
    /// The prefix on exec attributes denoting an input. 
    const TfToken inputs;
    /// \brief "interfaceOnly"
    /// 
    /// Possible value for 'connectability' metadata on  a ExecInput. It implies that the input can only connect to  a NodeGraph Input (which represents an interface override, not  a render-time dataflow connection), or another Input whose  connectability is also 'interfaceOnly'. 
    const TfToken interfaceOnly;
    /// \brief "outputs:"
    /// 
    /// The prefix on exec attributes denoting an output. 
    const TfToken outputs;
    /// A vector of all of the tokens listed above.
    const std::vector<TfToken> allTokens;
};

/// \var UsdExecTokens
///
/// A global variable with static, efficient \link TfToken TfTokens\endlink
/// for use in all public USD API.  \sa UsdExecTokensType
extern USDEXEC_API TfStaticData<UsdExecTokensType> UsdExecTokens;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
