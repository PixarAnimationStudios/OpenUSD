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
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_SETTINGS_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_SETTINGS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderSettings.h"

#include "RiTypesHelper.h" // for RtParamList

PXR_NAMESPACE_OPEN_SCOPE

class HdRenderIndex;
class HdPrman_RenderParam;

class HdPrman_RenderSettings final : public HdRenderSettings
{
public:
    HdPrman_RenderSettings(SdfPath const& id);

    ~HdPrman_RenderSettings() override;

    /// Public API.
    ///
    /// Returns whether the prim can be used to drive render pass execution.
    /// If false is returned, the render pass uses a combination of the
    /// legacy render settings map and render pass state to drive execution.
    //
    bool DriveRenderPass(
        bool interactive,
        bool renderPassHasAovBindings) const;

    // Called during render pass execution.
    // Updates necessary riley state (camera, render view, scene options) and
    // invokes riley->Render(..).
    //
    // NOTE: Current support is limited to "batch" (i.e., non-interactive)
    //       rendering.
    //
    bool UpdateAndRender(
        const HdRenderIndex *renderIndex,
        bool interactive,
        HdPrman_RenderParam *param);

    /// Virtual API.
    void Finalize(HdRenderParam *renderParam) override;

    void _Sync(HdSceneDelegate *sceneDelegate,
               HdRenderParam *renderParam,
               const HdDirtyBits *dirtyBits) override;

private:
    void _ProcessRenderTerminals(
        HdSceneDelegate *sceneDelegate,
        HdPrman_RenderParam *param);

    void _ProcessRenderProducts(HdPrman_RenderParam *param);

private:
    RtParamList _settingsOptions;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_VOLUME_H
