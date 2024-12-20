//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdUI/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdUITokensType::UsdUITokensType() :
    closed("closed", TfToken::Immortal),
    default_("default", TfToken::Immortal),
    lang("lang", TfToken::Immortal),
    localization("localization", TfToken::Immortal),
    localization_MultipleApplyTemplate_Language("localization:__INSTANCE_NAME__:language", TfToken::Immortal),
    minimized("minimized", TfToken::Immortal),
    open("open", TfToken::Immortal),
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
    Backdrop("Backdrop", TfToken::Immortal),
    LocalizationAPI("LocalizationAPI", TfToken::Immortal),
    NodeGraphNodeAPI("NodeGraphNodeAPI", TfToken::Immortal),
    SceneGraphPrimAPI("SceneGraphPrimAPI", TfToken::Immortal),
    allTokens({
        closed,
        default_,
        lang,
        localization,
        localization_MultipleApplyTemplate_Language,
        minimized,
        open,
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
        Backdrop,
        LocalizationAPI,
        NodeGraphNodeAPI,
        SceneGraphPrimAPI
    })
{
}

TfStaticData<UsdUITokensType> UsdUITokens;

PXR_NAMESPACE_CLOSE_SCOPE
