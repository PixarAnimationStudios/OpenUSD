//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "hdPrman/implicitSurfaceSceneIndexPlugin.h"

#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hdsi/implicitSurfaceSceneIndex.h"

#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HDPRMAN_TESSELLATE_IMPLICIT_SURFACES, false,
    "Tessellate implicit surfaces into meshes, "
    "instead of using Renderman implicits");

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdPrman_ImplicitSurfaceSceneIndexPlugin"))
);

static const char * const _pluginDisplayName = "Prman";

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<HdPrman_ImplicitSurfaceSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 0;

    // Prman natively supports various quadric primitives (including cone,
    // cylinder and sphere), generating them such that they are rotationally
    // symmetric about the Z axis. To support other spine axes, configure the
    // scene index to overload the transform to account for the change of basis.
    // For unsupported primitives such as capsules and cubes, generate the
    // mesh instead.
    // 
    HdDataSourceBaseHandle const axisToTransformSrc =
        HdRetainedTypedSampledDataSource<TfToken>::New(
            HdsiImplicitSurfaceSceneIndexTokens->axisToTransform);
    HdDataSourceBaseHandle const toMeshSrc =
        HdRetainedTypedSampledDataSource<TfToken>::New(
            HdsiImplicitSurfaceSceneIndexTokens->toMesh);

    static bool tessellate =
        (TfGetEnvSetting(HDPRMAN_TESSELLATE_IMPLICIT_SURFACES) == true);

    HdContainerDataSourceHandle inputArgs;
    if (tessellate) {
        // Tessellate everything (legacy behavior).
        inputArgs =
            HdRetainedContainerDataSource::New(
                HdPrimTypeTokens->sphere, toMeshSrc,
                HdPrimTypeTokens->cube, toMeshSrc,
                HdPrimTypeTokens->cone, toMeshSrc,
                HdPrimTypeTokens->cylinder, toMeshSrc,
                HdPrimTypeTokens->capsule, toMeshSrc,
                HdPrimTypeTokens->plane, toMeshSrc);
    } else {
        // Cone and cylinder need transforms updated, and cube and capsule
        // and plane still need to be tessellated.
        inputArgs =
            HdRetainedContainerDataSource::New(
                HdPrimTypeTokens->cone, axisToTransformSrc,
                HdPrimTypeTokens->cylinder, axisToTransformSrc,
                HdPrimTypeTokens->cube, toMeshSrc,
                HdPrimTypeTokens->capsule, toMeshSrc,
                HdPrimTypeTokens->plane, toMeshSrc);
    }

    HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
        _pluginDisplayName,
        _tokens->sceneIndexPluginName,
        inputArgs,
        insertionPhase,
        HdSceneIndexPluginRegistry::InsertionOrderAtStart);
}

HdPrman_ImplicitSurfaceSceneIndexPlugin::
HdPrman_ImplicitSurfaceSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdPrman_ImplicitSurfaceSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    return HdsiImplicitSurfaceSceneIndex::New(inputScene, inputArgs);
}

PXR_NAMESPACE_CLOSE_SCOPE
