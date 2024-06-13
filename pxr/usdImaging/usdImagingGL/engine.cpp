//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdImaging/usdImagingGL/engine.h"

#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/selectionSceneIndex.h"
#include "pxr/usdImaging/usdImaging/stageSceneIndex.h"
#include "pxr/usdImaging/usdImaging/rootOverridesSceneIndex.h"
#include "pxr/usdImaging/usdImaging/sceneIndices.h"

#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/usd/usdGeom/camera.h"
#include "pxr/usd/usdRender/tokens.h"
#include "pxr/usd/usdRender/settings.h"

#include "pxr/imaging/hd/materialBindingsSchema.h"
#include "pxr/imaging/hd/light.h"
#include "pxr/imaging/hd/rendererPlugin.h"
#include "pxr/imaging/hd/rendererPluginRegistry.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/systemMessages.h"
#include "pxr/imaging/hd/utils.h"
#include "pxr/imaging/hdsi/primTypePruningSceneIndex.h"
#include "pxr/imaging/hdsi/legacyDisplayStyleOverrideSceneIndex.h"
#include "pxr/imaging/hdsi/sceneGlobalsSceneIndex.h"
#include "pxr/imaging/hdx/pickTask.h"
#include "pxr/imaging/hdx/taskController.h"
#include "pxr/imaging/hdx/tokens.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/stl.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec3d.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(USDIMAGINGGL_ENGINE_DEBUG_SCENE_DELEGATE_ID, "/",
                      "Default usdImaging scene delegate id");

TF_DEFINE_ENV_SETTING(USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX, false,
                      "Use Scene Index API for imaging scene input");

namespace UsdImagingGLEngine_Impl
{

// Struct that holds application scene indices created via the
// scene index plugin registration callback facility.
struct _AppSceneIndices {
    HdsiSceneGlobalsSceneIndexRefPtr sceneGlobalsSceneIndex;
};

};

namespace {

// Use a static tracker to accommodate the use-case where an application spawns
// multiple engines.
using _RenderInstanceAppSceneIndicesTracker =
    HdUtils::RenderInstanceTracker<UsdImagingGLEngine_Impl::_AppSceneIndices>;
TfStaticData<_RenderInstanceAppSceneIndicesTracker>
    s_renderInstanceTracker;

// ----------------------------------------------------------------------------

SdfPath const&
_GetUsdImagingDelegateId()
{
    static SdfPath const delegateId =
        SdfPath(TfGetEnvSetting(USDIMAGINGGL_ENGINE_DEBUG_SCENE_DELEGATE_ID));

    return delegateId;
}

bool
_GetUseSceneIndices()
{
    // Use UsdImagingStageSceneIndex for input if:
    // - USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX is true (feature flag)
    // - HdRenderIndex has scene index emulation enabled (otherwise,
    //     AddInputScene won't work).
    static bool useSceneIndices =
        HdRenderIndex::IsSceneIndexEmulationEnabled() &&
        (TfGetEnvSetting(USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX) == true);

    return useSceneIndices;
}

std::string
_GetPlatformDependentRendererDisplayName(HfPluginDesc const &pluginDescriptor)
{
#if defined(__APPLE__)
    // Rendering for Storm is delegated to Hgi. We override the
    // display name for macOS since the Hgi implementation for
    // macOS uses Metal instead of GL. Eventually, this should
    // properly delegate to using Hgi to determine the display
    // name for Storm.
    static const TfToken _stormRendererPluginName("HdStormRendererPlugin");
    if (pluginDescriptor.id == _stormRendererPluginName) {
        return "Metal";
    }
#endif
    return pluginDescriptor.displayName;
}

} // anonymous namespace

//----------------------------------------------------------------------------
// Construction
//----------------------------------------------------------------------------

UsdImagingGLEngine::UsdImagingGLEngine(
    const Parameters &params)
  : UsdImagingGLEngine(
      params.rootPath,
      params.excludedPaths,
      params.invisedPaths,
      params.sceneDelegateID,
      params.driver,
      params.rendererPluginId,
      params.gpuEnabled,
      params.displayUnloadedPrimsWithBounds,
      params.allowAsynchronousSceneProcessing)
{
}

UsdImagingGLEngine::UsdImagingGLEngine(
    const HdDriver& driver,
    const TfToken& rendererPluginId,
    const bool gpuEnabled)
    : UsdImagingGLEngine(SdfPath::AbsoluteRootPath(),
            {},
            {},
            _GetUsdImagingDelegateId(),
            driver,
            rendererPluginId,
            gpuEnabled)
{
}

UsdImagingGLEngine::UsdImagingGLEngine(
    const SdfPath& rootPath,
    const SdfPathVector& excludedPaths,
    const SdfPathVector& invisedPaths,
    const SdfPath& sceneDelegateID,
    const HdDriver& driver,
    const TfToken& rendererPluginId,
    const bool gpuEnabled,
    const bool displayUnloadedPrimsWithBounds,
    const bool allowAsynchronousSceneProcessing)
    : _hgi()
    , _hgiDriver(driver)
    , _displayUnloadedPrimsWithBounds(displayUnloadedPrimsWithBounds)
    , _gpuEnabled(gpuEnabled)
    , _sceneDelegateId(sceneDelegateID)
    , _selTracker(std::make_shared<HdxSelectionTracker>())
    , _selectionColor(1.0f, 1.0f, 0.0f, 1.0f)
    , _domeLightCameraVisibility(true)
    , _rootPath(rootPath)
    , _excludedPrimPaths(excludedPaths)
    , _invisedPrimPaths(invisedPaths)
    , _isPopulated(false)
    , _allowAsynchronousSceneProcessing(allowAsynchronousSceneProcessing)
{
    if (!_gpuEnabled && _hgiDriver.name == HgiTokens->renderDriver &&
        _hgiDriver.driver.IsHolding<Hgi*>()) {
        TF_WARN("Trying to share GPU resources while disabling the GPU.");
        _gpuEnabled = true;
    }

    // _renderIndex, _taskController, and _sceneDelegate/_sceneIndex
    // are initialized by the plugin system.
    if (!SetRendererPlugin(!rendererPluginId.IsEmpty() ?
            rendererPluginId : _GetDefaultRendererPluginId())) {
        TF_CODING_ERROR("No renderer plugins found!");
    }
}

void
UsdImagingGLEngine::_DestroyHydraObjects()
{
    // Destroy objects in opposite order of construction.
    _engine = nullptr;
    _taskController = nullptr;
    if (_GetUseSceneIndices()) {
        if (_renderIndex && _sceneIndex) {
            _renderIndex->RemoveSceneIndex(_sceneIndex);
            _stageSceneIndex = nullptr;
            _rootOverridesSceneIndex = nullptr;
            _selectionSceneIndex = nullptr;
            _displayStyleSceneIndex = nullptr;
            _sceneIndex = nullptr;
        }
    } else {
        _sceneDelegate = nullptr;
    }

    // Drop the reference to application scene indices so they are destroyed
    // during render index destruction.
    {
        _appSceneIndices = nullptr;
        if (_renderIndex) {
            s_renderInstanceTracker->UnregisterInstance(
                _renderIndex->GetInstanceName());
        }
    }

    _renderIndex = nullptr;
    _renderDelegate = nullptr;
}

