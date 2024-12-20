//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
#include "pxr/usd/usdUI/api.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdUITokensType
///
/// \link UsdUITokens \endlink provides static, efficient
/// \link TfToken TfTokens\endlink for use in all public USD API.
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdUITokens also contains all of the \em allowedTokens values
/// declared for schema builtin attributes of 'token' scene description type.
/// Use UsdUITokens like so:
///
/// \code
///     gprim.GetMyTokenValuedAttr().Set(UsdUITokens->accessibility);
/// \endcode
struct UsdUITokensType {
    USDUI_API UsdUITokensType();
    /// \brief "accessibility"
    /// 
    /// Property namespace prefix for the UsdUIAccessibilityAPI schema.
    const TfToken accessibility;
    /// \brief "accessibility:__INSTANCE_NAME__:description"
    /// 
    /// UsdUIAccessibilityAPI
    const TfToken accessibility_MultipleApplyTemplate_Description;
    /// \brief "accessibility:__INSTANCE_NAME__:label"
    /// 
    /// UsdUIAccessibilityAPI
    const TfToken accessibility_MultipleApplyTemplate_Label;
    /// \brief "accessibility:__INSTANCE_NAME__:priority"
    /// 
    /// UsdUIAccessibilityAPI
    const TfToken accessibility_MultipleApplyTemplate_Priority;
    /// \brief "closed"
    /// 
    /// Possible value for UsdUINodeGraphNodeAPI::GetExpansionStateAttr()
    const TfToken closed;
    /// \brief "default"
    /// 
    /// Name of the anonymous instance name for the schema
    const TfToken default_;
    /// \brief "description"
    /// 
    /// Name of the description attribute
    const TfToken description;
    /// \brief "high"
    /// 
    /// Possible value for UsdUIAccessibilityAPI::GetPriorityAttr()
    const TfToken high;
    /// \brief "label"
    /// 
    /// Name of the label attribute
    const TfToken label;
    /// \brief "low"
    /// 
    /// Possible value for UsdUIAccessibilityAPI::GetPriorityAttr()
    const TfToken low;
    /// \brief "minimized"
    /// 
    /// Possible value for UsdUINodeGraphNodeAPI::GetExpansionStateAttr()
    const TfToken minimized;
    /// \brief "open"
    /// 
    /// Possible value for UsdUINodeGraphNodeAPI::GetExpansionStateAttr()
    const TfToken open;
    /// \brief "priority"
    /// 
    /// Name of the priority attribute
    const TfToken priority;
    /// \brief "standard"
    /// 
    /// Fallback value for UsdUIAccessibilityAPI::GetPriorityAttr()
    const TfToken standard;
    /// \brief "ui:description"
    /// 
    /// UsdUIBackdrop
    const TfToken uiDescription;
    /// \brief "ui:displayGroup"
    /// 
    /// UsdUISceneGraphPrimAPI
    const TfToken uiDisplayGroup;
    /// \brief "ui:displayName"
    /// 
    /// UsdUISceneGraphPrimAPI
    const TfToken uiDisplayName;
    /// \brief "ui:nodegraph:node:displayColor"
    /// 
    /// UsdUINodeGraphNodeAPI
    const TfToken uiNodegraphNodeDisplayColor;
    /// \brief "ui:nodegraph:node:docURI"
    /// 
    /// UsdUINodeGraphNodeAPI
    const TfToken uiNodegraphNodeDocURI;
    /// \brief "ui:nodegraph:node:expansionState"
    /// 
    /// UsdUINodeGraphNodeAPI
    const TfToken uiNodegraphNodeExpansionState;
    /// \brief "ui:nodegraph:node:icon"
    /// 
    /// UsdUINodeGraphNodeAPI
    const TfToken uiNodegraphNodeIcon;
    /// \brief "ui:nodegraph:node:pos"
    /// 
    /// UsdUINodeGraphNodeAPI
    const TfToken uiNodegraphNodePos;
    /// \brief "ui:nodegraph:node:size"
    /// 
    /// UsdUINodeGraphNodeAPI
    const TfToken uiNodegraphNodeSize;
    /// \brief "ui:nodegraph:node:stackingOrder"
    /// 
    /// UsdUINodeGraphNodeAPI
    const TfToken uiNodegraphNodeStackingOrder;
    /// \brief "AccessibilityAPI"
    /// 
    /// Schema identifer and family for UsdUIAccessibilityAPI
    const TfToken AccessibilityAPI;
    /// \brief "Backdrop"
    /// 
    /// Schema identifer and family for UsdUIBackdrop
    const TfToken Backdrop;
    /// \brief "NodeGraphNodeAPI"
    /// 
    /// Schema identifer and family for UsdUINodeGraphNodeAPI
    const TfToken NodeGraphNodeAPI;
    /// \brief "SceneGraphPrimAPI"
    /// 
    /// Schema identifer and family for UsdUISceneGraphPrimAPI
    const TfToken SceneGraphPrimAPI;
    /// A vector of all of the tokens listed above.
    const std::vector<TfToken> allTokens;
};

/// \var UsdUITokens
///
/// A global variable with static, efficient \link TfToken TfTokens\endlink
/// for use in all public USD API.  \sa UsdUITokensType
extern USDUI_API TfStaticData<UsdUITokensType> UsdUITokens;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
