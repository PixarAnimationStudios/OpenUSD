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

using RenderProducts = VtArray<HdRenderSettingsMap>;

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
_HasRenderProducts(const RenderProducts &renderProducts)
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

const HdPrman_RenderSettings *
_GetActiveRenderSettingsPrim(HdRenderIndex *renderIndex)
{
    SdfPath primPath;
    const bool hasActiveRenderSettingsPrim =
        HdUtils::HasActiveRenderSettingsPrim(
            renderIndex->GetTerminalSceneIndex(), &primPath);
    
    if (hasActiveRenderSettingsPrim) {
        return dynamic_cast<const HdPrman_RenderSettings*>(
            renderIndex->GetBprim(HdPrimTypeTokens->renderSettings, primPath));
    }

    return nullptr;
}

// This helper function also exists in testHdPrman
GfVec2i
_MultiplyAndRound(const GfVec2f &a, const GfVec2i &b)
{
    return GfVec2i(std::roundf(a[0] * b[0]),
                   std::roundf(a[1] * b[1]));
}

void
_SetShutterCurve(bool isInteractive, HdPrman_CameraContext *cameraContext)
{
    // XXX Until we add support for PxrCameraAPI, hardcode the shutter opening
    //     and closing rates as follows:
    if (isInteractive) {
        // Open instantaneously, remain fully open for the duration of the
        // shutter interval (set via the param RixStr.k_Ri_Shutter) and close
        // instantaneously.
        static const float pts[8] = {
            0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f
        };
        cameraContext->SetShutterCurve(0.0f, 1.0f, pts);
    } else {
        // Open instantaneously and start closing immediately, rapidly at first
        // decelerating until the end of the interval.
        static const float pts[8] = {
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.3f, 0.0f
        };
        cameraContext->SetShutterCurve(0.0f, 0.0f, pts);
    }
}

} // end anonymous namespace

void
HdPrman_RenderPass::_UpdateCameraPath(
    const HdPrman_RenderSettings * const renderSettingsPrim,
    const HdRenderPassStateSharedPtr &renderPassState,
    HdPrmanRenderDelegate * const renderDelegate,
    HdPrman_CameraContext *cameraContext)
{
    // Camera path can come from RenderProducts on the RenderSettings Prim.
    // or render pass state.
    if (renderSettingsPrim) {
        const HdRenderSettings::RenderProducts &renderProducts =
            renderSettingsPrim->GetRenderProducts();
        SdfPath cameraPath = renderProducts.at(0).cameraPath;
        cameraContext->SetCameraPath(cameraPath);
    } else if (const HdPrmanCamera * const cam =
            static_cast<const HdPrmanCamera *>(renderPassState->GetCamera())) {
        cameraContext->SetCameraPath(cam->GetId());
    }
}

