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
#include "hdPrman/interactiveRenderParam.h"
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
, _converged(false)
, _lastRenderedVersion(0)
, _lastSettingsVersion(0)
, _integrator(HdPrmanIntegratorTokens->PxrPathTracer)
, _quickIntegrator(HdPrmanIntegratorTokens->PxrDirectLighting)
, _quickIntegrateTime(_enableQuickIntegrate ? 0.2f : 0.0f)
, _quickIntegrate(false)
, _isPrimaryIntegrator(false)
{
    _interactiveRenderParam =
        std::dynamic_pointer_cast<HdPrman_InteractiveRenderParam>(renderParam);

    TF_VERIFY(_interactiveRenderParam);
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

void
HdPrman_InteractiveRenderPass::_Execute(
    HdRenderPassStateSharedPtr const& renderPassState,
    TfTokenVector const & renderTags)
{
    HD_TRACE_FUNCTION();
    
    static const RtUString us_PathTracer("PathTracer");

    if (!_interactiveRenderParam) {
        // If this is not an interactive context, don't use Hydra to drive
        // rendering and presentation of the framebuffer.  Instead, assume
        // we are just using Hydra to sync the scene contents to Riley.
        return;
    }
    if (_interactiveRenderParam->renderThread.IsPauseRequested()) {
        // No more updates if pause is pending
        return;
    }

    // Creates displays if needed
    HdRenderPassAovBindingVector const &aovBindings =
        renderPassState->GetAovBindings();

    _interactiveRenderParam->CreateDisplays(aovBindings);

    // Enable/disable the fallback light when the scene provides no lights.
    _interactiveRenderParam->SetFallbackLightsEnabled(
        _interactiveRenderParam->sceneLightCount == 0);

    // Likewise the render settings.
    HdPrmanRenderDelegate * const renderDelegate =
        static_cast<HdPrmanRenderDelegate*>(
            GetRenderIndex()->GetRenderDelegate());
    const int currentSettingsVersion =
        renderDelegate->GetRenderSettingsVersion();
    
    int32_t renderBufferWidth = 0;
    int32_t renderBufferHeight = 0;

    if (!_GetRenderBufferSize(aovBindings,
                              GetRenderIndex(),
                              &renderBufferWidth, &renderBufferHeight)) {
        // For legacy clients not using AOVs, take size of viewport.
        const GfVec4f vp = renderPassState->GetViewport();
        renderBufferWidth  = vp[2];
        renderBufferHeight = vp[3];
    }

    const HdPrmanCamera * const hdCam =
        dynamic_cast<HdPrmanCamera const *>(renderPassState->GetCamera());

    HdPrmanCameraContext &cameraContext =
        _interactiveRenderParam->GetCameraContext();
    cameraContext.SetCamera(hdCam);
    if (renderPassState->GetFraming().IsValid()) {
        // For new clients setting the camera framing.
        cameraContext.SetFraming(renderPassState->GetFraming());
    } else {
        // For old clients using the viewport.
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

    const bool camChanged = cameraContext.IsInvalid();
    cameraContext.MarkValid();

    if (_lastSettingsVersion != currentSettingsVersion || camChanged) {

        // AcquireRiley will stop rendering and increase sceneVersion
        // so that the render will be re-started below.
        riley::Riley * const riley = _interactiveRenderParam->AcquireRiley();

        _integrator = renderDelegate->GetRenderSetting<std::string>(
            HdPrmanRenderSettingsTokens->integratorName,
            HdPrmanIntegratorTokens->PxrPathTracer.GetString());
        _isPrimaryIntegrator =
            _integrator ==
                HdPrmanIntegratorTokens->PxrPathTracer.GetString() ||
            _integrator ==
                HdPrmanIntegratorTokens->PbsPathTracer.GetString();
        if (_enableQuickIntegrate)
        {
            _quickIntegrator = renderDelegate->GetRenderSetting<std::string>(
                HdPrmanRenderSettingsTokens->interactiveIntegrator,
                HdPrmanIntegratorTokens->PxrDirectLighting.GetString());

            _quickIntegrateTime = renderDelegate->GetRenderSetting<int>(
                HdPrmanRenderSettingsTokens->interactiveIntegratorTimeout,
                200) / 1000.f;
        }
        else
        {
            _quickIntegrateTime = 0.0f;

            riley::ShadingNode &integratorNode =
                _interactiveRenderParam->GetActiveIntegratorShadingNode();

            _interactiveRenderParam->SetIntegratorParamsFromRenderSettings(
                renderDelegate,
                _integrator,
                integratorNode.params);

            // XXX: Need to cast away constness because
            // SetIntegratorParamsFromCamera has wrong signature.

            HdPrmanCamera * const nonConstCam =
                const_cast<HdPrmanCamera *>(hdCam);

            _interactiveRenderParam->SetIntegratorParamsFromCamera(
                renderDelegate,
                nonConstCam,
                _integrator,
                integratorNode.params);

            RtUString integrator(_integrator.c_str());
            integratorNode.handle = integratorNode.name = integrator;

            riley->ModifyIntegrator(
                _interactiveRenderParam->GetActiveIntegratorId(), &integratorNode);
        }

        // Update convergence criteria.
        VtValue vtMaxSamples = renderDelegate->GetRenderSetting(
            HdRenderSettingsTokens->convergedSamplesPerPixel).Cast<int>();
        int maxSamples = TF_VERIFY(!vtMaxSamples.IsEmpty()) ?
            vtMaxSamples.UncheckedGet<int>() : 64; // RenderMan default
        _interactiveRenderParam->GetOptions().SetInteger(RixStr.k_hider_maxsamples,
                                                     maxSamples);

        VtValue vtPixelVariance = renderDelegate->GetRenderSetting(
            HdRenderSettingsTokens->convergedVariance).Cast<float>();
        float pixelVariance = TF_VERIFY(!vtPixelVariance.IsEmpty()) ?
            vtPixelVariance.UncheckedGet<float>() : 0.001f;
        _interactiveRenderParam->GetOptions().SetFloat(RixStr.k_Ri_PixelVariance,
                                                   pixelVariance);

        // Set Options from RenderSettings schema
        _interactiveRenderParam->SetOptionsFromRenderSettings(
            renderDelegate,
            _interactiveRenderParam->GetOptions());
        
        riley->SetOptions(
            _interactiveRenderParam->_GetDeprecatedOptionsPrunedList());

        _lastSettingsVersion = currentSettingsVersion;

        // Setup quick integrator and save ids of it and main
        if (_enableQuickIntegrate)
        {
            riley::ShadingNode integratorNode {
                riley::ShadingNode::Type::k_Integrator,
                RtUString(_quickIntegrator.c_str()),
                us_PathTracer,
                RtParamList()
            };
            integratorNode.params.SetInteger(
                RtUString("numLightSamples"), 1);
            integratorNode.params.SetInteger(
                RtUString("numBxdfSamples"), 1);
            _quickIntegratorId =
                riley->CreateIntegrator(riley::UserId::DefaultId(),
                    integratorNode);
        }

        // This can't be correct: imagine that we call _Execute while the
        // quick integrator is still working. Then _mainIntegratorId will
        // be set to the id of the quick integrator and the below code that
        // tries to switch to the main integrator with
        // SetIntegrator(_mainIntegratorId) will do nothing.
        //
        _mainIntegratorId = _interactiveRenderParam->GetActiveIntegratorId();
    }

    // Check if any camera update needed
    // TODO: This should be part of a Camera sprim; then we wouldn't
    // need to sync anything here.  Note that we'll need to solve
    // thread coordination for sprim sync/finalize first.
    const bool resolutionChanged =
        _interactiveRenderParam->resolution[0] != renderBufferWidth ||
        _interactiveRenderParam->resolution[1] != renderBufferHeight;

    if (camChanged ||
        resolutionChanged) {

        // AcquireRiley will stop rendering and increase sceneVersion
        // so that the render will be re-started below.
        riley::Riley * const riley = _interactiveRenderParam->AcquireRiley();

        if (resolutionChanged) {
            _interactiveRenderParam->resolution[0] = renderBufferWidth;
            _interactiveRenderParam->resolution[1] = renderBufferHeight;
            
            _interactiveRenderParam->GetOptions().SetIntegerArray(
                RixStr.k_Ri_FormatResolution,
                _interactiveRenderParam->resolution, 2);
            
            // There is currently only one render target per context
            if (_interactiveRenderParam->renderViews.size() == 1) {
                riley::RenderViewId const renderViewId =
                    _interactiveRenderParam->renderViews[0];
                
                auto it =
                    _interactiveRenderParam->renderTargets.find(renderViewId);
                
                if (it != _interactiveRenderParam->renderTargets.end()) {
                    riley::RenderTargetId const rtid = it->second;
                    const riley::Extent targetExtent = {
                        static_cast<uint32_t>(
                            _interactiveRenderParam->resolution[0]),
                        static_cast<uint32_t>(
                            _interactiveRenderParam->resolution[1]),
                        0};
                    riley->ModifyRenderTarget(
                        rtid, nullptr,
                        &targetExtent, nullptr, nullptr, nullptr);
                }
            }

            cameraContext.SetRileyOptionsInteractive(
                &(_interactiveRenderParam->GetOptions()),
                GfVec2i(renderBufferWidth, renderBufferHeight));
            
            riley->SetOptions(
                _interactiveRenderParam->_GetDeprecatedOptionsPrunedList());
        }

        cameraContext.UpdateRileyCameraAndClipPlanesInteractive(
            riley, 
            GfVec2i(renderBufferWidth, renderBufferHeight));

        if (hdCam) {
            // Update the framebuffer Z scaling
            _interactiveRenderParam->framebuffer.proj =
#if HD_API_VERSION >= 44
                hdCam->ComputeProjectionMatrix();
#else
                hdCam->GetProjectionMatrix();
#endif
        }
    }    
    
    // We need to capture the value of sceneVersion here after all
    // the above calls to AcquireRiley since AcquireRiley increases
    // the sceneVersion.
    const int currentSceneVersion =
        _interactiveRenderParam->sceneVersion.load();
    const bool needsStartRender = 
        currentSceneVersion != _lastRenderedVersion;

    if (needsStartRender) {
        _lastRenderedVersion = currentSceneVersion;

        // NOTE:
        //
        // _quickIntegrate enables hdPrman to go into a mode
        // where it will switch to PxrDirectLighting
        // integrator for a couple of interations
        // and then switch back to PxrPathTracer/PbsPathTracer
        // The thinking is that we want to use PxrDirectLighting for quick
        // camera tumbles. To enable this mode, the 
        // HD_PRMAN_ENABLE_QUICKINTEGRATE (bool) env var must be set.
        
        // If we're rendering but we're still in the quick integrate window,
        // check and see if we need to switch to the main integrator yet.

        if (_quickIntegrateTime > 0 && _isPrimaryIntegrator) {
            if (!_quickIntegrate) {
                // Start the frame with interactive integrator to give faster
                // time-to-first-buckets.
                _interactiveRenderParam->SetIntegrator(_quickIntegratorId);
                _quickIntegrate = true;
            }
        } else if (_quickIntegrateTime <= 0 || _quickIntegrate) {
            // Disable quick integrate
            _interactiveRenderParam->SetIntegrator(_mainIntegratorId);
            _quickIntegrate = false;
        }
        _interactiveRenderParam->StartRender();
        _frameStart = std::chrono::steady_clock::now();
    } else {
        if (_quickIntegrate &&
            _interactiveRenderParam->renderThread.IsRendering() &&
            _DiffTimeToNow(_frameStart) > _quickIntegrateTime) {

            _interactiveRenderParam->StopRender();
            _interactiveRenderParam->SetIntegrator(_mainIntegratorId);
            _interactiveRenderParam->StartRender();
            
            _quickIntegrate = false;
        }
    }

    _converged = !_interactiveRenderParam->renderThread.IsRendering();

    // Blit from the framebuffer to the currently selected AOVs.
    // Lock the framebuffer when reading so we don't overlap
    // with RenderMan's resize/writing.
    _interactiveRenderParam->framebuffer.mutex.lock();
    for(size_t aov = 0; aov < aovBindings.size(); ++aov) {
        if(!TF_VERIFY(aovBindings[aov].renderBuffer)) {
            continue;
        }
        HdPrmanRenderBuffer *rb = static_cast<HdPrmanRenderBuffer*>(
            aovBindings[aov].renderBuffer);

        if (_interactiveRenderParam->framebuffer.newData) {
            rb->Blit(_interactiveRenderParam->framebuffer.aovs[aov].format,
                     _interactiveRenderParam->framebuffer.w,
                     _interactiveRenderParam->framebuffer.h,
                     reinterpret_cast<uint8_t*>(
                         _interactiveRenderParam->
                         framebuffer.aovs[aov].pixels.data()));
        }
        // Forward convergence state to the render buffers...
        rb->SetConverged(_converged);
    }
    if (_interactiveRenderParam->framebuffer.newData) {
        _interactiveRenderParam->framebuffer.newData = false;
    }
    _interactiveRenderParam->framebuffer.mutex.unlock();
}

PXR_NAMESPACE_CLOSE_SCOPE
