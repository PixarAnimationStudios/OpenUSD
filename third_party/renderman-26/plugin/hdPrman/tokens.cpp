//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(HdPrmanTokens, HD_PRMAN_TOKENS);
TF_DEFINE_PUBLIC_TOKENS(HdPrmanRileyPrimTypeTokens,
                        HD_PRMAN_RILEY_PRIM_TYPE_TOKENS);
TF_DEFINE_PUBLIC_TOKENS(HdPrmanRileyAdditionalRoleTokens,
                        HD_PRMAN_RILEY_ADDITIONAL_ROLE_TOKENS);
TF_DEFINE_PUBLIC_TOKENS(HdPrmanRenderParamTokens, HD_PRMAN_RENDER_PARAM_TOKENS);
TF_DEFINE_PUBLIC_TOKENS(HdPrmanPluginTokens, HD_PRMAN_PLUGIN_TOKENS);

TF_MAKE_STATIC_DATA(std::vector<std::string>, _pluginDisplayNameTokens) {
    _pluginDisplayNameTokens->push_back("Prman");
    _pluginDisplayNameTokens->push_back("RenderMan RIS");
    _pluginDisplayNameTokens->push_back("RenderMan XPU");
    _pluginDisplayNameTokens->push_back("RenderMan XPU - CPU");
    _pluginDisplayNameTokens->push_back("RenderMan XPU - GPU");
}

const std::vector<std::string>& HdPrman_GetPluginDisplayNames() {
    return *_pluginDisplayNameTokens;
}


PXR_NAMESPACE_CLOSE_SCOPE