UsdImagingGLEngine::~UsdImagingGLEngine()
{
    TF_PY_ALLOW_THREADS_IN_SCOPE();

    _DestroyHydraObjects();
}

//----------------------------------------------------------------------------
// Rendering
//----------------------------------------------------------------------------

void
UsdImagingGLEngine::PrepareBatch(
    const UsdPrim& root, 
    const UsdImagingGLRenderParams& params)
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return;
    }

    HD_TRACE_FUNCTION();

    if (_CanPrepare(root)) {
        if (!_isPopulated) {
            auto stage = root.GetStage();
            if (_GetUseSceneIndices()) {
                TF_VERIFY(_stageSceneIndex);
                _stageSceneIndex->SetStage(stage);

                // XXX(USD-7113): Add pruning based on _rootPath,
                // _excludedPrimPaths

                // XXX(USD-7114): Add draw mode support based on
                // params.enableUsdDrawModes.

                // XXX(USD-7115): Add invis overrides from _invisedPrimPaths.
            } else {
                TF_VERIFY(_sceneDelegate);
                _sceneDelegate->SetUsdDrawModesEnabled(
                    params.enableUsdDrawModes);
                _sceneDelegate->Populate(
                    stage->GetPrimAtPath(_rootPath),
                    _excludedPrimPaths);
                _sceneDelegate->SetInvisedPrimPaths(_invisedPrimPaths);

                // This is only necessary when using the legacy scene delegate.
                // The stage scene index provides this functionality.
                _SetActiveRenderSettingsPrimFromStageMetadata(stage);
            }

            _isPopulated = true;
        }

        _PreSetTime(params);

        // SetTime will only react if time actually changes.
        if (_GetUseSceneIndices()) {
            _stageSceneIndex->SetTime(params.frame);
        } else {
            _sceneDelegate->SetTime(params.frame);
        }

        _SetSceneGlobalsCurrentFrame(params.frame);
        _PostSetTime(params);
    }
}

void
UsdImagingGLEngine::_PrepareRender(const UsdImagingGLRenderParams& params)
{
    TF_VERIFY(_taskController);

    _taskController->SetFreeCameraClipPlanes(params.clipPlanes);

    TfTokenVector renderTags;
    _ComputeRenderTags(params, &renderTags);
    _taskController->SetRenderTags(renderTags);

    _taskController->SetRenderParams(
        _MakeHydraUsdImagingGLRenderParams(params));

    // Forward scene materials enable option.
    if (_GetUseSceneIndices()) {
        if (_materialPruningSceneIndex) {
            _materialPruningSceneIndex->SetEnabled(
                !params.enableSceneMaterials);
        }
        if (_lightPruningSceneIndex) {
            _lightPruningSceneIndex->SetEnabled(
                !params.enableSceneLights);
        }
    } else {
        _sceneDelegate->SetSceneMaterialsEnabled(params.enableSceneMaterials);
        _sceneDelegate->SetSceneLightsEnabled(params.enableSceneLights);
    }
}

void
UsdImagingGLEngine::_SetActiveRenderSettingsPrimFromStageMetadata(
    UsdStageWeakPtr stage)
{
    if (!TF_VERIFY(_renderIndex) || !TF_VERIFY(stage)) {
        return;
    }

    // If we already have an opinion, skip the stage metadata.
    if (!HdUtils::HasActiveRenderSettingsPrim(
            _renderIndex->GetTerminalSceneIndex())) {

        std::string pathStr;
        if (stage->HasAuthoredMetadata(
                UsdRenderTokens->renderSettingsPrimPath)) {
            stage->GetMetadata(
                UsdRenderTokens->renderSettingsPrimPath, &pathStr);
        }
        // Add the delegateId prefix since the scene globals scene index is 
        // inserted into the merging scene index.
        if (!pathStr.empty()) {
            SetActiveRenderSettingsPrimPath(
                SdfPath(pathStr).ReplacePrefix(
                    SdfPath::AbsoluteRootPath(), _sceneDelegateId));
        }
    }
}

void
UsdImagingGLEngine::_UpdateDomeLightCameraVisibility()
{
    if (!_renderIndex->IsSprimTypeSupported(HdPrimTypeTokens->domeLight)) {
        return;
    }

    // Check to see if the dome light camera visibility has changed, and mark
    // the dome light prim as dirty if it has.
    //
    // Note: The dome light camera visibility setting is handled via the
    // HdRenderSettingsMap on the HdRenderDelegate because this ensures all
    // backends can access this setting when they need to.

    // The absence of a setting in the map is the same as camera visibility
    // being on.
    const bool domeLightCamVisSetting = _renderDelegate->
        GetRenderSetting<bool>(
            HdRenderSettingsTokens->domeLightCameraVisibility,
            true);
    if (_domeLightCameraVisibility != domeLightCamVisSetting) {
        // Camera visibility state changed, so we need to mark any dome lights
        // as dirty to ensure they have the proper state on all backends.
        _domeLightCameraVisibility = domeLightCamVisSetting;

        SdfPathVector domeLights = _renderIndex->GetSprimSubtree(
            HdPrimTypeTokens->domeLight, SdfPath::AbsoluteRootPath());
        for (SdfPathVector::iterator domeLightIt = domeLights.begin();
                                     domeLightIt != domeLights.end();
                                     ++domeLightIt) {
            _renderIndex->GetChangeTracker().MarkSprimDirty(
                *domeLightIt, HdLight::DirtyParams);
        }
    }
}

void
UsdImagingGLEngine::_SetBBoxParams(
    const BBoxVector& bboxes,
    const GfVec4f& bboxLineColor,
    float bboxLineDashSize)
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return;
    }

    HdxBoundingBoxTaskParams params;
    params.bboxes = bboxes;
    params.color = bboxLineColor;
    params.dashSize = bboxLineDashSize;

    _taskController->SetBBoxParams(params);
}

void
UsdImagingGLEngine::RenderBatch(
    const SdfPathVector& paths, 
    const UsdImagingGLRenderParams& params)
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return;
    }

    _UpdateHydraCollection(&_renderCollection, paths, params);
    _taskController->SetCollection(_renderCollection);

    _PrepareRender(params);

    SetColorCorrectionSettings(params.colorCorrectionMode, params.ocioDisplay,
        params.ocioView, params.ocioColorSpace, params.ocioLook);

    _SetBBoxParams(params.bboxes, params.bboxLineColor, params.bboxLineDashSize);

    // XXX App sets the clear color via 'params' instead of setting up Aovs 
    // that has clearColor in their descriptor. So for now we must pass this
    // clear color to the color AOV.
    HdAovDescriptor colorAovDesc = 
        _taskController->GetRenderOutputSettings(HdAovTokens->color);
    if (colorAovDesc.format != HdFormatInvalid) {
        colorAovDesc.clearValue = VtValue(params.clearColor);
        _taskController->SetRenderOutputSettings(
            HdAovTokens->color, colorAovDesc);
    }

    _taskController->SetEnableSelection(params.highlight);
    VtValue selectionValue(_selTracker);
    _engine->SetTaskContextData(HdxTokens->selectionState, selectionValue);

    _UpdateDomeLightCameraVisibility();

    _Execute(params, _taskController->GetRenderingTasks());
}

