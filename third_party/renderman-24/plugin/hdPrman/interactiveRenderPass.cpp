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
#include "hdPrman/interactiveRenderPass.h"
#include "hdPrman/camera.h"
#include "hdPrman/framebuffer.h"
#include "hdPrman/renderParam.h"
#include "hdPrman/renderBuffer.h"
#include "hdPrman/renderDelegate.h"
#include "hdPrman/rixStrings.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderPassState.h"

#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/envSetting.h"

#include "Riley.h"
#include "RixShadingUtils.h"
#include "RixPredefinedStrings.hpp"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HD_PRMAN_ENABLE_QUICKINTEGRATE, false,
                      "Enable interactive integrator");

static bool _enableQuickIntegrate =
    TfGetEnvSetting(HD_PRMAN_ENABLE_QUICKINTEGRATE);

HdPrman_InteractiveRenderPass::HdPrman_InteractiveRenderPass(
    HdRenderIndex *index,
    HdRprimCollection const &collection,
    std::shared_ptr<HdPrman_RenderParam> renderParam)
: HdRenderPass(index, collection)
, _renderParam(renderParam)
, _converged(false)
, _lastRenderedVersion(0)
, _quickIntegrateTime(0.2f)
{
    TF_VERIFY(_renderParam);
}

HdPrman_InteractiveRenderPass::~HdPrman_InteractiveRenderPass() = default;

bool
HdPrman_InteractiveRenderPass::IsConverged() const
{
    return _converged;
}

// Return the seconds between now and then.
static double
_DiffTimeToNow(std::chrono::steady_clock::time_point const& then)
{
    std::chrono::duration<double> diff;
    diff = std::chrono::duration_cast<std::chrono::duration<double>>
                                (std::chrono::steady_clock::now()-then);
    return(diff.count());
}

static
const HdRenderBuffer *
_GetRenderBuffer(const HdRenderPassAovBinding& aov,
                 const HdRenderIndex * const renderIndex)
{
    if (aov.renderBuffer) {
        return aov.renderBuffer;
    }
    
    return
        dynamic_cast<HdRenderBuffer*>(
            renderIndex->GetBprim(
                HdPrimTypeTokens->renderBuffer,
                aov.renderBufferId));
}

static
bool
_GetRenderBufferSize(const HdRenderPassAovBindingVector &aovBindings,
                     const HdRenderIndex * const renderIndex,
                     int32_t * const width,
                     int32_t * const height)
{
    for (const HdRenderPassAovBinding &aovBinding : aovBindings) {
        if (const HdRenderBuffer * const renderBuffer =
                        _GetRenderBuffer(aovBinding, renderIndex)) {
            *width  = renderBuffer->GetWidth();
            *height = renderBuffer->GetHeight();
            return true;
        } else {
            TF_CODING_ERROR("No render buffer available for AOV "
                            "%s", aovBinding.aovName.GetText());
        }
    }

    return false;
}

static
bool
_UsesPrimaryIntegrator(const HdRenderDelegate * const renderDelegate)
{
    const std::string &integrator =
        renderDelegate->GetRenderSetting<std::string>(
            HdPrmanRenderSettingsTokens->integratorName,
            HdPrmanIntegratorTokens->PxrPathTracer.GetString());
    return
        integrator == HdPrmanIntegratorTokens->PxrPathTracer.GetString() ||
        integrator == HdPrmanIntegratorTokens->PbsPathTracer.GetString();
}

