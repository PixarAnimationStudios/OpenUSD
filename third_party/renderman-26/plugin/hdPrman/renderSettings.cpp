//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/renderSettings.h"

#if PXR_VERSION >= 2308

#include "hdPrman/debugCodes.h"
#include "hdPrman/debugUtil.h"
#include "hdPrman/camera.h"
#include "hdPrman/cameraContext.h"
#if PXR_VERSION <= 2308
#include "hdPrman/renderDelegate.h"
#endif
#include "hdPrman/renderParam.h"
#include "hdPrman/renderViewContext.h"
#include "hdPrman/rixStrings.h"
#include "hdPrman/utils.h"

#include "pxr/imaging/hd/cameraSchema.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/imaging/hd/utils.h"
#include "pxr/imaging/hdsi/renderSettingsFilteringSceneIndex.h"

#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/envSetting.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

// This env var exists only to compare results from driving the render pass 
// using the task's aov bindings v/s using the render settings prim.
// This is currently relevant and limited to non-interactive rendering
// (e.g., in an application like usdrecord).
//
// See DriveRenderPass(..) below for more info.
// 
TF_DEFINE_ENV_SETTING(HD_PRMAN_RENDER_SETTINGS_DRIVE_RENDER_PASS, false,
                      "Drive the render pass using the first RenderProduct on "
                      "the render settings prim when the render pass has "
                      "AOV bindings.");

TF_DEFINE_ENV_SETTING(HD_PRMAN_RENDER_SETTINGS_BUNDLE_RENDER_PRODUCTS, false,
                      "If true, all render products for the active render "
                      "settings are rendered within the same render view.");

TF_DEFINE_PRIVATE_TOKENS(
    _renderTerminalTokens, // properties in PxrRenderTerminalsAPI
    ((outputsRiIntegrator, "outputs:ri:integrator"))
    ((outputsRiSampleFilters, "outputs:ri:sampleFilters"))
    ((outputsRiDisplayFilters, "outputs:ri:displayFilters"))
);

#if PXR_VERSION <= 2308
TF_DEFINE_PRIVATE_TOKENS(
    _legacyTokens,
    ((fallbackPath, "/Render/__HdsiRenderSettingsFilteringSceneIndex__FallbackSettings"))
    ((renderScope, "/Render"))
    (shutterInterval)
);
#endif


