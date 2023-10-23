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

// Return the seconds between now and then.
double
_DiffTimeToNow(std::chrono::steady_clock::time_point const &then)
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

void
_MarkBindingsAsConverged(
    HdRenderPassAovBindingVector const &aovBindings,
    const HdRenderIndex * const renderIndex)
{
    for (const HdRenderPassAovBinding aovBinding : aovBindings) {
        HdPrmanRenderBuffer * const rb =
            static_cast<HdPrmanRenderBuffer*>(
                renderIndex->GetBprim(
                    HdPrimTypeTokens->renderBuffer, aovBinding.renderBufferId));

        if (!TF_VERIFY(rb)) {
            continue;
        }
        rb->SetConverged(true);
    }
}

const HdRenderBuffer *
_GetRenderBuffer(const HdRenderPassAovBinding &aov,
                 const HdRenderIndex * const renderIndex)
{
    if (aov.renderBuffer) {
        return aov.renderBuffer;
    }
    
    return dynamic_cast<HdRenderBuffer*>(
        renderIndex->GetBprim(
            HdPrimTypeTokens->renderBuffer, aov.renderBufferId));
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

GfVec2i
_ResolveResolution(
    const HdRenderPassAovBindingVector &aovBindings,
    const HdRenderIndex * const renderIndex,
    const HdPrman_CameraContext &cameraContext)
{
    GfVec2i resolution(0);
    if (!aovBindings.empty()) {
        _GetRenderBufferSize(aovBindings, renderIndex, &resolution);
    } else if (cameraContext.GetFraming().IsValid()) {
        // This path is exercised when using the legacy render spec with the
        // test harness.
        resolution = cameraContext.GetResolutionFromDisplayWindow();
    } else {
        TF_WARN("Failed to resolve resolution.\n");
    }
    return resolution;
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

using _LegacyRenderProducts = VtArray<HdRenderSettingsMap>;

bool
_HasLegacyRenderProducts(
    HdPrmanRenderDelegate const * renderDelegate,
    _LegacyRenderProducts *legacyProducts)
{
    if (legacyProducts) {
        *legacyProducts =
            renderDelegate->GetRenderSetting<_LegacyRenderProducts>(
                HdPrmanRenderSettingsTokens->delegateRenderProducts, {});

        for (const auto &legacyProduct : *legacyProducts) {
            if (!legacyProduct.empty()) {
                return true;
            }
        }
    }

    return false;
}

bool
_HasLegacyRenderSpec(
    HdPrmanRenderDelegate const * renderDelegate,
    VtDictionary *legacyRenderSpec)
{
    if (legacyRenderSpec) {
        *legacyRenderSpec =
            renderDelegate->GetRenderSetting<VtDictionary>(
                HdPrmanRenderSettingsTokens->experimentalRenderSpec, {});

        TF_DEBUG(HDPRMAN_RENDER_PASS).Msg("Has legacy render spec = %d\n",
            !legacyRenderSpec->empty());
        return !legacyRenderSpec->empty();
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
_UpdateRprimVisibilityForPass(
    TfTokenVector const & renderTags,
    HdRenderIndex *index,
    riley::Riley *riley )
{
    for (auto id : index->GetRprimIds()) {
        HdRprim const *rprim = index->GetRprim(id);
        const TfToken tag = rprim->GetRenderTag();

        // If the rprim's render tag is not in the pass's list of tags it's
        // definitely not visible, but if it is, look at the rprim's visibility.
        const bool vis =
            std::count(renderTags.begin(), renderTags.end(), tag) &&
            rprim->IsVisible();

        HdPrman_GprimBase const * hdprman_rprim =
                dynamic_cast<HdPrman_GprimBase const *>(rprim);
        if (hdprman_rprim) {
            hdprman_rprim->UpdateInstanceVisibility( vis, riley);
        }
    }
}

} // end anonymous namespace


void
HdPrman_RenderPass::_UpdateCameraPath(
    const HdRenderPassStateSharedPtr &renderPassState,
    HdPrman_CameraContext *cameraContext)
{
    if (const HdPrmanCamera * const cam =
            static_cast<const HdPrmanCamera *>(renderPassState->GetCamera())) {
        cameraContext->SetCameraPath(cam->GetId());
    }
}

// Update the camera framing and window policy from the renderPassState.
// Return true if the dataWindow has changed.
bool
HdPrman_RenderPass::_UpdateCameraFramingAndWindowPolicy(
    const HdRenderPassStateSharedPtr &renderPassState,
    HdPrman_CameraContext *cameraContext)
{
    cameraContext->SetWindowPolicy(renderPassState->GetWindowPolicy());

    const GfRect2i prevDataWindow = cameraContext->GetFraming().dataWindow;

    if (renderPassState->GetFraming().IsValid()) {
        // For new clients setting the camera framing.
        cameraContext->SetFraming(renderPassState->GetFraming());
    } else {
        // For old clients using the viewport. This relies on AOV bindings.
        if (renderPassState->GetAovBindings().empty()) {
            TF_CODING_ERROR("Failed to resolve framing.\n");
            return false;
        }

        GfVec2i resolution;
        _GetRenderBufferSize(renderPassState->GetAovBindings(),
                             GetRenderIndex(), 
                             &resolution);

        const GfVec4f &vp = renderPassState->GetViewport();
        cameraContext->SetFraming(
            CameraUtilFraming(
                GfRect2i(
                    // Note that the OpenGL-style viewport is y-Up
                    // but the camera framing is y-Down, so converting here.
                    GfVec2i(vp[0], resolution[1] - (vp[1] + vp[3])),
                    vp[2], vp[3])));
    }

    return cameraContext->GetFraming().dataWindow != prevDataWindow;
}


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

    bool legacySettingsChanged = false;
    {
        // Legacy settings version tracking.
        const int currentLegacySettingsVersion =
            renderDelegate->GetRenderSettingsVersion();
        legacySettingsChanged = _renderParam->GetLastLegacySettingsVersion() 
                                != currentLegacySettingsVersion;
        if (legacySettingsChanged) {
            // Note: UpdateLegacyOptions() only updates the legacy options
            //       param list; it does not call SetRileyOptions().
            _renderParam->UpdateLegacyOptions();
            _renderParam->SetLastLegacySettingsVersion(
                currentLegacySettingsVersion);
        }
    }

    _UpdateRprimVisibilityFromTaskRenderTags(renderTags);

    // ------------------------------------------------------------------------
    // Determine if we can drive the render pass using the render settings
    // prim. The execution diverges from the task / legacy render settings
    // map data flow and is handled explicitly below.
    //
    // NOTE: Current support is limited.
    //       See HdPrman_RenderSettings::DriveRenderPass.
    //
    const HdRenderPassAovBindingVector &aovBindings =
        renderPassState->GetAovBindings();
    const bool passHasAovBindings = !aovBindings.empty();

    // Solaris uses the legacy render settings map to specify render products.
    // Don't use the render settings prim even if we can in this scenario.
    _LegacyRenderProducts legacyProducts;
    const bool hasLegacyProducts =
        _HasLegacyRenderProducts(renderDelegate, &legacyProducts);

    VtDictionary legacyRenderSpec;
    const bool hasLegacyRenderSpec =
        _HasLegacyRenderSpec(renderDelegate, &legacyRenderSpec);

    const bool isInteractive = renderDelegate->IsInteractive();

    HdPrman_RenderSettings * const rsPrim =
        _GetDrivingRenderSettingsPrim();
    const bool driveWithRenderSettingsPrim =
        !hasLegacyProducts &&
        !hasLegacyRenderSpec &&
        rsPrim &&
        rsPrim->DriveRenderPass(isInteractive,
                                passHasAovBindings);
    
    if (driveWithRenderSettingsPrim) {
        HdPrman_RenderParam * const param = _renderParam.get();
        
        const bool success =
            rsPrim->UpdateAndRender(GetRenderIndex(), isInteractive, param);

        // Mark all the associated RenderBuffers as converged since 
        // they are not being used in favor of the RenderProducts from the
        // RenderSettings prim.
        // XXX When we add support to drive interactive rendering with
        //     render settings, this workaround will need to be addressed.
        if (success) {
            if (passHasAovBindings) {
                _MarkBindingsAsConverged(aovBindings, GetRenderIndex());
            }
            _converged = true;

            return;
        }
        TF_WARN("Could not drive render pass successfully using render settings"
                " prim %s. Falling back to legacy (task driven) path.\n",
                rsPrim->GetId().GetText());
    }

    //
    // ------------------------------------------------------------------------
    // Update framing and window policy on the camera context.
    // Resolve resolution prior to render view creation below.
    //

    HdPrman_CameraContext &cameraContext = _renderParam->GetCameraContext();
    _UpdateCameraPath(renderPassState, &cameraContext);
    const bool dataWindowChanged = _UpdateCameraFramingAndWindowPolicy(
        renderPassState, &cameraContext);
    // XXX This should come from the camera.
    cameraContext.SetFallbackShutterCurve(isInteractive);
    const bool camChanged = cameraContext.IsInvalid();
    cameraContext.MarkValid();
    
    // Data flow for resolution is a bit convoluted.
    const GfVec2i resolution = _ResolveResolution(
        aovBindings, GetRenderIndex(), cameraContext);

    const bool resolutionChanged = _renderParam->GetResolution() != resolution;
    if (resolutionChanged) {
        _renderParam->SetResolution(resolution);
    }
    //
    // ------------------------------------------------------------------------
    // Create/update the Riley RenderView.
    //
    // There is divergence in whether the render view (and associated resouces)
    // are always re-created or updated in the branches below and the
    // resolution used for the render target. For the latter, we specifically
    // update the resolution on the render view context below. 
    //
    if (hasLegacyProducts) {
        // Use RenderProducts from the RenderSettingsMap (Solaris)
        int frame =
            renderDelegate->GetRenderSetting<int>(
                HdPrmanRenderSettingsTokens->houdiniFrame, 1);
        _renderParam->CreateRenderViewFromLegacyProducts(legacyProducts, frame);

    } else if (!passHasAovBindings) {
        // Note: This handles the case that we are rendering with the 
        // render spec through the HdPrman test harness.

        if (hasLegacyRenderSpec) {
            // If we just switched from a render pass state with AOV bindings
            // to one without, we attempt to create a new render view from
            // the render spec - and can free the intermediate framebuffer the
            // AOV display driver writes into.
            //
            const bool createRenderView =
                _renderParam->DeleteFramebuffer() || legacySettingsChanged;

            if (createRenderView) {
                _renderParam->CreateRenderViewFromRenderSpec(legacyRenderSpec);
            }
        } else {
            TF_WARN("Could not create render view because the render pass "
                    "has no AOV bindings, driving render settings prim OR "
                    "legacy render spec.");
            return;
        }

    } else {
        // Use AOV-bindings to create render view with displays that
        // have drivers writing into the intermediate framebuffer blitted
        // to the AOVs.
        _renderParam->CreateFramebufferAndRenderViewFromAovs(aovBindings);
    }

    HdPrman_RenderViewContext &rvCtx = _renderParam->GetRenderViewContext();
    if (!TF_VERIFY(rvCtx.GetRenderViewId() != riley::RenderViewId::InvalidId(),
                   "Render view creation failed.\n")) {
        return;
    }

    if (resolutionChanged) {
        rvCtx.SetResolution(resolution, _renderParam->AcquireRiley());
    }
    //
    // ------------------------------------------------------------------------

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

    if (camChanged || resolutionChanged) {
        riley::Riley * const riley = _renderParam->AcquireRiley();

        // Resolution affects the data flow to riley in the following ways:
        // 1. Render target size (associated with the render view)
        // 2. The "Ri:FormatResolution" and "Ri:CropWindow" scene options
        // 3. The "Ri:ScreenWindow" param on the riley camera
        //
        // (1) was handled earlier.
        
        // Handle (2) ...
        if (resolutionChanged) {
            _renderParam->GetLegacyOptions().SetIntegerArray(
                RixStr.k_Ri_FormatResolution,
                resolution.data(), 2);
        }

        if (resolutionChanged || dataWindowChanged) {
            // The data window in the framing may have changed even if
            // the resolution didn't. This will make sure the Ri:CropWindow
            // option gets updated.
            cameraContext.SetRileyOptionsInteractive(
                &(_renderParam->GetLegacyOptions()),
                resolution);
        }

        // and (3).
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

    // Commit updated scene options.
    {
        const bool updateLegacyOptions =
            legacySettingsChanged || camChanged || resolutionChanged;
        if (updateLegacyOptions) {
            _renderParam->SetRileyOptions();
        }
    }

    if (HdPrmanFramebuffer * const framebuffer =
            _renderParam->GetFramebuffer()) {
        if (const HdCamera * const cam =
                cameraContext.GetCamera(GetRenderIndex())) {
            // Update the framebuffer Z scaling
#if HD_API_VERSION >= 44
            framebuffer->proj = cam->ComputeProjectionMatrix();
#else
            framebuffer->proj = cam->GetProjectionMatrix();
#endif
        }
    }

    if (renderDelegate->IsInteractive()) {
        // This path uses the render thread to start the render.
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

// XXX Data flow for purpose is currently using the task's render tags.
//     Update to factor render settings prim's opinion.
void
HdPrman_RenderPass::_UpdateRprimVisibilityFromTaskRenderTags(
    TfTokenVector const &renderTags)
{
    const int taskRenderTagsVersion =
        GetRenderIndex()->GetChangeTracker().GetTaskRenderTagsVersion();
    const int rprimRenderTagVersion =
            GetRenderIndex()->GetChangeTracker().GetRenderTagVersion();
 
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
}

HdPrman_RenderSettings*
HdPrman_RenderPass::_GetDrivingRenderSettingsPrim() const
{
    return dynamic_cast<HdPrman_RenderSettings*>(
        GetRenderIndex()->GetBprim(HdPrimTypeTokens->renderSettings,
        _renderParam->GetDrivingRenderSettingsPrimPath()));
}


PXR_NAMESPACE_CLOSE_SCOPE
