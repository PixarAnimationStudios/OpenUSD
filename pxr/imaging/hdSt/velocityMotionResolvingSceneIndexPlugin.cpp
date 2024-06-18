//
// Copyright 2024 Pixar
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
#include "pxr/imaging/hdSt/velocityMotionResolvingSceneIndexPlugin.h"

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexObserver.h"
#include "pxr/imaging/hd/sceneIndexPlugin.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hdsi/velocityMotionResolvingSceneIndex.h"

#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/type.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (fps)
    ((sceneIndexPluginName, "HdSt_VelocityMotionResolvingSceneIndexPlugin")));

static const char* const _pluginDisplayName = "GL";

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<HdSt_VelocityMotionResolvingSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    // This one should go after implicit surface conversion
    static const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 0;
    const HdContainerDataSourceHandle inputArgs = 
        HdRetainedContainerDataSource::New(
            _tokens->fps,
            // TODO: Get real framerate!
            HdRetainedTypedSampledDataSource<float>::New(24.0));
    HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
        _pluginDisplayName,
        _tokens->sceneIndexPluginName,
        inputArgs,
        insertionPhase,
        HdSceneIndexPluginRegistry::InsertionOrderAtEnd);
}

HdSt_VelocityMotionResolvingSceneIndexPlugin::
HdSt_VelocityMotionResolvingSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdSt_VelocityMotionResolvingSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr& inputScene,
    const HdContainerDataSourceHandle& inputArgs)
{
    return HdsiVelocityMotionResolvingSceneIndex::New(inputScene, inputArgs);
}

PXR_NAMESPACE_CLOSE_SCOPE
