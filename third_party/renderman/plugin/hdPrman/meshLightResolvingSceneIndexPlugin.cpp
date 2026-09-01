//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "hdPrman/meshLightResolvingSceneIndexPlugin.h"

#include "hdPrman/meshLightResolvingSceneIndex.h"
#include "hdPrman/tokens.h"

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceMaterialNetworkInterface.h"
#include "pxr/imaging/hd/materialNetworkInterface.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

#include "pxr/pxr.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdPrman_MeshLightResolvingSceneIndexPlugin"))
);

TF_MAKE_STATIC_DATA(
    std::vector<HdPrman_MeshLightResolvingSceneIndexPlugin
        ::ExtractMaterialGlowConnectionCallback>,
    _extractMaterialGlowConnectionCallbacks)
{
    _extractMaterialGlowConnectionCallbacks->clear();
}

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<HdPrman_MeshLightResolvingSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    // We need an "insertion point" that's *after* general material resolve.
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 115;
    const HdContainerDataSourceHandle inputRIS = HdRetainedContainerDataSource::New(
        HdPrmanDisplayNamesTokens->RenderManRIS,
        HdRetainedTypedSampledDataSource<bool>::New(true)
    );
    for(auto const& pluginDisplayName : HdPrman_GetPluginDisplayNames()) {
        // Pass a token through to signify RIS so we can disable motion blur.
        const bool isRIS = (pluginDisplayName == HdPrmanDisplayNamesTokens->RenderManRIS);
        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            pluginDisplayName,
            _tokens->sceneIndexPluginName,
            isRIS ? inputRIS : nullptr,
            insertionPhase,
            HdSceneIndexPluginRegistry::InsertionOrderAtStart);
    }
}

HdPrman_MeshLightResolvingSceneIndexPlugin::
HdPrman_MeshLightResolvingSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdPrman_MeshLightResolvingSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    TfRegistryManager::GetInstance()
        .SubscribeTo<HdPrman_MeshLightResolvingSceneIndexPlugin>();

    const bool isRIS = (inputArgs && inputArgs->Get(HdPrmanDisplayNamesTokens->RenderManRIS));
    return HdPrmanMeshLightResolvingSceneIndex::New(inputScene, isRIS);
}

/* static */
void
HdPrman_MeshLightResolvingSceneIndexPlugin
    ::RegisterExtractMaterialGlowConnectionCallback(
    const HdPrman_MeshLightResolvingSceneIndexPlugin
        ::ExtractMaterialGlowConnectionCallback& callback)
{
    _extractMaterialGlowConnectionCallbacks->push_back(callback);
}

/* static */
HdMaterialNetworkInterface::InputConnectionResult
HdPrman_MeshLightResolvingSceneIndexPlugin::ExtractMaterialGlowConnection(
    const SdfPath& matPrimPath,
    const HdContainerDataSourceHandle& matPrimDS,
    const HdDataSourceMaterialNetworkInterface& matNI)
{
    HdMaterialNetworkInterface::InputConnectionResult result;
    for (const auto& callback : *_extractMaterialGlowConnectionCallbacks) {
        result = callback(matPrimPath, matPrimDS, matNI);
        if (result.first) {
            return result;
        }
    }
    return { false, { TfToken(), TfToken() } };
}

PXR_NAMESPACE_CLOSE_SCOPE