void 
UsdImagingGLEngine::Render(
    const UsdPrim& root, 
    const UsdImagingGLRenderParams &params)
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    PrepareBatch(root, params);

    // XXX(UsdImagingPaths): This bit is weird: we get the stage from "root",
    // gate population by _rootPath (which may be different), and then pass
    // root.GetPath() to hydra as the root to draw from. Note that this
    // produces incorrect results in UsdImagingDelegate for native instancing.
    const SdfPathVector paths = {
        root.GetPath().ReplacePrefix(
            SdfPath::AbsoluteRootPath(), _sceneDelegateId)
    };

    RenderBatch(paths, params);
}

bool
UsdImagingGLEngine::IsConverged() const
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return true;
    }

    return _taskController->IsConverged();
}

//----------------------------------------------------------------------------
// Root and Transform Visibility
//----------------------------------------------------------------------------

void
UsdImagingGLEngine::SetRootTransform(GfMatrix4d const& xf)
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return;
    }

    if (_GetUseSceneIndices()) {
        _rootOverridesSceneIndex->SetRootTransform(xf);
    } else {
        _sceneDelegate->SetRootTransform(xf);
    }
}

void
UsdImagingGLEngine::SetRootVisibility(const bool isVisible)
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return;
    }

    if (_GetUseSceneIndices()) {
        _rootOverridesSceneIndex->SetRootVisibility(isVisible);
    } else {
        _sceneDelegate->SetRootVisibility(isVisible);
    }
}

//----------------------------------------------------------------------------
// Camera and Light State
//----------------------------------------------------------------------------

void
UsdImagingGLEngine::SetRenderViewport(GfVec4d const& viewport)
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return;
    }

    _taskController->SetRenderViewport(viewport);
}

void
UsdImagingGLEngine::SetFraming(CameraUtilFraming const& framing)
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return;
    }

    _taskController->SetFraming(framing);
}

void
UsdImagingGLEngine::SetOverrideWindowPolicy(
    const std::optional<CameraUtilConformWindowPolicy> &policy)
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return;
    }

    _taskController->SetOverrideWindowPolicy(policy);
}

void
UsdImagingGLEngine::SetRenderBufferSize(GfVec2i const& size)
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return;
    }

    _taskController->SetRenderBufferSize(size);
}

void
UsdImagingGLEngine::SetWindowPolicy(CameraUtilConformWindowPolicy policy)
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return;
    }

    // Note: Free cam uses SetCameraState, which expects the frustum to be
    // pre-adjusted for the viewport size.

    if (_GetUseSceneIndices()) {
        // XXX(USD-7115): window policy
    } else {
        // The usdImagingDelegate manages the window policy for scene cameras.
        _sceneDelegate->SetWindowPolicy(policy);
    }
}

void
UsdImagingGLEngine::SetCameraPath(SdfPath const& id)
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return;
    }

    _taskController->SetCameraPath(id);

    // The camera that is set for viewing will also be used for
    // time sampling.
    // XXX(HYD-2304): motion blur shutter window.
    if (!_GetUseSceneIndices()) {
        _sceneDelegate->SetCameraForSampling(id);
    }
}

void 
UsdImagingGLEngine::SetCameraState(const GfMatrix4d& viewMatrix,
                                   const GfMatrix4d& projectionMatrix)
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return;
    }

    _taskController->SetFreeCameraMatrices(viewMatrix, projectionMatrix);
}

void
UsdImagingGLEngine::SetLightingState(GlfSimpleLightingContextPtr const &src)
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return;
    }

    _taskController->SetLightingState(src);
}

void
UsdImagingGLEngine::SetLightingState(
    GlfSimpleLightVector const &lights,
    GlfSimpleMaterial const &material,
    GfVec4f const &sceneAmbient)
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return;
    }

    // we still use _lightingContextForOpenGLState for convenience, but
    // set the values directly.
    if (!_lightingContextForOpenGLState) {
        _lightingContextForOpenGLState = GlfSimpleLightingContext::New();
    }
    _lightingContextForOpenGLState->SetLights(lights);
    _lightingContextForOpenGLState->SetMaterial(material);
    _lightingContextForOpenGLState->SetSceneAmbient(sceneAmbient);
    _lightingContextForOpenGLState->SetUseLighting(lights.size() > 0);

    _taskController->SetLightingState(_lightingContextForOpenGLState);
}

//----------------------------------------------------------------------------
// Selection Highlighting
//----------------------------------------------------------------------------

void
UsdImagingGLEngine::SetSelected(SdfPathVector const& paths)
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return;
    }

    if (_GetUseSceneIndices()) {
        _selectionSceneIndex->ClearSelection();

        for (const SdfPath &path : paths) {
            _selectionSceneIndex->AddSelection(path);
        }
        return;
    }

    TF_VERIFY(_sceneDelegate);

    // populate new selection
    HdSelectionSharedPtr const selection = std::make_shared<HdSelection>();
    // XXX: Usdview currently supports selection on click. If we extend to
    // rollover (locate) selection, we need to pass that mode here.
    static const HdSelection::HighlightMode mode =
        HdSelection::HighlightModeSelect;
    for (SdfPath const& path : paths) {
        _sceneDelegate->PopulateSelection(mode,
                                          path,
                                          UsdImagingDelegate::ALL_INSTANCES,
                                          selection);
    }

    // set the result back to selection tracker
    _selTracker->SetSelection(selection);
}

void
UsdImagingGLEngine::ClearSelected()
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return;
    }

    if (_GetUseSceneIndices()) {
        _selectionSceneIndex->ClearSelection();
        return;
    }
    
    TF_VERIFY(_selTracker);

    _selTracker->SetSelection(std::make_shared<HdSelection>());
}

HdSelectionSharedPtr
UsdImagingGLEngine::_GetSelection() const
{
    if (HdSelectionSharedPtr const selection = _selTracker->GetSelectionMap()) {
        return selection;
    }

    return std::make_shared<HdSelection>();
}

void
UsdImagingGLEngine::AddSelected(SdfPath const &path, int instanceIndex)
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return;
    }

    if (_GetUseSceneIndices()) {
        _selectionSceneIndex->AddSelection(path);
        return;
    }

    TF_VERIFY(_sceneDelegate);

    HdSelectionSharedPtr const selection = _GetSelection();

    // XXX: Usdview currently supports selection on click. If we extend to
    // rollover (locate) selection, we need to pass that mode here.
    static const HdSelection::HighlightMode mode =
        HdSelection::HighlightModeSelect;
    _sceneDelegate->PopulateSelection(mode, path, instanceIndex, selection);

    // set the result back to selection tracker
    _selTracker->SetSelection(selection);
}

void
UsdImagingGLEngine::SetSelectionColor(GfVec4f const& color)
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return;
    }

    _selectionColor = color;
    _taskController->SetSelectionColor(_selectionColor);
}

//----------------------------------------------------------------------------
// Picking
//----------------------------------------------------------------------------