// Update the Camera Framing from the RenderSettingsPrim, or RenderPassState
// Return true if the dataWindow has changed.
bool
HdPrman_RenderPass::_UpdateCameraFraming(
    const HdPrman_RenderSettings * const renderSettingsPrim,
    const HdRenderPassStateSharedPtr &renderPassState,
    HdPrman_CameraContext *cameraContext,
    GfVec2i *resolution)
{
    // Update Camera Framing on the Camera Context
    const GfRect2i prevDataWindow = cameraContext->GetFraming().dataWindow;

    if (renderSettingsPrim) {
        // For clients relying on the RenderSettings Prim
        const HdRenderSettings::RenderProduct &product =
            renderSettingsPrim->GetRenderProducts().at(0);
        
        *resolution = product.resolution;
        GfRange2f displayWindow(GfVec2f(0.0f), GfVec2f(*resolution));
        GfRange2f dataWindowNDC = product.dataWindowNDC;
        const GfRect2i dataWindow(
            _MultiplyAndRound(dataWindowNDC.GetMin(), *resolution),
            _MultiplyAndRound(dataWindowNDC.GetMax(), *resolution) - GfVec2i(1));

        cameraContext->SetFraming(
            CameraUtilFraming(
                displayWindow, dataWindow, product.pixelAspectRatio));
        cameraContext->SetWindowPolicy(
            HdUtils::ToConformWindowPolicy(product.aspectRatioConformPolicy));
    } else if (renderPassState->GetFraming().IsValid()) {
        // For new clients setting the camera framing.
        cameraContext->SetFraming(renderPassState->GetFraming());
        cameraContext->SetWindowPolicy(renderPassState->GetWindowPolicy());
    } else {
        // For old clients using the viewport.
        _GetRenderBufferSize(renderPassState->GetAovBindings(),
                             GetRenderIndex(),
                             resolution);

        const GfVec4f &vp = renderPassState->GetViewport();
        cameraContext->SetFraming(
            CameraUtilFraming(
                GfRect2i(
                    // Note that the OpenGL-style viewport is y-Up
                    // but the camera framing is y-Down, so converting here.
                    GfVec2i(vp[0], (*resolution)[1] - (vp[1] + vp[3])),
                    vp[2], vp[3])));
        cameraContext->SetWindowPolicy(renderPassState->GetWindowPolicy());
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

    // XXX The active render settings prim is currently used to create the
    //     riley render view *only* when AOV bindings aren't available.
    //     
    //     Add support to use it to drive the:
    //     - riley options (these should override opinions from the legacy
    //       render settings map)
    //     - active camera(s)
    //     - framing and data window
    //

    const HdRenderPassAovBindingVector &aovBindings =
        renderPassState->GetAovBindings();

    // Render settings can come from the legacy render settings map OR/AND
    // the active render settings prim. We handle changes to the render settings
    // prim during Sync.
    const HdPrman_RenderSettings * const rsPrim =
        _GetActiveRenderSettingsPrim(GetRenderIndex());
    
    // Currently we only use the RenderSettings Prim in two situations
    // 1. Behind the HD_PRMAN_RENDER_SETTINGS_PRIM_DRIVE_RENDER_PASS env var,
    //    with a valid Render Settings Prim, and a renderDelegate that is NOT
    //    interactive.
    // 2. In the HdPrman Test harness when we do not have any aovBindings.
    // 
    // XXX Interactive viewport rendering using hdPrman currently relies on 
    // having AOV bindings and uses the "hydra" Display Driver to write 
    // rendered pixels into an intermediate framebuffer which is then blit 
    // into the Hydra AOVs. Using the render settings prim to drive the render 
    // pass in an interactive viewport setting is not yet supported.
    // 
    // XXX Using the render settings prim to drive the render pass in an 
    // interactive viewport setting is not yet supported. Interactive viewport 
    // rendering using hdPrman currently relies on having AOV bindings and uses
    // the "hydra" Display Driver to write rendered pixels into an intermediate 
    // framebuffer which is then blit into the Hydra AOVs. 
    const bool validRenderSettings = rsPrim && rsPrim->IsValid();
    const bool driveWithRenderSettingsPrim = 
        (validRenderSettings && rsPrim->DriveRenderPass() 
            && !renderDelegate->IsInteractive()) || aovBindings.empty();
    TF_DEBUG(HDPRMAN_RENDER_PASS).Msg(
        "Drive with RenderSettingsPrim: \n"
        " - HD_RENDER_SETTINGS_PRIM_DRIVE_RENDER_PASS = %d\n"
        " - validRenderSettingsPrim = %d\n - interactive renderDelegate %d\n",
        rsPrim->DriveRenderPass(), validRenderSettings, 
        renderDelegate->IsInteractive());

    bool legacySettingsChanged = false;
    {
        // Legacy settings version tracking.
        const int currentLegacySettingsVersion =
            renderDelegate->GetRenderSettingsVersion();
        legacySettingsChanged = _renderParam->GetLastLegacySettingsVersion() 
                                != currentLegacySettingsVersion;
        if (legacySettingsChanged) {
            _renderParam->SetLastLegacySettingsVersion(
                currentLegacySettingsVersion);
        }
    }

    // Update the Camera Context
    HdPrman_CameraContext &cameraContext = _renderParam->GetCameraContext();
    _UpdateCameraPath(
        (driveWithRenderSettingsPrim) ? rsPrim : nullptr, 
        renderPassState, renderDelegate, &cameraContext);

    GfVec2i resolution;
    const bool dataWindowChanged =
        _UpdateCameraFraming(
            (driveWithRenderSettingsPrim) ? rsPrim : nullptr, 
            renderPassState, &cameraContext, &resolution);

    _SetShutterCurve(renderDelegate->IsInteractive(), &cameraContext);

    const bool camChanged = cameraContext.IsInvalid();
    cameraContext.MarkValid();

    // Create the Riley RenderView 
    const RenderProducts& renderProducts =
        renderDelegate->GetRenderSetting<RenderProducts>(
            HdPrmanRenderSettingsTokens->delegateRenderProducts, {});

    if (_HasRenderProducts(renderProducts)) {
        // Use RenderProducts from the RenderSettingsMap (Solaris)
        int frame =
            renderDelegate->GetRenderSetting<int>(
                HdPrmanRenderSettingsTokens->houdiniFrame, 1);
        _renderParam->CreateRenderViewFromProducts(renderProducts, frame);

    } else if (driveWithRenderSettingsPrim) {
        // Note: This includes the case that we are rendering with the 
        // render spec through the HdPrman test harness, in addition to using
        // the render settings prim 

        // If we just switched from a render pass state with AOV bindings
        // to one without, we attempt to create a new render view from
        // the render settings prim or render spec - and can free the
        // intermediate framebuffer the AOV display driver writes into.
        //
        // XXX When using the render settings prim, factor whether the products
        //     were dirtied to re-create the render view.
        const bool createRenderView =
            _renderParam->DeleteFramebuffer() || legacySettingsChanged;

        // If we do not have a valid render settings prim, look up the 
        // legacy render settings map for the render spec dictionary to create
        // the render view.
        if (createRenderView) {
            if (validRenderSettings) {
                TF_DEBUG(HDPRMAN_RENDER_PASS)
                    .Msg("Create Riley RenderView from the RenderSettings Prim"
                         " <%s>.\n", rsPrim->GetId().GetText());
                // XXX Should return whether it was successful or not.
                _renderParam->CreateRenderViewFromRenderSettingsPrim(*rsPrim);

            } else {
                TF_DEBUG(HDPRMAN_RENDER_PASS)
                    .Msg("Create Riley RenderView from the RenderSpec.\n");
                const VtDictionary &renderSpec =
                    renderDelegate->GetRenderSetting<VtDictionary>(
                        HdPrmanRenderSettingsTokens->experimentalRenderSpec,
                        VtDictionary());
                
                // XXX Should return whether it was successful or not.
                _renderParam->CreateRenderViewFromRenderSpec(renderSpec);
                resolution = cameraContext.GetResolutionFromDisplayWindow();
            }
        }

    } else {
        // Use AOV-bindings to create render view with displays that
        // have drivers writing into the intermediate framebuffer blitted
        // to the AOVs.
        TF_DEBUG(HDPRMAN_RENDER_PASS)
            .Msg("Create Riley RenderView from AOV's\n");
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

    // Update options from the legacy settings map.
    if (legacySettingsChanged) {
        // This should happen before the camera related settings below,
        // because some of those, like Ri:FormatResolution should win
        // when coming from the camera context.
        _renderParam->UpdateLegacyOptions();
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
            
            _renderParam->GetLegacyOptions().SetIntegerArray(
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
                &(_renderParam->GetLegacyOptions()),
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
        _RestartRenderIfNecessary(renderDelegate);
    } else {
        _RenderInMainThread();
    }
    
    if (HdPrmanFramebuffer * const framebuffer =
            _renderParam->GetFramebuffer()) {
        _Blit(framebuffer, aovBindings, _converged);
    }

    // If the RenderPass is driven by the RenderSettings prim and we also have
    // AOV bindings, mark all the associated RenderBuffers as converged since 
    // they are not being used in favor of the RenderProducts from the
    // RenderSettings prim.
    if (driveWithRenderSettingsPrim && !aovBindings.empty()) {
        _MarkBindingsAsConverged(aovBindings, GetRenderIndex());
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
