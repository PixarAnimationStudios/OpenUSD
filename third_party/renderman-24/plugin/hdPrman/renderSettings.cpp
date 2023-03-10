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
#include "hdPrman/renderSettings.h"
#include "hdPrman/renderParam.h"

#include "pxr/imaging/hd/sceneDelegate.h"


PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((outputsRiSampleFilters, "outputs:ri:sampleFilters"))
    ((outputsRiDisplayFilters, "outputs:ri:displayFilters"))
);

HdPrman_RenderSettings::HdPrman_RenderSettings(SdfPath const& id)
    : HdRenderSettings(id)
{
}

HdPrman_RenderSettings::~HdPrman_RenderSettings() = default;

void HdPrman_RenderSettings::Finalize(HdRenderParam *renderParam)
{
}

void HdPrman_RenderSettings::_Sync(
    HdSceneDelegate *sceneDelegate,
    HdRenderParam *renderParam,
    const HdDirtyBits *dirtyBits)
{
    HdPrman_RenderParam *param = static_cast<HdPrman_RenderParam*>(renderParam);

    if (*dirtyBits & HdRenderSettings::DirtyNamespacedSettings) {
        // NamespacedSettings contains all the Prman-specific Render Settings
        const VtDictionary& namespacedSettings = GetNamespacedSettings();

        // Set the SampleFilters connected to this Render Settings prim
        const auto sampleFilterIt = namespacedSettings.find(
            _tokens->outputsRiSampleFilters.GetString());
        if (sampleFilterIt != namespacedSettings.end()) {
            param->SetConnectedSampleFilterPaths(sceneDelegate,
                sampleFilterIt->second.GetWithDefault<SdfPathVector>());
        }
        // Set the DisplayFilters connected to this Render Settings prim
        const auto displayFilterIt = namespacedSettings.find(
            _tokens->outputsRiDisplayFilters.GetString());
        if (displayFilterIt != namespacedSettings.end()) {
            param->SetConnectedDisplayFilterPaths(sceneDelegate, 
                displayFilterIt->second.GetWithDefault<SdfPathVector>());
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