void
HdPrman_InteractiveRenderPass::_Execute(
    HdRenderPassStateSharedPtr const& renderPassState,
    TfTokenVector const & renderTags)
{
    HD_TRACE_FUNCTION();
    
    static const RtUString us_PathTracer("PathTracer");

    if (!_renderParam) {
        // If this is not an interactive context, don't use Hydra to drive
        // rendering and presentation of the framebuffer.  Instead, assume
        // we are just using Hydra to sync the scene contents to Riley.
        return;
    }
    if (_renderParam->renderThread.IsPauseRequested()) {
        // No more updates if pause is pending
        return;
    }

    HdRenderPassAovBindingVector const &aovBindings =
        renderPassState->GetAovBindings();

    // Likewise the render settings.
    HdPrmanRenderDelegate * const renderDelegate =
        static_cast<HdPrmanRenderDelegate*>(
            GetRenderIndex()->GetRenderDelegate());
    const int currentSettingsVersion =
        renderDelegate->GetRenderSettingsVersion();
    
    int32_t renderBufferWidth = 0;
    int32_t renderBufferHeight = 0;

    const HdPrmanCamera * const hdCam =
        dynamic_cast<HdPrmanCamera const *>(renderPassState->GetCamera());

    HdPrmanCameraContext &cameraContext =
        _renderParam->GetCameraContext();
    cameraContext.SetCamera(hdCam);
    if (renderPassState->GetFraming().IsValid()) {
        // For new clients setting the camera framing.
        cameraContext.SetFraming(renderPassState->GetFraming());
    } else {
        // For old clients using the viewport.
        _GetRenderBufferSize(aovBindings,
                             GetRenderIndex(),
                             &renderBufferWidth, &renderBufferHeight);

        const GfVec4f vp = renderPassState->GetViewport();
        cameraContext.SetFraming(
            CameraUtilFraming(
                GfRect2i(
                    // Note that the OpenGL-style viewport is y-Up
                    // but the camera framing is y-Down, so converting here.
                    GfVec2i(vp[0], renderBufferHeight - (vp[1] + vp[3])),
                    vp[2], vp[3])));
    }

    cameraContext.SetWindowPolicy(renderPassState->GetWindowPolicy());

    // A hack to make tests pass.
    // testHdPrman was hard-coding a particular shutter curve for offline
    // renders. Ideally, we would have a render setting or camera attribute
    // to control the curve instead.
    if (renderDelegate->IsInteractive()) {
        static const float pts[8] = {
            0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f
        };
        cameraContext.SetShutterCurve(0.0f, 1.0f, pts);
    } else {
        static const float pts[8] = {
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.3f, 0.0f
        };
        cameraContext.SetShutterCurve(0.0f, 0.0f, pts);
    }

    const bool camChanged = cameraContext.IsInvalid();
    cameraContext.MarkValid();

    const int lastVersion = _renderParam->GetLastSettingsVersion();

    if (aovBindings.empty()) {
        // When there are no AOV-bindings, use render spec from
        // render settings to create render view.
        bool createRenderView = false;
        if (!_renderParam->framebuffer->aovs.empty()) {
            // If we just switched from render pass state with AOV bindings
            // to one without, we need to create a new render view from
            // the render spec - and can free the intermediate framebuffer
            // the AOV display driver writes into.
            _renderParam->framebuffer->aovs.clear();
            createRenderView = true;
        }
        if (lastVersion != currentSettingsVersion) {
            // Re-create new render view since render spec might have
            // changed.
            createRenderView = true;
        }

        if (createRenderView) {
            const VtDictionary &renderSpec =
                renderDelegate->GetRenderSetting<VtDictionary>(
                    HdPrmanRenderSettingsTokens->experimentalRenderSpec,
                    VtDictionary());
            _renderParam->CreateRenderViewFromSpec(renderSpec);
        }

        const GfVec2i resolution =
            cameraContext.GetResolutionFromDisplayWindow();
        renderBufferWidth = resolution[0];
        renderBufferHeight = resolution[1];
    } else {
        // Use AOV-bindings to create render view with displays that
        // have drivers writing into the intermediate framebuffer blitted
        // to the AOVs.
        _renderParam->CreateRenderViewFromAovs(aovBindings);
        
        _GetRenderBufferSize(aovBindings,
                             GetRenderIndex(),
                             &renderBufferWidth, &renderBufferHeight);
    }

    if (lastVersion != currentSettingsVersion || camChanged) {

        // AcquireRiley will stop rendering and increase sceneVersion
        // so that the render will be re-started below.
        riley::Riley * const riley = _renderParam->AcquireRiley();

        _renderParam->UpdateIntegrator(renderDelegate);
        _renderParam->UpdateQuickIntegrator(renderDelegate);

        if (_enableQuickIntegrate)
        {
            _quickIntegrateTime = renderDelegate->GetRenderSetting<int>(
                HdPrmanRenderSettingsTokens->interactiveIntegratorTimeout,
                200) / 1000.f;
        }

        // Update convergence criteria.
        VtValue vtMaxSamples = renderDelegate->GetRenderSetting(
            HdRenderSettingsTokens->convergedSamplesPerPixel).Cast<int>();
        int maxSamples = TF_VERIFY(!vtMaxSamples.IsEmpty()) ?
            vtMaxSamples.UncheckedGet<int>() : 64; // RenderMan default
        _renderParam->GetOptions().SetInteger(RixStr.k_hider_maxsamples,
                                                     maxSamples);

        VtValue vtPixelVariance = renderDelegate->GetRenderSetting(
            HdRenderSettingsTokens->convergedVariance).Cast<float>();
        float pixelVariance = TF_VERIFY(!vtPixelVariance.IsEmpty()) ?
            vtPixelVariance.UncheckedGet<float>() : 0.001f;
        _renderParam->GetOptions().SetFloat(RixStr.k_Ri_PixelVariance,
                                                   pixelVariance);

        // Set Options from RenderSettings schema
        _renderParam->SetOptionsFromRenderSettings(
            renderDelegate,
            _renderParam->GetOptions());
        
        riley->SetOptions(
            _renderParam->_GetDeprecatedOptionsPrunedList());

        _renderParam->SetLastSettingsVersion(currentSettingsVersion);
    }

    // Check if any camera update needed
    // TODO: This should be part of a Camera sprim; then we wouldn't
    // need to sync anything here.  Note that we'll need to solve
    // thread coordination for sprim sync/finalize first.
    const bool resolutionChanged =
        _renderParam->resolution[0] != renderBufferWidth ||
        _renderParam->resolution[1] != renderBufferHeight;

    if (camChanged ||
        resolutionChanged) {

        // AcquireRiley will stop rendering and increase sceneVersion
        // so that the render will be re-started below.
        riley::Riley * const riley = _renderParam->AcquireRiley();

        if (resolutionChanged) {
            _renderParam->resolution[0] = renderBufferWidth;
            _renderParam->resolution[1] = renderBufferHeight;
            
            _renderParam->GetOptions().SetIntegerArray(
                RixStr.k_Ri_FormatResolution,
                _renderParam->resolution, 2);
            
            _renderParam->GetRenderViewContext().SetResolution(
                GfVec2i(renderBufferWidth, renderBufferHeight),
                riley);

            cameraContext.SetRileyOptionsInteractive(
                &(_renderParam->GetOptions()),
                GfVec2i(renderBufferWidth, renderBufferHeight));
            
            riley->SetOptions(
                _renderParam->_GetDeprecatedOptionsPrunedList());
        }

        if (aovBindings.empty()) {
            cameraContext.UpdateRileyCameraAndClipPlanes(riley);
        } else {
            // When using AOV-bindings, we setup the camera slightly
            // differently.
            cameraContext.UpdateRileyCameraAndClipPlanesInteractive(
                riley, 
                GfVec2i(renderBufferWidth, renderBufferHeight));
        }

        if (hdCam) {
            // Update the framebuffer Z scaling
            _renderParam->framebuffer->proj =
#if HD_API_VERSION >= 44
                hdCam->ComputeProjectionMatrix();
#else
                hdCam->GetProjectionMatrix();
#endif
        }
    }    
    
    if (renderDelegate->IsInteractive()) {
        _RestartRenderIfNecessary(renderDelegate);
    } else {
        _RenderInMainThread();
    }
    
    if (!aovBindings.empty()) {
        _Blit(aovBindings);
    }
}
   
