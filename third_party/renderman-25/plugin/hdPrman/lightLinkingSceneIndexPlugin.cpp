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

#include "pxr/imaging/hd/version.h"

// There was no hdsi/version.h before this HD_API_VERSION.
#if HD_API_VERSION >= 58 

#include "pxr/imaging/hdsi/version.h"

#if HDSI_API_VERSION >= 13
#define HDPRMAN_USE_LIGHT_LINKING_SCENE_INDEX
#endif

#endif // #if HD_API_VERSION >= 58

#ifdef HDPRMAN_USE_LIGHT_LINKING_SCENE_INDEX

#include "pxr/imaging/hd/sceneIndexPlugin.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hdsi/lightLinkingSceneIndex.h"

#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdPrman_LightLinkingSceneIndexPlugin"))
);

TF_DEFINE_ENV_SETTING(HDPRMAN_ENABLE_LIGHT_LINKING_SCENE_INDEX, true, 
    "Enable registration for the light linking scene index.");

////////////////////////////////////////////////////////////////////////////////
// Plugin registration
////////////////////////////////////////////////////////////////////////////////

static const char * const _rendererDisplayName = "Prman";
class HdPrman_LightLinkingSceneIndexPlugin;

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<HdPrman_LightLinkingSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    if (TfGetEnvSetting(HDPRMAN_ENABLE_LIGHT_LINKING_SCENE_INDEX)) {

        // XXX Picking an arbitrary phase for now. If a procedural were to
        //     generate light prims, we'd want this to be after it.
        //     HdGpSceneIndexPlugin::GetInsertionPhase() currently returns 2.
        //
        const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 4;

        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            _rendererDisplayName,
            _tokens->sceneIndexPluginName,
            // XXX Update inputArgs to provide the list of geometry types
            //     supported by hdPrman.
            nullptr, 
            insertionPhase,
            HdSceneIndexPluginRegistry::InsertionOrderAtStart);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Scene Index Implementation
////////////////////////////////////////////////////////////////////////////////

class HdPrman_LightLinkingSceneIndexPlugin :
    public HdSceneIndexPlugin
{
public:
    HdPrman_LightLinkingSceneIndexPlugin() = default;

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override
    {
        return HdsiLightLinkingSceneIndex::New(inputScene, inputArgs);
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDPRMAN_USE_LIGHT_LINKING_SCENE_INDEX