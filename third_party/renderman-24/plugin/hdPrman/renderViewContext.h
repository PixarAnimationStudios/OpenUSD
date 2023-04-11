//
// Copyright 2021 Pixar
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
#ifndef EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_VIEW_CONTEXT_H
#define EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_VIEW_CONTEXT_H

#include "pxr/pxr.h"
#include "hdPrman/api.h"

#include "Riley.h"

#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec2f.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// Descriptor to create a render man render view together
/// with associated render outputs and displays.
///
struct HdPrman_RenderViewDesc
{
    riley::CameraId cameraId;
    riley::IntegratorId integratorId;
    riley::SampleFilterList sampleFilterList;
    riley::DisplayFilterList displayFilterList;
    GfVec2i resolution;
    
    struct RenderOutputDesc
    {
        RenderOutputDesc();

        RtUString name;
        riley::RenderOutputType type;
        RtUString sourceName;
        RtUString rule;
        RtUString filter;
        GfVec2f filterWidth;
        float relativePixelVariance;
        RtParamList params;
    };

    std::vector<RenderOutputDesc> renderOutputDescs;

    struct DisplayDesc
    {
        RtUString name;
        RtUString driver;
        RtParamList params;

        std::vector<size_t> renderOutputIndices;
    };
    
    std::vector<DisplayDesc> displayDescs;
};

/// Manages a render man render view together with associated
/// render target, render ouputs and displays.
///
class HdPrman_RenderViewContext final
{
public:
    HdPrman_RenderViewContext();

    void CreateRenderView(
        const HdPrman_RenderViewDesc &desc,
        riley::Riley * riley);
    void SetIntegratorId(
        riley::IntegratorId id,
        riley::Riley * riley);
    void SetResolution(
        const GfVec2i &resolution,
        riley::Riley * riley);

    riley::RenderViewId GetRenderViewId() const { return _renderViewId; }

private:
    HdPrman_RenderViewContext(const HdPrman_RenderViewContext &) = delete;

    void _DestroyRenderView(riley::Riley * riley);

    std::vector<riley::RenderOutputId> _renderOutputIds;
    std::vector<riley::DisplayId> _displayIds;
    riley::RenderTargetId _renderTargetId;
    riley::RenderViewId _renderViewId;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  //EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_VIEW_CONTEXT_H