namespace {

// Translate properties in PxrOptionsAPI to the Riley name.
RtUString
_GetRiName(std::string const &propertyName)
{
    // strip the "ri:" prefix if present, but don't strip the "Ri:" namespace.
    // e.g. schema attribute "ri:hider:maxsamples" maps to "hider:maxsamples"
    //      (or the pre-defined UString Rix::k_hider_maxsamples)
    //      while "ri:Ri:CropWindow" maps to "Ri:CropWindow" (or
    //      the UString k_riCropWindow)
    //
    if (TfStringStartsWith(propertyName, "ri:")) {
        return RtUString(propertyName.c_str() + 3);
    }

    // Unhandled property. This likely indicates an issue with namespace 
    // filtering upstream.
    TF_WARN("Could not translate settings property %s to RtUString.",
            propertyName.c_str());
    return RtUString(propertyName.c_str());
}

RtParamList
_GenerateParamList(VtDictionary const &settings)
{
    RtParamList options;

    for (auto const &pair : settings) {
        // Skip render terminal connections.
        const std::string &name = pair.first;
        const TfToken tokenName(name);
        if (tokenName == _renderTerminalTokens->outputsRiIntegrator    ||
            tokenName == _renderTerminalTokens->outputsRiSampleFilters ||
            tokenName == _renderTerminalTokens->outputsRiDisplayFilters) {

            continue;
        }

        RtUString const riName = _GetRiName(name);
        VtValue const &val = pair.second;
        HdPrman_Utils::SetParamFromVtValue(
            riName, val, /* role */ TfToken(), &options);
    }

    return options;
}

GfVec2i
_MultiplyAndRound(const GfVec2f &a, const GfVec2i &b)
{
    return GfVec2i(std::roundf(a[0] * b[0]),
                   std::roundf(a[1] * b[1]));
}

bool
_HasNonFallbackRenderSettingsPrim(const HdSceneIndexBaseRefPtr &si)
{
    if (!si) {
        return false;
    }
    
#if PXR_VERSION >= 2311
    const SdfPath &renderScope =
        HdsiRenderSettingsFilteringSceneIndex::GetRenderScope();
    const SdfPath &fallbackPrimPath =
        HdsiRenderSettingsFilteringSceneIndex::GetFallbackPrimPath();
#else
    const SdfPath &renderScope = SdfPath(_legacyTokens->renderScope);
    const SdfPath &fallbackPrimPath = SdfPath(_legacyTokens->fallbackPath);
#endif

    for (const SdfPath &path : HdSceneIndexPrimView(si, renderScope)) {
        if (path != fallbackPrimPath &&
            si->GetPrim(path).primType == HdPrimTypeTokens->renderSettings) {
            return true;
        }
    }
    return false;
}

#if PXR_VERSION <= 2308
CameraUtilConformWindowPolicy
_ToConformWindowPolicy(const TfToken &token)
{
    if (token == HdAspectRatioConformPolicyTokens->adjustApertureWidth) {
        return CameraUtilMatchVertically;
    }
    if (token == HdAspectRatioConformPolicyTokens->adjustApertureHeight) {
        return CameraUtilMatchHorizontally;
    }
    if (token == HdAspectRatioConformPolicyTokens->expandAperture) {
        return CameraUtilFit;
    }
    if (token == HdAspectRatioConformPolicyTokens->cropAperture) {
        return CameraUtilCrop;
    }
    if (token == HdAspectRatioConformPolicyTokens->adjustPixelAspectRatio) {
        return CameraUtilDontConform;
    }

    TF_WARN(
        "Invalid aspectRatioConformPolicy value '%s', "
        "falling back to expandAperture.", token.GetText());

    return CameraUtilFit;
}
#endif

// Update the camera path, framing and shutter curve on the camera context from
// the render product.
void
_UpdateCameraContextFromProduct(
    HdRenderSettings::RenderProduct const &product,
    HdPrman_CameraContext *cameraContext)
{
    TF_DEBUG(HDPRMAN_RENDER_PASS).Msg(
        "Updating camera context from product %s\n", product.name.GetText());

    GfVec2i const &resolution = product.resolution;
    const GfRange2f displayWindow(GfVec2f(0.0f), GfVec2f(resolution));
    const GfRange2f dataWindowNDC = product.dataWindowNDC;
    const GfRect2i dataWindow(
        _MultiplyAndRound(dataWindowNDC.GetMin(), resolution),
        _MultiplyAndRound(dataWindowNDC.GetMax(), resolution) - GfVec2i(1));

    // Set the camera path to allow UpdateRileyCameraAndClipPlanes to fetch
    // necessary data from the camera Sprim.
    cameraContext->SetCameraPath(product.cameraPath);
    cameraContext->SetFraming(
        CameraUtilFraming(
            displayWindow, dataWindow, product.pixelAspectRatio));
    cameraContext->SetWindowPolicy(
#if PXR_VERSION <= 2308
        _ToConformWindowPolicy(product.aspectRatioConformPolicy));
#else
        HdUtils::ToConformWindowPolicy(product.aspectRatioConformPolicy));
#endif
#if HD_API_VERSION >= 64
    cameraContext->SetDisableDepthOfField(product.disableDepthOfField);
#endif
}

// Update the riley camera params using state on the camera Sprim and the
// camera context.
void
_UpdateRileyCamera(
    riley::Riley *riley,
    const HdRenderIndex *renderIndex,
    const SdfPath &cameraPathFromProduct,
    HdPrman_CameraContext *cameraContext)
{
    if (cameraContext->IsInvalid()) {
        TF_DEBUG(HDPRMAN_RENDER_PASS).Msg(
            "Updating riley camera %u using camera prim %s\n",
            cameraContext->GetCameraId().AsUInt32(),
            cameraContext->GetCameraPath().GetText());

        cameraContext->UpdateRileyCameraAndClipPlanes(riley, renderIndex);
        cameraContext->MarkValid();
    }
}

