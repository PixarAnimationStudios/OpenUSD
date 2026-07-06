//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "pxr/imaging/hdSt/implicitSurfaceSceneIndexPlugin.h"

#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hdsi/implicitSurfaceSceneIndex.h"
#include "pxr/imaging/hdSt/renderDelegate.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdSt_ImplicitSurfaceSceneIndexPlugin"))
);

static const char * const _pluginDisplayName = "GL";

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<HdSt_ImplicitSurfaceSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 0;

    HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
        _pluginDisplayName,
        _tokens->sceneIndexPluginName,
        /* inputArgs = */ nullptr,
        insertionPhase,
        HdSceneIndexPluginRegistry::InsertionOrderAtStart);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Implementation of HdSt_ImplicitSurfaceSceneIndexPlugin

HdSt_ImplicitSurfaceSceneIndexPlugin::
HdSt_ImplicitSurfaceSceneIndexPlugin() = default;

namespace {
const HdContainerDataSourceHandle _LocalInputArgs()
{
    // Configure the scene index to generate meshes for implicit primitives
    // that are not natively supported by Storm. With
    // HDST_ENABLE_NATIVE_SPHERES enabled, spheres can be rendered natively,
    // so we avoid converting them to meshes here.
    HdDataSourceBaseHandle const toMeshSrc =
    HdRetainedTypedSampledDataSource<TfToken>::New(
        HdsiImplicitSurfaceSceneIndexTokens->toMesh);
    
    TfTokenVector names = {
        HdPrimTypeTokens->cube,
        HdPrimTypeTokens->cone,
        HdPrimTypeTokens->cylinder,
        HdPrimTypeTokens->capsule,
        HdPrimTypeTokens->plane
    };

    if (!HdStRenderDelegate::IsEnabledNativeSphereRenderingSupport()) {
        names.emplace_back(HdPrimTypeTokens->sphere);
    }

    std::vector<HdDataSourceBaseHandle> values(names.size(), toMeshSrc);

    return HdRetainedContainerDataSource::New(names.size(),
                                              names.data(),
                                              values.data());
}
}

HdSceneIndexBaseRefPtr
HdSt_ImplicitSurfaceSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    // Define inputArgs here instead of in the TF_REGISTRY_FUNCTION block.
    // In the future, we may consider renaming the inputArgs parameter to
    // something like "sceneIndexGraphCreateArgs" to allow the app and renderer
    // plugin to provide arguments for scene indices instantiated via the
    // scene index plugin system.

    static HdContainerDataSourceHandle const localInputArgs = _LocalInputArgs();

    return HdsiImplicitSurfaceSceneIndex::New(inputScene, localInputArgs);
}

PXR_NAMESPACE_CLOSE_SCOPE
