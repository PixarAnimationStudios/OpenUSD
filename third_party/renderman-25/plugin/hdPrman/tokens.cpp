//
// Copyright 2022 Pixar
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
#include "hdPrman/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(HdPrmanTokens, HD_PRMAN_TOKENS);
TF_DEFINE_PUBLIC_TOKENS(HdPrmanRileyPrimTypeTokens,
                        HD_PRMAN_RILEY_PRIM_TYPE_TOKENS);
TF_DEFINE_PUBLIC_TOKENS(HdPrmanRileyAdditionalRoleTokens,
                        HD_PRMAN_RILEY_ADDITIONAL_ROLE_TOKENS);
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

