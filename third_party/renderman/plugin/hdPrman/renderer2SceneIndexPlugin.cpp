//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/dependencyForwardingSceneIndexPlugin.h"
#include "hdPrman/rileyConversionSceneIndex.h"
#include "hdPrman/rileyFallbackMaterial.h"
#include "hdPrman/rileyGlobalsSceneIndex.h"
#include "hdPrman/tokens.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/sceneIndexUtil.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdPrman_Renderer2SceneIndexPlugin"))
);

// Scene index plugin that appends scene indices relevant to the
// hdPrman 2.0 pathway. This is early work in progress.
// 
class HdPrman_Renderer2SceneIndexPlugin : public HdSceneIndexPlugin
{
public:
    HdPrman_Renderer2SceneIndexPlugin() = default;

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override
    {
        // The order of the following scene indices doesn't seem to be
        // critical. Preserving the order from when each of them were separate
        // plugins just in case:
        // 1. HdPrman_RileyFallbackMaterialSceneIndexPlugin
        // 2. HdPrman_RileyConversionSceneIndexPlugin
        // 3. HdPrman_RileyGlobalsSceneIndexPlugin

        // Note that (3) isn't guarded by the
        // HD_PRMAN_EXPERIMENTAL_RILEY_SCENE_INDEX_OBSERVER setting.

#ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER
        const bool envEnabled = TfGetEnvSetting(
            HD_PRMAN_EXPERIMENTAL_RILEY_SCENE_INDEX_OBSERVER);
        HdSceneIndexBaseRefPtr si = inputScene;
        if (envEnabled) {
            si = HdPrman_RileyFallbackMaterial::AppendSceneIndex(si);
            si = HdPrman_RileyConversionSceneIndex::New(si);
        }
        si = HdPrman_RileyGlobalsSceneIndex::New(si, inputArgs);

        if (TfGetEnvSetting<bool>(HD_USE_ENCAPSULATING_SCENE_INDICES)) {
            si = HdMakeEncapsulatingSceneIndex({ inputScene }, si);
            si->SetDisplayName("HdPrman 2.0 plugins");
        }

        return si;
#else
        return inputScene;
#endif
    }
};


////////////////////////////////////////////////////////////////////////////////
// Plugin registration
////////////////////////////////////////////////////////////////////////////////

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<HdPrman_Renderer2SceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    // Towards the end before the dependency forwarding scene index.
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase =
        HdPrman_DependencyForwardingSceneIndexPlugin::GetInsertionPhase() - 1;

    for( auto const& rendererDisplayName : HdPrman_GetPluginDisplayNames()) {
        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            rendererDisplayName,
            _tokens->sceneIndexPluginName,
            /* inputArgs = */ nullptr,
            insertionPhase,
            HdSceneIndexPluginRegistry::InsertionOrderAtEnd);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
