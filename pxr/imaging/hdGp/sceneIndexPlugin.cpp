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
#include "pxr/imaging/hdGp/sceneIndexPlugin.h"
#include "pxr/imaging/hdGp/generativeProceduralResolvingSceneIndex.h"
#include "pxr/imaging/hdGp/generativeProceduralPluginRegistry.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"

#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (proceduralPrimTypeName)
);

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<HdGpSceneIndexPlugin>();
}

TF_DEFINE_ENV_SETTING(HDGP_INCLUDE_DEFAULT_RESOLVER, false,
    "Register a default hydra generative procedural resolver to the scene index"
    " chain.");

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    // For now, do not add the procedural resolving scene index by default but
    // allow activation of a default configured instance via envvar.
    if (TfGetEnvSetting(HDGP_INCLUDE_DEFAULT_RESOLVER) == true) {

        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            TfToken(), // empty token means all renderers
            TfToken("HdGpSceneIndexPlugin"),
            nullptr,   // no argument data necessary
            HdGpSceneIndexPlugin::GetInsertionPhase(),
            HdSceneIndexPluginRegistry::InsertionOrderAtStart);
    }
}

HdGpSceneIndexPlugin::HdGpSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdGpSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    // Ensure that procedurals are discovered are prior to the scene index
    // querying for specific procedurals. Absence of this was causing a test
    // case to non-deterministically fail due to not finding a registered
    // procedural.
    HdGpGenerativeProceduralPluginRegistry::GetInstance();
    
    HdSceneIndexBaseRefPtr result = inputScene;

    if (inputArgs) {
        using _TokenDs = HdTypedSampledDataSource<TfToken>;
        if (_TokenDs::Handle tds =_TokenDs::Cast(
                inputArgs->Get(_tokens->proceduralPrimTypeName))) {
            return HdGpGenerativeProceduralResolvingSceneIndex::New(result,
                tds->GetTypedValue(0.0f));
        }
    }

    return HdGpGenerativeProceduralResolvingSceneIndex::New(result);
}

PXR_NAMESPACE_CLOSE_SCOPE