#if PXR_VERSION >= 2405
// Update the Frame number from the Stage Global Scene Index
void
_UpdateFrame(
    const HdSceneIndexBaseRefPtr &terminalSi,
    RtParamList *options)
{
    // Get the Frame from the Terminal Scene Index 
    double frame;
    if (!HdUtils::GetCurrentFrame(terminalSi, &frame)) {
        return;
    }

    // k_Ri_Frame in Riley is an integer, not float.
    // As an explicit policy choice, round down.
    const int intFrame(floor(frame));

    // Store on the options list to be used in a later Riley.SetOptions() call 
    HdPrman_Utils::SetParamFromVtValue(
        RixStr.k_Ri_Frame, VtValue(intFrame),
        /* role */ TfToken(), options);
}
#endif

// Create/update the render view and associated resources based on the
// render product.
void
_UpdateRenderViewContext(
    HdRenderSettings::RenderProducts const &products,
    HdPrman_RenderParam *param,
    HdPrman_RenderViewContext *renderViewContext)
{
    // The (lone) render view is managed by render param currently.
    param->CreateRenderViewFromRenderSettingsProducts(
        products, renderViewContext);
}

// Factor the product's motion blur opinion and camera's shutter.
GfVec2f
_ResolveShutterInterval(
    HdRenderSettings::RenderProduct const &product,
    HdPrman_CameraContext const &cameraContext,
    HdRenderIndex const *renderIndex)
{
    if (product.disableMotionBlur) {
        return GfVec2f(0.f);
    }

    GfVec2f shutter(0.0f, 0.5f); // fallback 180' shutter.
    if (const HdCamera * const camera =
            cameraContext.GetCamera(renderIndex)) {
        shutter[0] = camera->GetShutterOpen();
        shutter[1] = camera->GetShutterClose();
    }
    return shutter;
}

bool
_SetOptionsAndRender(
    const HdPrman_CameraContext &cameraContext,
    const HdPrman_RenderViewContext &renderViewContext,
    RtParamList renderSettingsPrimOptions,
    GfVec2f const &shutter,
    bool interactive,
    HdPrman_RenderParam *param)
{
    const riley::RenderViewId rvId = renderViewContext.GetRenderViewId();
    if (rvId == riley::RenderViewId::InvalidId()) {
        TF_CODING_ERROR("Invalid render view provided.\n");
        return false;
    }

    // NOTE: renderSettingsPrimOptions is passed by value on purpose to let us
    // override the shutter without stomping over the prim's copy.
    cameraContext.SetRileyOptions(&renderSettingsPrimOptions);
    renderSettingsPrimOptions.SetFloatArray(
        RixStr.k_Ri_Shutter, shutter.data(), 2);

    param->SetRenderSettingsPrimOptions(renderSettingsPrimOptions);
    param->SetRileyOptions();

    const riley::RenderViewId renderViews[] = { rvId };
    
    RtParamList renderOptions;
    static RtUString const US_RENDERMODE("renderMode");
    static RtUString const US_BATCH("batch");
    static RtUString const US_INTERACTIVE = RtUString("interactive");

    renderOptions.SetString(
        US_RENDERMODE, interactive? US_INTERACTIVE : US_BATCH);
    
    param->AcquireRiley()->Render(
        {static_cast<uint32_t>(TfArraySize(renderViews)), renderViews},
        renderOptions);

    return true;
}

} // end anonymous namespace


HdPrman_RenderSettings::HdPrman_RenderSettings(SdfPath const& id)
    : HdRenderSettings(id)
{
}

HdPrman_RenderSettings::~HdPrman_RenderSettings() = default;

bool
HdPrman_RenderSettings::DriveRenderPass(
    bool interactive,
    bool renderPassHasAovBindings) const
{
    // As of this writing, the scenarios where we use the render settings prim
    // to drive render pass execution are:
    // 1. In an application like usdrecord wherein the render delegate is
    //    not interactive and the render task has AOV bindings by enabling the
    //    setting HD_PRMAN_RENDER_SETTINGS_DRIVE_RENDER_PASS.
    //
    // 2. The hdPrman test harness where the task does not have AOV bindings.
    // 
    // XXX Interactive viewport rendering using hdPrman currently relies on 
    // AOV bindings from the task and uses the "hydra" Display Driver to write 
    // rendered pixels into an intermediate framebuffer which is then blit 
    // into the Hydra AOVs. Using the render settings prim to drive the render 
    // pass in an interactive viewport setting is not yet supported.
    //

    static const bool driveRenderPassWithAovBindings =
        TfGetEnvSetting(HD_PRMAN_RENDER_SETTINGS_DRIVE_RENDER_PASS);

    const bool result =
        IsValid() &&
        (driveRenderPassWithAovBindings || !renderPassHasAovBindings) &&
        !interactive;

    TF_DEBUG(HDPRMAN_RENDER_SETTINGS).Msg(
        "Drive with RenderSettingsPrim = %d\n"
        " - HD_PRMAN_RENDER_SETTINGS_DRIVE_RENDER_PASS = %d\n"
        " - valid = %d\n"
        " - interactive renderDelegate %d\n",
        result, driveRenderPassWithAovBindings, IsValid(), interactive);

    return result;
}

