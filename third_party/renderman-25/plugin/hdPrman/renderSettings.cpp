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
#include "hdPrman/debugCodes.h"
#include "hdPrman/debugUtil.h"
#include "hdPrman/renderParam.h"
#include "hdPrman/rixStrings.h"
#include "hdPrman/utils.h"

#include "pxr/imaging/hd/cameraSchema.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/utils.h"

#include <string>
#include <iostream> // XXX

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _renderTerminalTokens, // properties in PxrRenderTerminalsAPI
    ((outputsRiIntegrator, "outputs:ri:integrator"))
    ((outputsRiSampleFilters, "outputs:ri:sampleFilters"))
    ((outputsRiDisplayFilters, "outputs:ri:displayFilters"))
);


namespace {

// Translate properties in PxrOptionsAPI to the Riley name.
RtUString
_GetRiName(std::string const &propertyName)
{
    // strip the "ri:" prefix if present, but don't strip the "Ri:" namespace.
    // e.g. schema attribute "ri:hider:maxsamples" maps to "hider:maxsamples"
    //      (or the pre-defined UString Rix::k_hider_maxsamples)
    //      while "ri:Ri:CropWindow" maps to "Ri:CropWindow" (or
    //      the UString k_riCropWindow)
    //
    if (TfStringStartsWith(propertyName, "ri:")) {
        return RtUString(propertyName.c_str() + 3);
    }

    // Unhandled property. This likely indicates an issue with namespace 
    // filtering upstream.
    TF_WARN("Could not translate settings property %s to RtUString.",
            propertyName.c_str());
    return RtUString(propertyName.c_str());
}

RtParamList
_GenerateParamList(VtDictionary const &settings)
{
    RtParamList options;

    for (auto const &pair : settings) {
        // Skip render terminal connections.
        const std::string &name = pair.first;
        const TfToken tokenName(name);
        if (tokenName == _renderTerminalTokens->outputsRiIntegrator    ||
            tokenName == _renderTerminalTokens->outputsRiSampleFilters ||
            tokenName == _renderTerminalTokens->outputsRiDisplayFilters) {

            continue;
        }

        RtUString const riName = _GetRiName(name);
        VtValue const &val = pair.second;
        HdPrman_Utils::SetParamFromVtValue(
            riName, val, /* role */ TfToken(), &options);
    }

    return options;
}

}


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

    // XXX Preserve existing data flow for clients that don't populate the
    //     sceneGlobals.activeRenderSettingsPrim locator at the root prim of the
    //     scene index. In this scenario, integrator, sample and display
    //     filters connected to the render settings prim are used. This works
    //     only when a single render settings prim is present in the scene.
    //     When multiple render settings prims are present in the scene, because
    //     the Sync order is non-deterministic, the last sync'd prim's opinions
    //     would be used.
    //

    const bool hasActiveRsp = HdUtils::HasActiveRenderSettingsPrim(
        sceneDelegate->GetRenderIndex().GetTerminalSceneIndex());
     
    if (IsActive() || !hasActiveRsp) {
        if (*dirtyBits & HdRenderSettings::DirtyNamespacedSettings) {
            // NamespacedSettings contains all the Prman-specific render
            // settings opinions.
            const VtDictionary& namespacedSettings = GetNamespacedSettings();
        
            // Handle attributes ...
            // Note: We don't get fine-grained invalidation per-setting, so we
            //       recompute all settings.
            _settingsOptions = _GenerateParamList(namespacedSettings);

            TF_DEBUG(HDPRMAN_RENDER_SETTINGS).Msg(
                "Processed dirty namespaced settings for %s and generated the "
                "param list %s", GetId().GetText(),
                HdPrmanDebugUtil::RtParamListToString(_settingsOptions).c_str()
            );

            // ... and connections.
            // Set the integrator connected to this Render Settings prim
            {
                // XXX Should use SdfPath rather than a vector.
                const SdfPathVector paths = VtDictionaryGet<SdfPathVector>(
                    namespacedSettings,
                    _renderTerminalTokens->outputsRiIntegrator.GetString(),
                    VtDefault = SdfPathVector());

                param->SetRenderSettingsIntegratorPath(sceneDelegate,
                    paths.empty()? SdfPath::EmptyPath() : paths.front());
            }

            // Set the SampleFilters connected to this Render Settings prim
            {
                const SdfPathVector paths = VtDictionaryGet<SdfPathVector>(
                    namespacedSettings,
                    _renderTerminalTokens->outputsRiSampleFilters.GetString(),
                    VtDefault = SdfPathVector());

                param->SetConnectedSampleFilterPaths(sceneDelegate, paths);
            }

            // Set the DisplayFilters connected to this Render Settings prim
            {
                const SdfPathVector paths = VtDictionaryGet<SdfPathVector>(
                    namespacedSettings,
                    _renderTerminalTokens->outputsRiDisplayFilters.GetString(),
                    VtDefault = SdfPathVector());

                param->SetConnectedDisplayFilterPaths(sceneDelegate, paths);
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
