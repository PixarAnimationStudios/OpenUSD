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
#include "hdPrman/renderPass.h"
#include "hdPrman/camera.h"
#include "hdPrman/debugCodes.h"
#include "hdPrman/debugUtil.h"
#include "hdPrman/gprimbase.h"
#include "hdPrman/framebuffer.h"
#include "hdPrman/renderParam.h"
#include "hdPrman/renderBuffer.h"
#include "hdPrman/renderDelegate.h"
#include "hdPrman/renderSettings.h"
#include "hdPrman/rixStrings.h"
#include "hdPrman/utils.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/utils.h"

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

HdPrman_RenderPass::HdPrman_RenderPass(
    HdRenderIndex *index,
    HdRprimCollection const &collection,
    std::shared_ptr<HdPrman_RenderParam> renderParam)
: HdRenderPass(index, collection)
, _renderParam(renderParam)
, _converged(false)
, _lastRenderedVersion(0)
, _lastTaskRenderTagsVersion(0)
, _lastRprimRenderTagVersion(0)
, _lastRenderSettingsPrimVersion(0)
, _quickIntegrateTime(0.2f)
{
    TF_VERIFY(_renderParam);
}

HdPrman_RenderPass::~HdPrman_RenderPass() = default;

bool
HdPrman_RenderPass::IsConverged() const
{
    return _converged;
}

namespace {

using RenderProducts = VtArray<HdRenderSettingsMap>;

// Return the seconds between now and then.
double
_DiffTimeToNow(std::chrono::steady_clock::time_point const& then)
{
    std::chrono::duration<double> diff;
    diff = std::chrono::duration_cast<std::chrono::duration<double>>
                                (std::chrono::steady_clock::now()-then);
    return(diff.count());
}

void
_Blit(HdPrmanFramebuffer * const framebuffer,
      HdRenderPassAovBindingVector const &aovBindings,
      const bool converged)
{
    // Blit from the framebuffer to the currently selected AOVs.
    // Lock the framebuffer when reading so we don't overlap
    // with RenderMan's resize/writing.
    std::lock_guard<std::mutex> lock(framebuffer->mutex);

    const bool newData = framebuffer->newData.exchange(false);

    for(size_t aov = 0; aov < aovBindings.size(); ++aov) {
        HdPrmanRenderBuffer *const rb =
            static_cast<HdPrmanRenderBuffer*>(
                aovBindings[aov].renderBuffer);

        if(!TF_VERIFY(rb)) {
            continue;
        }

        if (newData) {
            rb->Blit(framebuffer->aovBuffers[aov].desc.format,
                     framebuffer->w,
                     framebuffer->h,
                     reinterpret_cast<uint8_t*>(
                         framebuffer->aovBuffers[aov].pixels.data()));
        }
        // Forward convergence state to the render buffers...
        rb->SetConverged(converged);
    }
}

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

bool
_GetRenderBufferSize(const HdRenderPassAovBindingVector &aovBindings,
                     const HdRenderIndex * const renderIndex,
                     GfVec2i * const resolution)
{
    for (const HdRenderPassAovBinding &aovBinding : aovBindings) {
        if (const HdRenderBuffer * const renderBuffer =
                        _GetRenderBuffer(aovBinding, renderIndex)) {
            (*resolution)[0] = renderBuffer->GetWidth();
            (*resolution)[1] = renderBuffer->GetHeight();
            return true;
        } else {
            TF_CODING_ERROR("No render buffer available for AOV "
                            "%s", aovBinding.aovName.GetText());
        }
    }

    return false;
}

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

bool
_HasRenderProducts( const RenderProducts& renderProducts)
{
    for (const auto &renderProduct : renderProducts) {
        if (!renderProduct.empty()) {
            return true;
        }
    }
    return false;
}

// Update visibility settings of riley instances for the active render tags.
//
// The render pass's _Execute method takes a list of renderTags,
// and only rprims with those tags should be visible,
// so we need to figure out the corresponding riley instance ids
// and update the visibility settings in riley.
// It might seem like the rprims would receive a Sync call
// to deal with this, but they only do when they first become visible.
// After that tag based visibility is a per-pass problem.

void
_UpdateRprimVisibilityForPass(TfTokenVector const & renderTags,
                             HdRenderIndex *index,
                             riley::Riley *riley )
{
    for(auto id : index->GetRprimIds()) {
        HdRprim const *rprim = index->GetRprim(id);
        const TfToken tag = rprim->GetRenderTag();

        // If the rprim's render tag is not in the pass's list of tags it's
        // definitely not visible, but if it is, look at the rprim's visibility.
        const bool vis =
            std::count(renderTags.begin(), renderTags.end(), tag) &&
            rprim->IsVisible();

        HdPrman_GprimBase const * hdprman_rprim =
                dynamic_cast<HdPrman_GprimBase const *>(rprim);
        if(hdprman_rprim) {
            hdprman_rprim->UpdateInstanceVisibility( vis, riley);
        }
    }
}

} // end anonymous namespace

