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
#include "hdPrman/extComputationPrimvarPruningSceneIndexPlugin.h"

#include "hdPrman/tokens.h"

#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"

#include "pxr/pxr.h"
#if PXR_VERSION >= 2402
#include "pxr/imaging/hdsi/extComputationPrimvarPruningSceneIndex.h"
#endif

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdPrman_ExtComputationPrimvarPruningSceneIndexPlugin"))
);

////////////////////////////////////////////////////////////////////////////////
// Plugin registrations
////////////////////////////////////////////////////////////////////////////////

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<
        HdPrman_ExtComputationPrimvarPruningSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    // Needs to be inserted earlier to allow plugins that follow to transform
    // primvar data without having to concern themselves about computed
    // primvars.
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 0;

    // Register the plugins conditionally.
    for(const auto& rendererDisplayName : HdPrman_GetPluginDisplayNames()) {
        // Register the plugins conditionally.
        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            rendererDisplayName,
            _tokens->sceneIndexPluginName,
            nullptr, // no argument data necessary
            insertionPhase,
            HdSceneIndexPluginRegistry::InsertionOrderAtStart);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Scene Index Implementations
////////////////////////////////////////////////////////////////////////////////

HdPrman_ExtComputationPrimvarPruningSceneIndexPlugin::
HdPrman_ExtComputationPrimvarPruningSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdPrman_ExtComputationPrimvarPruningSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    TF_UNUSED(inputArgs);
#if PXR_VERSION >= 2402
    return HdSiExtComputationPrimvarPruningSceneIndex::New(inputScene);
#else
    return inputScene;
#endif
}

PXR_NAMESPACE_CLOSE_SCOPE
