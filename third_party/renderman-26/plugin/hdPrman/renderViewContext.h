//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_VIEW_CONTEXT_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_VIEW_CONTEXT_H

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

    void DeleteRenderView(riley::Riley * riley);

    void SetIntegratorId(
        riley::IntegratorId id,
        riley::Riley * riley);

    void SetResolution(
        const GfVec2i &resolution,
        riley::Riley * riley);

    riley::RenderViewId GetRenderViewId() const { return _renderViewId; }

private:
    HdPrman_RenderViewContext(const HdPrman_RenderViewContext &) = delete;

    std::vector<riley::RenderOutputId> _renderOutputIds;
    std::vector<riley::DisplayId> _displayIds;
    riley::RenderTargetId _renderTargetId;
    riley::RenderViewId _renderViewId;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  //EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_VIEW_CONTEXT_H

