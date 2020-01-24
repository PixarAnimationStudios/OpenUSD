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
#ifndef EXT_RMANPKG_22_0_PLUGIN_RENDERMAN_PLUGIN_HDX_PRMAN_CONTEXT_H
#define EXT_RMANPKG_22_0_PLUGIN_RENDERMAN_PLUGIN_HDX_PRMAN_CONTEXT_H

#include "pxr/pxr.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/imaging/hd/renderThread.h"
#include "hdPrman/context.h"
#include "framebuffer.h"

#include "Riley.h"

#include <condition_variable>
#include <mutex>
#include <thread>
#include <atomic>

class RixRiCtl;
class RixParamList;

PXR_NAMESPACE_OPEN_SCOPE

class SdfPath;
class HdSceneDelegate;
class HdRenderDelegate;

// HdxPrman_InteractiveContext supports interactive rendering workflows.
// Specifically, this means it provides:
//
// - a built-in Riley camera used for the RenderPass
// - a framebuffer for returning image results
// - concurrent, background rendering support.
//
struct HdxPrman_InteractiveContext : public HdPrman_Context
{
    // A framebuffer to hold PRMan results.
    // The d_hydra.so renderman display driver handles updates via IPC.
    HdxPrmanFramebuffer framebuffer;

    // The viewport camera to use.
    // Updated from render pass state.
    riley::CameraId cameraId;
        
    // Count of scene lights.  Maintained by the delegate.
    int sceneLightCount;

    HdxPrman_InteractiveContext();
    virtual ~HdxPrman_InteractiveContext();
    // Start connection to Renderman.
    void Begin(HdRenderDelegate *renderDelegate);
    // End connection to Renderman, cancelling any ongoing render.
    void End();
    // Indicate whether fallback lights should be enabled.
    void SetFallbackLightsEnabled(bool);

    // Request Riley (and the HdRenderThread) to stop.
    void StopRender();

    // Render thread for background rendering.
    HdRenderThread renderThread;

    // Scene version counter.
    std::atomic<int> sceneVersion;

private:
    // The fallback light.  HdxPrman_RenderPass calls
    // SetFallbackLightsEnabled() to maintain visibility
    // of the fallback light XOR other lights in the scene.
    riley::LightInstanceId _fallbackLight;
    riley::LightShaderId _fallbackLightShader;
    RixParamList *_fallbackLightAttrs;
    bool _fallbackLightEnabled;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_22_0_PLUGIN_RENDERMAN_PLUGIN_HDX_PRMAN_CONTEXT_H