bool 
UsdImagingGLEngine::TestIntersection(
    const GfMatrix4d &viewMatrix,
    const GfMatrix4d &projectionMatrix,
    const UsdPrim& root,
    const UsdImagingGLRenderParams& params,
    GfVec3d *outHitPoint,
    GfVec3d *outHitNormal,
    SdfPath *outHitPrimPath,
    SdfPath *outHitInstancerPath,
    int *outHitInstanceIndex,
    HdInstancerContext *outInstancerContext)
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return false;
    }

    PrepareBatch(root, params);

    // XXX(UsdImagingPaths): This is incorrect...  "Root" points to a USD
    // subtree, but the subtree in the hydra namespace might be very different
    // (e.g. for native instancing).  We need a translation step.
    const SdfPathVector paths = {
        root.GetPath().ReplacePrefix(
            SdfPath::AbsoluteRootPath(), _sceneDelegateId)
    };
    _UpdateHydraCollection(&_intersectCollection, paths, params);

    _PrepareRender(params);

    HdxPickHitVector allHits;
    HdxPickTaskContextParams pickParams;
    pickParams.resolveMode = HdxPickTokens->resolveNearestToCenter;
    pickParams.viewMatrix = viewMatrix;
    pickParams.projectionMatrix = projectionMatrix;
    pickParams.clipPlanes = params.clipPlanes;
    pickParams.collection = _intersectCollection;
    pickParams.outHits = &allHits;
    const VtValue vtPickParams(pickParams);

    _engine->SetTaskContextData(HdxPickTokens->pickParams, vtPickParams);
    _Execute(params, _taskController->GetPickingTasks());

    // Since we are in nearest-hit mode, we expect allHits to have
    // a single point in it.
    if (allHits.size() != 1) {
        return false;
    }

    HdxPickHit &hit = allHits[0];

    if (outHitPoint) {
        *outHitPoint = hit.worldSpaceHitPoint;
    }

    if (outHitNormal) {
        *outHitNormal = hit.worldSpaceHitNormal;
    }

    if (_sceneDelegate) {
        hit.objectId = _sceneDelegate->GetScenePrimPath(
            hit.objectId, hit.instanceIndex, outInstancerContext);
        hit.instancerId = _sceneDelegate->ConvertIndexPathToCachePath(
            hit.instancerId).GetAbsoluteRootOrPrimPath();
    } else {
        const HdxPrimOriginInfo info = HdxPrimOriginInfo::FromPickHit(
            _renderIndex.get(), hit);
        hit.objectId = info.GetFullPath();
        HdInstancerContext ctx = info.ComputeInstancerContext();
        if (!ctx.empty()) {
            hit.instancerId = ctx.back().first;
            if (outInstancerContext) {
                *outInstancerContext = std::move(ctx);
            }
        }
    }

    if (outHitPrimPath) {
        *outHitPrimPath = hit.objectId;
    }
    if (outHitInstancerPath) {
        *outHitInstancerPath = hit.instancerId;
    }
    if (outHitInstanceIndex) {
        *outHitInstanceIndex = hit.instanceIndex;
    }

    return true;
}

bool
UsdImagingGLEngine::DecodeIntersection(
    unsigned char const primIdColor[4],
    unsigned char const instanceIdColor[4],
    SdfPath *outHitPrimPath,
    SdfPath *outHitInstancerPath,
    int *outHitInstanceIndex,
    HdInstancerContext *outInstancerContext)
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return false;
    }

    if (_GetUseSceneIndices()) {
        // XXX(HYD-2299): picking
        return false;
    }

    TF_VERIFY(_sceneDelegate);

    const int primId = HdxPickTask::DecodeIDRenderColor(primIdColor);
    const int instanceIdx = HdxPickTask::DecodeIDRenderColor(instanceIdColor);
    SdfPath primPath =
        _sceneDelegate->GetRenderIndex().GetRprimPathFromPrimId(primId);

    if (primPath.IsEmpty()) {
        return false;
    }

    SdfPath delegateId, instancerId;
    _sceneDelegate->GetRenderIndex().GetSceneDelegateAndInstancerIds(
        primPath, &delegateId, &instancerId);

    primPath = _sceneDelegate->GetScenePrimPath(
        primPath, instanceIdx, outInstancerContext);
    instancerId = _sceneDelegate->ConvertIndexPathToCachePath(instancerId)
                  .GetAbsoluteRootOrPrimPath();

    if (outHitPrimPath) {
        *outHitPrimPath = primPath;
    }
    if (outHitInstancerPath) {
        *outHitInstancerPath = instancerId;
    }
    if (outHitInstanceIndex) {
        *outHitInstanceIndex = instanceIdx;
    }

    return true;
}

//----------------------------------------------------------------------------
// Renderer Plugin Management
//----------------------------------------------------------------------------

/* static */
TfTokenVector
UsdImagingGLEngine::GetRendererPlugins()
{
    HfPluginDescVector pluginDescriptors;
    HdRendererPluginRegistry::GetInstance().GetPluginDescs(&pluginDescriptors);

    TfTokenVector plugins;
    for(size_t i = 0; i < pluginDescriptors.size(); ++i) {
        plugins.push_back(pluginDescriptors[i].id);
    }
    return plugins;
}

/* static */
std::string
UsdImagingGLEngine::GetRendererDisplayName(TfToken const &id)
{
    HfPluginDesc pluginDescriptor;
    bool foundPlugin = HdRendererPluginRegistry::GetInstance().
        GetPluginDesc(id, &pluginDescriptor);

    if (!foundPlugin) {
        return std::string();
    }

    return _GetPlatformDependentRendererDisplayName(pluginDescriptor);
}

bool
UsdImagingGLEngine::GetGPUEnabled() const
{
    return _gpuEnabled;
}

TfToken
UsdImagingGLEngine::GetCurrentRendererId() const
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return TfToken();
    }

    return _renderDelegate.GetPluginId();
}

void
UsdImagingGLEngine::_InitializeHgiIfNecessary()
{
    // If the client of UsdImagingGLEngine does not provide a HdDriver, we
    // construct a default one that is owned by UsdImagingGLEngine.
    // The cleanest pattern is for the client app to provide this since you
    // may have multiple UsdImagingGLEngines in one app that ideally all use
    // the same HdDriver and Hgi to share GPU resources.
    if (_gpuEnabled && _hgiDriver.driver.IsEmpty()) {
        _hgi = Hgi::CreatePlatformDefaultHgi();
        _hgiDriver.name = HgiTokens->renderDriver;
        _hgiDriver.driver = VtValue(_hgi.get());
    }
}

