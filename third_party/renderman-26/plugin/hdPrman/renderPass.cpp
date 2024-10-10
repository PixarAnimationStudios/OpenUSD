//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/renderPass.h" 

#include "hdPrman/camera.h"
#include "hdPrman/debugCodes.h"
#include "hdPrman/debugUtil.h"
#include "hdPrman/framebuffer.h"
#include "hdPrman/renderBuffer.h"
#include "hdPrman/renderDelegate.h"
#include "hdPrman/renderParam.h"
#if PXR_VERSION >= 2308
#include "hdPrman/renderSettings.h"
#endif
#include "hdPrman/rixStrings.h"
#include "hdPrman/utils.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/rprim.h"
#if PXR_VERSION >= 2308
#include "pxr/imaging/hd/utils.h"
#endif

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/token.h"

#include <Riley.h>

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
, _projection(HdPrmanProjectionTokens->PxrPerspective)
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
    for (const HdRenderPassAovBinding& aovBinding : aovBindings) {
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
    const HdPrman_CameraContext &cameraContext,
    bool hasLegacyProducts)
{
    GfVec2i resolution(0);
    if (!aovBindings.empty() && !hasLegacyProducts) {
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

        return !legacyRenderSpec->empty();
    }

    return false;
}

template <typename T>
T
GetProductSetting(
    const HdRenderSettingsMap& settingsMap, const TfToken& key, const T& def)
{
    auto val = settingsMap.find(key);
    if (val != settingsMap.end() && val->second.IsHolding<T>()) {
        return val->second.UncheckedGet<T>();
    }
    return def;
}

