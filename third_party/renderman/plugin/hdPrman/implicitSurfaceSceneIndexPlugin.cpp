//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "hdPrman/implicitSurfaceSceneIndexPlugin.h"

#if PXR_VERSION >= 2208

#include "hdPrman/tokens.h"

#include "pxr/imaging/hd/dataSourceTypeDefs.h"
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

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<HdPrman_ImplicitSurfaceSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 0;

    for( auto const& rendererPluginName : HdPrman_GetPluginDisplayNames() ) {
        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            rendererPluginName,
            _tokens->sceneIndexPluginName,
            /* inputArgs = */nullptr,
            insertionPhase,
            HdSceneIndexPluginRegistry::InsertionOrderAtStart);
    }
}

namespace {

// XPU, XPU-CPU and XPU-GPU all need implicit surfaces tessellated for now, so
// we don't need to distinguish between them here.
enum class PRManVariant {
    RIS,
    XPU
};

std::string
_GetRendererDisplayName(
    const HdContainerDataSourceHandle &inputArgs)
{
    if (!TF_VERIFY(inputArgs)) {
        return "";
    }
    const auto nameDs = HdStringDataSource::Cast(
        inputArgs->Get(HdSceneIndexPluginRegistryTokens->rendererDisplayName));
    
    return nameDs? nameDs->GetTypedValue(0.0) : "";
}

PRManVariant
_GetPRManVariant(const HdContainerDataSourceHandle& inputArgs)
{
    return _GetRendererDisplayName(inputArgs) ==
        HdPrmanDisplayNamesTokens->RenderManRIS
        ? PRManVariant::RIS
        : PRManVariant::XPU;
}

static const HdDataSourceBaseHandle toMeshSrc =
    HdRetainedTypedSampledDataSource<TfToken>::New(
        HdsiImplicitSurfaceSceneIndexTokens->toMesh);

HdContainerDataSourceHandle
_GetTessellateAllDs()
{
    static const auto tessellateAllDs =
        HdRetainedContainerDataSource::New(
            HdPrimTypeTokens->sphere, toMeshSrc,
            HdPrimTypeTokens->cube, toMeshSrc,
            HdPrimTypeTokens->cone, toMeshSrc,
            HdPrimTypeTokens->cylinder, toMeshSrc,
#if PXR_VERSION >= 2411
            HdPrimTypeTokens->plane, toMeshSrc,
#endif
            HdPrimTypeTokens->capsule, toMeshSrc);

    return tessellateAllDs;
}

HdContainerDataSourceHandle
_GetTessellateRISDs()
{
    static const HdDataSourceBaseHandle axisToTransformSrc =
        HdRetainedTypedSampledDataSource<TfToken>::New(
            HdsiImplicitSurfaceSceneIndexTokens->axisToTransform);

    // RIS natively supports various quadric primitives (including cone,
    // cylinder and sphere), generating them such that they are rotationally
    // symmetric about the Z axis. To support other spine axes, configure the
    // scene index to overload the transform to account for the change of basis.
    // For unsupported primitives such as capsules and cubes, generate the
    // mesh instead.
    // 
    // Cone and cylinder need transforms updated, and cube and capsule
    // and plane still need to be tessellated.
    static const auto tessellateRISDs =
        HdRetainedContainerDataSource::New(
            HdPrimTypeTokens->cone, axisToTransformSrc,
            HdPrimTypeTokens->cylinder, axisToTransformSrc,
            HdPrimTypeTokens->cube, toMeshSrc,
    #if PXR_VERSION >= 2411
            HdPrimTypeTokens->plane, toMeshSrc,
    #endif
            HdPrimTypeTokens->capsule, toMeshSrc);

    return tessellateRISDs;
}

HdContainerDataSourceHandle
_GetImplicitInputArgs(PRManVariant variant)
{
    static bool tessellateAll =
        TfGetEnvSetting(HDPRMAN_TESSELLATE_IMPLICIT_SURFACES);

    if (tessellateAll) {
        return _GetTessellateAllDs();
    }

    if (variant == PRManVariant::RIS) {
        return _GetTessellateRISDs();
    }
    
    // XPU currently needs all implicit surfaces to be tessellated
    // while we wait for implicit sphere support to be added.
    return _GetTessellateAllDs();
}

} // anonymous namespace

HdPrman_ImplicitSurfaceSceneIndexPlugin::
HdPrman_ImplicitSurfaceSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdPrman_ImplicitSurfaceSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    return HdsiImplicitSurfaceSceneIndex::New(
        inputScene, _GetImplicitInputArgs(_GetPRManVariant(inputArgs)));
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_VERSION >= 2208