bool
UsdImagingGLEngine::SetRendererPlugin(TfToken const &id)
{
    _InitializeHgiIfNecessary();

    HdRendererPluginRegistry &registry =
        HdRendererPluginRegistry::GetInstance();

    TfToken resolvedId;
    if (id.IsEmpty()) {
        // Special case: id == TfToken() selects the first supported plugin in
        // the list.
        resolvedId = registry.GetDefaultPluginId(_gpuEnabled);
    } else {
        HdRendererPluginHandle plugin = registry.GetOrCreateRendererPlugin(id);
        if (plugin && plugin->IsSupported(_gpuEnabled)) {
            resolvedId = id;
        } else {
            TF_CODING_ERROR("Invalid plugin id or plugin is unsupported: %s",
                            id.GetText());
            return false;
        }
    }

    if (_renderDelegate && _renderDelegate.GetPluginId() == resolvedId) {
        return true;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    HdPluginRenderDelegateUniqueHandle renderDelegate =
        registry.CreateRenderDelegate(resolvedId);
    if (!renderDelegate) {
        return false;
    }

    _SetRenderDelegateAndRestoreState(std::move(renderDelegate));

    return true;
}

void
UsdImagingGLEngine::_SetRenderDelegateAndRestoreState(
    HdPluginRenderDelegateUniqueHandle &&renderDelegate)
{
    // Pull old scene/task controller state. Note that the scene index/delegate
    // may not have been created, if this is the first time through this
    // function, so we guard for null and use default values for xform/vis.
    GfMatrix4d rootTransform = GfMatrix4d(1.0);
    bool rootVisibility = true;

    if (_GetUseSceneIndices()) {
        if (_rootOverridesSceneIndex) {
            rootTransform = _rootOverridesSceneIndex->GetRootTransform();
            rootVisibility = _rootOverridesSceneIndex->GetRootVisibility();
        }
    } else {
        if (_sceneDelegate) {
            rootTransform = _sceneDelegate->GetRootTransform();
            rootVisibility = _sceneDelegate->GetRootVisibility();
        }
    }

    HdSelectionSharedPtr const selection = _GetSelection();

    // Rebuild the imaging stack
    _SetRenderDelegate(std::move(renderDelegate));

    // Reload saved state.
    if (_GetUseSceneIndices()) {
        _rootOverridesSceneIndex->SetRootTransform(rootTransform);
        _rootOverridesSceneIndex->SetRootVisibility(rootVisibility);
    } else {
        _sceneDelegate->SetRootTransform(rootTransform);
        _sceneDelegate->SetRootVisibility(rootVisibility);
    }
    _selTracker->SetSelection(selection);
    _taskController->SetSelectionColor(_selectionColor);
}

SdfPath
UsdImagingGLEngine::_ComputeControllerPath(
    const HdPluginRenderDelegateUniqueHandle &renderDelegate)
{
    const std::string pluginId =
        TfMakeValidIdentifier(renderDelegate.GetPluginId().GetText());
    const TfToken rendererName(
        TfStringPrintf("_UsdImaging_%s_%p", pluginId.c_str(), this));

    return _sceneDelegateId.AppendChild(rendererName);
}

void
UsdImagingGLEngine::_RegisterApplicationSceneIndices()
{
    // SGSI
    {
        // Insert earlier so downstream scene indices can query and be notified
        // of changes and also declare their dependencies (e.g., to support
        // rendering color spaces).
        const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 0;

        // Note:
        // The pattern used below registers the static member fn as a callback,
        // which retreives the scene index instance using the
        // renderInstanceId argument of the callback.

        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            std::string(), // empty string implies all renderers
            &UsdImagingGLEngine::_AppendSceneGlobalsSceneIndexCallback,
            /* inputArgs = */ nullptr,
            insertionPhase,
            HdSceneIndexPluginRegistry::InsertionOrderAtStart
        );
    }
}

/* static */
HdSceneIndexBaseRefPtr
UsdImagingGLEngine::_AppendSceneGlobalsSceneIndexCallback(
        const std::string &renderInstanceId,
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs)
{
    UsdImagingGLEngine_Impl::_AppSceneIndicesSharedPtr appSceneIndices =
        s_renderInstanceTracker->GetInstance(renderInstanceId);
    
    if (appSceneIndices) {
        auto &sgsi = appSceneIndices->sceneGlobalsSceneIndex;
        sgsi = HdsiSceneGlobalsSceneIndex::New(inputScene);
        sgsi->SetDisplayName("Scene Globals Scene Index");
        return sgsi;
    }

    TF_CODING_ERROR("Did not find appSceneIndices instance for %s,",
                    renderInstanceId.c_str());
    return inputScene;
}

HdSceneIndexBaseRefPtr
UsdImagingGLEngine::_AppendOverridesSceneIndices(
    HdSceneIndexBaseRefPtr const &inputScene)
{
    HdSceneIndexBaseRefPtr sceneIndex = inputScene;

    static HdContainerDataSourceHandle const materialPruningInputArgs =
        HdRetainedContainerDataSource::New(
            HdsiPrimTypePruningSceneIndexTokens->primTypes,
            HdRetainedTypedSampledDataSource<TfTokenVector>::New(
                { HdPrimTypeTokens->material }),
            HdsiPrimTypePruningSceneIndexTokens->bindingToken,
            HdRetainedTypedSampledDataSource<TfToken>::New(
                HdMaterialBindingsSchema::GetSchemaToken()));

    // Prune scene materials prior to flattening inherited
    // materials bindings and resolving material bindings
    sceneIndex = _materialPruningSceneIndex =
        HdsiPrimTypePruningSceneIndex::New(
            sceneIndex, materialPruningInputArgs);

    static HdContainerDataSourceHandle const lightPruningInputArgs =
        HdRetainedContainerDataSource::New(
            HdsiPrimTypePruningSceneIndexTokens->primTypes,
            HdRetainedTypedSampledDataSource<TfTokenVector>::New(
                HdLightPrimTypeTokens()),
            HdsiPrimTypePruningSceneIndexTokens->doNotPruneNonPrimPaths,
            HdRetainedTypedSampledDataSource<bool>::New(
                false));

    sceneIndex = _lightPruningSceneIndex =
        HdsiPrimTypePruningSceneIndex::New(
            sceneIndex, lightPruningInputArgs);

    sceneIndex = _rootOverridesSceneIndex =
        UsdImagingRootOverridesSceneIndex::New(sceneIndex);

    return sceneIndex;
}

void
UsdImagingGLEngine::_SetRenderDelegate(
    HdPluginRenderDelegateUniqueHandle &&renderDelegate)
{
    // This relies on SetRendererPlugin to release the GIL...

    // Destruction
    _DestroyHydraObjects();

    _isPopulated = false;

    // Use the render delegate ptr (rather than 'this' ptr) for generating
    // the unique id.
    const std::string renderInstanceId =
        TfStringPrintf("UsdImagingGLEngine_%s_%p",
            renderDelegate.GetPluginId().GetText(),
            (void *) renderDelegate.Get());

    // Application scene index callback registration and
    // engine-renderInstanceId tracking.
    {
        // Register application managed scene indices via the callback
        // facility which will be invoked during render index construction.
        static std::once_flag registerOnce;
        std::call_once(registerOnce, _RegisterApplicationSceneIndices);

        _appSceneIndices =
            std::make_shared<UsdImagingGLEngine_Impl::_AppSceneIndices>();

        // Register the app scene indices with the render instance id
        // that is provided to the render index constructor below.
        s_renderInstanceTracker->RegisterInstance(
            renderInstanceId, _appSceneIndices);
    }

    // Creation
    // Use the new render delegate.
    _renderDelegate = std::move(renderDelegate);

    // Recreate the render index
    _renderIndex.reset(
        HdRenderIndex::New(
            _renderDelegate.Get(), {&_hgiDriver}, renderInstanceId));

    if (_GetUseSceneIndices()) {
        UsdImagingCreateSceneIndicesInfo info;
        info.displayUnloadedPrimsWithBounds = _displayUnloadedPrimsWithBounds;
        info.overridesSceneIndexCallback =
            std::bind(
                &UsdImagingGLEngine::_AppendOverridesSceneIndices,
                this, std::placeholders::_1);

        const UsdImagingSceneIndices sceneIndices =
            UsdImagingCreateSceneIndices(info);
        
        _stageSceneIndex = sceneIndices.stageSceneIndex;
        _selectionSceneIndex = sceneIndices.selectionSceneIndex;
        _sceneIndex = sceneIndices.finalSceneIndex;

        _sceneIndex = _displayStyleSceneIndex =
            HdsiLegacyDisplayStyleOverrideSceneIndex::New(_sceneIndex);

        _renderIndex->InsertSceneIndex(_sceneIndex, _sceneDelegateId);
    } else {
        _sceneDelegate = std::make_unique<UsdImagingDelegate>(
                _renderIndex.get(), _sceneDelegateId);

        _sceneDelegate->SetDisplayUnloadedPrimsWithBounds(
            _displayUnloadedPrimsWithBounds);
    }

    if (_allowAsynchronousSceneProcessing) {
        if (HdSceneIndexBaseRefPtr si = _renderIndex->GetTerminalSceneIndex()) {
            si->SystemMessage(HdSystemMessageTokens->asyncAllow, nullptr);
        }
    }

    _taskController = std::make_unique<HdxTaskController>(
        _renderIndex.get(),
        _ComputeControllerPath(_renderDelegate),
        _gpuEnabled);

    // The task context holds on to resources in the render
    // deletegate, so we want to destroy it first and thus
    // create it last.
    _engine = std::make_unique<HdEngine>();
}

