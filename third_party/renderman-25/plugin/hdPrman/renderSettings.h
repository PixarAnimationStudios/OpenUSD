//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_SETTINGS_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_SETTINGS_H

#include "pxr/pxr.h"
#if PXR_VERSION >= 2308

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

#if PXR_VERSION <= 2308
    bool IsValid() const;
#endif
private:
    void _ProcessRenderTerminals(
        HdSceneDelegate *sceneDelegate,
        HdPrman_RenderParam *param);

    void _ProcessRenderProducts(HdPrman_RenderParam *param);

private:
    RtParamList _settingsOptions;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_VERSION >= 2308

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_VOLUME_H
