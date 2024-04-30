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

#include "hdPrman/implicitSurfaceSceneIndexPlugin.h"

#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hdsi/implicitSurfaceSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

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

    HdContainerDataSourceHandle const inputArgs =
        HdRetainedContainerDataSource::New(
            HdPrimTypeTokens->cone, axisToTransformSrc,
            HdPrimTypeTokens->cylinder, axisToTransformSrc,
            // HdSphereSchema doesn't specify an axis, so it can be omitted.
            HdPrimTypeTokens->cube, toMeshSrc,
            HdPrimTypeTokens->capsule, toMeshSrc);

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
