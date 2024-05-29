//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_PASS_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_PASS_H

#include "pxr/imaging/hd/renderPass.h"

#include "pxr/base/tf/token.h"

#include "pxr/pxr.h"

#include <chrono>

PXR_NAMESPACE_OPEN_SCOPE

class HdPrman_RenderParam;
class HdPrman_CameraContext;
class HdPrman_RenderSettings;

class HdPrman_RenderPass final : public HdRenderPass
{
public:
    HdPrman_RenderPass(
        HdRenderIndex *index,
        HdRprimCollection const &collection,
        std::shared_ptr<HdPrman_RenderParam> renderParam);
    ~HdPrman_RenderPass() override;

    bool IsConverged() const override;

protected:
    void _Execute(HdRenderPassStateSharedPtr const &renderPassState,
                  TfTokenVector const &renderTags) override;

private:
    void _RenderInMainThread();
    void _RestartRenderIfNecessary(HdRenderDelegate *renderDelegate);

    // Helpers to update the Camera Context inside _Execute()
    void _UpdateCameraPath(
        const HdRenderPassStateSharedPtr &renderPassState,
        HdPrman_CameraContext *cameraContext);
    
    bool _UpdateCameraFramingAndWindowPolicy(
        const HdRenderPassStateSharedPtr &renderPassState,
        HdPrman_CameraContext *cameraContext);

    void _UpdateActiveRenderTagsIfChanged(
        const TfTokenVector& taskRenderTags);

    HdPrman_RenderSettings* _GetDrivingRenderSettingsPrim() const;

    std::shared_ptr<HdPrman_RenderParam> _renderParam;
    bool _converged;
    int _lastRenderedVersion;
    int _lastTaskRenderTagsVersion;
    int _lastRprimRenderTagVersion;

    std::chrono::steady_clock::time_point _frameStart;
    float _quickIntegrateTime;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_PASS_H