void
HdPrman_RenderPass::_Execute(
    HdRenderPassStateSharedPtr const& renderPassState,
    TfTokenVector const & renderTags)
{
    HD_TRACE_FUNCTION();
    
    if (!TF_VERIFY(_renderParam)) {
        return;
    }

    HdPrmanRenderDelegate * const renderDelegate =
        static_cast<HdPrmanRenderDelegate*>(
            GetRenderIndex()->GetRenderDelegate());

    if (renderDelegate->IsInteractive()) {
        if (_renderParam->IsPauseRequested()) {
            // No more updates if pause is pending
            return;
        }
    } else {
        // Delete the render thread if there is one
        // (if switching from interactive to offline rendering).
        _renderParam->DeleteRenderThread();
    }

    // XXX The active render settings prim is currently used to create the
    //     riley render view *only* when AOV bindings aren't available.
    //     
    //     Add support to use it to drive the:
    //     - riley options (these should override opinions from the legacy
    //       render settings map)
    //     - active camera(s)
    //     - framing and data window
    //

    // Render settings can come from the legacy render settings map OR/AND
    // the active render settings prim. We need to track and handle changes
    // to either of them.
    bool legacySettingsChanged = false;
    bool primSettingsChanged = false;
    const HdPrman_RenderSettings *renderSettingsPrim = nullptr;
    {
        // Legacy settings.
        const int currentLegacySettingsVersion =
            renderDelegate->GetRenderSettingsVersion();
        legacySettingsChanged = _renderParam->GetLastLegacySettingsVersion() 
                                != currentLegacySettingsVersion;
        if (legacySettingsChanged) {
            _renderParam->SetLastLegacySettingsVersion(
                currentLegacySettingsVersion);
        }

        // Prim settings.
        SdfPath curRsPrimPath;
        const bool hasActiveRenderSettingsPrim =
            HdUtils::HasActiveRenderSettingsPrim(
                GetRenderIndex()->GetTerminalSceneIndex(), &curRsPrimPath);
        
        if (_lastRenderSettingsPrimPath != curRsPrimPath) {
            _lastRenderSettingsPrimVersion = 0;
            _lastRenderSettingsPrimPath = curRsPrimPath;
        }

        if (hasActiveRenderSettingsPrim) {
            renderSettingsPrim = dynamic_cast<const HdPrman_RenderSettings*>(
                GetRenderIndex()->GetBprim(
                    HdPrimTypeTokens->renderSettings, curRsPrimPath));

            if (TF_VERIFY(renderSettingsPrim)) {
                const unsigned int curSettingsVersion =
                    renderSettingsPrim->GetSettingsVersion();
                
                primSettingsChanged =
                    curSettingsVersion != _lastRenderSettingsPrimVersion;
                
                if (primSettingsChanged) {
                    _lastRenderSettingsPrimVersion = curSettingsVersion;
                }
            }
        }

    }
    
    HdPrman_CameraContext &cameraContext = _renderParam->GetCameraContext();
    // Camera path can come from render pass state or
    // from render settings. The camera from the render pass state
    // wins.
    if (const HdPrmanCamera * const cam =
            static_cast<const HdPrmanCamera *>(
                renderPassState->GetCamera())) {
        cameraContext.SetCameraPath(cam->GetId());
    } else {
        const VtDictionary &renderSpec =
            renderDelegate->GetRenderSetting<VtDictionary>(
                HdPrmanRenderSettingsTokens->experimentalRenderSpec,
                VtDictionary());
        const SdfPath &cameraPath =
            VtDictionaryGet<SdfPath>(
                renderSpec,
                HdPrmanExperimentalRenderSpecTokens->camera,
                VtDefault = SdfPath());
        if (!cameraPath.IsEmpty()) {
            cameraContext.SetCameraPath(cameraPath);
        }
    }


    HdRenderPassAovBindingVector const &aovBindings =
        renderPassState->GetAovBindings();
    GfVec2i resolution;

    const GfRect2i prevDataWindow = cameraContext.GetFraming().dataWindow;
    if (renderPassState->GetFraming().IsValid()) {
        // For new clients setting the camera framing.
        cameraContext.SetFraming(renderPassState->GetFraming());
    } else {
        // For old clients using the viewport.
        _GetRenderBufferSize(aovBindings,
                             GetRenderIndex(),
                             &resolution);

        const GfVec4f vp = renderPassState->GetViewport();
        cameraContext.SetFraming(
            CameraUtilFraming(
                GfRect2i(
                    // Note that the OpenGL-style viewport is y-Up
                    // but the camera framing is y-Down, so converting here.
                    GfVec2i(vp[0], resolution[1] - (vp[1] + vp[3])),
                    vp[2], vp[3])));
    }
    const bool dataWindowChanged =
        (cameraContext.GetFraming().dataWindow != prevDataWindow);

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

    const RenderProducts& renderProducts =
        renderDelegate->GetRenderSetting<RenderProducts>(
            HdPrmanRenderSettingsTokens->delegateRenderProducts, {});
    
    if (_HasRenderProducts(renderProducts)) {
        // Code path for Solaris render products
        int frame =
            renderDelegate->GetRenderSetting<int>(
                HdPrmanRenderSettingsTokens->houdiniFrame, 1);
        _renderParam->CreateRenderViewFromProducts(renderProducts, frame);

    } else if (aovBindings.empty()) {
        // Note: This is currently exercised by the hdPrman test harness,
        //       although it is possible to exercise this if the render task
        //       does not provide AOV bindings.

        // When there are no AOV-bindings, check if we have an active render
        // settings prim. If not, look up the legacy render settings map for
        // the render spec dictionary to create the render view.

        // If we just switched from a render pass state with AOV bindings
        // to one without, we attempt to create a new render view from
        // the render settings prim or render spec - and can free the
        // intermediate framebuffer the AOV display driver writes into.
        //

        // XXX When using the render settings prim, factor whether the products
        //     were dirtied to re-create the render view.
        const bool createRenderView =
            _renderParam->DeleteFramebuffer() || legacySettingsChanged;

        if (createRenderView) {
            // Use the Render Settings Prim if it generates render products.
            if (renderSettingsPrim &&
                !renderSettingsPrim->GetRenderProducts().empty()) {

                // XXX Should return whether it was successful or not.
                _renderParam->CreateRenderViewFromRenderSettingsPrim(
                    *renderSettingsPrim);

            } else {
                // Try to use the experimentalRenderSpec dictionary.
                const VtDictionary &renderSpec =
                    renderDelegate->GetRenderSetting<VtDictionary>(
                        HdPrmanRenderSettingsTokens->experimentalRenderSpec,
                        VtDictionary());
                
                // XXX Should return whether it was successful or not.
                _renderParam->CreateRenderViewFromRenderSpec(renderSpec);
            }
        }

        // XXX Data flow for resolution is from the render pass state's
        //     framing / viewport-and-renderbuffer-resolution.
        //     Update to factor render settings prim's products.
        resolution = cameraContext.GetResolutionFromDisplayWindow();

    } else {
        // Use AOV-bindings to create render view with displays that
        // have drivers writing into the intermediate framebuffer blitted
        // to the AOVs.
        _renderParam->CreateFramebufferAndRenderViewFromAovs(aovBindings);
        
        _GetRenderBufferSize(aovBindings,
                             GetRenderIndex(),
                             &resolution);
    }

    const int taskRenderTagsVersion =
            GetRenderIndex()->GetChangeTracker().GetTaskRenderTagsVersion();
    const int rprimRenderTagVersion =
            GetRenderIndex()->GetChangeTracker().GetRenderTagVersion();
 
    // XXX Data flow for purpose is from the task's render tags.
    //     Update to factor render settings prim's opinion.
    {
        // Update visibility settings of riley instances for active render tags.
        if (!_lastTaskRenderTagsVersion && !_lastRprimRenderTagVersion) {
            // No need to update the first time, only when the tags change.
            _lastTaskRenderTagsVersion = taskRenderTagsVersion;
            _lastRprimRenderTagVersion = rprimRenderTagVersion;

        } else if((taskRenderTagsVersion != _lastTaskRenderTagsVersion) ||
                (rprimRenderTagVersion != _lastRprimRenderTagVersion)) {
            // AcquireRiley will stop rendering and increase sceneVersion
            // so that the render will be re-started below.
            riley::Riley * const riley = _renderParam->AcquireRiley();
            _UpdateRprimVisibilityForPass( renderTags, GetRenderIndex(), riley);
            _lastTaskRenderTagsVersion = taskRenderTagsVersion;
            _lastRprimRenderTagVersion = rprimRenderTagVersion;
        }
    }

    // XXX Integrator params are updated from certain settings on the legacy
    //     settings map as well as the camera.
    const bool updateIntegrators = legacySettingsChanged || camChanged;
    if (updateIntegrators) {
        _renderParam->UpdateIntegrator(GetRenderIndex());
        _renderParam->UpdateQuickIntegrator(GetRenderIndex());

        if (_enableQuickIntegrate)
        {
            _quickIntegrateTime = renderDelegate->GetRenderSetting<int>(
                HdPrmanRenderSettingsTokens->interactiveIntegratorTimeout,
                200) / 1000.f;
        }
    }

    // Check if the riley camera and/or legacy options need to be updated.
    const bool resolutionChanged = _renderParam->resolution != resolution;

    if (camChanged || resolutionChanged) {

        // AcquireRiley will stop rendering and increase sceneVersion
        // so that the render will be re-started below.
        riley::Riley * const riley = _renderParam->AcquireRiley();

        if (resolutionChanged) {
            _renderParam->resolution = resolution;
            
            _renderParam->GetOptions().SetIntegerArray(
                RixStr.k_Ri_FormatResolution,
                resolution.data(), 2);
            
            _renderParam->GetRenderViewContext().SetResolution(
                resolution,
                riley);
        }

        if (resolutionChanged || dataWindowChanged) {
            // The data window in the framing may have changed even if
            // the resolution didn't. This will make sure the Ri:CropWindow
            // option gets updated.
            cameraContext.SetRileyOptionsInteractive(
                &(_renderParam->GetOptions()),
                resolution);
        }

        if (aovBindings.empty()) {
            cameraContext.UpdateRileyCameraAndClipPlanes(
                riley,
                GetRenderIndex());
        } else {
            // When using AOV-bindings, we setup the camera slightly
            // differently.
            cameraContext.UpdateRileyCameraAndClipPlanesInteractive(
                riley, 
                GetRenderIndex(),
                resolution);
        }
    }

    if (legacySettingsChanged) {
        // Update options from the legacy settings map.
        _renderParam->SetOptionsFromRenderSettingsMap(
            renderDelegate->GetRenderSettingsMap(),
            _renderParam->GetOptions());
    }

    // Commit settings options to riley
    {
        // Legacy settings (i.e., _renderParam->GetOptions())
        const bool updateLegacyOptions =
            legacySettingsChanged || camChanged || resolutionChanged;
        if (updateLegacyOptions) {
            RtParamList prunedOptions = HdPrman_Utils::PruneDeprecatedOptions(
                _renderParam->GetOptions());

            riley::Riley * const riley = _renderParam->AcquireRiley();
            riley->SetOptions(prunedOptions);

            TF_DEBUG(HDPRMAN_RENDER_SETTINGS).Msg(
                "Setting options from legacy data flow: \n%s", 
                HdPrmanDebugUtil::RtParamListToString(prunedOptions).c_str());
        }

        // XXX Until we get to a cleaner data flow, we have to handle settings
        //     opinions from legacy and (render settings) prim sources.
        //     Commit the prim's opinions last to override the legacy opinion.
        //     This needs to be done when either of the opinions are updated.
        //
        if (renderSettingsPrim &&
            (updateLegacyOptions || primSettingsChanged)) {
            RtParamList prunedOptions = HdPrman_Utils::PruneDeprecatedOptions(
                renderSettingsPrim->GetOptions());

            riley::Riley * const riley = _renderParam->AcquireRiley();
            riley->SetOptions(prunedOptions);

            TF_DEBUG(HDPRMAN_RENDER_SETTINGS).Msg(
                "Setting options from render settings prim %s:\n %s",
                renderSettingsPrim->GetId().GetText(),
                HdPrmanDebugUtil::RtParamListToString(prunedOptions).c_str());
        }
    }

    if (HdPrmanFramebuffer * const framebuffer =
            _renderParam->GetFramebuffer()) {
        if (const HdCamera * const cam =
                cameraContext.GetCamera(GetRenderIndex())) {
            // Update the framebuffer Z scaling
            framebuffer->proj =
#if HD_API_VERSION >= 44
                cam->ComputeProjectionMatrix();
#else
                cam->GetProjectionMatrix();
#endif
        }
    }

    if (renderDelegate->IsInteractive()) {
        _RestartRenderIfNecessary(renderDelegate);
    } else {
        _RenderInMainThread();
    }
    
    if (HdPrmanFramebuffer * const framebuffer =
            _renderParam->GetFramebuffer()) {
        _Blit(framebuffer, aovBindings, _converged);
    }
}

void
HdPrman_RenderPass::_RestartRenderIfNecessary(
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
        && !_renderParam->IsRendering();
}

void
HdPrman_RenderPass::_RenderInMainThread()
{
    riley::Riley * const riley = _renderParam->AcquireRiley();

    _renderParam->SetActiveIntegratorId(
        _renderParam->GetIntegratorId());
    
    HdPrman_RenderViewContext &ctx =
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