void
HdPrman_InteractiveRenderPass::_RestartRenderIfNecessary(
    HdRenderDelegate * const renderDelegate)
{
    const bool needsRestart =
        _renderParam->sceneVersion.load() != _lastRenderedVersion;
    
    if (needsRestart) {
        // NOTE:
        //
        // _quickIntegrate enables hdPrman to go into a mode
        // where it will switch to PxrDirectLighting
        // integrator for a couple of interations
        // and then switch back to PxrPathTracer/PbsPathTracer
        // The thinking is that we want to use PxrDirectLighting for quick
        // camera tumbles. To enable this mode, the 
        // HD_PRMAN_ENABLE_QUICKINTEGRATE (bool) env var must be set.
        
        // Start renders using the quick integrator if:
        // - the corresponding env var is enabled
        // - the time out is positive
        // - the main integrator is an (expensive) primary integrator.
        const bool useQuickIntegrator =
            _enableQuickIntegrate && 
            _quickIntegrateTime > 0 &&
            _UsesPrimaryIntegrator(renderDelegate);
        const riley::IntegratorId integratorId =
            useQuickIntegrator
                ? _renderParam->GetQuickIntegratorId()
                : _renderParam->GetIntegratorId();
        if (integratorId != _renderParam->GetActiveIntegratorId()) {
            _renderParam->SetActiveIntegratorId(integratorId);
        }

        _renderParam->StartRender();
        _frameStart = std::chrono::steady_clock::now();
    } else {
        // If we are using the quick integrator...
        if (_renderParam->GetActiveIntegratorId() !=
                _renderParam->GetIntegratorId()) {
            // ... and the quick integrate time has passed, ...
            if (_DiffTimeToNow(_frameStart) > _quickIntegrateTime) {
                // Set the active integrator.
                // Note that SetActiveIntegrator is stopping the renderer
                // (implicitly through AcquireRiley).
                _renderParam->SetActiveIntegratorId(
                    _renderParam->GetIntegratorId());
                _renderParam->StartRender();
            }
        }
    }

    // We need to capture the value of sceneVersion here after all
    // the above calls to AcquireRiley since AcquireRiley increases
    // the sceneVersion. Note that setting the call to SetActiveIntegratorId
    // is also implicitly calling AcquireRiley.
    _lastRenderedVersion = _renderParam->sceneVersion.load();

    _converged =
        (_renderParam->GetActiveIntegratorId() ==
         _renderParam->GetIntegratorId())
        && !_renderParam->renderThread.IsRendering();
}

