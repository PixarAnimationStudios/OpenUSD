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
#ifndef EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_INT_CONTEXT_H
#define EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_INT_CONTEXT_H

#include "pxr/pxr.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/imaging/hd/renderThread.h"
#include "hdPrman/context.h"
#include "hdPrman/rixStrings.h"
#include "hdPrman/framebuffer.h"
#include "hdPrman/renderBuffer.h"

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

// HdPrman_InteractiveContext supports interactive rendering workflows.
// Specifically, this means it provides:
//
// - a built-in Riley camera used for the RenderPass
// - a framebuffer for returning image results
// - concurrent, background rendering support.
//
struct HdPrman_InteractiveContext : public HdPrman_Context
{
    // A framebuffer to hold PRMan results.
    // The d_hydra.so renderman display driver handles updates via IPC.
    HdPrmanFramebuffer framebuffer;

    // The integrator to use.
    // Updated from render pass state.
    riley::IntegratorId integratorId;

    // The viewport camera to use.
    // Updated from render pass state.
    riley::CameraId cameraId;
        
    // Count of scene lights.  Maintained by the delegate.
    int sceneLightCount;

    HdPrman_InteractiveContext();
    virtual ~HdPrman_InteractiveContext();

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

    // Checks whether context was successfully initialized.
    // ie. riley was created
    bool IsValid() const;

    // Creates displays in riley based on aovBindings vector
    bool CreateDisplays(const HdRenderPassAovBindingVector& aovBindings);

    // Render thread for background rendering.
    HdRenderThread renderThread;

    // Scene version counter.
    std::atomic<int> sceneVersion;

    // Active render viewports
    std::vector<riley::RenderViewId> renderViews;

    // For now, the renderPass needs the render target for each view, for
    // resolution edits, so we need to keep track of these too.
    std::map<riley::RenderViewId, riley::RenderTargetId> renderTargets;

    riley::IntegratorId GetIntegrator();
    void SetIntegrator(riley::IntegratorId integratorId);

    // Full option description
    RtParamList _options;

    // The following should not be given to Riley::SetOptions() anymore.
    static const std::vector<RtUString> _deprecatedRileyOptions;

    // The following were previously options,
    // but now need to be provided as camera properties.
    static const std::vector<RtUString> _newRileyCameraProperties;

    int32_t resolution[2];

    // Some quantites previously given as options now need to be provided
    // through different Riley APIs. However, it is still convenient for these
    // values to be stored in _options (for now). This method returns a pruned
    // copy of the options, to be provided to SetOptions().
    RtParamList _GetDeprecatedOptionsPrunedList();

    // Some quantities previously given as options now need to be provided
    // through CreateCamera() or ModifyCamera(). This method retrieve these
    // values from _options and add them to the given paramlist.
    RtParamList _GetCameraPropertiesFromDeprecatedOptions();


private:
    // Initialize things, like riley, that need to succeed
    // in order for Begin to be called.
    void _Initialize();

    // The fallback light.  HdPrman_RenderPass calls
    // SetFallbackLightsEnabled() to maintain visibility
    // of the fallback light XOR other lights in the scene.
    riley::LightInstanceId _fallbackLight;
    riley::LightShaderId _fallbackLightShader;
    RtParamList _fallbackLightAttrs;
    bool _fallbackLightEnabled;
    bool _didBeginRiley;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_INT_CONTEXT_H