bool
HdPrman_RenderSettings::UpdateAndRender(
    const HdRenderIndex *renderIndex,
    bool interactive,
    HdPrman_RenderParam *param)
{
    TF_DEBUG(HDPRMAN_RENDER_PASS).Msg(
        "UpdateAndRender called for render settings prim %s\n.",
        GetId().GetText());

    if (!IsValid()) {
        TF_CODING_ERROR(
            "Render settings prim %s does not have valid render products.\n",
            GetId().GetText());
        return false;
    }
    if (interactive) {
        TF_CODING_ERROR(
            "Support for driving interactive renders using a render settings "
            "prim is not yet available.\n");
        return false;
    }

    bool success = true;

    const size_t numProducts = GetRenderProducts().size();

    // The camera and render view contexts are currently managed by render
    // param. We have only one instance of each, so we process the products
    // sequentially, updating the riley resources associated with each of the
    // contexts, prior to invoking _Render. This isn't a big concern in
    // non-interactive rendering, but will be for an interactive usage.
    //
    // We can avoid thrashing the Riley resources by managing a camera context
    // and render view context per-product.
    //
    HdPrman_CameraContext &cameraContext = param->GetCameraContext();
    HdPrman_RenderViewContext &renderViewContext =
        param->GetRenderViewContext();

    for (size_t prodIdx = 0; prodIdx < numProducts; prodIdx++) {
        auto const &product = GetRenderProducts().at(prodIdx);

        if (product.renderVars.empty()) {
            TF_WARN("--- Skipping empty render product %s ...\n",
                    product.name.GetText());
            continue;
        }

        TF_DEBUG(HDPRMAN_RENDER_PASS).Msg(
            "--- Processing render product %s ...\n", product.name.GetText()); 

        // XXX This can be moved to _Sync once we have a camera context
        //     per-product.
        _UpdateCameraContextFromProduct(product, &cameraContext);

        // This _cannot_ be moved to Sync since the camera Sprim wouldn't have
        // been updated.
        _UpdateRileyCamera(
            param->AcquireRiley(),
            renderIndex,
            product.cameraPath,
            &cameraContext);

        const GfVec2f shutter =
            _ResolveShutterInterval(product, cameraContext, renderIndex);

        // This _cannot_ be moved to Sync either because the render terminal
        // Sprims wouldn't have been updated.

        if (TfGetEnvSetting(HD_PRMAN_RENDER_SETTINGS_BUNDLE_RENDER_PRODUCTS)) {
            _UpdateRenderViewContext(
                GetRenderProducts(), param, &renderViewContext);
        } else {
            _UpdateRenderViewContext(
                {product}, param, &renderViewContext);
        }
        
        const bool result =
            _SetOptionsAndRender(
                cameraContext,
                renderViewContext,
                _settingsOptions,
                shutter,
                interactive,
                param);
        
        if (TfDebug::IsEnabled(HDPRMAN_RENDER_PASS)) {
            if (result) {
                TfDebug::Helper().Msg(
                    "--- Rendered product %s.\n", product.name.GetText()); 
            } else {
                TfDebug::Helper().Msg(
                    "!!! Did not render product %s.\n", product.name.GetText()); 
            }
        }

        success = success && result;

        if (TfGetEnvSetting(HD_PRMAN_RENDER_SETTINGS_BUNDLE_RENDER_PRODUCTS)) {
            // Done.
            break;
        }
    }

    return success;
}