void
HdPrman_InteractiveRenderPass::_Blit(
    HdRenderPassAovBindingVector const &aovBindings)
{
    // Blit from the framebuffer to the currently selected AOVs.
    // Lock the framebuffer when reading so we don't overlap
    // with RenderMan's resize/writing.
    _renderParam->framebuffer->mutex.lock();
    for(size_t aov = 0; aov < aovBindings.size(); ++aov) {
        if(!TF_VERIFY(aovBindings[aov].renderBuffer)) {
            continue;
        }
        HdPrmanRenderBuffer *rb = static_cast<HdPrmanRenderBuffer*>(
            aovBindings[aov].renderBuffer);

        if (_renderParam->framebuffer->newData) {
            rb->Blit(_renderParam->framebuffer->aovs[aov].format,
                     _renderParam->framebuffer->w,
                     _renderParam->framebuffer->h,
                     reinterpret_cast<uint8_t*>(
                         _renderParam->
                         framebuffer->aovs[aov].pixels.data()));
        }
        // Forward convergence state to the render buffers...
        rb->SetConverged(_converged);
    }
    if (_renderParam->framebuffer->newData) {
        _renderParam->framebuffer->newData = false;
    }
    _renderParam->framebuffer->mutex.unlock();
}

void
HdPrman_InteractiveRenderPass::_RenderInMainThread()
{
    riley::Riley * const riley = _renderParam->AcquireRiley();

    _renderParam->SetActiveIntegratorId(
        _renderParam->GetIntegratorId());
    
    HdPrmanRenderViewContext &ctx =
        _renderParam->GetRenderViewContext();
    
    const riley::RenderViewId renderViews[] = { ctx.GetRenderViewId() };
    
    RtParamList renderOptions;
    static RtUString const US_RENDERMODE("renderMode");
    static RtUString const US_BATCH("batch");   
    renderOptions.SetString(US_RENDERMODE, US_BATCH);   
    
    riley->Render(
        {static_cast<uint32_t>(TfArraySize(renderViews)), renderViews},
        renderOptions);
    
    _converged = true;
}



PXR_NAMESPACE_CLOSE_SCOPE
