//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/renderSettingsFilteringSceneIndexPlugin.h"

#if PXR_VERSION >= 2308

#include "hdPrman/tokens.h"

#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/renderSettingsSchema.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hdsi/renderSettingsFilteringSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdPrman_RenderSettingsFilteringSceneIndexPlugin"))
);

TF_DEFINE_PRIVATE_TOKENS(
    _namespaceTokens,
    (ri)
    ((outputsRi, "outputs:ri"))
);

namespace {

// Note:
// In hdPrman, for the first SetOptions call, we compose a small set of default
// options, settings from the legacy render settings map and those from the
// environment (see HdPrman_RenderParam::SetRileyOptions).
// The fallback scene index prim isn't editable and since its opinions compose
// over those from the legacy map, we return an empty data source for
// namespaced settings to allow it to be overriden by the legacy render settings
// map. We'll revisit this in the future as we transition away from the legacy
// data flow.
//
HdContainerDataSourceHandle
_BuildFallbackNamespacedSettingsDataSource()
{
    return HdRetainedContainerDataSource::New();
}

HdContainerDataSourceHandle
_BuildFallbackRenderSettingsSchemaDataSource()
{
    // Return a valid data source to configure the scene index to add the
    // fallback prim if necessary.
    return
        HdRenderSettingsSchema::Builder()
        .SetNamespacedSettings(_BuildFallbackNamespacedSettingsDataSource())
        .SetActive(
            HdRetainedTypedSampledDataSource<bool>::New(false))
        // XXX: Add fallback render products, color space, purposes, etc.
        .Build();
}

HdContainerDataSourceHandle
_BuildFallbackRenderSettingsPrimDataSource()
{
    return
        HdRetainedContainerDataSource::New(
            HdRenderSettingsSchemaTokens->renderSettings,
            _BuildFallbackRenderSettingsSchemaDataSource());
}

}

////////////////////////////////////////////////////////////////////////////////
// Plugin registrations
////////////////////////////////////////////////////////////////////////////////

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<
        HdPrman_RenderSettingsFilteringSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 1;

    // Configure the scene index to:
    // 1. Filter in settings that have either the ri: or outputs:ri namespaces.
    // 2. Insert a fallback render settings prim.
    //
    // (2) is a workaround to address Riley's requirements around scene options.
    // See HdPrman_RenderParam::SetRileyOptions and
    //     HdPrman_RenderSettings::_Sync for further info.
    //
    const HdContainerDataSourceHandle inputArgs =
        HdRetainedContainerDataSource::New(
            HdsiRenderSettingsFilteringSceneIndexTokens->namespacePrefixes,
            HdRetainedTypedSampledDataSource<VtArray<TfToken>>::New(
#if PXR_VERSION >= 2311
                {_namespaceTokens->ri, _namespaceTokens->outputsRi}),
            HdsiRenderSettingsFilteringSceneIndexTokens->fallbackPrimDs,
            _BuildFallbackRenderSettingsPrimDataSource() );
#else
                {_namespaceTokens->ri, _namespaceTokens->outputsRi}));
#endif

    for( auto const& pluginDisplayName : HdPrman_GetPluginDisplayNames()) {
        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            pluginDisplayName,
            _tokens->sceneIndexPluginName,
            inputArgs,
            insertionPhase,
            HdSceneIndexPluginRegistry::InsertionOrderAtStart);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Scene Index Implementations
////////////////////////////////////////////////////////////////////////////////

HdPrman_RenderSettingsFilteringSceneIndexPlugin::
HdPrman_RenderSettingsFilteringSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdPrman_RenderSettingsFilteringSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    return HdsiRenderSettingsFilteringSceneIndex::New(inputScene, inputArgs);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_VERSION >= 2308
