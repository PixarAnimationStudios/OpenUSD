//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdUI/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdUITokensType::UsdUITokensType() :
    accessibility("accessibility", TfToken::Immortal),
    accessibility_MultipleApplyTemplate_Description("accessibility:__INSTANCE_NAME__:description", TfToken::Immortal),
    accessibility_MultipleApplyTemplate_Label("accessibility:__INSTANCE_NAME__:label", TfToken::Immortal),
    accessibility_MultipleApplyTemplate_Priority("accessibility:__INSTANCE_NAME__:priority", TfToken::Immortal),
    closed("closed", TfToken::Immortal),
    default_("default", TfToken::Immortal),
    description("description", TfToken::Immortal),
    high("high", TfToken::Immortal),
    label("label", TfToken::Immortal),
    low("low", TfToken::Immortal),
    minimized("minimized", TfToken::Immortal),
    open("open", TfToken::Immortal),
    priority("priority", TfToken::Immortal),
    standard("standard", TfToken::Immortal),
    uiDescription("ui:description", TfToken::Immortal),
    uiDisplayGroup("ui:displayGroup", TfToken::Immortal),
    uiDisplayName("ui:displayName", TfToken::Immortal),
    uiNodegraphNodeDisplayColor("ui:nodegraph:node:displayColor", TfToken::Immortal),
    uiNodegraphNodeDocURI("ui:nodegraph:node:docURI", TfToken::Immortal),
    uiNodegraphNodeExpansionState("ui:nodegraph:node:expansionState", TfToken::Immortal),
    uiNodegraphNodeIcon("ui:nodegraph:node:icon", TfToken::Immortal),
    uiNodegraphNodePos("ui:nodegraph:node:pos", TfToken::Immortal),
    uiNodegraphNodeSize("ui:nodegraph:node:size", TfToken::Immortal),
    uiNodegraphNodeStackingOrder("ui:nodegraph:node:stackingOrder", TfToken::Immortal),
    AccessibilityAPI("AccessibilityAPI", TfToken::Immortal),
    Backdrop("Backdrop", TfToken::Immortal),
    NodeGraphNodeAPI("NodeGraphNodeAPI", TfToken::Immortal),
    SceneGraphPrimAPI("SceneGraphPrimAPI", TfToken::Immortal),
    allTokens({
        accessibility,
        accessibility_MultipleApplyTemplate_Description,
        accessibility_MultipleApplyTemplate_Label,
        accessibility_MultipleApplyTemplate_Priority,
        closed,
        default_,
        description,
        high,
        label,
        low,
        minimized,
        open,
        priority,
        standard,
        uiDescription,
        uiDisplayGroup,
        uiDisplayName,
        uiNodegraphNodeDisplayColor,
        uiNodegraphNodeDocURI,
        uiNodegraphNodeExpansionState,
        uiNodegraphNodeIcon,
        uiNodegraphNodePos,
        uiNodegraphNodeSize,
        uiNodegraphNodeStackingOrder,
        AccessibilityAPI,
        Backdrop,
        NodeGraphNodeAPI,
        SceneGraphPrimAPI
    })
{
}

TfStaticData<UsdUITokensType> UsdUITokens;

PXR_NAMESPACE_CLOSE_SCOPE