//----------------------------------------------------------------------------
// AOVs and Renderer Settings
//----------------------------------------------------------------------------

TfTokenVector
UsdImagingGLEngine::GetRendererAovs() const
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return {};
    }

    if (_renderIndex->IsBprimTypeSupported(HdPrimTypeTokens->renderBuffer)) {

        static const TfToken candidates[] =
            { HdAovTokens->primId,
              HdAovTokens->depth,
              HdAovTokens->normal,
              HdAovTokensMakePrimvar(TfToken("st")) };

        TfTokenVector aovs = { HdAovTokens->color };
        for (auto const& aov : candidates) {
            if (_renderDelegate->GetDefaultAovDescriptor(aov).format 
                    != HdFormatInvalid) {
                aovs.push_back(aov);
            }
        }
        return aovs;
    }
    return TfTokenVector();
}

bool
UsdImagingGLEngine::SetRendererAov(TfToken const &id)
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return false;
    }

    if (_renderIndex->IsBprimTypeSupported(HdPrimTypeTokens->renderBuffer)) {
        _taskController->SetRenderOutputs({id});
        return true;
    }
    return false;
}

HgiTextureHandle
UsdImagingGLEngine::GetAovTexture(
    TfToken const& name) const
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return HgiTextureHandle();
    }

    VtValue aov;
    HgiTextureHandle aovTexture;

    if (_engine->GetTaskContextData(name, &aov)) {
        if (aov.IsHolding<HgiTextureHandle>()) {
            aovTexture = aov.Get<HgiTextureHandle>();
        }
    }

    return aovTexture;
}

HdRenderBuffer*
UsdImagingGLEngine::GetAovRenderBuffer(TfToken const& name) const
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return nullptr;
    }

    return _taskController->GetRenderOutput(name);
}

UsdImagingGLRendererSettingsList
UsdImagingGLEngine::GetRendererSettingsList() const
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return {};
    }

    const HdRenderSettingDescriptorList descriptors =
        _renderDelegate->GetRenderSettingDescriptors();
    UsdImagingGLRendererSettingsList ret;

    for (auto const& desc : descriptors) {
        UsdImagingGLRendererSetting r;
        r.key = desc.key;
        r.name = desc.name;
        r.defValue = desc.defaultValue;

        // Use the type of the default value to tell us what kind of
        // widget to create...
        if (r.defValue.IsHolding<bool>()) {
            r.type = UsdImagingGLRendererSetting::TYPE_FLAG;
        } else if (r.defValue.IsHolding<int>() ||
                   r.defValue.IsHolding<unsigned int>()) {
            r.type = UsdImagingGLRendererSetting::TYPE_INT;
        } else if (r.defValue.IsHolding<float>()) {
            r.type = UsdImagingGLRendererSetting::TYPE_FLOAT;
        } else if (r.defValue.IsHolding<std::string>()) {
            r.type = UsdImagingGLRendererSetting::TYPE_STRING;
        } else {
            TF_WARN("Setting '%s' with type '%s' doesn't have a UI"
                    " implementation...",
                    r.name.c_str(),
                    r.defValue.GetTypeName().c_str());
            continue;
        }
        ret.push_back(r);
    }

    return ret;
}

VtValue
UsdImagingGLEngine::GetRendererSetting(TfToken const& id) const
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return VtValue();
    }

    return _renderDelegate->GetRenderSetting(id);
}

void
UsdImagingGLEngine::SetRendererSetting(TfToken const& id, VtValue const& value)
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return;
    }

    _renderDelegate->SetRenderSetting(id, value);
}

void
UsdImagingGLEngine::SetActiveRenderPassPrimPath(SdfPath const &path)
{
    if (ARCH_UNLIKELY(!_appSceneIndices)) {
        return;
    }
    auto &sgsi = _appSceneIndices->sceneGlobalsSceneIndex;
    if (ARCH_UNLIKELY(!sgsi)) {
        return;
    }

    sgsi->SetActiveRenderPassPrimPath(path);
}

void
UsdImagingGLEngine::SetActiveRenderSettingsPrimPath(SdfPath const &path)
{
    if (ARCH_UNLIKELY(!_appSceneIndices)) {
        return;
    }
    auto &sgsi = _appSceneIndices->sceneGlobalsSceneIndex;
    if (ARCH_UNLIKELY(!sgsi)) {
        return;
    }

    sgsi->SetActiveRenderSettingsPrimPath(path);
}

void UsdImagingGLEngine::_SetSceneGlobalsCurrentFrame(UsdTimeCode const &time)
{
    if (ARCH_UNLIKELY(!_appSceneIndices)) {
        return;
    }
    auto &sgsi = _appSceneIndices->sceneGlobalsSceneIndex;
    if (ARCH_UNLIKELY(!sgsi)) {
        return;
    }

    sgsi->SetCurrentFrame(time.GetValue());
}

/* static */
SdfPathVector
UsdImagingGLEngine::GetAvailableRenderSettingsPrimPaths(UsdPrim const& root)
{
    // UsdRender OM uses the convention that all render settings prims must
    // live under /Render.
    static const SdfPath renderRoot("/Render");

    const auto stage = root.GetStage();

    SdfPathVector paths;
    if (UsdPrim render = stage->GetPrimAtPath(renderRoot)) {
        for (const UsdPrim child : render.GetChildren()) {
            if (child.IsA<UsdRenderSettings>()) {
                paths.push_back(child.GetPrimPath());
            }
        }
    }
    return paths;
}

void
UsdImagingGLEngine::SetEnablePresentation(bool enabled)
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return;
    }

    _taskController->SetEnablePresentation(enabled);
}

void
UsdImagingGLEngine::SetPresentationOutput(
    TfToken const &api,
    VtValue const &framebuffer)
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return;
    }

    _userFramebuffer = framebuffer;
    _taskController->SetPresentationOutput(api, framebuffer);
}

