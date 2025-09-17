//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/sceneIndices.h"

#include "pxr/usdImaging/usdImaging/drawModeSceneIndex.h"
#include "pxr/usdImaging/usdImaging/extentResolvingSceneIndex.h"
#include "pxr/usdImaging/usdImaging/instanceProxyPathTranslationSceneIndex.h"
// #include "pxr/usdImaging/usdImaging/instanceProxyPathTranslationSceneIndex.h"
#include "pxr/usdImaging/usdImaging/materialBindingsResolvingSceneIndex.h"
#include "pxr/usdImaging/usdImaging/niPrototypePropagatingSceneIndex.h"
#include "pxr/usdImaging/usdImaging/piPrototypePropagatingSceneIndex.h"
#include "pxr/usdImaging/usdImaging/renderSettingsFlatteningSceneIndex.h"
#include "pxr/usdImaging/usdImaging/sceneIndexPlugin.h"
#include "pxr/usdImaging/usdImaging/selectionSceneIndex.h"
#include "pxr/usdImaging/usdImaging/stageSceneIndex.h"
#include "pxr/usdImaging/usdImaging/unloadedDrawModeSceneIndex.h"

#include "pxr/usdImaging/usdImaging/geomModelSchema.h"
#include "pxr/usdImaging/usdImaging/modelSchema.h"
#include "pxr/usdImaging/usdImaging/materialBindingsSchema.h"

#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/purposeSchema.h"
#include "pxr/imaging/hd/noticeBatchingSceneIndex.h"
#include "pxr/imaging/hd/sceneIndexUtil.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(
    USDIMAGING_SET_STAGE_AFTER_CHAINING_SCENE_INDICES, true,
    "If true (default), set the stage on the scene index *after* creating the "
    "usdImaging scene indices graph. This results in added notices flowing "
    "through the graph."
    "If false, scene indices downstream of the stage scene index won't receive "
    "added notices, and may need to query the input scene index for prim "
    "discovery and bookkeeping."
    "Each of these options have different performance characteristics.");

TF_REGISTRY_FUNCTION(TfType)
{
    TfRegistryManager::GetInstance().SubscribeTo<UsdImagingSceneIndexPlugin>();
}

static
bool
_ShouldSetStageAfterChainingSceneIndices()
{
    static const bool result =
        TfGetEnvSetting(USDIMAGING_SET_STAGE_AFTER_CHAINING_SCENE_INDICES);
    return result;
}

static
HdSceneIndexBaseRefPtr
_AddPluginSceneIndices(HdSceneIndexBaseRefPtr sceneIndex)
{
    TRACE_FUNCTION();
    
    for (const UsdImagingSceneIndexPluginUniquePtr &sceneIndexPlugin :
             UsdImagingSceneIndexPlugin::GetAllSceneIndexPlugins()) {
        sceneIndex = sceneIndexPlugin->AppendSceneIndex(sceneIndex);
    }
    
    return sceneIndex;
}

static
HdContainerDataSourceHandle
_AdditionalStageSceneIndexInputArgs(
    const bool displayUnloadedPrimsWithBounds)
{
    if (!displayUnloadedPrimsWithBounds) {
        return nullptr;
    }
    static HdContainerDataSourceHandle const ds =
        HdRetainedContainerDataSource::New(
            UsdImagingStageSceneIndexTokens->includeUnloadedPrims,
            HdRetainedTypedSampledDataSource<bool>::New(true));
    return ds;
}

// Use extentsHint (of models) for purpose geometry
static
HdContainerDataSourceHandle
_ExtentResolvingSceneIndexInputArgs()
{
    HdDataSourceBaseHandle const purposeDataSources[] = {
        HdRetainedTypedSampledDataSource<TfToken>::New(
            HdTokens->geometry) };

    return
        HdRetainedContainerDataSource::New(
            UsdImagingExtentResolvingSceneIndexTokens->purposes,
            HdRetainedSmallVectorDataSource::New(
                TfArraySize(purposeDataSources),
                purposeDataSources));
}

static
std::string
_GetStageName(UsdStageRefPtr const &stage)
{
    if (!stage) {
        return {};
    }
    SdfLayerHandle const rootLayer = stage->GetRootLayer();
    if (!rootLayer) {
        return {};
    }
    return rootLayer->GetIdentifier();
}

static
TfTokenVector
_InstanceDataSourceNames()
{
    TRACE_FUNCTION();
    
    // In order for USD instances to share a prototype they must share the
    // following Hydra schemas, which are used for Hydra-side instance
    // aggregation.  Due to their inheritance semantics these schemas may
    // require different aggregation of prototypes in Hydra as compared
    // to the underlying USD stage prototypes.
    TfTokenVector result = {
        UsdImagingMaterialBindingsSchema::GetSchemaToken(),
        HdPurposeSchema::GetSchemaToken(),
        UsdImagingGeomModelSchema::GetSchemaToken(),
        // We include the model schema in order to aggregate scene indices by
        // assetInfo, which may be used in material networks for texture
        // asset resolution.  See HdDataSourceMaterialNetworkInterface::
        // GetModelAssetName().
        UsdImagingModelSchema::GetSchemaToken()
    };

    for (const UsdImagingSceneIndexPluginUniquePtr &plugin :
             UsdImagingSceneIndexPlugin::GetAllSceneIndexPlugins()) {
        for (const TfToken &name : plugin->InstanceDataSourceNames()) {
            result.push_back(name);
        }
    }

    return result;
};