// Take into account the render settings resolution, dataWindowNDC,
// pixelAspectRatio and aspectRatioConformPolicy for the camera framing.
void
_ComputeCameraFramingFromSettings(
    HdRenderPassStateSharedPtr const& renderPassState,
    HdPrmanRenderDelegate* const renderDelegate,
    const _LegacyRenderProducts& renderProducts,
    const GfVec2i renderBufferSize)
{
    // Get the resolution
    GfVec2i resolution = renderDelegate->GetRenderSetting<GfVec2i>(
        HdPrmanRenderSettingsTokens->resolution, renderBufferSize);

    // Get the data window NDC
    static const GfVec4f dataWindowDefault(0.0, 0.0, 1.0, 1.0);
    GfVec4f dataWindow = renderDelegate->GetRenderSetting<GfVec4f>(
        HdPrmanRenderSettingsTokens->dataWindowNDC, dataWindowDefault);

    // Get the pixel aspect ratio
    static const float pixelAspectRatioDefault(1.0);
    float pixelAspectRatio = renderDelegate->GetRenderSetting<float>(
        HdPrmanRenderSettingsTokens->pixelAspectRatio, pixelAspectRatioDefault);

    // Get the aspect ratio conform policy
    std::string aspectRatioConformPolicy
        = renderDelegate->GetRenderSetting<std::string>(
            HdPrmanRenderSettingsTokens->aspectRatioConformPolicy,
            HdAspectRatioConformPolicyTokens->expandAperture);

    // Render Product Settings > Render Settings
    for (const HdRenderSettingsMap& renderProduct : renderProducts) {
        resolution = GetProductSetting<GfVec2i>(
            renderProduct, HdPrmanRenderSettingsTokens->resolution, resolution);
        dataWindow = GetProductSetting<GfVec4f>(
            renderProduct, HdPrmanRenderSettingsTokens->dataWindowNDC,
            dataWindow);
        pixelAspectRatio = GetProductSetting<float>(
            renderProduct, HdPrmanRenderSettingsTokens->pixelAspectRatio,
            pixelAspectRatio);
        aspectRatioConformPolicy = GetProductSetting<std::string>(
            renderProduct,
            HdPrmanRenderSettingsTokens->aspectRatioConformPolicy,
            aspectRatioConformPolicy);
    }

    // Create the camera framing
    CameraUtilFraming framing;
    framing.displayWindow
        = GfRange2f(GfVec2f(0, 0), GfVec2f(resolution[0], resolution[1]));
    // dataWindowNDC y-up but dataWindow y-down :(
    framing.dataWindow = GfRect2i(
        GfVec2i(
            ceilf(resolution[0] * dataWindow[0]),
            resolution[1] - ceilf(resolution[1] * dataWindow[3])),
        GfVec2i(
            ceilf(resolution[0] * dataWindow[2]) - 1,
            resolution[1] - ceilf(resolution[1] * dataWindow[1]) - 1));
    framing.pixelAspectRatio = pixelAspectRatio;

    // Data windows are supposed to be relative to the render buffer. Offset the
    // data window to start at zero. This assumes the data window equals the
    // renderbuffer, which can be incorrect but is okay for our needs.
    framing.displayWindow = GfRange2f(
        framing.displayWindow.GetMin() - framing.dataWindow.GetMin(),
        framing.displayWindow.GetMax() - framing.dataWindow.GetMin());
    framing.dataWindow.Translate(-framing.dataWindow.GetMin());

    // Map aspectRatioConformPolicy setting to CameraUtilConformWindowPolicy
    CameraUtilConformWindowPolicy conformPolicy = CameraUtilDontConform;
    if (aspectRatioConformPolicy
        == HdAspectRatioConformPolicyTokens->expandAperture) {
        conformPolicy = CameraUtilFit;
    } else if (aspectRatioConformPolicy
        == HdAspectRatioConformPolicyTokens->cropAperture) {
        conformPolicy = CameraUtilCrop;
    } else if (aspectRatioConformPolicy
        == HdAspectRatioConformPolicyTokens->adjustApertureWidth) {
        conformPolicy = CameraUtilMatchVertically;
    } else if (aspectRatioConformPolicy
        == HdAspectRatioConformPolicyTokens->adjustApertureHeight) {
        conformPolicy = CameraUtilMatchHorizontally;
    } else if (aspectRatioConformPolicy
        == HdAspectRatioConformPolicyTokens->adjustPixelAspectRatio) {
        conformPolicy = CameraUtilDontConform;
    }

    // Update the render pass state
#if PXR_VERSION > 2311
    renderPassState->SetCamera(renderPassState->GetCamera());
    renderPassState->SetFraming(framing);
    renderPassState->SetOverrideWindowPolicy(
        std::optional<CameraUtilConformWindowPolicy>(conformPolicy));
#else
    renderPassState->SetCameraAndFraming(
        renderPassState->GetCamera(), framing,
        std::pair<bool, CameraUtilConformWindowPolicy>(true, conformPolicy));
#endif
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
    HdPrmanRenderDelegate * const renderDelegate,
    HdPrman_CameraContext *cameraContext)
{
    const GfRect2i prevDataWindow = cameraContext->GetFraming().dataWindow;

    if (renderPassState->GetFraming().IsValid()) {
        // For new clients setting the camera framing.
        cameraContext->SetFraming(renderPassState->GetFraming());
        cameraContext->SetWindowPolicy(renderPassState->GetWindowPolicy());
    } else {
        // Note, commenting this out; it leads to prman crashing in Houdini 19.5
        // // For old clients using the viewport. This relies on AOV bindings.
        // if (renderPassState->GetAovBindings().empty()) {
        //     TF_CODING_ERROR("Failed to resolve framing.\n");
        //     return false;
        // }
        const _LegacyRenderProducts& renderProducts =
            renderDelegate->GetRenderSetting<_LegacyRenderProducts>(
                HdPrmanRenderSettingsTokens->delegateRenderProducts, {});
        HdRenderPassAovBindingVector const &aovBindings =
            renderPassState->GetAovBindings();
        GfVec2i resolution;
        // Size of AOV buffers
        if (!_GetRenderBufferSize(aovBindings, GetRenderIndex(), &resolution)) {
            // For clients not using AOVs, take size of viewport.
            const GfVec4f vp = renderPassState->GetViewport();
            resolution[0] = vp[2];
            resolution[1] = vp[3];
        }

        if (renderProducts.empty()) {
            const GfVec4f &vp = renderPassState->GetViewport();
            cameraContext->SetFraming(
                CameraUtilFraming(
                    GfRect2i(
                        // Note that the OpenGL-style viewport is y-Up
                        // but the camera framing is y-Down, so converting here.
                        GfVec2i(vp[0], resolution[1] - (vp[1] + vp[3])),
                        vp[2], vp[3])));
        } else {
            // If no camera framing was provided,
            // try to get the framing from the render settings.
            _ComputeCameraFramingFromSettings(
                renderPassState, renderDelegate, renderProducts, resolution);
            cameraContext->SetFraming(renderPassState->GetFraming());
        }
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
    
    _UpdateActiveRenderTagsIfChanged(renderTags);

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

#if PXR_VERSION >= 2308
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
#endif

    //
    // ------------------------------------------------------------------------
    // Update framing and window policy on the camera context.
    // Resolve resolution prior to render view creation below.
    //

    HdPrman_CameraContext &cameraContext = _renderParam->GetCameraContext();
    const bool framingValid = renderPassState->GetFraming().IsValid();
    _UpdateCameraPath(renderPassState, &cameraContext);
    const bool dataWindowChanged = _UpdateCameraFramingAndWindowPolicy(
        renderPassState, renderDelegate, &cameraContext);
    const bool camChanged = cameraContext.IsInvalid();
    cameraContext.MarkValid();

    // Data flow for resolution is a bit convoluted.
    const GfVec2i resolution =
        _renderParam->IsXpu() ? // Remove once XPU handles under/overscan.
        cameraContext.GetResolutionFromDataWindow() :
        _ResolveResolution(
            aovBindings, GetRenderIndex(), cameraContext, hasLegacyProducts);

    const bool resolutionChanged = _renderParam->GetResolution() != resolution;
    if (resolutionChanged) {
        _renderParam->SetResolution(resolution);
    }

#ifdef DO_FALLBACK_LIGHTS
    // Enable/disable the fallback light when the scene provides no lights.
    _renderParam->SetFallbackLightsEnabled(
        !_renderParam->HasSceneLights());
#else
    _renderParam->SetFallbackLightsEnabled(false);
#endif

    int frame =
        renderDelegate->GetRenderSetting<int>(
            HdPrmanRenderSettingsTokens->houdiniFrame, 1);
    const bool frameChanged = _renderParam->frame != frame;
    _renderParam->frame = frame;


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
        if (frameChanged) {
            riley::Riley * const riley = _renderParam->AcquireRiley();
            if(!riley) {
                return;
            }
            _renderParam->GetRenderViewContext().DeleteRenderView(riley);
        }
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
#if PXR_VERSION >= 2308
        _renderParam->CreateFramebufferAndRenderViewFromAovs(aovBindings,
                                                             rsPrim);
#else
        _renderParam->CreateFramebufferAndRenderViewFromAovs(aovBindings);
#endif
    }

    HdPrman_RenderViewContext &rvCtx = _renderParam->GetRenderViewContext();
    if (!TF_VERIFY(rvCtx.GetRenderViewId() != riley::RenderViewId::InvalidId(),
                   "Render view creation failed.\n")) {
        return;
    }

    if (resolutionChanged || camChanged) {
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
        if (aovBindings.empty() || hasLegacyProducts) {
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

    // Update options from the legacy settings map.
    if (legacySettingsChanged) {
        _renderParam->UpdateLegacyOptions();

        // Set Projection Settings
        _projection = renderDelegate->GetRenderSetting<std::string>(
            HdPrmanRenderSettingsTokens->projectionName,
            _projection);

        RtParamList projectionParams;
        _renderParam->SetProjectionParamsFromRenderSettings(
            (HdPrmanRenderDelegate*)renderDelegate,
            _projection,
             projectionParams);

        cameraContext.SetProjectionOverride(RtUString(_projection.c_str()),
                                            projectionParams);

        // Set Resolution, Crop Window, Pixel Aspect Ratio,
        // and update camera settings.
        // For valid framing this was handled above.
        if(!framingValid) {
            riley::Riley * const riley = _renderParam->AcquireRiley();
            if (_renderParam->IsXpu()) {
                // This can be removed once XPU handles under/overscan correctly.
                cameraContext.SetRileyOptionsInteractive(
                    &(_renderParam->GetLegacyOptions()), _renderParam->GetResolution());
                cameraContext.UpdateRileyCameraAndClipPlanesInteractive(
                    riley, GetRenderIndex(), _renderParam->GetResolution());
            }
            else {
                cameraContext.SetRileyOptions(&(_renderParam->GetLegacyOptions()));
                cameraContext.UpdateRileyCameraAndClipPlanes(
                    riley, GetRenderIndex());
            }
        }

        cameraContext.SetDisableDepthOfField(
            renderDelegate->GetRenderSetting<bool>(
                HdPrmanRenderSettingsTokens->disableDepthOfField, false));

        // Set Display and Sample Filters
        _renderParam->SetFiltersFromRenderSettings(
            (HdPrmanRenderDelegate*)renderDelegate);
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

    if (isInteractive) {
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

        if(_renderParam->GetRenderViewContext().GetRenderViewId() !=
           riley::RenderViewId::InvalidId()) {
            _renderParam->StartRender();
            _frameStart = std::chrono::steady_clock::now();
        } else {
            _renderParam->FatalError("No display found. Try raster output type.");
            return;
        }
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
HdPrman_RenderPass::_UpdateActiveRenderTagsIfChanged(
    const TfTokenVector& taskRenderTags)
{
    const int taskRenderTagsVersion =
        GetRenderIndex()->GetChangeTracker().GetTaskRenderTagsVersion();
    const int rprimRenderTagVersion =
            GetRenderIndex()->GetChangeTracker().GetRenderTagVersion();
    if ((taskRenderTagsVersion != _lastTaskRenderTagsVersion) ||
        (rprimRenderTagVersion != _lastRprimRenderTagVersion)) {
        _renderParam->SetActiveRenderTags(taskRenderTags, GetRenderIndex());
        _lastTaskRenderTagsVersion = taskRenderTagsVersion;
        _lastRprimRenderTagVersion = rprimRenderTagVersion;
    }
}

#if PXR_VERSION >= 2308
HdPrman_RenderSettings*
HdPrman_RenderPass::_GetDrivingRenderSettingsPrim() const
{
    return dynamic_cast<HdPrman_RenderSettings*>(
        GetRenderIndex()->GetBprim(HdPrimTypeTokens->renderSettings,
        _renderParam->GetDrivingRenderSettingsPrimPath()));
}
#endif


PXR_NAMESPACE_CLOSE_SCOPE
