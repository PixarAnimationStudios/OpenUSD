//
// Copyright 2019 Pixar
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
#ifndef EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_INT_RENDER_PARAM_H
#define EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_INT_RENDER_PARAM_H

#include "pxr/pxr.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/imaging/hd/renderThread.h"
#include "hdPrman/renderParam.h"
#include "hdPrman/rixStrings.h"
#include "hdPrman/framebuffer.h"
#include "hdPrman/renderBuffer.h"
#include "hdPrman/cameraContext.h"

#include "Riley.h"

#include <condition_variable>
#include <mutex>
#include <thread>
#include <atomic>

class RixRiCtl;

PXR_NAMESPACE_OPEN_SCOPE

class SdfPath;
class HdSceneDelegate;
class HdRenderDelegate;

// HdPrman_InteractiveRenderParam supports interactive rendering workflows.
// Specifically, this means it provides:
//
// - a built-in Riley camera used for the RenderPass
// - a framebuffer for returning image results
// - concurrent, background rendering support.
//
class HdPrman_InteractiveRenderParam : public HdPrman_RenderParam
{
public:

    // A framebuffer to hold PRMan results.
    // The d_hydra.so renderman display driver handles updates via IPC.
    HdPrmanFramebuffer framebuffer;

    // The viewport camera to use.
    // Updated from render pass state.
    riley::CameraId cameraId;
        
    // Count of scene lights.  Maintained by the delegate.
    int sceneLightCount;

    HdPrman_InteractiveRenderParam();
    ~HdPrman_InteractiveRenderParam() override;

    // Start connection to Renderman.
    void Begin(HdRenderDelegate *renderDelegate);
    // Starts riley and the thread if needed, and tells the thread render
    void StartRender();
    // End connection to Renderman, cancelling any ongoing render.
    void End();
    // Indicate whether fallback lights should be enabled.
    void SetFallbackLightsEnabled(bool);

    // Request Riley (and the HdRenderThread) to stop.
    void StopRender();

    // Query whether or not the HdRenderThread is running.
    bool IsRenderStopped();

    // Checks whether render param was successfully initialized.
    // ie. riley was created
    bool IsValid() const;

    // Creates displays in riley based on aovBindings vector
    void CreateDisplays(const HdRenderPassAovBindingVector& aovBindings);

    // Invalidate texture at path.
    void InvalidateTexture(const std::string &path);

    // Request edit access (stopping the renderer and marking the contex to restart
    // the renderer when executing the render pass) to the Riley scene and return it.
    riley::Riley * AcquireRiley() override;

    // Render thread for background rendering.
    HdRenderThread renderThread;

    // Scene version counter.
    std::atomic<int> sceneVersion;

    // Active render viewports
    std::vector<riley::RenderViewId> renderViews;

    // For now, the renderPass needs the render target for each view, for
    // resolution edits, so we need to keep track of these too.
    std::map<riley::RenderViewId, riley::RenderTargetId> renderTargets;

    void SetIntegrator(riley::IntegratorId integratorId);

    int32_t resolution[2];

    // Some quantites previously given as options now need to be provided
    // through different Riley APIs. However, it is still convenient for these
    // values to be stored in _options (for now). This method returns a pruned
    // copy of the options, to be provided to SetOptions().
    RtParamList _GetDeprecatedOptionsPrunedList();

    // Provides external access to resources used to set parameters for
    // options and the active integrator.
    RtParamList &GetOptions() override;
    riley::IntegratorId GetActiveIntegratorId() override;
    riley::ShadingNode &GetActiveIntegratorShadingNode() override;

    HdPrmanCameraContext &GetCameraContext() override;

private:
    // The integrator to use.
    // Updated from render pass state.
    riley::IntegratorId _integratorId;

    // Initialize things, like riley, that need to succeed
    // in order for Begin to be called.
    void _Initialize();

    void _RenderThreadCallback();

    // Full option description
    RtParamList _options;

    // The fallback light.  HdPrman_RenderPass calls
    // SetFallbackLightsEnabled() to maintain visibility
    // of the fallback light XOR other lights in the scene.
    riley::LightInstanceId _fallbackLight;
    riley::LightShaderId _fallbackLightShader;
    RtParamList _fallbackLightAttrs;
    bool _fallbackLightEnabled;
    bool _didBeginRiley;

    riley::ShadingNode _activeIntegratorShadingNode;

    HdPrmanCameraContext _cameraContext;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_INT_RENDER_PARAM_H