static
TfTokenVector
_ProxyPathTranslationDataSourceNames()
{
    TRACE_FUNCTION();
    
    TfTokenVector result = {
        // Translate material bindings to instance proxies.
        UsdImagingMaterialBindingsSchema::GetSchemaToken(),
    };

    for (const UsdImagingSceneIndexPluginUniquePtr &plugin :
             UsdImagingSceneIndexPlugin::GetAllSceneIndexPlugins()) {
        for (const TfToken &name :
             plugin->ProxyPathTranslationDataSourceNames()) {
            result.push_back(name);
        }
    }

    return result;
};

UsdImagingSceneIndices
UsdImagingCreateSceneIndices(
    const UsdImagingCreateSceneIndicesInfo &createInfo)
{
    TRACE_FUNCTION();

    UsdImagingSceneIndices result;

    HdSceneIndexBaseRefPtr sceneIndex;
    
    sceneIndex = result.stageSceneIndex =
        UsdImagingStageSceneIndex::New(
            HdOverlayContainerDataSource::OverlayedContainerDataSources(
                _AdditionalStageSceneIndexInputArgs(
                    createInfo.displayUnloadedPrimsWithBounds),
                createInfo.stageSceneIndexInputArgs));

    if (!_ShouldSetStageAfterChainingSceneIndices()) {
        // Downstream scene indices will not receive added notices since they
        // haven't been chained yet.
        result.stageSceneIndex->SetStage(createInfo.stage);
    }
    
    if (createInfo.overridesSceneIndexCallback) {
        sceneIndex =
            createInfo.overridesSceneIndexCallback(sceneIndex);
    }

    if (createInfo.displayUnloadedPrimsWithBounds) {
        sceneIndex =
            UsdImagingUnloadedDrawModeSceneIndex::New(sceneIndex);
    }
    
    sceneIndex =
        UsdImagingExtentResolvingSceneIndex::New(
            sceneIndex, _ExtentResolvingSceneIndexInputArgs());

    {
        TRACE_FUNCTION_SCOPE("UsdImagingPiPrototypePropagatingSceneIndex");

        sceneIndex =
            UsdImagingPiPrototypePropagatingSceneIndex::New(sceneIndex);
    }

    {
        TRACE_FUNCTION_SCOPE("UsdImagingNiPrototypePropagatingSceneIndex");

        // UsdImagingNiPrototypePropagatingSceneIndex

        // Names of data sources that need to have the same values
        // across native instances for the instances be aggregated
        // together.
        static const TfTokenVector instanceDataSourceNames =
            _InstanceDataSourceNames();

        using SceneIndexAppendCallback =
            UsdImagingNiPrototypePropagatingSceneIndex::
            SceneIndexAppendCallback;

        // The draw mode scene index needs to be inserted multiple times
        // during prototype propagation because:
        // - A native instance can be grouped under a prim with non-trivial
        //   draw mode. In this case, the draw mode scene index needs to
        //   filter out the native instance before instance aggregation.
        // - A native instance itself can have a non-trivial draw mode.
        //   In this case, we want to aggregate the native instances
        //   with the same draw mode, so we need to run instance aggregation
        //   first.
        // - Advanced scenarios such as native instances in USD prototypes
        //   and the composition semantics of draw mode: the draw mode is
        //   inherited but apply draw mode is not and the draw mode is
        //   only applied when it is non-trivial and apply draw mode is true.
        //
        // Thus, we give the prototype propagating scene index a callback.
        //
        SceneIndexAppendCallback callback;
        if (createInfo.addDrawModeSceneIndex) {
            callback = [](HdSceneIndexBaseRefPtr const &inputSceneIndex) {
                return UsdImagingDrawModeSceneIndex::New(
                    inputSceneIndex, /* inputArgs = */ nullptr); };
        }

        sceneIndex =
            UsdImagingNiPrototypePropagatingSceneIndex::New(
                sceneIndex, instanceDataSourceNames, callback);
    }

    sceneIndex = result.postInstancingNoticeBatchingSceneIndex =
        HdNoticeBatchingSceneIndex::New(sceneIndex);

    // Names of data sources that contain SdfPath-valued data
    // sources that may target instance proxies, and which require
    // translation to corresponding prototype paths.
    static const TfTokenVector proxyPathTranslationDataSourceNames =
        _ProxyPathTranslationDataSourceNames();

    sceneIndex = UsdImaging_InstanceProxyPathTranslationSceneIndex::New(
        sceneIndex, proxyPathTranslationDataSourceNames);

    sceneIndex = UsdImagingMaterialBindingsResolvingSceneIndex::New(
                        sceneIndex, /* inputArgs = */ nullptr);

    sceneIndex =
        _AddPluginSceneIndices(sceneIndex);
    
    sceneIndex = result.selectionSceneIndex =
        UsdImagingSelectionSceneIndex::New(sceneIndex);
    
    sceneIndex =
        UsdImagingRenderSettingsFlatteningSceneIndex::New(sceneIndex);

    if (TfGetEnvSetting<bool>(HD_USE_ENCAPSULATING_SCENE_INDICES)) {
        sceneIndex = HdMakeEncapsulatingSceneIndex({}, sceneIndex);
        sceneIndex->SetDisplayName(
            "UsdImaging " + _GetStageName(createInfo.stage));
    }

    result.finalSceneIndex = sceneIndex;

    if (_ShouldSetStageAfterChainingSceneIndices()) {
        // Setting the stage populates the scene index and results in added
        // notices flowing downstream.
        result.stageSceneIndex->SetStage(createInfo.stage);
    }

    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