// ---------------------------------------------------------------------
// Command API
// ---------------------------------------------------------------------

HdCommandDescriptors 
UsdImagingGLEngine::GetRendererCommandDescriptors() const
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return HdCommandDescriptors();
    }

    return _renderDelegate->GetCommandDescriptors();
}

bool
UsdImagingGLEngine::InvokeRendererCommand(
    const TfToken &command, const HdCommandArgs &args) const
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return false;
    }

    return _renderDelegate->InvokeCommand(command, args);
}

// ---------------------------------------------------------------------
// Control of background rendering threads.
// ---------------------------------------------------------------------
bool
UsdImagingGLEngine::IsPauseRendererSupported() const
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return false;
    }

    return _renderDelegate->IsPauseSupported();
}

bool
UsdImagingGLEngine::PauseRenderer()
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return false;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    return _renderDelegate->Pause();
}

bool
UsdImagingGLEngine::ResumeRenderer()
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return false;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    return _renderDelegate->Resume();
}

bool
UsdImagingGLEngine::IsStopRendererSupported() const
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return false;
    }

    return _renderDelegate->IsStopSupported();
}

bool
UsdImagingGLEngine::StopRenderer()
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return false;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    return _renderDelegate->Stop();
}

bool
UsdImagingGLEngine::RestartRenderer()
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return false;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    return _renderDelegate->Restart();
}

//----------------------------------------------------------------------------
// Color Correction
//----------------------------------------------------------------------------
void 
UsdImagingGLEngine::SetColorCorrectionSettings(
    TfToken const& colorCorrectionMode,
    TfToken const& ocioDisplay,
    TfToken const& ocioView,
    TfToken const& ocioColorSpace,
    TfToken const& ocioLook)
{
    if (ARCH_UNLIKELY(!_renderDelegate) ||
        !IsColorCorrectionCapable()) {
        return;
    }

    HdxColorCorrectionTaskParams hdParams;
    hdParams.colorCorrectionMode = colorCorrectionMode;
    hdParams.displayOCIO = ocioDisplay.GetString();
    hdParams.viewOCIO = ocioView.GetString();
    hdParams.colorspaceOCIO = ocioColorSpace.GetString();
    hdParams.looksOCIO = ocioLook.GetString();
    _taskController->SetColorCorrectionParams(hdParams);
}

bool
UsdImagingGLEngine::IsColorCorrectionCapable()
{
    return true;
}

//----------------------------------------------------------------------------
// Resource Information
//----------------------------------------------------------------------------

VtDictionary
UsdImagingGLEngine::GetRenderStats() const
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return VtDictionary();
    }

    return _renderDelegate->GetRenderStats();
}

Hgi*
UsdImagingGLEngine::GetHgi()
{
    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return nullptr;
    }

    return _hgi.get();
}

//----------------------------------------------------------------------------
// Private/Protected
//----------------------------------------------------------------------------

HdRenderIndex *
UsdImagingGLEngine::_GetRenderIndex() const
{
    return _renderIndex.get();
}

void 
UsdImagingGLEngine::_Execute(const UsdImagingGLRenderParams &params,
                             HdTaskSharedPtrVector tasks)
{
    {
        // Release the GIL before calling into hydra, in case any hydra plugins
        // call into python.
        TF_PY_ALLOW_THREADS_IN_SCOPE();
        _engine->Execute(_renderIndex.get(), &tasks);
    }
}

bool 
UsdImagingGLEngine::_CanPrepare(const UsdPrim& root)
{
    HD_TRACE_FUNCTION();

    if (!TF_VERIFY(root, "Attempting to draw an invalid/null prim\n")) 
        return false;

    if (!root.GetPath().HasPrefix(_rootPath)) {
        TF_CODING_ERROR("Attempting to draw path <%s>, but engine is rooted"
                    "at <%s>\n",
                    root.GetPath().GetText(),
                    _rootPath.GetText());
        return false;
    }

    return true;
}

static int
_GetRefineLevel(float c)
{
    // TODO: Change complexity to refineLevel when we refactor UsdImaging.
    //
    // Convert complexity float to refine level int.
    int refineLevel = 0;

    // to avoid floating point inaccuracy (e.g. 1.3 > 1.3f)
    c = std::min(c + 0.01f, 2.0f);

    if (1.0f <= c && c < 1.1f) { 
        refineLevel = 0;
    } else if (1.1f <= c && c < 1.2f) { 
        refineLevel = 1;
    } else if (1.2f <= c && c < 1.3f) { 
        refineLevel = 2;
    } else if (1.3f <= c && c < 1.4f) { 
        refineLevel = 3;
    } else if (1.4f <= c && c < 1.5f) { 
        refineLevel = 4;
    } else if (1.5f <= c && c < 1.6f) { 
        refineLevel = 5;
    } else if (1.6f <= c && c < 1.7f) { 
        refineLevel = 6;
    } else if (1.7f <= c && c < 1.8f) { 
        refineLevel = 7;
    } else if (1.8f <= c && c <= 2.0f) { 
        refineLevel = 8;
    } else {
        TF_CODING_ERROR("Invalid complexity %f, expected range is [1.0,2.0]\n", 
                c);
    }
    return refineLevel;
}

void
UsdImagingGLEngine::_PreSetTime(const UsdImagingGLRenderParams& params)
{
    HD_TRACE_FUNCTION();

    const int refineLevel = _GetRefineLevel(params.complexity);

    if (_GetUseSceneIndices()) {
        // The UsdImagingStageSceneIndex has no complexity opinion.
        // We force the value here upon all prims.
        _displayStyleSceneIndex->SetRefineLevel({true, refineLevel});

        _stageSceneIndex->ApplyPendingUpdates();
    } else {
        // Set the fallback refine level; if this changes from the
        // existing value, all prim refine levels will be dirtied.
        _sceneDelegate->SetRefineLevelFallback(refineLevel);

        // Apply any queued up scene edits.
        _sceneDelegate->ApplyPendingUpdates();
    }
}

void
UsdImagingGLEngine::_PostSetTime(const UsdImagingGLRenderParams& params)
{
    HD_TRACE_FUNCTION();
}