void HdPrman_RenderSettings::Finalize(HdRenderParam *renderParam)
{
    HdPrman_RenderParam *param = static_cast<HdPrman_RenderParam*>(renderParam);
    if (param->GetDrivingRenderSettingsPrimPath() == GetId()) {
        // Could set it to the fallback, but it isn't well-formed as of this
        // writing and serves only to set composed scene options.
        // i.e.
        // if (HdPrmanRenderParam::HasSceneIndexPlugin(
        //         HdPrmanPluginTokens->renderSettings)) {
        //     renderParam->SetDrivingRenderSettingsPrimPath(
        //         HdsiRenderSettingsFilteringSceneIndex::GetFallbackPrimPath());
        // }

        // For now, just reset to an empty path.
        param->SetDrivingRenderSettingsPrimPath(SdfPath::EmptyPath());

        // XXX
        // Once management of contexts is moved local to the prim, this should
        // be updated to destroy associated riley resources.
    }
}

void HdPrman_RenderSettings::_Sync(
    HdSceneDelegate *sceneDelegate,
    HdRenderParam *renderParam,
    const HdDirtyBits *dirtyBits)
{
    TF_DEBUG(HDPRMAN_RENDER_SETTINGS).Msg(
        "Syncing render settings prim %s (dirty bits = %x)...\n{",
        GetId().GetText(), *dirtyBits);
        
    HdPrman_RenderParam *param = static_cast<HdPrman_RenderParam*>(renderParam);

    HdSceneIndexBaseRefPtr terminalSi =
        sceneDelegate->GetRenderIndex().GetTerminalSceneIndex();

    // We defer the first SetOptions call to correctly handle immutable scene 
    // options authored on a render settings prim to below (SetRileyOptions).
    // To accommodate scenes without a render settings prim, a fallback
    // prim is always inserted via a scene index plugin.
    // However, due to the non-deterministic nature of Sync, we need to guard
    // against the fallback prim's opinion being committed on the first
    // SetOptions when an authored prim is present.
    //
#if PXR_VERSION >= 2311
    if (GetId() == HdsiRenderSettingsFilteringSceneIndex::GetFallbackPrimPath()
#else
    if (GetId() == SdfPath(_legacyTokens->fallbackPath)
#endif
        && _HasNonFallbackRenderSettingsPrim(terminalSi)) {

        TF_DEBUG(HDPRMAN_RENDER_SETTINGS).Msg(
            "Short-circuiting sync for fallback render settings prim %s because"
            "an authored render setting prim is present.\n", GetId().GetText());
        
        return;
    }

    if (*dirtyBits & HdRenderSettings::DirtyNamespacedSettings) {
        // Note: We don't get fine-grained invalidation per-setting, so we
        //       recompute all settings. Since this resets the param list, we
        //       re-add the shutter interval param explicitly below.
        _settingsOptions = _GenerateParamList(GetNamespacedSettings());
    }

#if PXR_VERSION >= 2311
    if (*dirtyBits & HdRenderSettings::DirtyShutterInterval ||
        *dirtyBits & HdRenderSettings::DirtyNamespacedSettings) {
        if (GetShutterInterval().IsHolding<GfVec2d>()) {
            HdPrman_Utils::SetParamFromVtValue(
                RixStr.k_Ri_Shutter,
                GetShutterInterval(),
                /* role = */ TfToken(),
                &_settingsOptions);
        }
    }
#endif

#if PXR_VERSION >= 2405
    if (*dirtyBits & HdRenderSettings::DirtyFrameNumber ||
        *dirtyBits & HdRenderSettings::DirtyNamespacedSettings) {
        _UpdateFrame(terminalSi, &_settingsOptions);
    }
#endif

    // XXX Preserve existing data flow for clients that don't populate the
    //     sceneGlobals.activeRenderSettingsPrim locator at the root prim of the
    //     scene index. In this scenario, scene options and render terminals
    //     connected to the render settings prim are used. This works
    //     only when a single render settings prim is present in the scene
    //     (not including the fallback prim inserted via the scene index).
    //
    //     When multiple render settings prims are present in the scene, because
    //     the Sync order is non-deterministic, the last sync'd prim's mutable
    //     opinions and the first sync'd prim's immutable opinions would win.
    //
    const bool hasActiveRsp = HdUtils::HasActiveRenderSettingsPrim(terminalSi);
    
    if (IsActive() || !hasActiveRsp) {

        param->SetDrivingRenderSettingsPrimPath(GetId());

#if PXR_VERSION >= 2405
        if (*dirtyBits & HdRenderSettings::DirtyNamespacedSettings ||
            *dirtyBits & HdRenderSettings::DirtyActive ||
            *dirtyBits & HdRenderSettings::DirtyShutterInterval ||
            *dirtyBits & HdRenderSettings::DirtyFrameNumber) {
#elif PXR_VERSION >= 2311
        if (*dirtyBits & HdRenderSettings::DirtyNamespacedSettings ||
            *dirtyBits & HdRenderSettings::DirtyActive ||
            *dirtyBits & HdRenderSettings::DirtyShutterInterval) {
#else
        if (*dirtyBits & HdRenderSettings::DirtyNamespacedSettings ||
            *dirtyBits & HdRenderSettings::DirtyActive) {
#endif
            
            // Handle attributes ...
            param->SetRenderSettingsPrimOptions(_settingsOptions);
            param->SetRileyOptions();
        }

        // ... and connections.
        if (*dirtyBits & HdRenderSettings::DirtyNamespacedSettings ||
            *dirtyBits & HdRenderSettings::DirtyActive) {

            _ProcessRenderTerminals(sceneDelegate, param);
        }

        if (*dirtyBits & HdRenderSettings::DirtyRenderProducts) {
            _ProcessRenderProducts(param);
        }
    }

    TF_DEBUG(HDPRMAN_RENDER_SETTINGS).Msg(
        "}\nDone syncing render settings prim %s.\n", GetId().GetText());
        
}

void
HdPrman_RenderSettings::_ProcessRenderTerminals(
    HdSceneDelegate *sceneDelegate,
    HdPrman_RenderParam *param)
{
    const VtDictionary& namespacedSettings = GetNamespacedSettings();

    // Set the integrator connected to this Render Settings prim
    {
        // XXX Should use SdfPath rather than a vector.
        const SdfPathVector paths = VtDictionaryGet<SdfPathVector>(
            namespacedSettings,
            _renderTerminalTokens->outputsRiIntegrator.GetString(),
            VtDefault = SdfPathVector());

        param->SetRenderSettingsIntegratorPath(sceneDelegate,
            paths.empty()? SdfPath::EmptyPath() : paths.front());
    }

    // Set the SampleFilters connected to this Render Settings prim
    {
        const SdfPathVector paths = VtDictionaryGet<SdfPathVector>(
            namespacedSettings,
            _renderTerminalTokens->outputsRiSampleFilters.GetString(),
            VtDefault = SdfPathVector());

        param->SetConnectedSampleFilterPaths(sceneDelegate, paths);
    }

    // Set the DisplayFilters connected to this Render Settings prim
    {
        const SdfPathVector paths = VtDictionaryGet<SdfPathVector>(
            namespacedSettings,
            _renderTerminalTokens->outputsRiDisplayFilters.GetString(),
            VtDefault = SdfPathVector());

        param->SetConnectedDisplayFilterPaths(sceneDelegate, paths);
    }
}

void
HdPrman_RenderSettings::_ProcessRenderProducts(HdPrman_RenderParam *param)
{
    if (GetRenderProducts().empty()) {
        return;
    }
    // Fallback path for apps using an older version of Hydra wherein 
    // the computed "unioned shutter interval" on the render settings 
    // prim via HdsiRenderSettingsFilteringSceneIndex is not available.
    // In this scenario, the *legacy* scene options param list is updated
    // with the camera shutter interval of the first render product
    // during HdPrmanCamera::Sync. The riley shutter interval needs to
    // be set before any time-sampled primvars are synced.
    // 
#if PXR_VERSION >= 2311
    if (GetShutterInterval().IsEmpty()) {
#else
    {
#endif
        // Set the camera path here so that HdPrmanCamera::Sync can detect
        // whether it is syncing the current camera to set the riley shutter
        // interval. See SetRileyShutterIntervalFromCameraContextCameraPath
        // for additional context.
        const SdfPath &cameraPath = GetRenderProducts().at(0).cameraPath;
        param->GetCameraContext().SetCameraPath(cameraPath);
    }

#if HD_API_VERSION >= 64
    // This will override the f-stop value on the camera
    param->GetCameraContext().SetDisableDepthOfField(
        GetRenderProducts().at(0).disableDepthOfField);
#endif
}

#if PXR_VERSION <= 2308
bool
HdPrman_RenderSettings::IsValid() const
{
    return (!GetRenderProducts().empty() &&
            !GetRenderProducts()[0].cameraPath.IsEmpty());
}
#endif

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_VERSION >= 2308
