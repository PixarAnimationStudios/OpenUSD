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
#ifndef USDUI_TOKENS_H
#define USDUI_TOKENS_H

/// \file usdUI/tokens.h

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// 
// This is an automatically generated file (by usdGenSchema.py).
// Do not hand-edit!
// 
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#include "pxr/pxr.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \hideinitializer
#define USDUI_TOKENS \
    (closed) \
    (minimized) \
    (open) \
    ((uiNodegraphNodeDisplayColor, "ui:nodegraph:node:displayColor")) \
    ((uiNodegraphNodeExpansionState, "ui:nodegraph:node:expansionState")) \
    ((uiNodegraphNodeIcon, "ui:nodegraph:node:icon")) \
    ((uiNodegraphNodePos, "ui:nodegraph:node:pos")) \
    ((uiNodegraphNodeStackingOrder, "ui:nodegraph:node:stackingOrder"))

/// \anchor UsdUITokens
///
/// <b>UsdUITokens</b> provides static, efficient TfToken's for
/// use in all public USD API
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdUITokens also contains all of the \em allowedTokens values declared
/// for schema builtin attributes of 'token' scene description type.
/// Use UsdUITokens like so:
///
/// \code
///     gprim.GetVisibilityAttr().Set(UsdUITokens->invisible);
/// \endcode
///
/// The tokens are:
/// \li <b>closed</b> - Possible value for UsdUINodeGraphNodeAPI::GetUiNodegraphNodeExpansionStateAttr()
/// \li <b>minimized</b> - Possible value for UsdUINodeGraphNodeAPI::GetUiNodegraphNodeExpansionStateAttr()
/// \li <b>open</b> - Possible value for UsdUINodeGraphNodeAPI::GetUiNodegraphNodeExpansionStateAttr()
/// \li <b>uiNodegraphNodeDisplayColor</b> - UsdUINodeGraphNodeAPI
/// \li <b>uiNodegraphNodeExpansionState</b> - UsdUINodeGraphNodeAPI
/// \li <b>uiNodegraphNodeIcon</b> - UsdUINodeGraphNodeAPI
/// \li <b>uiNodegraphNodePos</b> - UsdUINodeGraphNodeAPI
/// \li <b>uiNodegraphNodeStackingOrder</b> - UsdUINodeGraphNodeAPI
TF_DECLARE_PUBLIC_TOKENS(UsdUITokens, USDUI_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