/* static */
bool
UsdImagingGLEngine::_UpdateHydraCollection(
    HdRprimCollection *collection,
    SdfPathVector const& roots,
    UsdImagingGLRenderParams const& params)
{
    if (collection == nullptr) {
        TF_CODING_ERROR("Null passed to _UpdateHydraCollection");
        return false;
    }

    // choose repr
    HdReprSelector reprSelector = HdReprSelector(HdReprTokens->smoothHull);
    const bool refined = params.complexity > 1.0;
    
    if (params.drawMode == UsdImagingGLDrawMode::DRAW_POINTS) {
        reprSelector = HdReprSelector(HdReprTokens->points);
    } else if (params.drawMode == UsdImagingGLDrawMode::DRAW_GEOM_FLAT ||
        params.drawMode == UsdImagingGLDrawMode::DRAW_SHADED_FLAT) {
        // Flat shading
        reprSelector = HdReprSelector(HdReprTokens->hull);
    } else if (
        params.drawMode == UsdImagingGLDrawMode::DRAW_WIREFRAME_ON_SURFACE) {
        // Wireframe on surface
        reprSelector = HdReprSelector(refined ?
            HdReprTokens->refinedWireOnSurf : HdReprTokens->wireOnSurf);
    } else if (params.drawMode == UsdImagingGLDrawMode::DRAW_WIREFRAME) {
        // Wireframe
        reprSelector = HdReprSelector(refined ?
            HdReprTokens->refinedWire : HdReprTokens->wire);
    } else {
        // Smooth shading
        reprSelector = HdReprSelector(refined ?
            HdReprTokens->refined : HdReprTokens->smoothHull);
    }

    // By default our main collection will be called geometry
    const TfToken colName = HdTokens->geometry;

    // Check if the collection needs to be updated (so we can avoid the sort).
    SdfPathVector const& oldRoots = collection->GetRootPaths();

    // inexpensive comparison first
    bool match = collection->GetName() == colName &&
                 oldRoots.size() == roots.size() &&
                 collection->GetReprSelector() == reprSelector;

    // Only take the time to compare root paths if everything else matches.
    if (match) {
        // Note that oldRoots is guaranteed to be sorted.
        for(size_t i = 0; i < roots.size(); i++) {
            // Avoid binary search when both vectors are sorted.
            if (oldRoots[i] == roots[i])
                continue;
            // Binary search to find the current root.
            if (!std::binary_search(oldRoots.begin(), oldRoots.end(), roots[i])) 
            {
                match = false;
                break;
            }
        }

        // if everything matches, do nothing.
        if (match) return false;
    }

    // Recreate the collection.
    *collection = HdRprimCollection(colName, reprSelector);
    collection->SetRootPaths(roots);

    return true;
}

/* static */
HdxRenderTaskParams
UsdImagingGLEngine::_MakeHydraUsdImagingGLRenderParams(
    UsdImagingGLRenderParams const& renderParams)
{
    // Note this table is dangerous and making changes to the order of the 
    // enums in UsdImagingGLCullStyle, will affect this with no compiler help.
    static const HdCullStyle USD_2_HD_CULL_STYLE[] =
    {
        HdCullStyleDontCare,              // Cull No Opinion (unused)
        HdCullStyleNothing,               // CULL_STYLE_NOTHING,
        HdCullStyleBack,                  // CULL_STYLE_BACK,
        HdCullStyleFront,                 // CULL_STYLE_FRONT,
        HdCullStyleBackUnlessDoubleSided, // CULL_STYLE_BACK_UNLESS_DOUBLE_SIDED
    };
    static_assert(((sizeof(USD_2_HD_CULL_STYLE) / 
                    sizeof(USD_2_HD_CULL_STYLE[0])) 
              == (size_t)UsdImagingGLCullStyle::CULL_STYLE_COUNT),
        "enum size mismatch");

    HdxRenderTaskParams params;

    params.overrideColor       = renderParams.overrideColor;
    params.wireframeColor      = renderParams.wireframeColor;

    if (renderParams.drawMode == UsdImagingGLDrawMode::DRAW_GEOM_ONLY ||
        renderParams.drawMode == UsdImagingGLDrawMode::DRAW_POINTS) {
        params.enableLighting = false;
    } else {
        params.enableLighting =  renderParams.enableLighting &&
                                !renderParams.enableIdRender;
    }

    params.enableIdRender      = renderParams.enableIdRender;
    params.depthBiasUseDefault = true;
    params.depthFunc           = HdCmpFuncLess;
    params.cullStyle           = USD_2_HD_CULL_STYLE[
        (size_t)renderParams.cullStyle];

    if (renderParams.alphaThreshold < 0.0) {
        // If no alpha threshold is set, use default of 0.1.
        params.alphaThreshold = 0.1f;
    } else {
        params.alphaThreshold = renderParams.alphaThreshold;
    }

    params.enableSceneMaterials = renderParams.enableSceneMaterials;
    params.enableSceneLights = renderParams.enableSceneLights;

    // We don't provide the following because task controller ignores them:
    // - params.camera
    // - params.viewport

    return params;
}

//static
void
UsdImagingGLEngine::_ComputeRenderTags(UsdImagingGLRenderParams const& params,
                                       TfTokenVector *renderTags)
{
    // Calculate the rendertags needed based on the parameters passed by
    // the application
    renderTags->clear();
    renderTags->reserve(4);
    renderTags->push_back(HdRenderTagTokens->geometry);
    if (params.showGuides) {
        renderTags->push_back(HdRenderTagTokens->guide);
    }
    if (params.showProxy) {
        renderTags->push_back(HdRenderTagTokens->proxy);
    }
    if (params.showRender) {
        renderTags->push_back(HdRenderTagTokens->render);
    }
}

/* static */
TfToken
UsdImagingGLEngine::_GetDefaultRendererPluginId()
{
    static const std::string defaultRendererDisplayName = 
        TfGetenv("HD_DEFAULT_RENDERER", "");

    if (defaultRendererDisplayName.empty()) {
        return TfToken();
    }

    HfPluginDescVector pluginDescs;
    HdRendererPluginRegistry::GetInstance().GetPluginDescs(&pluginDescs);

    // Look for the one with the matching display name
    for (size_t i = 0; i < pluginDescs.size(); ++i) {
        if (pluginDescs[i].displayName == defaultRendererDisplayName) {
            return pluginDescs[i].id;
        }
    }

    TF_WARN("Failed to find default renderer with display name '%s'.",
            defaultRendererDisplayName.c_str());

    return TfToken();
}

UsdImagingDelegate *
UsdImagingGLEngine::_GetSceneDelegate() const
{
    if (_GetUseSceneIndices()) {
        // XXX(USD-7118): this API needs to be removed for full
        // scene index support.
        TF_CODING_ERROR("_GetSceneDelegate API is unsupported");
        return nullptr;
    } else {
        return _sceneDelegate.get();
    }
}

HdEngine *
UsdImagingGLEngine::_GetHdEngine()
{
    return _engine.get();
}

HdxTaskController *
UsdImagingGLEngine::_GetTaskController() const
{
    return _taskController.get();
}

bool
UsdImagingGLEngine::PollForAsynchronousUpdates() const
{
    class _Observer : public HdSceneIndexObserver
    {
    public:

        void PrimsAdded(
                const HdSceneIndexBase &sender,
                const AddedPrimEntries &entries) override
        {

            _changed = true;
        }

        void PrimsRemoved(
            const HdSceneIndexBase &sender,
            const RemovedPrimEntries &entries) override
        {
            _changed = true;
        }

        void PrimsDirtied(
            const HdSceneIndexBase &sender,
            const DirtiedPrimEntries &entries) override
        {
            _changed = true;
        }

        void PrimsRenamed(
            const HdSceneIndexBase &sender,
            const RenamedPrimEntries &entries) override
        {
            _changed = true;
        }

        bool IsChanged() { return _changed; }
    private:
        bool _changed = false;
    };

    if (_allowAsynchronousSceneProcessing && _renderIndex) {
        if (HdSceneIndexBaseRefPtr si = _renderIndex->GetTerminalSceneIndex()) {
            _Observer ob;
            si->AddObserver(HdSceneIndexObserverPtr(&ob));
            si->SystemMessage(HdSystemMessageTokens->asyncPoll, nullptr);
            si->RemoveObserver(HdSceneIndexObserverPtr(&ob));
            return ob.IsChanged();
        }
    }

    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE

